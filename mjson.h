// Copyright (c) 2018 Cesanta Software Limited
// All rights reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef MJSON_H_
#define MJSON_H_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MJSON_ENABLE_PRINT
#define MJSON_ENABLE_PRINT 1
#endif

#ifndef MJSON_ENABLE_RPC
#define MJSON_ENABLE_RPC 1
#endif

#ifndef MJSON_ENABLE_BASE64
#define MJSON_ENABLE_BASE64 1
#endif

#ifndef MJSON_RPC_IN_BUF_SIZE
#define MJSON_RPC_IN_BUF_SIZE 256
#endif

enum {
  MJSON_ERROR_INVALID_INPUT = -1,
  MJSON_ERROR_TOO_DEEP = -2,
};

enum mjson_tok {
  MJSON_TOK_INVALID = 0,
  MJSON_TOK_KEY = 1,
  MJSON_TOK_STRING = 11,
  MJSON_TOK_NUMBER = 12,
  MJSON_TOK_TRUE = 13,
  MJSON_TOK_FALSE = 14,
  MJSON_TOK_NULL = 15,
  MJSON_TOK_ARRAY = 91,
  MJSON_TOK_OBJECT = 123,
};
#define MJSON_TOK_IS_VALUE(t) ((t) > 10 && (t) < 20)

typedef void (*mjson_cb_t)(int ev, const char *s, int off, int len, void *ud);

#ifndef MJSON_MAX_DEPTH
#define MJSON_MAX_DEPTH 20
#endif

int mjson(const char *s, int len, mjson_cb_t cb, void *ud);

enum mjson_tok mjson_find(const char *s, int len, const char *jp,
                          const char **tokptr, int *toklen);

double mjson_get_number(const char *s, int len, const char *path, double def);

int mjson_get_bool(const char *s, int len, const char *path, int dflt);

int mjson_get_string(const char *s, int len, const char *path, char *to, int n);

#if MJSON_ENABLE_BASE64
int mjson_get_base64(const char *s, int len, const char *path, char *to, int n);
#endif

#if MJSON_ENABLE_PRINT

struct mjson_out {
  int (*print)(struct mjson_out *, const char *buf, int len);
  union {
    struct {
      char *ptr;
      int size, len, overflow;
    } fixed_buf;
    char **dynamic_buf;
    FILE *fp;
  } u;
};

#define MJSON_OUT_FIXED_BUF(buf, buflen) \
  {                                      \
    mjson_print_fixed_buf, {             \
      { (buf), (buflen), 0, 0 }          \
    }                                    \
  }

#define MJSON_OUT_DYNAMIC_BUF(buf) \
  {                                \
    mjson_print_dynamic_buf, {     \
      { (char *) (buf), 0, 0, 0 }  \
    }                              \
  }

#define MJSON_OUT_FILE(fp)       \
  {                              \
    mjson_print_file, {          \
      { (char *) (fp), 0, 0, 0 } \
    }                            \
  }

int mjson_printf(struct mjson_out *out, const char *fmt, ...);

int mjson_vprintf(struct mjson_out *out, const char *fmt, va_list ap);

int mjson_print_str(struct mjson_out *out, const char *s, int len);

int mjson_print_int(struct mjson_out *out, int value, int is_signed);

int mjson_print_long(struct mjson_out *out, long value, int is_signed);

int mjson_print_file(struct mjson_out *out, const char *ptr, int len);

int mjson_print_fixed_buf(struct mjson_out *out, const char *ptr, int len);

int mjson_print_dynamic_buf(struct mjson_out *out, const char *ptr, int len);

#endif  /* MJSON_ENABLE_PRINT */

#if MJSON_ENABLE_RPC

void jsonrpc_init(int (*sender)(const char *, int, void *),
                  void (*response_cb)(const char *, int, void *),
                  void *userdata, const char *version);


struct jsonrpc_request {
  const char *params;     // Points to the "params" in the request frame
  int params_len;         // Length of the "params"
  const char *id;         // Points to the "id" in the request frame
  int id_len;             // Length of the "id"
  struct mjson_out *out;  // Output stream
  void *userdata;         // Callback's user data as specified at export time
};

struct jsonrpc_method {
  const char *method;
  int method_sz;
  void (*cb)(struct jsonrpc_request *);
  void *cbdata;
  struct jsonrpc_method *next;
};

/*
 * Main RPC context, stores current request information and a list of
 * exported RPC methods.
 */
struct jsonrpc_ctx {
  struct jsonrpc_method *methods;
  void *userdata;
  int (*sender)(const char *buf, int len, void *userdata);
  void (*response_cb)(const char *buf, int len, void *userdata);
  int in_len;
  char in[MJSON_RPC_IN_BUF_SIZE];
};

/* Registers function fn under the given name within the given RPC context */
#define jsonrpc_ctx_export(ctx, name, fn, ud)                                \
  do {                                                                       \
    static struct jsonrpc_method m = {(name), sizeof(name) - 1, (fn), 0, 0}; \
    m.cbdata = (ud);                                                         \
    m.next = (ctx)->methods;                                                 \
    (ctx)->methods = &m;                                                     \
  } while (0)

void jsonrpc_ctx_init(struct jsonrpc_ctx *ctx,
                      int (*send_cb)(const char *, int, void *),
                      void (*response_cb)(const char *, int, void *),
                      void *userdata, const char *version);

int jsonrpc_ctx_call(struct jsonrpc_ctx *ctx, const char *fmt, ...);

void jsonrpc_return_error(struct jsonrpc_request *r, int code,
                          const char *message_fmt, ...);

void jsonrpc_return_success(struct jsonrpc_request *r, const char *result_fmt, ...);

void jsonrpc_ctx_process(struct jsonrpc_ctx *ctx, char *req, int req_sz);

void jsonrpc_ctx_process_byte(struct jsonrpc_ctx *ctx, unsigned char ch);

extern struct jsonrpc_ctx jsonrpc_default_context;

#define jsonrpc_export(name, fn, ud) \
  jsonrpc_ctx_export(&jsonrpc_default_context, (name), (fn), (ud))

#if !defined(_MSC_VER) || _MSC_VER >= 1700
#define jsonrpc_call(fmt, ...) \
  jsonrpc_ctx_call(&jsonrpc_default_context, fmt, __VA_ARGS__)
#endif

#define jsonrpc_process(buf, len) \
  jsonrpc_ctx_process(&jsonrpc_default_context, (buf), (len))

#define jsonrpc_process_byte(x) \
  jsonrpc_ctx_process_byte(&jsonrpc_default_context, (x))

#endif /* MJSON_ENABLE_RPC */

#endif /* MJSON_H_ */
