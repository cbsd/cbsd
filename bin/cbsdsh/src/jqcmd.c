#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "shell.h"
#include "builtins.h"

#include <jq.h>
#include <jv.h>

enum {
	JQ_OPT_SLURP = 1,
	JQ_OPT_RAW_INPUT = 2,
	JQ_OPT_PROVIDE_NULL = 4,
	JQ_OPT_RAW_OUTPUT = 8,
	JQ_OPT_RAW_OUTPUT0 = 16,
	JQ_OPT_ASCII_OUTPUT = 32,
	JQ_OPT_COLOR_OUTPUT = 64,
	JQ_OPT_NO_COLOR_OUTPUT = 128,
	JQ_OPT_SORTED_OUTPUT = 256,
	JQ_OPT_FROM_FILE = 512,
	JQ_OPT_RAW_NO_LF = 1024,
	JQ_OPT_UNBUFFERED = 2048,
	JQ_OPT_EXIT_STATUS = 4096,
	JQ_OPT_SEQ = 8192
};

enum {
	JQ_OK = 0,
	JQ_OK_NULL_KIND = -1,
	JQ_ERROR_SYSTEM = 2,
	JQ_ERROR_COMPILE = 3,
	JQ_OK_NO_OUTPUT = -4,
	JQ_ERROR_UNKNOWN = 5
};

static void
jq_usage_short(void)
{
	fprintf(stderr,
	    "usage: jq [options] <filter> [file...]\n"
	    "       jq [options] -f <file> [file...]\n"
	    "       jq [options] --args [strings...]\n"
	    "       jq [options] --jsonargs [JSON_TEXTS...]\n");
}

static int
jq_isoptish(const char *text)
{
	return text[0] == '-' &&
	    (text[1] == '-' || isalpha((unsigned char)text[1]));
}

static int
jq_isoption(const char **text, char shortopt, const char *longopt,
    int is_short)
{
	if (is_short) {
		if (shortopt && **text == shortopt) {
			(*text)++;
			if (!**text)
				*text = NULL;
			return 1;
		}
	} else {
		if (!strcmp(*text, longopt)) {
			*text = NULL;
			return 1;
		}
	}
	return 0;
}

static int
jq_append_program(char **program, size_t *program_len, const char *chunk,
    size_t chunk_len)
{
	size_t new_len = *program_len + chunk_len + 1;
	char *next = realloc(*program, new_len + 1);

	if (next == NULL)
		return 0;
	memcpy(next + *program_len, chunk, chunk_len);
	next[new_len - 1] = '\n';
	next[new_len] = '\0';
	*program = next;
	*program_len = new_len;
	return 1;
}

static int
jq_process(jq_state *jq, jv value, int jq_flags, int dumpopts, int options)
{
	int ret = JQ_OK_NO_OUTPUT;

	jq_start(jq, value, jq_flags);
	for (;;) {
		jv result = jq_next(jq);
		if (!jv_is_valid(result)) {
			if (jv_invalid_has_msg(jv_copy(result))) {
				jv msg = jv_invalid_get_msg(jv_copy(result));
				jv input_pos = jq_util_input_get_position(jq);
				fprintf(stderr, "jq: error (at %s): %s\n",
				    jv_string_value(input_pos),
				    jv_string_value(msg));
				jv_free(input_pos);
				jv_free(msg);
				ret = JQ_ERROR_UNKNOWN;
			}
			jv_free(result);
			break;
		}

		if ((options & JQ_OPT_RAW_OUTPUT) &&
		    jv_get_kind(result) == JV_KIND_STRING) {
			if (options & JQ_OPT_ASCII_OUTPUT) {
				jv_dumpf(jv_copy(result), stdout,
				    JV_PRINT_ASCII);
			} else if ((options & JQ_OPT_RAW_OUTPUT0) &&
			    strlen(jv_string_value(result)) !=
			    (unsigned long)jv_string_length_bytes(
				jv_copy(result))) {
				jv_free(result);
				result = jv_invalid_with_msg(jv_string(
				    "Cannot dump a string containing NUL with --raw-output0 option"));
				jv_free(result);
				ret = JQ_ERROR_UNKNOWN;
				break;
			} else {
				fwrite(jv_string_value(result), 1,
				    (size_t)jv_string_length_bytes(
					jv_copy(result)),
				    stdout);
			}
			ret = JQ_OK;
			jv_free(result);
		} else {
			if (jv_get_kind(result) == JV_KIND_FALSE ||
			    jv_get_kind(result) == JV_KIND_NULL)
				ret = JQ_OK_NULL_KIND;
			else
				ret = JQ_OK;
			if (options & JQ_OPT_SEQ)
				fwrite("\036", 1, 1, stdout);
			jv_dumpf(result, stdout, dumpopts);
		}

		if (!(options & JQ_OPT_RAW_NO_LF))
			fwrite("\n", 1, 1, stdout);
		if (options & JQ_OPT_RAW_OUTPUT0)
			fwrite("\0", 1, 1, stdout);
		if (options & JQ_OPT_UNBUFFERED)
			fflush(stdout);
	}

	if (jq_halted(jq)) {
		jv exit_code = jq_get_exit_code(jq);
		if (!jv_is_valid(exit_code))
			ret = JQ_OK;
		else if (jv_get_kind(exit_code) == JV_KIND_NUMBER)
			ret = (int)jv_number_value(exit_code);
		else
			ret = JQ_ERROR_UNKNOWN;
		jv_free(exit_code);

		jv error_message = jq_get_error_message(jq);
		if (jv_get_kind(error_message) == JV_KIND_STRING) {
			fwrite(jv_string_value(error_message), 1,
			    (size_t)jv_string_length_bytes(jv_copy(error_message)),
			    stderr);
		} else if (jv_get_kind(error_message) != JV_KIND_NULL &&
		    jv_is_valid(error_message)) {
			error_message = jv_dump_string(error_message, 0);
			fprintf(stderr, "%s\n", jv_string_value(error_message));
		}
		fflush(stderr);
		jv_free(error_message);
	}

	return ret;
}

/*
 * jqcmd:
 *   jq-compatible builtin using libjq.
 */
int
jqcmd(int argc, char **argv)
{
	jq_state *jq = NULL;
	jq_util_input_state *input_state = NULL;
	jv ARGS = jv_array();
	jv program_arguments = jv_object();
	jv lib_search_paths = jv_null();
	int options = 0;
	int parser_flags = 0;
	int jq_flags = 0;
	int ret = JQ_OK_NO_OUTPUT;
	int last_result = -1;
	int nfiles = 0;
	int compiled = 0;
	int further_args_are_strings = 0;
	int further_args_are_json = 0;
	int dumpopts = JV_PRINT_INDENT_FLAGS(2);
	const char *program = NULL;
	char *program_buf = NULL;
	size_t program_len = 0;

	jq = jq_init();
	if (jq == NULL) {
		fprintf(stderr, "jq: init failed\n");
		return JQ_ERROR_SYSTEM;
	}

	input_state = jq_util_input_init(NULL, NULL);

	for (int i = 1; i < argc; i++) {
		const char *text = argv[i];
		int is_short = 0;

		if (!jq_isoptish(text)) {
			if (!program && !(options & JQ_OPT_FROM_FILE)) {
				program = text;
				continue;
			}
			if (further_args_are_strings) {
				ARGS = jv_array_append(ARGS,
				    jv_string(text));
				continue;
			}
			if (further_args_are_json) {
				jv v = jv_parse(text);
				if (!jv_is_valid(v)) {
					fprintf(stderr,
					    "jq: invalid JSON in --jsonargs\n");
					ret = JQ_ERROR_SYSTEM;
					goto out;
				}
				ARGS = jv_array_append(ARGS, v);
				continue;
			}
			jq_util_input_add_input(input_state, text);
			nfiles++;
			continue;
		}

		if (!strcmp(text, "--")) {
			for (i++; i < argc; i++) {
				if (!program && !(options & JQ_OPT_FROM_FILE)) {
					program = argv[i];
					continue;
				}
				if (further_args_are_strings) {
					ARGS = jv_array_append(ARGS,
					    jv_string(argv[i]));
					continue;
				}
				if (further_args_are_json) {
					jv v = jv_parse(argv[i]);
					if (!jv_is_valid(v)) {
						fprintf(stderr,
						    "jq: invalid JSON in --jsonargs\n");
						ret = JQ_ERROR_SYSTEM;
						goto out;
					}
					ARGS = jv_array_append(ARGS, v);
					continue;
				}
				jq_util_input_add_input(input_state, argv[i]);
				nfiles++;
			}
			break;
		}

		is_short = (text[1] != '-');
		if (is_short)
			text++;
		else
			text += 2;

		while (text && *text) {
			if (jq_isoption(&text, 'n', "null-input", is_short)) {
				options |= JQ_OPT_PROVIDE_NULL;
			} else if (jq_isoption(&text, 'R', "raw-input",
			    is_short)) {
				options |= JQ_OPT_RAW_INPUT;
			} else if (jq_isoption(&text, 's', "slurp", is_short)) {
				options |= JQ_OPT_SLURP;
			} else if (jq_isoption(&text, 'c',
			    "compact-output", is_short)) {
				dumpopts &= ~JV_PRINT_PRETTY;
			} else if (jq_isoption(&text, 'r', "raw-output",
			    is_short)) {
				options |= JQ_OPT_RAW_OUTPUT;
			} else if (jq_isoption(&text, 0, "raw-output0",
			    is_short)) {
				options |= JQ_OPT_RAW_OUTPUT | JQ_OPT_RAW_OUTPUT0;
			} else if (jq_isoption(&text, 'j', "join-output",
			    is_short)) {
				options |= JQ_OPT_RAW_OUTPUT | JQ_OPT_RAW_NO_LF;
			} else if (jq_isoption(&text, 'a', "ascii-output",
			    is_short)) {
				options |= JQ_OPT_ASCII_OUTPUT;
			} else if (jq_isoption(&text, 'S', "sort-keys",
			    is_short)) {
				options |= JQ_OPT_SORTED_OUTPUT;
			} else if (jq_isoption(&text, 'C', "color-output",
			    is_short)) {
				options |= JQ_OPT_COLOR_OUTPUT;
			} else if (jq_isoption(&text, 'M',
			    "monochrome-output", is_short)) {
				options |= JQ_OPT_NO_COLOR_OUTPUT;
			} else if (jq_isoption(&text, 0, "tab", is_short)) {
				dumpopts &= ~JV_PRINT_INDENT_FLAGS(7);
				dumpopts |= JV_PRINT_TAB | JV_PRINT_PRETTY;
			} else if (jq_isoption(&text, 0, "indent",
			    is_short)) {
				const char *arg = NULL;
				char *end = NULL;
				long indent;
				if (is_short && text && *text) {
					arg = text;
					text = NULL;
				} else if (i < argc - 1) {
					arg = argv[++i];
				} else {
					fprintf(stderr,
					    "jq: --indent takes one parameter\n");
					ret = JQ_ERROR_SYSTEM;
					goto out;
				}
				errno = 0;
				indent = strtol(arg, &end, 10);
				if (errno || indent < -1 || indent > 7 ||
				    end == arg || *end) {
					fprintf(stderr,
					    "jq: --indent takes a number between -1 and 7\n");
					ret = JQ_ERROR_SYSTEM;
					goto out;
				}
				dumpopts &= ~(JV_PRINT_TAB |
				    JV_PRINT_INDENT_FLAGS(7));
				dumpopts |= JV_PRINT_INDENT_FLAGS(indent);
			} else if (jq_isoption(&text, 0, "unbuffered",
			    is_short)) {
				options |= JQ_OPT_UNBUFFERED;
			} else if (jq_isoption(&text, 0, "stream", is_short)) {
				parser_flags |= JV_PARSE_STREAMING;
			} else if (jq_isoption(&text, 0, "stream-errors",
			    is_short)) {
				parser_flags |=
				    JV_PARSE_STREAMING | JV_PARSE_STREAM_ERRORS;
			} else if (jq_isoption(&text, 0, "seq", is_short)) {
				options |= JQ_OPT_SEQ;
			} else if (jq_isoption(&text, 'e', "exit-status",
			    is_short)) {
				options |= JQ_OPT_EXIT_STATUS;
			} else if (jq_isoption(&text, 0, "args", is_short)) {
				further_args_are_strings = 1;
				further_args_are_json = 0;
			} else if (jq_isoption(&text, 0, "jsonargs",
			    is_short)) {
				further_args_are_strings = 0;
				further_args_are_json = 1;
			} else if (jq_isoption(&text, 0, "arg", is_short)) {
				if (i >= argc - 2) {
					fprintf(stderr,
					    "jq: --arg takes two parameters\n");
					ret = JQ_ERROR_SYSTEM;
					goto out;
				}
				if (!jv_object_has(jv_copy(program_arguments),
				    jv_string(argv[i + 1]))) {
					program_arguments = jv_object_set(
					    program_arguments,
					    jv_string(argv[i + 1]),
					    jv_string(argv[i + 2]));
				}
				i += 2;
				text = NULL;
			} else if (jq_isoption(&text, 0, "argjson",
			    is_short)) {
				if (i >= argc - 2) {
					fprintf(stderr,
					    "jq: --argjson takes two parameters\n");
					ret = JQ_ERROR_SYSTEM;
					goto out;
				}
				if (!jv_object_has(jv_copy(program_arguments),
				    jv_string(argv[i + 1]))) {
					jv v = jv_parse(argv[i + 2]);
					if (!jv_is_valid(v)) {
						fprintf(stderr,
						    "jq: invalid JSON text passed to --argjson\n");
						ret = JQ_ERROR_SYSTEM;
						goto out;
					}
					program_arguments = jv_object_set(
					    program_arguments,
					    jv_string(argv[i + 1]), v);
				}
				i += 2;
				text = NULL;
			} else if (jq_isoption(&text, 0, "rawfile",
			    is_short) ||
			    jq_isoption(&text, 0, "slurpfile", is_short) ||
			    jq_isoption(&text, 0, "argfile", is_short)) {
				int raw = strstr(argv[i], "rawfile") != NULL;
				const char *which = raw ? "rawfile" : "slurpfile";
				if (i >= argc - 2) {
					fprintf(stderr,
					    "jq: --%s takes two parameters\n",
					    which);
					ret = JQ_ERROR_SYSTEM;
					goto out;
				}
				if (!jv_object_has(jv_copy(program_arguments),
				    jv_string(argv[i + 1]))) {
					jv data = jv_load_file(argv[i + 2],
					    raw);
					if (!jv_is_valid(data)) {
						data = jv_invalid_get_msg(data);
						fprintf(stderr,
						    "jq: Bad JSON in --%s %s %s: %s\n",
						    which, argv[i + 1], argv[i + 2],
						    jv_string_value(data));
						jv_free(data);
						ret = JQ_ERROR_SYSTEM;
						goto out;
					}
					program_arguments = jv_object_set(
					    program_arguments,
					    jv_string(argv[i + 1]), data);
				}
				i += 2;
				text = NULL;
			} else if (jq_isoption(&text, 'f', "from-file",
			    is_short)) {
				const char *arg = NULL;
				if (is_short && text && *text) {
					arg = text;
					text = NULL;
				} else if (i < argc - 1) {
					arg = argv[++i];
				} else {
					fprintf(stderr,
					    "jq: -f takes a parameter\n");
					ret = JQ_ERROR_SYSTEM;
					goto out;
				}
				jv data = jv_load_file(arg, 1);
				if (!jv_is_valid(data)) {
					data = jv_invalid_get_msg(data);
					fprintf(stderr, "jq: %s\n",
					    jv_string_value(data));
					jv_free(data);
					ret = JQ_ERROR_SYSTEM;
					goto out;
				}
				if (!jq_append_program(&program_buf,
				    &program_len, jv_string_value(data),
				    (size_t)jv_string_length_bytes(
					jv_copy(data)))) {
					jv_free(data);
					ret = JQ_ERROR_SYSTEM;
					goto out;
				}
				jv_free(data);
				options |= JQ_OPT_FROM_FILE;
			} else if (jq_isoption(&text, 'L', "library-path",
			    is_short)) {
				const char *arg = NULL;
				if (is_short && text && *text) {
					arg = text;
					text = NULL;
				} else if (i < argc - 1) {
					arg = argv[++i];
				} else {
					fprintf(stderr,
					    "jq: -L takes a parameter\n");
					ret = JQ_ERROR_SYSTEM;
					goto out;
				}
				if (jv_get_kind(lib_search_paths) ==
				    JV_KIND_NULL)
					lib_search_paths = jv_array();
				lib_search_paths = jv_array_append(
				    lib_search_paths, jv_string(arg));
			} else if (jq_isoption(&text, 'h', "help", is_short)) {
				jq_usage_short();
				ret = JQ_OK;
				goto out;
			} else if (jq_isoption(&text, 'V', "version",
			    is_short)) {
				printf("jq (libjq)\n");
				ret = JQ_OK;
				goto out;
			} else {
				fprintf(stderr, "jq: Unknown option\n");
				jq_usage_short();
				ret = JQ_ERROR_SYSTEM;
				goto out;
			}
		}
	}

	if (!program && !(options & JQ_OPT_FROM_FILE))
		program = ".";

	if (jv_get_kind(lib_search_paths) == JV_KIND_NULL) {
		lib_search_paths = JV_ARRAY(jv_string("~/.jq"),
		    jv_string("$ORIGIN/../lib/jq"),
		    jv_string("$ORIGIN/../lib"));
	}
	jq_set_attr(jq, jv_string("JQ_LIBRARY_PATH"), lib_search_paths);
	jq_set_attr(jq, jv_string("JQ_ORIGIN"), jv_string("."));

	if (isatty(STDOUT_FILENO)) {
		dumpopts |= JV_PRINT_ISATTY | JV_PRINT_COLOR;
		const char *no_color = getenv("NO_COLOR");
		if (no_color && *no_color)
			dumpopts &= ~JV_PRINT_COLOR;
	}
	if (options & JQ_OPT_SORTED_OUTPUT)
		dumpopts |= JV_PRINT_SORTED;
	if (options & JQ_OPT_ASCII_OUTPUT)
		dumpopts |= JV_PRINT_ASCII;
	if (options & JQ_OPT_COLOR_OUTPUT)
		dumpopts |= JV_PRINT_COLOR;
	if (options & JQ_OPT_NO_COLOR_OUTPUT)
		dumpopts &= ~JV_PRINT_COLOR;

	jq_set_colors(getenv("JQ_COLORS"));

	ARGS = JV_OBJECT(jv_string("positional"), ARGS,
	    jv_string("named"), jv_copy(program_arguments));
	program_arguments = jv_object_set(program_arguments, jv_string("ARGS"),
	    jv_copy(ARGS));

	if (options & JQ_OPT_FROM_FILE) {
		if (!program_buf || !*program_buf)
			program = ".";
		else
			program = program_buf;
	}

	compiled = jq_compile_args(jq, program, jv_copy(program_arguments));
	if (!compiled) {
		ret = JQ_ERROR_COMPILE;
		goto out;
	}

	if (options & JQ_OPT_SEQ)
		parser_flags |= JV_PARSE_SEQ;

	if (options & JQ_OPT_RAW_INPUT)
		jq_util_input_set_parser(input_state, NULL,
		    (options & JQ_OPT_SLURP) ? 1 : 0);
	else
		jq_util_input_set_parser(input_state,
		    jv_parser_new(parser_flags),
		    (options & JQ_OPT_SLURP) ? 1 : 0);

	jq_set_input_cb(jq, jq_util_input_next_input_cb, input_state);

	if (nfiles == 0)
		jq_util_input_add_input(input_state, "-");

	if (options & JQ_OPT_PROVIDE_NULL) {
		ret = jq_process(jq, jv_null(), jq_flags, dumpopts, options);
	} else {
		jv value;
		while (jq_util_input_errors(input_state) == 0 &&
		    (jv_is_valid((value = jq_util_input_next_input(input_state))) ||
		    jv_invalid_has_msg(jv_copy(value)))) {
			if (jv_is_valid(value)) {
				ret = jq_process(jq, value, jq_flags, dumpopts,
				    options);
				if (ret <= 0 && ret != JQ_OK_NO_OUTPUT)
					last_result = (ret != JQ_OK_NULL_KIND);
				if (jq_halted(jq))
					break;
				continue;
			}

			jv msg = jv_invalid_get_msg(value);
			if (!(options & JQ_OPT_SEQ)) {
				ret = JQ_ERROR_UNKNOWN;
				fprintf(stderr, "jq: parse error: %s\n",
				    jv_string_value(msg));
				jv_free(msg);
				break;
			}
			fprintf(stderr, "jq: ignoring parse error: %s\n",
			    jv_string_value(msg));
			jv_free(msg);
		}
	}

	if (jq_util_input_errors(input_state) != 0)
		ret = JQ_ERROR_SYSTEM;

out:
	if (program_buf)
		free(program_buf);
	jv_free(ARGS);
	jv_free(program_arguments);
	jq_util_input_free(&input_state);
	jq_teardown(&jq);
	fflush(stdout);

	if (options & JQ_OPT_EXIT_STATUS) {
		if (ret != JQ_OK_NO_OUTPUT)
			return abs(ret);
		switch (last_result) {
		case -1:
			return abs(JQ_OK_NO_OUTPUT);
		case 0:
			return abs(JQ_OK_NULL_KIND);
		default:
			return abs(JQ_OK);
		}
	}

	return ret > 0 ? ret : 0;
}
