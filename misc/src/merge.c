/*
 * merge.c - merge multiple POSIX shell-style config files.
 *
 usage: merge [-o /tmp/out.conf] /path/1.conf 2.conf 3.conf...
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *
xstrdup(const char *s)
{
	size_t len;
	char *p;

	if (!s)
		return NULL;
	len = strlen(s) + 1;
	p = malloc(len);
	if (!p)
		return NULL;
	memcpy(p, s, len);
	return p;
}

struct entry {
	char	*name;
	char	*text;
};

struct entry_array {
	struct entry	*items;
	size_t		count;
	size_t		capacity;
};

static void
free_entries(struct entry_array *arr)
{
	size_t i;

	if (!arr || !arr->items)
		return;
	for (i = 0; i < arr->count; i++) {
		free(arr->items[i].name);
		free(arr->items[i].text);
	}
	free(arr->items);
	arr->items = NULL;
	arr->count = 0;
	arr->capacity = 0;
}

static int
ensure_capacity(struct entry_array *arr, size_t needed)
{
	size_t new_cap;
	struct entry *tmp;

	if (arr->capacity >= needed)
		return 0;

	new_cap = arr->capacity ? arr->capacity * 2 : 16;
	while (new_cap < needed)
		new_cap *= 2;

	tmp = realloc(arr->items, new_cap * sizeof(*tmp));
	if (!tmp)
		return -1;
	arr->items = tmp;
	arr->capacity = new_cap;
	return 0;
}

static int
find_entry_index(const struct entry_array *arr, const char *name)
{
	size_t i;

	for (i = 0; i < arr->count; i++) {
		if (strcmp(arr->items[i].name, name) == 0)
			return (int)i;
	}
	return -1;
}

static int
set_entry(struct entry_array *arr, const char *name, const char *text)
{
	int idx;
	char *name_copy, *text_copy;

	idx = find_entry_index(arr, name);

	text_copy = xstrdup(text);
	if (!text_copy)
		return -1;

	if (idx >= 0) {
		/* Replace existing text, keep original order. */
		free(arr->items[idx].text);
		arr->items[idx].text = text_copy;
		return 0;
	}

	/* New entry. */
	name_copy = xstrdup(name);
	if (!name_copy) {
		free(text_copy);
		return -1;
	}

	if (ensure_capacity(arr, arr->count + 1) != 0) {
		free(name_copy);
		free(text_copy);
		return -1;
	}

	arr->items[arr->count].name = name_copy;
	arr->items[arr->count].text = text_copy;
	arr->count++;
	return 0;
}

static int
append_to_buffer(char **buf, size_t *cap, size_t *len, const char *chunk)
{
	size_t chunk_len, needed;
	char *tmp;

	chunk_len = strlen(chunk);
	needed = *len + chunk_len + 1;
	if (needed > *cap) {
		size_t new_cap = *cap ? *cap * 2 : 256;

		while (new_cap < needed)
			new_cap *= 2;
		tmp = realloc(*buf, new_cap);
		if (!tmp)
			return -1;
		*buf = tmp;
		*cap = new_cap;
	}
	memcpy(*buf + *len, chunk, chunk_len);
	*len += chunk_len;
	(*buf)[*len] = '\0';
	return 0;
}

static int
line_continues(const char *line)
{
	size_t len;

	if (!line)
		return 0;
	len = strlen(line);
	if (len == 0)
		return 0;
	/* Strip trailing newline, then trailing spaces/tabs. */
	if (len > 0 && line[len - 1] == '\n')
		len--;
	while (len > 0 && (line[len - 1] == ' ' || line[len - 1] == '\t'))
		len--;
	if (len == 0)
		return 0;
	return line[len - 1] == '\\';
}

static int
parse_param_name(const char *line, char *name_buf, size_t name_buf_sz)
{
	const char *p, *eq, *start, *end;
	size_t len;

	if (!line || !name_buf || name_buf_sz == 0)
		return 0;

	p = line;
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '\0' || *p == '\n')
		return 0;
	if (*p == '#')
		return 0;

	eq = strchr(p, '=');
	if (!eq)
		return 0;

	start = p;
	end = eq - 1;
	while (end > start && isspace((unsigned char)*end))
		end--;
	if (end < start)
		return 0;

	len = (size_t)(end - start + 1);
	if (len >= name_buf_sz)
		len = name_buf_sz - 1;

	memcpy(name_buf, start, len);
	name_buf[len] = '\0';
	return 1;
}

static int
is_comment_or_blank(const char *line)
{
	const char *p = line;

	if (!p)
		return 1;
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '\0' || *p == '\n')
		return 1;
	if (*p == '#')
		return 1;
	return 0;
}

static int
process_file(const char *path, struct entry_array *entries)
{
	FILE *fp;
	char line[4096];

	fp = fopen(path, "r");
	if (!fp) {
		fprintf(stderr, "no such source file: %s\n", path);
		return 0; /* non-fatal */
	}

	while (fgets(line, sizeof(line), fp)) {
		char name[256];

		if (is_comment_or_blank(line))
			continue;

		if (!parse_param_name(line, name, sizeof(name)))
			continue;

		/* Collect full assignment block (handle backslash-newline). */
		{
			char *block = NULL;
			size_t cap = 0, len = 0;

			if (append_to_buffer(&block, &cap, &len, line) != 0) {
				free(block);
				fclose(fp);
				return -1;
			}

			while (line_continues(line)) {
				if (!fgets(line, sizeof(line), fp))
					break;
				if (append_to_buffer(&block, &cap, &len, line) != 0) {
					free(block);
					fclose(fp);
					return -1;
				}
			}

			if (set_entry(entries, name, block) != 0) {
				free(block);
				fclose(fp);
				return -1;
			}
			free(block);
		}
	}

	fclose(fp);
	return 0;
}

static int
ends_with_newline(const char *s)
{
	size_t len;

	if (!s)
		return 0;
	len = strlen(s);
	if (len == 0)
		return 0;
	return s[len - 1] == '\n';
}

int
main(int argc, char *argv[])
{
	struct entry_array entries;
	char *out_path = NULL;
	FILE *out_fp = NULL;
	int i;
	int ret = 0;

	entries.items = NULL;
	entries.count = 0;
	entries.capacity = 0;

	if (argc < 2) {
		fprintf(stderr,
		    "Usage: %s [-o OUTPUT] source1 [source2 ...]\n",
		    argv[0]);
		return 1;
	}

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-o") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr,
				    "error: -o requires an output file\n");
				ret = 1;
				goto out;
			}
			out_path = argv[++i];
		} else {
			if (process_file(argv[i], &entries) != 0) {
				ret = 1;
				goto out;
			}
		}
	}

	if (entries.count == 0) {
		/* Nothing to merge; still success. */
		goto out;
	}

	if (out_path) {
		out_fp = fopen(out_path, "w");
		if (!out_fp) {
			perror("failed to open output file");
			ret = 1;
			goto out;
		}
	}

	for (i = 0; i < (int)entries.count; i++) {
		const char *text = entries.items[i].text;

		if (text && *text) {
			fputs(text, stdout);
			if (out_fp)
				fputs(text, out_fp);
			if (!ends_with_newline(text)) {
				fputc('\n', stdout);
				if (out_fp)
					fputc('\n', out_fp);
			}
		}
		if (i + 1 < (int)entries.count) {
			fputc('\n', stdout);
			if (out_fp)
				fputc('\n', out_fp);
		}
	}

out:
	if (out_fp)
		fclose(out_fp);
	free_entries(&entries);
	return ret;
}

