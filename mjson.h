// Copyright (c) 2018 Sergey Lyubka
// All rights reserved

enum {
  MJSON_ERROR_INVALID_INPUT = -1,
  MJSON_ERROR_TOO_DEEP = -2,
  MJSON_ERROR_INCOMPLETE = -3,
};

enum mjson_tok {
  MSJON_TOK_INVALID,
  MJSON_TOK_STR,
  MJSON_TOK_NUM,
  MJSON_TOK_TRUE,
  MJSON_TOK_FALSE,
  MJSON_TOK_NULL,
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
    enum mjson_tok tok = MSJON_TOK_INVALID;
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
          tok = MJSON_TOK_NUM;
        } else if (c == '"') {
          int n = mjson_pass_string(&s[i + 1], len - i);
          if (n < 0) return n;
          i += n + 1;
          tok = MJSON_TOK_STR;
        } else {
          return MJSON_ERROR_INVALID_INPUT;
        }
        if (depth == 0) return i + 1;
        expecting = S_COMMA_OR_EOO;
        break;

      case S_KEY:
        if (c == '"') {
          int n = mjson_pass_string(&s[i + 1], len - i);
          if (n < 0) return n;
          i += n + 1;
          tok = MJSON_TOK_STR;
          expecting = S_COLON;
        } else {
          return MJSON_ERROR_INVALID_INPUT;
        }
        break;

      case S_COLON:
        if (c == ':') {
          expecting = S_VALUE;
          tok = ':';
        } else {
          return MJSON_ERROR_INVALID_INPUT;
        }
        break;

      case S_COMMA_OR_EOO:
        if (depth <= 0) return MJSON_ERROR_INVALID_INPUT;
        if (c == ',') {
          expecting = (nesting[depth - 1] == '{') ? S_KEY : S_VALUE;
          tok = ',';
        } else if (c == ']' || c == '}') {
          // In ascii table, the distance between `[` and `]` is 2.
          // Ditto for `{` and `}`. Hence +2 in the code below.
          if (c != nesting[depth - 1] + 2) return MJSON_ERROR_INVALID_INPUT;
          depth--;
          if (depth == 0) return i + 1;
        } else {
          return MJSON_ERROR_INVALID_INPUT;
        }
        break;
    }
    // printf("%.*s\t%d %d\n", i - start + 1, &s[start], depth, expecting);
    MJSONCALL(tok);
  }
  return MJSON_ERROR_INCOMPLETE;
}
