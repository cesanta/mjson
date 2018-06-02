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

# Parsing API

```c
// Parse JSON string `s, len`, calling callback `cb` for each token
int mjson(const char *s, int len, mjson_cb_t cb, void *cbdata);

// Find an element by its JSONPATH. Save found element in tokptr, toklen.
// Example: s, len is a JSON string: {"foo": { "bar": [ 1, 2, 3] }, "baz": true} 
// assert(mjson_find(s, len, "$.foo.bar[1]", &p, &n) == MJSON_TOK_NUMBER);
// assert(mjson_find(s, len, "$.baz", &p, &n) == MJSON_TOK_TRUE);
// assert(mjson_find(s, len, "$", &p, &n) == MJSON_TOK_OBJECT);
enum mjson_tok mjson_find(const char *s, int len, const char *path,
                          const char **tokptr, int *toklen);

// Find a number by its JSONPATH. If not found, return `default_val`.
// assert(mjson_find_number(s, len, "$.foo.bar[1]", 0) == 2);
double mjson_find_number(const char *s, int len, const char *path, double default_val);

// Find a boolean by its JSONPATH. If not found, return `default_val`.
// assert(mjson_find_bool(s, len, "$.baz", 0) == 1);
int mjson_find_bool(const char *s, int len, const char *path, int default_val);

// Find a string by its JSONPATH. Unescape into `to,sz`.
// If not found, return 0. If found, return the length of unescaped string.
// assume JSON string [ "abc", "de\r\n" ]
// assert(mjson_find_string(s, len, "$[1]", buf, buflen) != 0);
int mjson_find_string(const char *s, int len, const char *path, char *to, int sz);
```

# Emitting API

The emitting API uses "printer function" that accepts a buffer to output,
and an arbitrary pointer. It can print JSON to any destination - network
socket, file, auto-resizable memory region, etc.

mjson implements a printer function to a fixed size buffer, called
`mjson_fixed_buf_printer()` which works with `struct mjson_fixed_buf`.
The following example prints `{"a":123}` into a fixed size buffer:

```c
char buf[100];                                      // This is a fixed size buffer
struct mjson_fixed_buf fb = {tmp, sizeof(tmp), 0};  // A struct that describes it

mjson_print_buf("{", 1, mjson_fixed_buf_printer, &fb);
mjson_print_str("a", 1, mjson_fixed_buf_printer, &fb);
mjson_print_int(123, mjson_fixed_buf_printer, &fb);
mjson_print_buf("}", 1, mjson_fixed_buf_printer, &fb);
```
