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

#ifndef MJSON_H
#define MJSON_H

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

#ifndef ATTR
#define ATTR
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
int mjson_get_number(const char *s, int len, const char *path, double *v);
int mjson_get_bool(const char *s, int len, const char *path, int *v);
int mjson_get_string(const char *s, int len, const char *path, char *to, int n);

#if MJSON_ENABLE_BASE64
int mjson_get_base64(const char *s, int len, const char *path, char *to, int n);
#endif

#if MJSON_ENABLE_PRINT
typedef int (*mjson_print_fn_t)(const char *buf, int len, void *userdata);
typedef int (*mjson_vprint_fn_t)(mjson_print_fn_t, void *, va_list *);

struct mjson_fixedbuf {
  char *ptr;
  int size, len;
};

int mjson_printf(mjson_print_fn_t, void *, const char *fmt, ...);
int mjson_vprintf(mjson_print_fn_t, void *, const char *fmt, va_list ap);
int mjson_print_str(mjson_print_fn_t, void *, const char *s, int len);
int mjson_print_int(mjson_print_fn_t, void *, int value, int is_signed);
int mjson_print_long(mjson_print_fn_t, void *, long value, int is_signed);

int mjson_print_file(const char *ptr, int len, void *userdata);
int mjson_print_fixed_buf(const char *ptr, int len, void *userdata);
int mjson_print_dynamic_buf(const char *ptr, int len, void *userdata);

#endif  // MJSON_ENABLE_PRINT

#if MJSON_ENABLE_RPC

void jsonrpc_init(void (*response_cb)(const char *, int, void *),
                  void *userdata);

struct jsonrpc_request {
  const char *params;   // Points to the "params" in the request frame
  int params_len;       // Length of the "params"
  const char *id;       // Points to the "id" in the request frame
  int id_len;           // Length of the "id"
  mjson_print_fn_t fn;  // Printer function
  void *fndata;         // Printer function data
  void *userdata;       // Callback's user data as specified at export time
};

struct jsonrpc_method {
  const char *method;
  int method_sz;
  void (*cb)(struct jsonrpc_request *);
  void *cbdata;
  struct jsonrpc_method *next;
};

// Main RPC context, stores current request information and a list of
// exported RPC methods.
struct jsonrpc_ctx {
  struct jsonrpc_method *methods;
  void *userdata;
  void (*response_cb)(const char *buf, int len, void *userdata);
  int in_len;
  char in[MJSON_RPC_IN_BUF_SIZE];
};

// Registers function fn under the given name within the given RPC context
#define jsonrpc_ctx_export(ctx, name, fn, ud)                                \
  do {                                                                       \
    static struct jsonrpc_method m = {(name), sizeof(name) - 1, (fn), 0, 0}; \
    m.cbdata = (ud);                                                         \
    m.next = (ctx)->methods;                                                 \
    (ctx)->methods = &m;                                                     \
  } while (0)

void jsonrpc_ctx_init(struct jsonrpc_ctx *ctx,
                      void (*response_cb)(const char *, int, void *),
                      void *userdata);
int jsonrpc_call(mjson_print_fn_t fn, void *fndata, const char *fmt, ...);
void jsonrpc_return_error(struct jsonrpc_request *r, int code,
                          const char *message, const char *data_fmt, ...);
void jsonrpc_return_success(struct jsonrpc_request *r, const char *result_fmt,
                            ...);
void jsonrpc_ctx_process(struct jsonrpc_ctx *ctx, char *req, int req_sz,
                         mjson_print_fn_t fn, void *fndata);
void jsonrpc_ctx_process_byte(struct jsonrpc_ctx *ctx, unsigned char ch,
                              mjson_print_fn_t fn, void *fndata);

extern struct jsonrpc_ctx jsonrpc_default_context;

#define jsonrpc_export(name, fn, ud) \
  jsonrpc_ctx_export(&jsonrpc_default_context, (name), (fn), (ud))

#define jsonrpc_process(buf, len, fn, data) \
  jsonrpc_ctx_process(&jsonrpc_default_context, (buf), (len), (fn), (data))

#define jsonrpc_process_byte(x, fn, data) \
  jsonrpc_ctx_process_byte(&jsonrpc_default_context, (x), (fn), (data))

#define JSONRPC_ERROR_INVALID -32700    /* Invalid JSON was received */
#define JSONRPC_ERROR_NOT_FOUND -32601  /* The method does not exist */
#define JSONRPC_ERROR_BAD_PARAMS -32602 /* Invalid params passed */
#define JSONRPC_ERROR_INTERNAL -32603   /* Internal JSON-RPC error */

#endif  // MJSON_ENABLE_RPC
#endif  // MJSON_H

#ifndef MJSON_API_ONLY

#if !defined(_MSC_VER) || _MSC_VER >= 1700
#else
#define va_copy(x, y) (x) = (y)
#define snprintf _snprintf
#endif

static int mjson_esc(int c, int esc) {
  const char *p, *esc1 = "\b\f\n\r\t\\\"", *esc2 = "bfnrt\\\"";
  for (p = esc ? esc1 : esc2; *p != '\0'; p++) {
    if (*p == c) return esc ? esc2[p - esc1] : esc1[p - esc2];
  }
  return 0;
}

static int mjson_pass_string(const char *s, int len) {
  int i;
  for (i = 0; i < len; i++) {
    if (s[i] == '\\' && i + 1 < len && mjson_esc(s[i + 1], 1)) {
      i++;
    } else if (s[i] == '\0') {
      return MJSON_ERROR_INVALID_INPUT;
    } else if (s[i] == '"') {
      return i;
    }
  }
  return MJSON_ERROR_INVALID_INPUT;
}

int ATTR mjson(const char *s, int len, mjson_cb_t cb, void *ud) {
  enum { S_VALUE, S_KEY, S_COLON, S_COMMA_OR_EOO } expecting = S_VALUE;
  unsigned char nesting[MJSON_MAX_DEPTH];
  int i, depth = 0;
#define MJSONCALL(ev) \
  if (cb) cb(ev, s, start, i - start + 1, ud)

// In the ascii table, the distance between `[` and `]` is 2.
// Ditto for `{` and `}`. Hence +2 in the code below.
#define MJSONEOO()                                                     \
  do {                                                                 \
    if (c != nesting[depth - 1] + 2) return MJSON_ERROR_INVALID_INPUT; \
    depth--;                                                           \
    if (depth == 0) {                                                  \
      MJSONCALL(tok);                                                  \
      return i + 1;                                                    \
    }                                                                  \
  } while (0)

  for (i = 0; i < len; i++) {
    int start = i;
    unsigned char c = ((unsigned char *) s)[i];
    int tok = c;
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
    // printf("- %c [%.*s] %d %d\n", c, i, s, depth, expecting);
    switch (expecting) {
      case S_VALUE:
        if (c == '{') {
          if (depth >= (int) sizeof(nesting)) return MJSON_ERROR_TOO_DEEP;
          nesting[depth++] = c;
          expecting = S_KEY;
          break;
        } else if (c == '[') {
          if (depth >= (int) sizeof(nesting)) return MJSON_ERROR_TOO_DEEP;
          nesting[depth++] = c;
          break;
        } else if (c == ']' && depth > 0) {  // Empty array
          MJSONEOO();
        } else if (c == 't' && i + 3 < len && memcmp(&s[i], "true", 4) == 0) {
          i += 3;
          tok = MJSON_TOK_TRUE;
        } else if (c == 'n' && i + 3 < len && memcmp(&s[i], "null", 4) == 0) {
          i += 3;
          tok = MJSON_TOK_NULL;
        } else if (c == 'f' && i + 4 < len && memcmp(&s[i], "false", 5) == 0) {
          i += 4;
          tok = MJSON_TOK_FALSE;
        } else if (c == '-' || ((c >= '0' && c <= '9'))) {
          char *end = NULL;
          strtod(&s[i], &end);
          if (end != NULL) i += end - &s[i] - 1;
          tok = MJSON_TOK_NUMBER;
        } else if (c == '"') {
          int n = mjson_pass_string(&s[i + 1], len - i - 1);
          if (n < 0) return n;
          i += n + 1;
          tok = MJSON_TOK_STRING;
        } else {
          return MJSON_ERROR_INVALID_INPUT;
        }
        if (depth == 0) {
          MJSONCALL(tok);
          return i + 1;
        }
        expecting = S_COMMA_OR_EOO;
        break;

      case S_KEY:
        if (c == '"') {
          int n = mjson_pass_string(&s[i + 1], len - i - 1);
          if (n < 0) return n;
          i += n + 1;
          tok = MJSON_TOK_KEY;
          expecting = S_COLON;
        } else if (c == '}') {  // Empty object
          MJSONEOO();
          expecting = S_COMMA_OR_EOO;
        } else {
          return MJSON_ERROR_INVALID_INPUT;
        }
        break;

      case S_COLON:
        if (c == ':') {
          expecting = S_VALUE;
        } else {
          return MJSON_ERROR_INVALID_INPUT;
        }
        break;

      case S_COMMA_OR_EOO:
        if (depth <= 0) return MJSON_ERROR_INVALID_INPUT;
        if (c == ',') {
          expecting = (nesting[depth - 1] == '{') ? S_KEY : S_VALUE;
        } else if (c == ']' || c == '}') {
          MJSONEOO();
        } else {
          return MJSON_ERROR_INVALID_INPUT;
        }
        break;
    }
    MJSONCALL(tok);
  }
  return MJSON_ERROR_INVALID_INPUT;
}

struct msjon_get_data {
  const char *path;     // Lookup json path
  int pos;              // Current path index
  int d1;               // Current depth of traversal
  int d2;               // Expected depth of traversal
  int i1;               // Index in an array
  int i2;               // Expected index in an array
  int obj;              // If the value is array/object, offset where it starts
  const char **tokptr;  // Destination
  int *toklen;          // Destination length
  int tok;              // Returned token
};

static int mjson_plen(const char *s) {
  int i = 0;
  while (s[i] != '\0' && s[i] != '.' && s[i] != '[') i++;
  return i;
}

static void ATTR mjson_get_cb(int tok, const char *s, int off, int len,
                              void *ud) {
  struct msjon_get_data *data = (struct msjon_get_data *) ud;
  // printf("--> %2x %2d %2d %2d %2d\t'%s'\t'%.*s'\t\t'%.*s'\n", tok, data->d1,
  //        data->d2, data->i1, data->i2, data->path + data->pos, off, s, len,
  //        s + off);
  if (data->tok != MJSON_TOK_INVALID) return;  // Found

  if (tok == '{') {
    if (!data->path[data->pos] && data->d1 == data->d2) data->obj = off;
    data->d1++;
  } else if (tok == '[') {
    if (data->d1 == data->d2 && data->path[data->pos] == '[') {
      data->i1 = 0;
      data->i2 = strtod(&data->path[data->pos + 1], NULL);
      if (data->i1 == data->i2) {
        data->d2++;
        data->pos += 3;
      }
    }
    if (!data->path[data->pos] && data->d1 == data->d2) data->obj = off;
    data->d1++;
  } else if (tok == ',') {
    if (data->d1 == data->d2 + 1) {
      data->i1++;
      if (data->i1 == data->i2) {
        while (data->path[data->pos] != ']') data->pos++;
        data->pos++;
        data->d2++;
      }
    }
  } else if (tok == MJSON_TOK_KEY && data->d1 == data->d2 + 1 &&
             data->path[data->pos] == '.' && s[off] == '"' &&
             s[off + len - 1] == '"' &&
             mjson_plen(&data->path[data->pos + 1]) == len - 2 &&
             !memcmp(s + off + 1, &data->path[data->pos + 1], len - 2)) {
    data->d2++;
    data->pos += len - 1;
  } else if (tok == '}' || tok == ']') {
    data->d1--;
    if (!data->path[data->pos] && data->d1 == data->d2 && data->obj != -1) {
      data->tok = tok - 2;
      if (data->tokptr) *data->tokptr = s + data->obj;
      if (data->toklen) *data->toklen = off - data->obj + 1;
    }
  } else if (MJSON_TOK_IS_VALUE(tok)) {
    // printf("TOK --> %d\n", tok);
    if (data->d1 == data->d2 && !data->path[data->pos]) {
      data->tok = tok;
      if (data->tokptr) *data->tokptr = s + off;
      if (data->toklen) *data->toklen = len;
    }
  }
}

enum mjson_tok ATTR mjson_find(const char *s, int len, const char *jp,
                               const char **tokptr, int *toklen) {
  struct msjon_get_data data = {jp, 1,  0,      0,      0,
                                0,  -1, tokptr, toklen, MJSON_TOK_INVALID};
  if (jp[0] != '$') return MJSON_TOK_INVALID;
  if (mjson(s, len, mjson_get_cb, &data) < 0) return MJSON_TOK_INVALID;
  return (enum mjson_tok) data.tok;
}

int mjson_get_number(const char *s, int len, const char *path, double *v) {
  const char *p;
  int tok, n;
  if ((tok = mjson_find(s, len, path, &p, &n)) == MJSON_TOK_NUMBER) {
    if (v != NULL) *v = strtod(p, NULL);
  }
  return tok == MJSON_TOK_NUMBER ? 1 : 0;
}

int ATTR mjson_get_bool(const char *s, int len, const char *path, int *v) {
  int tok = mjson_find(s, len, path, NULL, NULL);
  if (tok == MJSON_TOK_TRUE && v != NULL) *v = 1;
  if (tok == MJSON_TOK_FALSE && v != NULL) *v = 0;
  return tok == MJSON_TOK_TRUE || tok == MJSON_TOK_FALSE ? 1 : 0;
}

static int ATTR mjson_unescape(const char *s, int len, char *to, int n) {
  int i, j;
  for (i = 0, j = 0; i < len && j < n; i++, j++) {
    if (s[i] == '\\' && i + 1 < len) {
      int c = mjson_esc(s[i + 1], 0);
      if (c == 0) return -1;
      to[j] = c;
      i++;
    } else {
      to[j] = s[i];
    }
  }
  if (j >= n) return -1;
  if (n > 0) to[j] = '\0';
  return j;
}

int ATTR mjson_get_string(const char *s, int len, const char *path, char *to,
                          int n) {
  const char *p;
  int sz;
  if (mjson_find(s, len, path, &p, &sz) != MJSON_TOK_STRING) return -1;
  return mjson_unescape(p + 1, sz - 2, to, n);
}

#if MJSON_ENABLE_BASE64
static int ATTR mjson_base64rev(int c) {
  if (c >= 'A' && c <= 'Z') {
    return c - 'A';
  } else if (c >= 'a' && c <= 'z') {
    return c + 26 - 'a';
  } else if (c >= '0' && c <= '9') {
    return c + 52 - '0';
  } else if (c == '+') {
    return 62;
  } else if (c == '/') {
    return 63;
  } else {
    return 64;
  }
}

static int ATTR mjson_base64_dec(const char *src, int n, char *dst, int dlen) {
  const char *end = src + n;
  int len = 0;
  while (src + 3 < end && len < dlen) {
    int a = mjson_base64rev(src[0]), b = mjson_base64rev(src[1]),
        c = mjson_base64rev(src[2]), d = mjson_base64rev(src[3]);
    dst[len++] = (a << 2) | (b >> 4);
    if (src[2] != '=' && len < dlen) {
      dst[len++] = (b << 4) | (c >> 2);
      if (src[3] != '=' && len < dlen) {
        dst[len++] = (c << 6) | d;
      }
    }
    src += 4;
  }
  if (len < dlen) dst[len] = '\0';
  return len;
}

int ATTR mjson_get_base64(const char *s, int len, const char *path, char *to,
                          int n) {
  const char *p;
  int sz;
  if (mjson_find(s, len, path, &p, &sz) != MJSON_TOK_STRING) return 0;
  return mjson_base64_dec(p + 1, sz - 2, to, n);
}
#endif  // MJSON_ENABLE_BASE64

#if MJSON_ENABLE_PRINT
int ATTR mjson_print_fixed_buf(const char *ptr, int len, void *fndata) {
  struct mjson_fixedbuf *fb = (struct mjson_fixedbuf *) fndata;
  int i, left = fb->size - fb->len;
  if (left < len) len = left;
  for (i = 0; i < len; i++) fb->ptr[fb->len + i] = ptr[i];
  fb->len += len;
  return len;
}

int ATTR mjson_print_dynamic_buf(const char *ptr, int len, void *fndata) {
  char *s, *buf = *(char **) fndata;
  int curlen = buf == NULL ? 0 : strlen(buf);
  if ((s = (char *) realloc(buf, curlen + len + 1)) == NULL) {
    return 0;
  } else {
    memcpy(s + curlen, ptr, len);
    s[curlen + len] = '\0';
    *(char **) fndata = s;
    return len;
  }
}

int ATTR mjson_print_file(const char *ptr, int len, void *userdata) {
  return fwrite(ptr, 1, len, (FILE *) userdata);
}

int ATTR mjson_print_buf(mjson_print_fn_t fn, void *fndata, const char *buf,
                         int len) {
  return fn(buf, len, fndata);
}

int ATTR mjson_print_int(mjson_print_fn_t fn, void *fndata, int value,
                         int is_signed) {
  char buf[20];
  int len = snprintf(buf, sizeof(buf), is_signed ? "%d" : "%u", value);
  return fn(buf, len, fndata);
}

int ATTR mjson_print_long(mjson_print_fn_t fn, void *fndata, long value,
                          int is_signed) {
  char buf[20];
  const char *fmt = (is_signed ? "%ld" : "%lu");
  int len = snprintf(buf, sizeof(buf), fmt, value);
  return fn(buf, len, fndata);
}

int ATTR mjson_print_dbl(mjson_print_fn_t fn, void *fndata, double d,
                         const char *fmt) {
  char buf[40];
  int n = snprintf(buf, sizeof(buf), fmt, d);
  return fn(buf, n, fndata);
}

int ATTR mjson_print_str(mjson_print_fn_t fn, void *fndata, const char *s,
                         int len) {
  int i, n = fn("\"", 1, fndata);
  for (i = 0; i < len; i++) {
    char c = mjson_esc(s[i], 1);
    if (c) {
      n += fn("\\", 1, fndata);
      n += fn(&c, 1, fndata);
    } else {
      n += fn(&s[i], 1, fndata);
    }
  }
  return n + fn("\"", 1, fndata);
}

#if MJSON_ENABLE_BASE64
int ATTR mjson_print_b64(mjson_print_fn_t fn, void *fndata,
                         const unsigned char *s, int n) {
  const char *t =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int i, len = fn("\"", 1, fndata);
  for (i = 0; i < n; i += 3) {
    int a = s[i], b = i + 1 < n ? s[i + 1] : 0, c = i + 2 < n ? s[i + 2] : 0;
    char buf[4] = {t[a >> 2], t[(a & 3) << 4 | (b >> 4)], '=', '='};
    if (i + 1 < n) buf[2] = t[(b & 15) << 2 | (c >> 6)];
    if (i + 2 < n) buf[3] = t[c & 63];
    len += fn(buf, sizeof(buf), fndata);
  }
  return len + fn("\"", 1, fndata);
}
#endif /* MJSON_ENABLE_BASE64 */

int ATTR mjson_vprintf(mjson_print_fn_t fn, void *fndata, const char *fmt,
                       va_list xap) {
  int i = 0, n = 0;
  va_list ap;
  va_copy(ap, xap);
  while (fmt[i] != '\0') {
    if (fmt[i] == '%') {
      char fc = fmt[++i];
      int is_long = 0;
      if (fc == 'l') {
        is_long = 1;
        fc = fmt[i + 1];
      }
      if (fc == 'Q') {
        char *buf = va_arg(ap, char *);
        n += mjson_print_str(fn, fndata, buf ? buf : "", buf ? strlen(buf) : 0);
      } else if (strncmp(&fmt[i], ".*Q", 3) == 0) {
        int len = va_arg(ap, int);
        char *buf = va_arg(ap, char *);
        n += mjson_print_str(fn, fndata, buf, len);
        i += 2;
      } else if (fc == 'd' || fc == 'u') {
        int is_signed = (fc == 'd');
        if (is_long) {
          long val = va_arg(ap, long);
          n += mjson_print_long(fn, fndata, val, is_signed);
          i++;
        } else {
          int val = va_arg(ap, int);
          n += mjson_print_int(fn, fndata, val, is_signed);
        }
      } else if (fc == 'B') {
        const char *s = va_arg(ap, int) ? "true" : "false";
        n += mjson_print_buf(fn, fndata, s, strlen(s));
      } else if (fc == 's') {
        char *buf = va_arg(ap, char *);
        n += mjson_print_buf(fn, fndata, buf, strlen(buf));
      } else if (strncmp(&fmt[i], ".*s", 3) == 0) {
        int len = va_arg(ap, int);
        char *buf = va_arg(ap, char *);
        n += mjson_print_buf(fn, fndata, buf, len);
        i += 2;
      } else if (fc == 'g') {
        n += mjson_print_dbl(fn, fndata, va_arg(ap, double), "%g");
      } else if (fc == 'f') {
        n += mjson_print_dbl(fn, fndata, va_arg(ap, double), "%f");
#if MJSON_ENABLE_BASE64
      } else if (fc == 'V') {
        int len = va_arg(ap, int);
        const char *buf = va_arg(ap, const char *);
        n += mjson_print_b64(fn, fndata, (unsigned char *) buf, len);
#endif
      } else if (fc == 'H') {
        const char *hex = "0123456789abcdef";
        int i, len = va_arg(ap, int);
        const unsigned char *p = va_arg(ap, const unsigned char *);
        n += fn("\"", 1, fndata);
        for (i = 0; i < len; i++) {
          n += fn(&hex[(p[i] >> 4) & 15], 1, fndata);
          n += fn(&hex[p[i] & 15], 1, fndata);
        }
        n += fn("\"", 1, fndata);
      } else if (fc == 'M') {
        mjson_vprint_fn_t vfn = va_arg(ap, mjson_vprint_fn_t);
        n += vfn(fn, fndata, &ap);
      }
      i++;
    } else {
      n += mjson_print_buf(fn, fndata, &fmt[i++], 1);
    }
  }
  va_end(xap);
  return n;
}

int ATTR mjson_printf(mjson_print_fn_t fn, void *fndata, const char *fmt, ...) {
  va_list ap;
  int len;
  va_start(ap, fmt);
  len = mjson_vprintf(fn, fndata, fmt, ap);
  va_end(ap);
  return len;
}
#endif /* MJSON_ENABLE_PRINT */

#if MJSON_IMPLEMENT_STRTOD
static inline int is_digit(int c) {
  return c >= '0' && c <= '9';
}

/* NOTE: strtod() implementation by Yasuhiro Matsumoto. */
double ATTR strtod(const char *str, char **end) {
  double d = 0.0;
  int sign = 1, n = 0;
  const char *p = str, *a = str;

  /* decimal part */
  if (*p == '-') {
    sign = -1;
    ++p;
  } else if (*p == '+')
    ++p;
  if (is_digit(*p)) {
    d = (double) (*p++ - '0');
    while (*p && is_digit(*p)) {
      d = d * 10.0 + (double) (*p - '0');
      ++p;
      ++n;
    }
    a = p;
  } else if (*p != '.')
    goto done;
  d *= sign;

  /* fraction part */
  if (*p == '.') {
    double f = 0.0;
    double base = 0.1;
    ++p;

    if (is_digit(*p)) {
      while (*p && is_digit(*p)) {
        f += base * (*p - '0');
        base /= 10.0;
        ++p;
        ++n;
      }
    }
    d += f * sign;
    a = p;
  }

  /* exponential part */
  if ((*p == 'E') || (*p == 'e')) {
    int e = 0;
    ++p;

    sign = 1;
    if (*p == '-') {
      sign = -1;
      ++p;
    } else if (*p == '+')
      ++p;

    if (is_digit(*p)) {
      while (*p == '0') ++p;
      e = (int) (*p++ - '0');
      while (*p && is_digit(*p)) {
        e = e * 10 + (int) (*p - '0');
        ++p;
      }
      e *= sign;
    } else if (!is_digit(*(a - 1))) {
      a = str;
      goto done;
    } else if (*p == 0)
      goto done;

    if (d == 2.2250738585072011 && e == -308) {
      d = 0.0;
      a = p;
      goto done;
    }
    if (d == 2.2250738585072012 && e <= -308) {
      d *= 1.0e-308;
      a = p;
      goto done;
    }
    {
      int i;
      for (i = 0; i < 10; i++) d *= 10;
    }
    a = p;
  } else if (p > str && !is_digit(*(p - 1))) {
    a = str;
    goto done;
  }

done:
  if (end) *end = (char *) a;
  return d;
}
#endif

#if MJSON_ENABLE_RPC
struct jsonrpc_ctx jsonrpc_default_context;
struct jsonrpc_userdata {
  mjson_print_fn_t fn;
  void *fndata;
};

static int ATTR jsonrpc_printer(const char *buf, int len, void *userdata) {
  struct jsonrpc_userdata *u = (struct jsonrpc_userdata *) userdata;
  return u->fn(buf, len, u->fndata);
}

int ATTR jsonrpc_call(mjson_print_fn_t fn, void *fndata, const char *fmt, ...) {
  va_list ap;
  int len;
  char ch = '\n';
  va_start(ap, fmt);
  len = mjson_vprintf(fn, fndata, fmt, ap);
  va_end(ap);
  fn(&ch, 1, fndata);
  return len;
}

void ATTR jsonrpc_return_errorv(struct jsonrpc_request *r, int code,
                                const char *message, const char *data_fmt,
                                va_list ap) {
  if (r->id_len == 0) return;
  mjson_printf(r->fn, r->fndata,
               "{\"id\":%.*s,\"error\":{\"code\":%d,\"message\":%Q", r->id_len,
               r->id, code, message == NULL ? "" : message);
  if (data_fmt != NULL) {
  	mjson_printf(r->fn, r->fndata, ",\"data\":");
    mjson_vprintf(r->fn, r->fndata, data_fmt, ap);
  }
  mjson_printf(r->fn, r->fndata, "}}\n");
}

void ATTR jsonrpc_return_error(struct jsonrpc_request *r, int code,
                               const char *message, const char *data_fmt, ...) {
  va_list ap;
  va_start(ap, data_fmt);
  jsonrpc_return_errorv(r, code, message, data_fmt, ap);
  va_end(ap);
}

void ATTR jsonrpc_return_successv(struct jsonrpc_request *r,
                                  const char *result_fmt, va_list ap) {
  if (r->id_len == 0) return;
  mjson_printf(r->fn, r->fndata, "{\"id\":%.*s,\"result\":", r->id_len, r->id);
  if (result_fmt != NULL) {
    mjson_vprintf(r->fn, r->fndata, result_fmt, ap);
  } else {
    mjson_printf(r->fn, r->fndata, "%s", "null");
  }
  mjson_printf(r->fn, r->fndata, "}\n");
}

void ATTR jsonrpc_return_success(struct jsonrpc_request *r,
                                 const char *result_fmt, ...) {
  va_list ap;
  va_start(ap, result_fmt);
  jsonrpc_return_successv(r, result_fmt, ap);
  va_end(ap);
}

void ATTR jsonrpc_ctx_process(struct jsonrpc_ctx *ctx, char *req, int req_sz,
                              mjson_print_fn_t fn, void *fndata) {
  const char *result = NULL;
  char method[50];
  int method_sz = 0, result_sz = 0;
  struct jsonrpc_method *m = NULL;
  struct jsonrpc_userdata d;
  struct jsonrpc_request r = {NULL, 0, NULL, 0, &jsonrpc_printer, NULL, NULL};

  d.fn = fn;
  d.fndata = fndata;
  r.fndata = &d;

  // Is is a response frame?
  mjson_find(req, req_sz, "$.result", &result, &result_sz);
  if (result != NULL && result_sz > 0) {
    if (ctx->response_cb != NULL) ctx->response_cb(req, req_sz, ctx->userdata);
    return;
  }

  // Method must exist and must be a string
  if ((method_sz = mjson_get_string(req, req_sz, "$.method", method,
                                    sizeof(method))) <= 0) {
    jsonrpc_call(fn, fndata, "{\"error\":{\"code\":-32700,\"message\":%.*Q}}",
                 req_sz, req);
    ctx->in_len = 0;
    return;
  }

  // id and params are optional
  mjson_find(req, req_sz, "$.id", &r.id, &r.id_len);
  mjson_find(req, req_sz, "$.params", &r.params, &r.params_len);

  for (m = ctx->methods; m != NULL; m = m->next) {
    if (m->method_sz == method_sz && !memcmp(m->method, method, method_sz)) {
      if (r.params == NULL) r.params = "";
      r.userdata = m->cbdata;
      m->cb(&r);
      break;
    }
  }
  if (m == NULL) {
    jsonrpc_return_error(&r, JSONRPC_ERROR_NOT_FOUND, "method not found", NULL);
  }
}

static int ATTR jsonrpc_print_methods(mjson_print_fn_t fn, void *fndata,
                                      va_list *ap) {
  struct jsonrpc_ctx *ctx = va_arg(*ap, struct jsonrpc_ctx *);
  struct jsonrpc_method *m;
  int len = 0;
  for (m = ctx->methods; m != NULL; m = m->next) {
    if (m != ctx->methods) len += mjson_print_buf(fn, fndata, ",", 1);
    len += mjson_print_str(fn, fndata, m->method, strlen(m->method));
  }
  return len;
}

static void rpclist(struct jsonrpc_request *r) {
  jsonrpc_return_success(r, "[%M]", jsonrpc_print_methods, r->userdata);
}

void ATTR jsonrpc_ctx_init(struct jsonrpc_ctx *ctx,
                           void (*response_cb)(const char *, int, void *),
                           void *userdata) {
  ctx->response_cb = response_cb;
  ctx->userdata = userdata;

  jsonrpc_ctx_export(ctx, "RPC.List", rpclist, ctx);
}

void ATTR jsonrpc_ctx_process_byte(struct jsonrpc_ctx *ctx, unsigned char ch,
                                   mjson_print_fn_t fn, void *p) {
  if (ctx->in_len >= (int) sizeof(ctx->in)) ctx->in_len = 0;  // Overflow
  if (ch == '\n') {  // If new line, parse frame
    if (ctx->in_len > 1) jsonrpc_ctx_process(ctx, ctx->in, ctx->in_len, fn, p);
    ctx->in_len = 0;
  } else {
    ctx->in[ctx->in_len] = ch;  // Append to the buffer
    ctx->in_len++;
  }
}

void ATTR jsonrpc_init(void (*response_cb)(const char *, int, void *),
                       void *userdata) {
  jsonrpc_ctx_init(&jsonrpc_default_context, response_cb, userdata);
}
#endif
#endif
