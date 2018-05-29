// Copyright (c) 2018 Sergey Lyubka
// All rights reserved

enum {
  MJSON_ERROR_INVALID_INPUT = -1,
  MJSON_ERROR_TOO_DEEP = -2,
  MJSON_ERROR_INCOMPLETE = -3,
};

enum mjson_tok {
  MJSON_TOK_INVALID = 0,
  MJSON_TOK_KEY = 1,
  MJSON_TOK_VALUE = 128,
  MJSON_TOK_STRING = 129,
  MJSON_TOK_NUMBER = 130,
  MJSON_TOK_TRUE = 131,
  MJSON_TOK_FALSE = 132,
  MJSON_TOK_NULL = 133,
};

typedef void (*mjson_cb_t)(int ev, const char *s, int len, void *ud);

#ifndef MJSON_MAX_DEPTH
#define MJSON_MAX_DEPTH 20
#endif

static int mjson_pass_string(const char *s, int len) {
  for (int i = 0; i < len; i++) {
    if (s[i] == '\\' && i + 1 < len && strchr("\b\f\n\r\t\\\"/", s[i + 1])) {
      i++;
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
  if (cb) cb(ev, &s[start], i - start + 1, ud)

  for (i = 0; i < len; i++) {
    int start = i;
    unsigned char c = ((unsigned char *) s)[i];
    enum mjson_tok tok = c;
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
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
          if (c == '-') i++;
          while (i < len && s[i + 1] >= '0' && s[i + 1] <= '9') i++;
          tok = MJSON_TOK_NUMBER;
        } else if (c == '"') {
          int n = mjson_pass_string(&s[i + 1], len - i);
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
          int n = mjson_pass_string(&s[i + 1], len - i);
          if (n < 0) return n;
          i += n + 1;
          tok = MJSON_TOK_KEY;
          expecting = S_COLON;
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
          // In the ascii table, the distance between `[` and `]` is 2.
          // Ditto for `{` and `}`. Hence +2 in the code below.
          if (c != nesting[depth - 1] + 2) return MJSON_ERROR_INVALID_INPUT;
          depth--;
          if (depth == 0) {
            MJSONCALL(tok);
            return i + 1;
          }
        } else {
          return MJSON_ERROR_INVALID_INPUT;
        }
        break;
    }
    MJSONCALL(tok);
  }
  return MJSON_ERROR_INCOMPLETE;
}

struct msjon_find_data {
  const char *path;
  int pos;
  const char **dst;
  int *dstlen;
  int tok;
};

static void mjson_find_cb(int ev, const char *s, int len, void *ud) {
  struct msjon_find_data *data = (struct msjon_find_data *) ud;
  printf("--> %d [%s] [%.*s]\n", ev, data->path + data->pos, len, s);
  if (ev == '{' && data->path[data->pos] == '.') {
    data->pos++;
  } else if (ev == MJSON_TOK_KEY &&
             !memcmp(s + 1, &data->path[data->pos], len - 2)) {
    data->pos += len - 2;
  } else if (ev == '}') {
    while (data->pos > 1 && data->path[data->pos] != '.') data->pos--;
  } else if (ev & MJSON_TOK_VALUE && data->path[data->pos] == '\0') {
    data->tok = ev;
    if (data->dst) *data->dst = s;
    if (data->dstlen) *data->dstlen = len;
  }
}

enum mjson_tok mjson_find(const char *s, int len, const char *path,
                          const char **tokptr, int *toklen) {
  struct msjon_find_data data = {path, 1, tokptr, toklen, MJSON_TOK_INVALID};
  // if (path[0] != '$') return MJSON_TOK_INVALID;
  mjson(s, len, mjson_find_cb, &data);
  return data.tok;
}
