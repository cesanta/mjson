# mjson

Minimalistic JSON parser and emitter for embedded systems

[![Build Status](https://travis-ci.org/cpq/mjson.svg?branch=master)](https://travis-ci.org/cpq/mjson)

## Features

- Tiny footprint, single-header ISO C / ISO C++ library
- State machine based parser, no allocations, no recursion
- Low level SAX API
- High level value fetching API - get values by their
    [jsonpath](https://github.com/json-path/JsonPath) 

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

# API reference

```c
int mjson(const char *s, int len, mjson_cb_t cb, void *cbdata);

enum mjson_tok mjson_find(const char *s, int len, const char *path,
                          const char **tokptr, int *toklen);

double mjson_find_number(const char *s, int len, const char *path, double default_val);
int mjson_find_bool(const char *s, int len, const char *path, int default_val);
int mjson_find_string(const char *s, int len, const char *path, char *to, int sz);
```

# License

[MIT](https://en.wikipedia.org/wiki/MIT_License)
