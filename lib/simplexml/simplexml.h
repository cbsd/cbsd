/*
 * Copyright (c) 2001-2002 Bruno Essmann <essmann@users.sourceforge.net>
 * All rights reserved.
 */
#ifndef __SIMPLE_XML
#define __SIMPLE_XML

#ifdef __cplusplus
extern "C" {
#endif

typedef void *SimpleXmlParser;

typedef enum simple_xml_event {
	FINISH_TAG,
	ADD_ATTRIBUTE,
	FINISH_ATTRIBUTES,
	ADD_CONTENT,
	ADD_SUBTAG
} SimpleXmlEvent;

typedef void *(*SimpleXmlTagHandler)(SimpleXmlParser parser,
    SimpleXmlEvent event, const char *szName, const char *szAttribute,
    const char *szValue);

extern SimpleXmlParser simpleXmlCreateParser(const char *sData, long nDataSize);

extern void simpleXmlDestroyParser(SimpleXmlParser parser);

extern int simpleXmlInitializeParser(SimpleXmlParser parser, const char *sData,
    long nDataSize);

int simpleXmlParse(SimpleXmlParser parser, SimpleXmlTagHandler handler);
char *simpleXmlGetErrorDescription(SimpleXmlParser parser);
long simpleXmlGetLineNumber(SimpleXmlParser parser);

#define SIMPLE_XML_USER_ERROR 1000

void simpleXmlParseAbort(SimpleXmlParser parser, int nErrorCode);
int simpleXmlPushUserData(SimpleXmlParser parser, void *pData);
void *simpleXmlPopUserData(SimpleXmlParser parser);
void *simpleXmlGetUserData(SimpleXmlParser parser);
void *simpleXmlGetUserDataAt(SimpleXmlParser parser, int nLevel);
#ifdef __cplusplus
}
#endif

#endif
