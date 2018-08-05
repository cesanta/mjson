# mjson - a JSON parser and emitter for embedded systems

[![Build Status](https://travis-ci.org/cpq/mjson.svg?branch=master)](https://travis-ci.org/cpq/mjson)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)


# Features

- Tiny footprint, single-header ISO C / ISO C++ library
- State machine based parser, no allocations, no recursion
- Low level SAX API
- High level API - fetch from JSON directly into C/C++ by
    [jsonpath](https://github.com/json-path/JsonPath)
- Dependencies: `string.h`, `stdio.h`
- MIT license

# Parsing API

## mjson_find()

```c
enum mjson_tok mjson_find(const char *s, int len, const char *path,
                          const char **tokptr, int *toklen);
```

In a JSON string `s`, `len`, find an element by its JSONPATH `path`.
Save found element in `tokptr`, `toklen`.
If not found, return `JSON_TOK_INVALID`. If found, return one of:
`MJSON_TOK_STRING`, `MJSON_TOK_NUMBER`, `MJSON_TOK_TRUE`, `MJSON_TOK_FALSE`,
`MJSON_TOK_NULL`, `MJSON_TOK_ARRAY`, `MJSON_TOK_OBJECT`. Example:

```c
// s, len is a JSON string: {"foo": { "bar": [ 1, 2, 3] }, "baz": true} 
char *p;
int n;
assert(mjson_find(s, len, "$.foo.bar[1]", &p, &n) == MJSON_TOK_NUMBER);
assert(mjson_find(s, len, "$.baz", &p, &n) == MJSON_TOK_TRUE);
assert(mjson_find(s, len, "$", &p, &n) == MJSON_TOK_OBJECT);
```

## mjson_find_number()

```c
double mjson_find_number(const char *s, int len, const char *path, double default_val);
```

In a JSON string `s`, `len`, return a number value by its JSONPATH `path`.
If not found, return `default_val`. Example:

```c
// s, len is a JSON string: {"foo": { "bar": [ 1, 2, 3] }, "baz": true} 
double v = mjson_find_number(s, len, "$.foo.bar[1]", 0);  // Assigns to 2
```

## mjson_find_bool()

```c
int mjson_find_bool(const char *s, int len, const char *path, int default_val);
```

In a JSON string `s`, `len`, return a value of a boolean by its JSONPATH `path`.
If not found, return `default_val`. Example:

```c
// s, len is a JSON string: {"foo": { "bar": [ 1, 2, 3] }, "baz": true} 
bool v = mjson_find_bool(s, len, "$.baz", false);   // Assigns to true
```

## mjson_find_string()

```c
int mjson_find_string(const char *s, int len, const char *path, char *to, int sz);
```
In a JSON string `s`, `len`, find a string by its JSONPATH `path` and unescape
it into a buffer `to`, `sz` with terminating `\0`.
If a string is not found, return 0.
If a string is found, return the length of unescaped string. Example:

```c
// s, len is a JSON string [ "abc", "de\r\n" ]
char buf[100];
int n = mjson_find_string(s, len, "$[1]", buf, sizeof(buf));  // Assigns to 4
```

## mjson_find_base64()

```c
int mjson_find_base64(const char *s, int len, const char *path, char *to, int sz);
```

In a JSON string `s`, `len`, find a string by its JSONPATH `path` and
base64 decode it into a buffer `to`, `sz` with terminating `\0`.
If a string is not found, return 0.
If a string is found, return the length of decoded string. Example:

```c
// s, len is a JSON string [ "MA==" ]
char buf[100];
int n = mjson_find_base64(s, len, "$[0]", buf, sizeof(buf));  // Assigns to 1
```


## mjson()

```c
int mjson(const char *s, int len, mjson_cb_t cb, void *cbdata);
```

Parse JSON string `s`, `len`, calling callback `cb` for each token. This
is a low-level SAX API, intended for fancy stuff like pretty printing, etc.


# Emitting API


The emitting API uses `struct mjson_out` descriptor that specifies printer
function and associated data. It can print JSON to any destination - network
socket, file, auto-resizable memory region, etc. Builtin descriptors
are:

- Fixed buffer. Prints into a fixed buffer area until the buffer is filled.
  When the buffer full, printing stops, i.e. the buffer is never overflown.
  Check `out.u.buf.overflow` flag to check for the overflow:
  ```c
  char buf[100];
  struct mjson_out out = MJSON_OUT_FIXED_BUF(buf, sizeof(buf));
  ```
- Dynamic buffer. Must be initialised to NULL, then grows using `realloc()`.
  The caller must `free()` the allocated string `buf`:
  ```c
  char *buf = NULL;
  struct mjson_out out = MJSON_OUT_DYNAMIC_BUF(&buf);
  ```
- File:
  ```c
  FILE *fp = fopen("settings.json", "w");
  struct mjson_out out = MJSON_OUT_FILE(fp);
  ```

It is trivial to make your own descriptor. Just define your own printing
function that accepts `struct mjson_out *` and put your own custom data
into the structure. For example, in order to print to a network socket:

```c
struct mjson_out out = {my_socket_printer, {(char *) sock, 0, 0, 0}};
```

## msjon_printf()

```c
int mjson_vprintf(struct mjson_out *out, const char *fmt, va_list ap);
int mjson_printf(struct mjson_out *out, const char *fmt, ...);
```

Print using `printf()`-like format string. Supported specifiers are:

- `%Q` print quoted escaped string. Expect NUL-terminated `char *`
- `%.*Q` print quoted escaped string. Expect `int, char *`
- `%s` print string as is. Expect NUL-terminated `char *`
- `%.*s` print string as is. Expect `int, char *`
- `%f` print floating point number. Expect `double`
- `%d` print integer number. Expect `int`
- `%B` print `true` or `false`. Expect `int`
- `%V` print quoted base64-encoded string. Expect `int, char *`
- `%M` print using custom print function. Expect `int (*)(struct mjson_out *, va_list *)`

The following example produces `{"a":1, "b":[1234]}` into the
dynamically-growing string `s`.
Note that the array is printed using a custom printer function:

```c
static int custom_printer(struct mjson_out *out, va_list *ap) {
  int value = va_arg(*ap, int);
  return mjson_printf(out, "[%d]", value);
}

...
char *s = NULL;
struct mjson_out out = MJSON_OUT_DYNAMIC_BUF(&s);
mjson_printf(&out, "{%Q:%d, %Q:%M}", "a", 1, "b", custom_printer, 1234);
/* At this point `s` contains: {"a":1, "b":[1234]}  */
free(s);
```
