# mjson - a JSON parser, emitter and JSON-RPC engine for embedded systems

[![Build Status](https://travis-ci.org/cesanta/mjson.svg?branch=master)](https://travis-ci.org/cesanta/mjson)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](https://opensource.org/licenses/MIT)

# Expose ANY microcontroller to the Internet

The main purpose of this library is to make it possible for ANY microcontroller
expose its own set of functions as simple cloud RESTful endpoints.

```
┌──────────────┐          ┌─────────────┐          ┌──────────────┐
│ target MCU   │   UART   │ bridge MCU  │    WSS   │     dash.    │
│ e.g. Arduino ├──────────┤ e.g. ESP32  ├──────────┤ mongoose-os. ├──█ REST
│   mjson.h    │ json-rpc │ Mongoose OS │ json-rpc │      com     │
╰──────────────┘          ╰─────────────┘          ╰──────────────┘
```

- Open `examples/shadow.ino` in Arduino IDE, build and flash on any Arduino
- Connect Arduino with any board (e.g. ESP8266) running
  [Mongoose OS](https://mongoose-os.com) over UART
- Configure ESP8266 on mDash ([see docs](https://mongoose-os.com/docs/mdash/registration.md))
- Call Arduino via RESTful

# Features

- Tiny footprint, single-header ISO C / ISO C++ library
- State machine based parser, no allocations, no recursion
- Low level SAX API
- High level API - fetch from JSON directly into C/C++ by
    [jsonpath](https://github.com/json-path/JsonPath)
- Dependencies: `string.h`, `stdio.h`
- Built-in RPC API: `rpc.list`, `sys.info`, `fs.{read,write,list,rename,remove}`
- MIT license

# Build options

- `-D MJSON_ENABLE_PRINT=0` disable emitting functionality, default: enabled
- `-D MJSON_IMPLEMENT_STRTOD=1` use own `strtod()`, default: stdlib is used
- `-D MJSON_MAX_DEPTH=30` define max object depth, default: 20
- `-D MJSON_ENABLE_RPC=0` disable RPC functionality, default: enabled
- `-D MJSON_ENABLE_FS=1` enable `fs.*` file RPC service, default: disabled
- `-D MJSON_ENABLE_BASE64=0` disable base64 parsing/printing, default: enabled

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
- `%g` print floating point number. Expect `double`
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

# JSON-RPC API

For the example, see `unit_test.c :: test_rpc()` function.

## jsonrpc_init

```c
void jsonrpc_init(int (*sender)(char *, int, void *), void *senderdata,
                  const char *version);
```

Initialize JSON-RPC context. The `sender()` function must be provided
by the caller, and it is responsible to send the prepared JSON-RPC
reply to the remote side - to the UART, or socket, or whatever.
The `version` is a firmware version passed to the `info` handler.

## jsonrpc_process

```c
jsonrpc_process(const char *buf, int buf_len);
```

Parse JSON-RPC frame contained in `buf`, and invoke a registered handler.

## jsonrpc_notify

```c
jsonrpc_notify((const char *fmt, ...))
```

Send JSON-RPC notification. The format must create a valid frame.


## jsonrpc_export

```c
#define jsonrpc_export(const char *name, int (*handler)(), void *handler_data)
```

Export JSON-RPC function. A function gets called by `jsonrpc_ctx_process()`,
which parses an incoming frame and calls a registered handler.
A handler function must return 0 for success, or non-0 for error. It could use
the printing API for creating the result, or error message for error.

## JSON-RPC example implementation

In the following example, we initialize JSON-RPC context, and call
a couple of JSON-RPC methods: a built-in `rpc.list` method which lists
all registered methods, and our own `foo` method.

The `sender()` implementation just prints the reply to the standard output,
but in real life it should send a reply to the real remote peer - UART, socket,
or whatever else.

```c
#include "mjson.h"

// A custom RPC handler. Many handlers can be registered.
static int foo(char *buf, int len, struct mjson_out *out, void *userdata) {
  double x = mjson_find_number(buf, len, "$[1]", 0);
  mjson_printf(out, "{%Q:%g,%Q:%Q}", "x", x, "ud", (char *) userdata);
  return 0;
}

// Sender function receives a reply frame and must forward it to the peer.
static int sender(char *buf, int len, void *privdata) {
  printf("%.*s\n", len, buf); // Print the JSON-RPC reply to stdout
  return len;
}

int main(void) {
  jsonrpc_init(sender, NULL, "1.0");

  // Call rpc.list
  char request1[] = "{\"id\": 1, \"method\": \"rpc.list\"}";
  jsonrpc_process(request1, strlen(request1));

  // Call non-existent method
  char request2[] = "{\"id\": 1, \"method\": \"foo\"}";
  jsonrpc_process(request2, strlen(request2));

  // Register our own function
  char request3[] = "{\"id\": 2, \"method\": \"foo\",\"params\":[0,1.23]}";
  jsonrpc_export("foo", foo, (void *) "hi");
  jsonrpc_process(request3, strlen(request3));

  return 0;
}
```
