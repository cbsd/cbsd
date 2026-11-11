# CIX arguments

`cixinit` reads the script's positional parameters and turns `name=value` into shell variables. `cixinit -j` does the same and also writes `CIX_JSON_ARGS`, a JSON document of those arguments. A later `cixinit -j` can read that document back, which is how a nested call keeps the same named parameters (`jexec jname='*'` and the per-jail call under it).

`CIX_JSON_ARGS` is an ordinary shell variable. A child process sees it only when the caller exports it.

## Named and positional arguments

`CIXARG` and `CIXOPTARG` are the declared names. `cixinit` copies them to `MYARG` and `MYOPTARG` with the `:type` suffix removed, then unsets the originals.

Before `--`:

- `name=value` for a declared name is assigned to the shell variable `name`. The value is the text after the first `=`.
- A name listed in `CIX_INIT_SKIP` is not assigned. The raw argument is appended to `CIX_OTHER_ARGS` instead. This is how a repeated flag such as `environment=` stays intact.
- Any other `name=value` is not a shell variable. It is appended to `CIX_OTHER_ARGS`.
- A word with no `=` is appended to `CIX_OTHER_ARGS`.
- `--` stops this assignment. The `--` token and every word after it, up to `WHERE`, are appended to `CIX_OTHER_ARGS` as they stand, even when they look like `name=value`.
- `WHERE` ends the argument list. The words after it become `CIXINIT_SQL_CONDITION` and are not arguments.

Without `-j`, a declared value that starts with `"` may be split across several positional parameters (`sysrc="a`, `b`, `c"`). `cixinit` joins those parameters with spaces until the closing quote, drops the quotes, and turns `\"` into `"`. The next parameter is joined only when it itself contains `"`, so a following `runasap=1` stays a separate argument. A value that does not start with `"` is the tail of that one parameter. `cixinit -j` does not do this: the value is the raw tail of that one parameter, quotes included, and the next parameter is never part of it.

## `cixinit -j`

`-j` assigns shell variables the same way, except for the quote joining above. It additionally sets `CIX_JSON_ARGS` to one JSON object:

```json
{
  "kw": {
    "jname": "test",
    "ip4_addr": "192.168.1.1",
    "environment": ["PATH=/bin", "HOME=/home/user"]
  },
  "argv": ["/bin/sh", "-c", "echo abcd"]
}
```

The string is produced by jq's `jv` library, so quotes, backslashes, and control characters in the values are escaped by the serializer.

`kw` holds named arguments:

- Every `name=value` before `--`, declared or not, becomes `kw.<name>`. An empty name (`=value`) is positional.
- The same scalar name used twice keeps the last value.
- Names listed in `CIXMULTIARG` are always arrays, `kw.<name>[]`, including when the name occurs once. Repeating the name appends another element. A listed name that was not passed is omitted.
- The value is the raw text after `=` in that one parameter. Quotes stay, `\"` is not unescaped, and the next parameter is not joined in. The shell variable receives the same text.

`argv` holds positional arguments:

- A word with no `=` is one element.
- After `--`, each remaining word up to `WHERE` is one element, including words that contain `=`. The `--` token itself is not an element.
- Words from `WHERE` onward are not elements.

With no arguments the document is `{"kw":{},"argv":[]}`.

## Reading `CIX_JSON_ARGS` back

When `cixinit -j` starts and `CIX_JSON_ARGS` is already set and not empty, that document is taken as arguments that have already been parsed. Each string `kw.<name>` is assigned to the shell variable `name`. An array of strings, which is how a `CIXMULTIARG` name is stored, stays in `kw` and is not assigned: a shell variable holds one value. `jexec jname='*'` can pass the original document down, and the per-jail call still has the other named parameters.

The positional parameters of this call are then parsed with the usual rules and applied on top of that state. A scalar name replaces `kw.<name>` and the shell variable. A `CIXMULTIARG` name appends one more string to the array already in `kw`. A positional parameter is appended to `argv`. Names and `argv` elements that this call does not mention stay as they were.

The new `CIX_JSON_ARGS` is that combined document: the previous one, with this call's arguments parsed over it. A child process sees the variable only when the caller exports it.
