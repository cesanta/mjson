// Copyright (c) 2018 Sergey Lyubka
// All rights reserved

#include <stdlib.h>
#include <string.h>

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

static int mjson_pass_string(const char *s, int len) {
  for (int i = 0; i < len; i++) {
    if (s[i] == '\\' && i + 1 < len && strchr("\b\f\n\r\t\\\"/", s[i + 1])) {
      i++;
    } else if (s[i] == '\0') {
      return MJSON_ERROR_INVALID_INPUT;
    } else if (s[i] == '"') {
      return i;
    }
  }
  return MJSON_ERROR_INVALID_INPUT;
}

static int mjson(const char *s, int len, mjson_cb_t cb, void *ud) {
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
        } else if (c == ']') {
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
        } else if (c == '}') {
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

struct msjon_find_data {
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

static void mjson_find_cb(int tok, const char *s, int off, int len, void *ud) {
  struct msjon_find_data *data = (struct msjon_find_data *) ud;
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
             data->path[data->pos] == '.' &&
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

enum mjson_tok mjson_find(const char *s, int len, const char *jp,
                          const char **tokptr, int *toklen) {
  struct msjon_find_data data = {jp, 1,  0,      0,      0,
                                 0,  -1, tokptr, toklen, MJSON_TOK_INVALID};
  if (jp[0] != '$') return MJSON_TOK_INVALID;
  if (mjson(s, len, mjson_find_cb, &data) < 0) return MJSON_TOK_INVALID;
  return data.tok;
}

double mjson_find_number(const char *s, int len, const char *path, double def) {
  const char *p;
  int n;
  double value = def;
  if (mjson_find(s, len, path, &p, &n) == MJSON_TOK_NUMBER) {
    value = strtod(p, NULL);
  }
  return value;
}

int mjson_find_bool(const char *s, int len, const char *path, int dflt) {
  int value = dflt, tok = mjson_find(s, len, path, NULL, NULL);
  if (tok == MJSON_TOK_TRUE) value = 1;
  if (tok == MJSON_TOK_FALSE) value = 0;
  return value;
}

static int mjson_unescape(const char *s, int len, char *to, int n) {
  int i, j;
  for (i = 0, j = 0; i < len && j < n; i++, j++) {
    if (s[i] == '\\' && i + 1 < len) {
      const char *esc1 = "bfnrt\\\"/";
      const char *esc2 = "\b\f\n\r\t\\\"/";
      const char *p = strchr(esc1, s[i + 1]);
      if (p != NULL) to[j] = esc2[p - esc1];
      i++;
    } else {
      to[j] = s[i];
    }
  }
  if (j >= n) return -1;
  if (n > 0) to[j] = '\0';
  return j;
}

int mjson_find_string(const char *s, int len, const char *path, char *to,
                      int n) {
  const char *p;
  int sz;
  if (mjson_find(s, len, path, &p, &sz) != MJSON_TOK_STRING) return 0;
  return mjson_unescape(p + 1, sz - 2, to, n);
}
