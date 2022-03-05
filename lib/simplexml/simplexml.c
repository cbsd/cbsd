/*
 * Copyright (c) 2001-2002 Bruno Essmann <essmann@users.sourceforge.net>
 * All rights reserved.
 */
#include <stdlib.h>
#include <string.h>
#include "simplexml.h"

#define FAIL 0
#define SUCCESS 1
#define NO_ERROR 0
#define NOT_PARSED 1
#define OUT_OF_MEMORY 2
#define EARLY_TERMINATION 3
#define ILLEGAL_AMPERSAND 4
#define NO_UNICODE_SUPPORT 5
#define GREATER_THAN_EXPECTED 6
#define QUOTE_EXPECTED 7
#define ILLEGAL_HANDLER 8
#define NOT_INITIALIZED 9
#define NO_DOCUMENT_TAG 10
#define MISMATCHED_END_TAG 11
#define ATTRIBUTE_EXPECTED 12
#define EQUAL_SIGN_EXPECTED 13
#define TAG_BEGIN_OPENING 0
#define TAG_BEGIN_CLOSING 1
#define TAG_END 2
#define PROCESSING_INSTRUCTION 3
#define DOCTYPE 4
#define COMMENT 5
#define ATTRIBUTE 6
#define CONTENT 7
#define UNKNOWN 8
#define SPACE ' '
#define LF '\xa'
#define CR '\xd'

typedef struct simplexml_value_buffer {

	char *sBuffer;

	long nSize;

	long nPosition;
} TSimpleXmlValueBuffer, *SimpleXmlValueBuffer;

typedef struct simplexml_user_data {
	void *pData;
	struct simplexml_user_data *next;
} TSimpleXmlUserData, *SimpleXmlUserData;

typedef struct simplexml_parser_state {

	int nError;

	SimpleXmlValueBuffer vbNextToken;

	int nNextToken;

	char *szAttribute;

	int nAttributeBufferSize;

	char *sInputData;

	long nInputDataSize;

	long nInputDataPos;

	long nInputLineNumber;

	SimpleXmlUserData pUserData;
} TSimpleXmlParserState, *SimpleXmlParserState;

SimpleXmlParserState createSimpleXmlParser(const char *sData, long nDataSize);
void destroySimpleXmlParser(SimpleXmlParserState parser);
int initializeSimpleXmlParser(SimpleXmlParserState parser, const char *sData,
    long nDataSize);
char *getSimpleXmlParseErrorDescription(SimpleXmlParserState parser);
int parseSimpleXml(SimpleXmlParserState parser, SimpleXmlTagHandler handler);
int parseOneTag(SimpleXmlParserState parser, SimpleXmlTagHandler parentHandler);
int readNextTagToken(SimpleXmlParserState parser);
int readNextContentToken(SimpleXmlParserState parser);
int readChar(SimpleXmlParserState parser);
char peekInputCharAt(SimpleXmlParserState parser, int nOffset);
char peekInputChar(SimpleXmlParserState parser);
int skipWhitespace(SimpleXmlParserState parser);
void skipInputChars(SimpleXmlParserState parser, int nAmount);
void skipInputChar(SimpleXmlParserState parser);
char readInputChar(SimpleXmlParserState parser);
int addNextTokenCharValue(SimpleXmlParserState parser, char c);
int addNextTokenStringValue(SimpleXmlParserState parser, char *szInput);

SimpleXmlValueBuffer createSimpleXmlValueBuffer(long nInitialSize);
void destroySimpleXmlValueBuffer(SimpleXmlValueBuffer vb);
int growSimpleXmlValueBuffer(SimpleXmlValueBuffer vb);
int appendCharToSimpleXmlValueBuffer(SimpleXmlValueBuffer vb, char c);
int appendStringToSimpleXmlValueBuffer(SimpleXmlValueBuffer vb,
    const char *szInput);
int zeroTerminateSimpleXmlValueBuffer(SimpleXmlValueBuffer vb);
int clearSimpleXmlValueBuffer(SimpleXmlValueBuffer vb);
int getSimpleXmlValueBufferContentLength(SimpleXmlValueBuffer vb);
int getSimpleXmlValueBufferContents(SimpleXmlValueBuffer vb, char *szOutput,
    long nMaxLen);
char *getInternalSimpleXmlValueBufferContents(SimpleXmlValueBuffer vb);

SimpleXmlParser
simpleXmlCreateParser(const char *sData, long nDataSize)
{
	return (SimpleXmlParser)createSimpleXmlParser(sData, nDataSize);
}

void
simpleXmlDestroyParser(SimpleXmlParser parser)
{
	destroySimpleXmlParser((SimpleXmlParserState)parser);
}

int
simpleXmlInitializeParser(SimpleXmlParser parser, const char *sData,
    long nDataSize)
{
	return initializeSimpleXmlParser((SimpleXmlParserState)parser, sData,
	    nDataSize);
}

int
simpleXmlParse(SimpleXmlParser parser, SimpleXmlTagHandler handler)
{
	if (parseSimpleXml((SimpleXmlParserState)parser, handler) == FAIL) {
		return ((SimpleXmlParserState)parser)->nError;
	}
	return 0;
}

char *
simpleXmlGetErrorDescription(SimpleXmlParser parser)
{
	return getSimpleXmlParseErrorDescription((SimpleXmlParserState)parser);
}

long
simpleXmlGetLineNumber(SimpleXmlParser parser)
{
	if (parser == NULL) {
		return -1;
	}
	return ((SimpleXmlParserState)parser)->nInputLineNumber + 1;
}

void
simpleXmlParseAbort(SimpleXmlParser parser, int nErrorCode)
{
	if (parser == NULL || nErrorCode < SIMPLE_XML_USER_ERROR) {
		return;
	}
	((SimpleXmlParserState)parser)->nError = nErrorCode;
}

int
simpleXmlPushUserData(SimpleXmlParser parser, void *pData)
{
	SimpleXmlUserData newUserData;
	if (parser == NULL || pData == NULL) {
		return 0;
	}
	newUserData = malloc(sizeof(TSimpleXmlUserData));
	if (newUserData == NULL) {
		return 0;
	}
	newUserData->pData = pData;
	if (((SimpleXmlParserState)parser)->pUserData == NULL) {
		newUserData->next = NULL;
	} else {
		newUserData->next = ((SimpleXmlParserState)parser)->pUserData;
	}
	((SimpleXmlParserState)parser)->pUserData = newUserData;
	return 1;
}

void *
simpleXmlPopUserData(SimpleXmlParser parser)
{
	if (parser == NULL) {
		return NULL;
	}
	if (((SimpleXmlParserState)parser)->pUserData == NULL) {
		return NULL;
	}
	void *pData;
	SimpleXmlUserData ud = ((SimpleXmlParserState)parser)->pUserData;
	((SimpleXmlParserState)parser)->pUserData = ud->next;
	pData = ud->pData;
	free(ud);
	return pData;
}

void *
simpleXmlGetUserDataAt(SimpleXmlParser parser, int nLevel)
{
	if (parser == NULL) {
		return NULL;
	} else {
		SimpleXmlUserData ud =
		    ((SimpleXmlParserState)parser)->pUserData;
		while (ud != NULL && nLevel > 0) {
			ud = ud->next;
			nLevel--;
		}
		if (ud != NULL && nLevel == 0) {
			return ud->pData;
		}
	}
	return NULL;
}

void *
simpleXmlGetUserData(SimpleXmlParser parser)
{
	return simpleXmlGetUserDataAt(parser, 0);
}

void *
simpleXmlNopHandler(SimpleXmlParser parser, SimpleXmlEvent event,
    const char *szName, const char *szAttribute, const char *szValue)
{

	return simpleXmlNopHandler;
}

SimpleXmlParserState
createSimpleXmlParser(const char *sData, long nDataSize)
{
	if (sData != NULL && nDataSize > 0) {
		SimpleXmlParserState parser = malloc(
		    sizeof(TSimpleXmlParserState));
		if (parser == NULL) {
			return NULL;
		}
		parser->nError = NOT_PARSED;
		parser->vbNextToken = createSimpleXmlValueBuffer(512);
		if (parser->vbNextToken == NULL) {
			free(parser);
			return NULL;
		}
		parser->szAttribute = NULL;
		parser->nAttributeBufferSize = 0;
		parser->sInputData = (char *)sData;
		parser->nInputDataSize = nDataSize;
		parser->nInputDataPos = 0;
		parser->nInputLineNumber = 0;
		parser->pUserData = NULL;
		return parser;
	}
	return NULL;
}

void
destroySimpleXmlParser(SimpleXmlParserState parser)
{
	if (parser != NULL) {
		if (parser->vbNextToken != NULL) {
			free(parser->vbNextToken);
		}
		if (parser->szAttribute != NULL) {
			free(parser->szAttribute);
		}
		{

			SimpleXmlUserData ud = parser->pUserData;
			while (ud != NULL) {
				SimpleXmlUserData next = ud->next;
				free(ud);
				ud = next;
			}
		}
		free(parser);
	}
}

int
initializeSimpleXmlParser(SimpleXmlParserState parser, const char *sData,
    long nDataSize)
{
	if (parser != NULL && sData != NULL && nDataSize > 0) {
		if (parser->vbNextToken == NULL) {
			return FAIL;
		}
		parser->nError = NOT_PARSED;
		clearSimpleXmlValueBuffer(parser->vbNextToken);
		parser->sInputData = (char *)sData;
		parser->nInputDataSize = nDataSize;
		parser->nInputDataPos = 0;
		parser->nInputLineNumber = 0;
		parser->pUserData = NULL;
		return SUCCESS;
	}
	return FAIL;
}

char *
getSimpleXmlParseErrorDescription(SimpleXmlParserState parser)
{
	if (parser == NULL) {
		return NULL;
	}
	switch (parser->nError) {
	case NO_ERROR:
		return NULL;
	case NOT_PARSED:
		return "parsing has not yet started";
	case OUT_OF_MEMORY:
		return "out of memory";
	case EARLY_TERMINATION:
		return "unexpected end of xml data";
	case ILLEGAL_AMPERSAND:
		return "illegal use of ampersand (&)";
	case NO_UNICODE_SUPPORT:
		return "unicode characters are not supported";
	case GREATER_THAN_EXPECTED:
		return "greater than sign (>) expected";
	case QUOTE_EXPECTED:
		return "quote (either \' or \") expected";
	case ILLEGAL_HANDLER:
		return "illegal xml handler specified";
	case NOT_INITIALIZED:
		return "xml parser not initialized";
	case NO_DOCUMENT_TAG:
		return "no document tag found";
	case MISMATCHED_END_TAG:
		return "mismatched end tag";
	case ATTRIBUTE_EXPECTED:
		return "attribute expected";
	case EQUAL_SIGN_EXPECTED:
		return "equal sign (=) expected";
	}
	if (parser->nError > SIMPLE_XML_USER_ERROR) {
		return "parsing aborted";
	}
	return "unknown error";
}

int
parseSimpleXml(SimpleXmlParserState parser, SimpleXmlTagHandler handler)
{
	if (parser == NULL || handler == NULL) {
		parser->nError = ILLEGAL_HANDLER;
		return FAIL;
	}

	if (parser->nError != NOT_PARSED) {
		parser->nError = NOT_INITIALIZED;
		return FAIL;
	}

	parser->nError = NO_ERROR;

	do {
		skipWhitespace(parser);
		clearSimpleXmlValueBuffer(parser->vbNextToken);
		if (readNextContentToken(parser) == FAIL) {
			if (parser->nError == EARLY_TERMINATION) {

				parser->nError = NO_DOCUMENT_TAG;
			}
			return FAIL;
		}
	} while (parser->nNextToken == PROCESSING_INSTRUCTION ||
	    parser->nNextToken == COMMENT || parser->nNextToken == DOCTYPE);

	if (parser->nNextToken == TAG_BEGIN_OPENING) {
		return parseOneTag(parser, handler);
	}

	parser->nError = NO_DOCUMENT_TAG;
	return FAIL;
}

int
parseOneTag(SimpleXmlParserState parser, SimpleXmlTagHandler parentHandler)
{
	SimpleXmlTagHandler handler;
	char *szTagName;

	if (getInternalSimpleXmlValueBufferContents(parser->vbNextToken) ==
	    NULL) {
		parser->nError = OUT_OF_MEMORY;
		return FAIL;
	}

	szTagName = strdup(
	    getInternalSimpleXmlValueBufferContents(parser->vbNextToken));
	if (szTagName == NULL) {
		parser->nError = OUT_OF_MEMORY;
		return FAIL;
	}
	clearSimpleXmlValueBuffer(parser->vbNextToken);

	handler = parentHandler((SimpleXmlParser)parser, ADD_SUBTAG, szTagName,
	    NULL, NULL);
	if (parser->nError != NO_ERROR) {
		return FAIL;
	}
	if (handler == NULL) {
		handler = simpleXmlNopHandler;
	}

	if (readNextTagToken(parser) == FAIL) {
		free(szTagName);
		return FAIL;
	}
	while (parser->nNextToken != TAG_END &&
	    parser->nNextToken != TAG_BEGIN_CLOSING) {

		if (parser->nNextToken == ATTRIBUTE) {
			if (getInternalSimpleXmlValueBufferContents(
				parser->vbNextToken) == NULL) {
				parser->nError = OUT_OF_MEMORY;
				free(szTagName);
				return FAIL;
			}
			handler((SimpleXmlParser)parser, ADD_ATTRIBUTE,
			    szTagName, parser->szAttribute,
			    getInternalSimpleXmlValueBufferContents(
				parser->vbNextToken));
			if (parser->nError != NO_ERROR) {
				free(szTagName);
				return FAIL;
			}
			clearSimpleXmlValueBuffer(parser->vbNextToken);
		} else {

			parser->nError = ATTRIBUTE_EXPECTED;
			free(szTagName);
			return FAIL;
		}
		if (readNextTagToken(parser) == FAIL) {
			free(szTagName);
			return FAIL;
		}
	}

	handler((SimpleXmlParser)parser, FINISH_ATTRIBUTES, szTagName, NULL,
	    NULL);
	if (parser->nError != NO_ERROR) {
		free(szTagName);
		return FAIL;
	}

	if (parser->nNextToken == TAG_BEGIN_CLOSING) {
		if (readNextContentToken(parser) == FAIL) {
			free(szTagName);
			return FAIL;
		}
		while (parser->nNextToken != TAG_END) {

			if (parser->nNextToken == TAG_BEGIN_OPENING) {

				if (parseOneTag(parser, handler) == FAIL) {
					free(szTagName);
					return FAIL;
				}
			} else if (parser->nNextToken == CONTENT) {

				if (getInternalSimpleXmlValueBufferContents(
					parser->vbNextToken) == NULL) {
					parser->nError = OUT_OF_MEMORY;
					free(szTagName);
					return FAIL;
				}
				handler((SimpleXmlParser)parser, ADD_CONTENT,
				    szTagName, NULL,
				    getInternalSimpleXmlValueBufferContents(
					parser->vbNextToken));
				if (parser->nError != NO_ERROR) {
					free(szTagName);
					return FAIL;
				}
				clearSimpleXmlValueBuffer(parser->vbNextToken);
			} else if (parser->nNextToken == COMMENT) {
			}

			clearSimpleXmlValueBuffer(parser->vbNextToken);

			if (readNextContentToken(parser) == FAIL) {
				free(szTagName);
				return FAIL;
			}
		}

		if (getInternalSimpleXmlValueBufferContents(
			parser->vbNextToken) == NULL) {
			parser->nError = OUT_OF_MEMORY;
			free(szTagName);
			return FAIL;
		}
		if (strcmp(szTagName,
			getInternalSimpleXmlValueBufferContents(
			    parser->vbNextToken)) != 0) {
			parser->nError = MISMATCHED_END_TAG;
			free(szTagName);
			return FAIL;
		}
	}

	clearSimpleXmlValueBuffer(parser->vbNextToken);

	handler((SimpleXmlParser)parser, FINISH_TAG, szTagName, NULL, NULL);
	if (parser->nError != NO_ERROR) {
		free(szTagName);
		return FAIL;
	}

	free(szTagName);
	return SUCCESS;
}

int
readNextTagToken(SimpleXmlParserState parser)
{

	if (peekInputChar(parser) == '/') {

		skipInputChar(parser);
		if (peekInputChar(parser) == '>') {
			parser->nNextToken = TAG_END;
			skipInputChar(parser);
		} else {
			parser->nError = GREATER_THAN_EXPECTED;
			return FAIL;
		}
	} else if (peekInputChar(parser) == '>') {

		parser->nNextToken = TAG_BEGIN_CLOSING;
		skipInputChar(parser);
	} else {

		char cQuote;
		parser->nNextToken = ATTRIBUTE;
		while (peekInputChar(parser) != '=' &&
		    peekInputChar(parser) > SPACE) {

			if (readChar(parser) == FAIL) {
				return FAIL;
			}
		}

		if (skipWhitespace(parser) == FAIL) {
			return FAIL;
		}
		if (peekInputChar(parser) != '=') {
			parser->nError = EQUAL_SIGN_EXPECTED;
			return FAIL;
		}

		skipInputChar(parser);

		if (parser->szAttribute == NULL ||
		    parser->nAttributeBufferSize <
			getSimpleXmlValueBufferContentLength(
			    parser->vbNextToken)) {
			if (parser->szAttribute != NULL) {
				free(parser->szAttribute);
			}
			parser->nAttributeBufferSize =
			    getSimpleXmlValueBufferContentLength(
				parser->vbNextToken);
			parser->szAttribute = malloc(
			    parser->nAttributeBufferSize);
		}
		if (parser->szAttribute == NULL) {
			parser->nError = OUT_OF_MEMORY;
			return FAIL;
		}
		if (getSimpleXmlValueBufferContents(parser->vbNextToken,
			parser->szAttribute,
			parser->nAttributeBufferSize) == FAIL) {
			parser->nError = OUT_OF_MEMORY;
			return FAIL;
		}
		clearSimpleXmlValueBuffer(parser->vbNextToken);

		if (skipWhitespace(parser) == FAIL) {
			return FAIL;
		}
		cQuote = readInputChar(parser);
		if (parser->nError != NO_ERROR) {
			return FAIL;
		}
		if (cQuote != '\'' && cQuote != '"') {
			parser->nError = QUOTE_EXPECTED;
			return FAIL;
		}
		while (peekInputChar(parser) != cQuote) {

			if (readChar(parser) == FAIL) {
				return FAIL;
			}
		}

		skipInputChar(parser);

		if (skipWhitespace(parser) == FAIL) {
			return FAIL;
		}
	}
	return SUCCESS;
}

int
readNextContentToken(SimpleXmlParserState parser)
{

	if (peekInputChar(parser) == '<') {

		skipInputChar(parser);
		if (peekInputChar(parser) == '/') {

			parser->nNextToken = TAG_END;
			skipInputChar(parser);
			while (peekInputChar(parser) > SPACE &&
			    peekInputChar(parser) != '>') {

				if (readChar(parser) == FAIL) {
					return FAIL;
				}
			}
			while (peekInputChar(parser) != '>') {
				skipInputChar(parser);
			}
			if (peekInputChar(parser) != '>') {
				parser->nError = EARLY_TERMINATION;
				return FAIL;
			}

			skipInputChar(parser);
		} else if (peekInputChar(parser) == '?') {

			parser->nNextToken = PROCESSING_INSTRUCTION;
			skipInputChar(parser);
			while (peekInputCharAt(parser, 0) != '?' ||
			    peekInputCharAt(parser, 1) != '>') {

				if (readChar(parser) == FAIL) {
					return FAIL;
				}
			}

			skipInputChars(parser, 2);
		} else if (peekInputChar(parser) == '!') {

			skipInputChar(parser);
			if (peekInputCharAt(parser, 0) == '-' &&
			    peekInputCharAt(parser, 1) == '-') {

				parser->nNextToken = COMMENT;
				skipInputChars(parser, 2);
				while (peekInputCharAt(parser, 0) != '-' ||
				    peekInputCharAt(parser, 1) != '-' ||
				    peekInputCharAt(parser, 2) != '>') {

					if (readChar(parser) == FAIL) {
						return FAIL;
					}
				}

				skipInputChars(parser, 3);
			} else if (peekInputCharAt(parser, 0) == 'D' &&
			    peekInputCharAt(parser, 1) == 'O' &&
			    peekInputCharAt(parser, 2) == 'C' &&
			    peekInputCharAt(parser, 3) == 'T' &&
			    peekInputCharAt(parser, 4) == 'Y' &&
			    peekInputCharAt(parser, 5) == 'P' &&
			    peekInputCharAt(parser, 6) == 'E') {

				int nCount = 1;
				parser->nNextToken = DOCTYPE;
				skipInputChars(parser, 7);
				while (nCount > 0) {
					if (peekInputChar(parser) == '>') {
						nCount--;
					} else if (peekInputChar(parser) ==
					    '<') {
						nCount++;
					}

					if (nCount > 0 &&
					    readChar(parser) == FAIL) {
						return FAIL;
					}
				}

				skipInputChar(parser);
			} else {

				parser->nNextToken = UNKNOWN;
				while (peekInputChar(parser) != '>') {

					if (readChar(parser) == FAIL) {
						return FAIL;
					}
				}

				skipInputChar(parser);
			}
		} else {

			parser->nNextToken = TAG_BEGIN_OPENING;
			while (peekInputChar(parser) > SPACE &&
			    peekInputChar(parser) != '/' &&
			    peekInputChar(parser) != '>') {

				if (readChar(parser) == FAIL) {
					return FAIL;
				}
			}

			if (skipWhitespace(parser) == FAIL) {
				return FAIL;
			}
		}
	} else {

		parser->nNextToken = CONTENT;
		while (peekInputChar(parser) != '<') {

			if (readChar(parser) == FAIL) {
				return FAIL;
			}
		}
	}
	return SUCCESS;
}

int
readChar(SimpleXmlParserState parser)
{
	char c = readInputChar(parser);
	if (c == '\0' && parser->nError != NO_ERROR) {
		return FAIL;
	}
	if (c == '&') {

		if (peekInputCharAt(parser, 0) == '#') {
			int nCode = 0;
			skipInputChar(parser);
			c = readInputChar(parser);
			if (c == 'x') {

				c = readInputChar(parser);
				while (c != ';') {
					if (c >= '0' && c <= '9') {
						nCode = (nCode * 16) +
						    (c - '0');
					} else if (c >= 'A' && c <= 'F') {
						nCode = (nCode * 16) +
						    (c - 'A' + 10);
					} else if (c >= 'a' && c <= 'f') {
						nCode = (nCode * 16) +
						    (c - 'a' + 10);
					} else {
						parser->nError =
						    ILLEGAL_AMPERSAND;
						return FAIL;
					}
					c = readInputChar(parser);
				}
			} else if (c >= '0' && c <= '9') {

				c = readInputChar(parser);
				while (c != ';') {
					if (c >= '0' && c <= '9') {
						nCode = (nCode * 16) +
						    (c - '0');
					} else {
						parser->nError =
						    ILLEGAL_AMPERSAND;
						return FAIL;
					}
					c = readInputChar(parser);
				}
			} else {

				parser->nError = ILLEGAL_AMPERSAND;
				return FAIL;
			}
			if (nCode > 255) {
				parser->nError = NO_UNICODE_SUPPORT;
				return FAIL;
			}
			return addNextTokenCharValue(parser, (char)nCode);
		} else if (peekInputCharAt(parser, 0) == 'a' &&
		    peekInputCharAt(parser, 1) == 'm' &&
		    peekInputCharAt(parser, 2) == 'p' &&
		    peekInputCharAt(parser, 3) == ';') {

			skipInputChars(parser, 4);
			return addNextTokenCharValue(parser, '&');
		} else if (peekInputCharAt(parser, 0) == 'a' &&
		    peekInputCharAt(parser, 1) == 'p' &&
		    peekInputCharAt(parser, 2) == 'o' &&
		    peekInputCharAt(parser, 3) == 's' &&
		    peekInputCharAt(parser, 4) == ';') {

			skipInputChars(parser, 5);
			return addNextTokenCharValue(parser, '\'');
		} else if (peekInputCharAt(parser, 0) == 'q' &&
		    peekInputCharAt(parser, 1) == 'u' &&
		    peekInputCharAt(parser, 2) == 'o' &&
		    peekInputCharAt(parser, 3) == 't' &&
		    peekInputCharAt(parser, 4) == ';') {

			skipInputChars(parser, 5);
			return addNextTokenCharValue(parser, '"');
		} else if (peekInputCharAt(parser, 0) == 'l' &&
		    peekInputCharAt(parser, 1) == 't' &&
		    peekInputCharAt(parser, 2) == ';') {

			skipInputChars(parser, 3);
			return addNextTokenCharValue(parser, '<');
		} else if (peekInputCharAt(parser, 0) == 'g' &&
		    peekInputCharAt(parser, 1) == 't' &&
		    peekInputCharAt(parser, 2) == ';') {

			skipInputChars(parser, 3);
			return addNextTokenCharValue(parser, '>');
		} else {

			parser->nError = ILLEGAL_AMPERSAND;
			return FAIL;
		}
	} else {

		return addNextTokenCharValue(parser, c);
	}
}

char
peekInputCharAt(SimpleXmlParserState parser, int nOffset)
{
	int nPos = parser->nInputDataPos + nOffset;
	if (nPos < 0 || nPos >= parser->nInputDataSize) {
		return '\0';
	}
	return parser->sInputData[nPos];
}

char
peekInputChar(SimpleXmlParserState parser)
{
	return peekInputCharAt(parser, 0);
}

int
skipWhitespace(SimpleXmlParserState parser)
{
	while (peekInputChar(parser) <= SPACE) {

		readInputChar(parser);
		if (parser->nError != NO_ERROR) {
			return FAIL;
		}
	}
	return SUCCESS;
}

void
skipInputChars(SimpleXmlParserState parser, int nAmount)
{
	int i;
	for (i = 0; i < nAmount; i++) {
		skipInputChar(parser);
	}
}

void
skipInputChar(SimpleXmlParserState parser)
{
	if (parser->nInputDataPos >= 0 &&
	    parser->nInputDataPos < parser->nInputDataSize) {
		if (parser->sInputData[parser->nInputDataPos] == LF) {

			parser->nInputLineNumber++;
		} else if (parser->sInputData[parser->nInputDataPos] == CR) {

			if (parser->nInputDataPos + 1 <
			    parser->nInputDataSize) {
				if (parser->sInputData[parser->nInputDataPos +
					1] != LF) {
					parser->nInputLineNumber++;
				}
			}
		}
	}
	parser->nInputDataPos++;
}

char
readInputChar(SimpleXmlParserState parser)
{
	char cRead;
	if (parser->nInputDataPos < 0 ||
	    parser->nInputDataPos >= parser->nInputDataSize) {
		parser->nError = EARLY_TERMINATION;
		return '\0';
	}
	cRead = parser->sInputData[parser->nInputDataPos];
	skipInputChar(parser);
	return cRead;
}

int
addNextTokenCharValue(SimpleXmlParserState parser, char c)
{
	if (appendCharToSimpleXmlValueBuffer(parser->vbNextToken, c) == FAIL) {
		parser->nError = OUT_OF_MEMORY;
		return FAIL;
	}
	return SUCCESS;
}

int
addNextTokenStringValue(SimpleXmlParserState parser, char *szInput)
{
	while (*szInput != '\0') {
		if (addNextTokenCharValue(parser, *szInput) == FAIL) {
			return FAIL;
		}
		szInput++;
	}
	return SUCCESS;
}

SimpleXmlValueBuffer
createSimpleXmlValueBuffer(long nInitialSize)
{
	SimpleXmlValueBuffer vb = malloc(sizeof(TSimpleXmlValueBuffer));
	if (vb == NULL) {
		return NULL;
	}
	vb->sBuffer = malloc(nInitialSize);
	if (vb->sBuffer == NULL) {
		free(vb);
		return NULL;
	}
	vb->nSize = nInitialSize;
	vb->nPosition = 0;
	return vb;
}

void
destroySimpleXmlValueBuffer(SimpleXmlValueBuffer vb)
{
	if (vb != NULL) {
		if (vb->sBuffer != NULL) {
			free(vb->sBuffer);
		}
		free(vb);
	}
}

int
growSimpleXmlValueBuffer(SimpleXmlValueBuffer vb)
{
	char *sOldBuffer = vb->sBuffer;
	char *sNewBuffer = malloc(vb->nSize * 2);
	if (sNewBuffer == NULL) {
		return FAIL;
	}
	memcpy(sNewBuffer, vb->sBuffer, vb->nSize);
	vb->sBuffer = sNewBuffer;
	vb->nSize = vb->nSize * 2;
	free(sOldBuffer);
	return SUCCESS;
}

int
appendCharToSimpleXmlValueBuffer(SimpleXmlValueBuffer vb, char c)
{
	if (vb == NULL) {
		return FAIL;
	}
	if (vb->nPosition >= vb->nSize) {
		if (growSimpleXmlValueBuffer(vb) == FAIL) {
			return FAIL;
		}
	}
	vb->sBuffer[vb->nPosition++] = c;
	return SUCCESS;
}

int
appendStringToSimpleXmlValueBuffer(SimpleXmlValueBuffer vb, const char *szInput)
{
	while (*szInput != '\0') {
		if (appendCharToSimpleXmlValueBuffer(vb, *szInput) == FAIL) {
			return FAIL;
		}
		szInput++;
	}
	return SUCCESS;
}

int
zeroTerminateSimpleXmlValueBuffer(SimpleXmlValueBuffer vb)
{
	if (vb == NULL) {
		return FAIL;
	}
	if (vb->nPosition >= vb->nSize) {
		if (growSimpleXmlValueBuffer(vb) == FAIL) {
			return FAIL;
		}
	}
	vb->sBuffer[vb->nPosition] = '\0';
	return SUCCESS;
}

int
clearSimpleXmlValueBuffer(SimpleXmlValueBuffer vb)
{
	if (vb == NULL) {
		return FAIL;
	}
	vb->nPosition = 0;
	return SUCCESS;
}

int
getSimpleXmlValueBufferContentLength(SimpleXmlValueBuffer vb)
{
	if (vb == NULL) {
		return 0;
	}
	return vb->nPosition + 1;
}

int
getSimpleXmlValueBufferContents(SimpleXmlValueBuffer vb, char *szOutput,
    long nMaxLen)
{
	int nMax;
	if (vb == NULL) {
		return FAIL;
	}
	nMaxLen -= 1;
	nMax = nMaxLen < vb->nPosition ? nMaxLen : vb->nPosition;
	memcpy(szOutput, vb->sBuffer, nMax);
	szOutput[nMax] = '\0';
	return SUCCESS;
}

char *
getInternalSimpleXmlValueBufferContents(SimpleXmlValueBuffer vb)
{
	if (zeroTerminateSimpleXmlValueBuffer(vb) == FAIL) {
		return NULL;
	}
	return vb->sBuffer;
}
