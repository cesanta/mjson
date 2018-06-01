# mjson

Minimalistic JSON parser and emitter for embedded systems

[![Build Status](https://travis-ci.org/cpq/mjson.svg?branch=master)](https://travis-ci.org/cpq/mjson)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)


## Features

- Tiny footprint, single-header ISO C / ISO C++ library
- State machine based parser, no allocations, no recursion
- Low level SAX API
- High level API - fetch from JSON directly into C/C++ by
    [jsonpath](https://github.com/json-path/JsonPath)
- Dependencies: `memcmp()`, `strtod()`, `strchr()`
- MIT license

## Example: parse JSON-RPC frame

```c
const char *p;
int n;

if (mjson_find(buf, len, "$.id", &p, &n) != MJSON_TOK_INVALID) {
  printf("FRAME ID: [%.*s]\n", n, p);
}

if (mjson_find(buf, len, "$.method", &p, &n) == MJSON_TOK_STRING) {
  printf("FRAME METHOD: [%.*s]\n", n, p);
}

if (mjson_find(buf, len, "$.params", &p, &n) != MJSON_TOK_INVALID) {
  printf("FRAME PARAMS: [%.*s]\n", n, p);
}
```

# Parsing API

```c
int mjson(const char *s, int len, mjson_cb_t cb, void *cbdata);

enum mjson_tok mjson_find(const char *s, int len, const char *path,
                          const char **tokptr, int *toklen);

double mjson_find_number(const char *s, int len, const char *path, double default_val);
int mjson_find_bool(const char *s, int len, const char *path, int default_val);
int mjson_find_string(const char *s, int len, const char *path, char *to, int sz);
```

# Emitting API

The emitting API uses "printer function" that accepts a buffer to output,
and an arbitrary pointer. It can print JSON to any destination - network
socket, file, auto-resizable memory region, etc. mjson implements
a printer function to a fixed size buffer.

The following example prints `{"a":123}` into a fixed size buffer:

```c
char buf[100];
struct mjson_fixed_buf fb = {tmp, sizeof(tmp), 0};
mjson_print_buf("{", 1, mjson_fixed_buf_printer, &fb)
mjson_print_str("a", 1, mjson_fixed_buf_printer, &fb)
mjson_print_int(123, mjson_fixed_buf_printer, &fb)
mjson_print_buf("}", 1, mjson_fixed_buf_printer, &fb)
```