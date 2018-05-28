// Copyright (c) 2018 Sergey Lyubka
// All rights reserved

typedef void (*mjson_cb_t)(int ev, char *s, int len, void *ud);

enum { MJSON_ERROR_INVALID_INPUT = -1, MJSON_ERROR_INCOMPLETE_INPUT = 0 };

static int mjson_is(int c, const char *choices) {
  int i;
  for (i = 0; choices[i] != '\0'; i++)
    if (choices[i] == c) return 1;
  return 0;
}

static int mjson_pass_string(const char *s, int len) {
  int i;
  for (i = 0; i < len; i++) {
    if (s[i] == '\\' && i + 1 < len && mjson_is(s[i + 1], "\b\f\n\r\t\\\"/")) {
      i++;
    } else if (s[i] == '"') {
      return i;
    }
  }
  return MJSON_ERROR_INVALID_INPUT;
}

static int mjson(char *s, int len, mjson_cb_t cb, void *ud) {
  enum { S_VALUE, S_KEY, S_COLON, S_COMMA_OR_EOO } expecting = S_VALUE;
  unsigned char nesting[20];
  int i, depth = 0;
// #define MJSONCALL(ev) cb(ev, s, i, &s[start], i - start, ud)
#define MJSONCALL(ev) cb(ev, s, i + 1, ud)

  for (i = 0; i < len; i++) {
    int start = i;
    unsigned char c = ((unsigned char *) s)[i];
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
    switch (expecting) {
      case S_VALUE:
        if (c == '{') {
          nesting[depth++] = c;
          expecting = S_KEY;
          break;
        } else if (c == '[') {
          nesting[depth++] = c;
          break;
        } else if (c == 't' && i + 3 < len && memcmp(&s[i], "true", 4) == 0) {
          i += 3;
        } else if (c == 'n' && i + 3 < len && memcmp(&s[i], "null", 4) == 0) {
          i += 3;
        } else if (c == 'f' && i + 4 < len && memcmp(&s[i], "false", 5) == 0) {
          i += 4;
        } else if (c == '-' || ((c >= '0' && c <= '9'))) {
          if (c == '-') i++;
          while (i < len && s[i] >= '0' && s[i] <= '9') i++;
        } else {
          return MJSON_ERROR_INVALID_INPUT;
        }
        if (depth == 0) return i;
        expecting = S_COMMA_OR_EOO;
        break;

      case S_KEY:
        if (c == '"') {
          int n = mjson_pass_string(&s[i + 1], len - i);
          if (n <= 0) return n;
          i += n + 1;
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
          // printf("AAAA %c %c\n", c, nesting[depth - 1] + 2);
          // In ascii table, the distance between `[` and `]` is 2.
          // Ditto for `{` and `}`. Hence +2 in the code below.
          if (c != nesting[depth - 1] + 2) return MJSON_ERROR_INVALID_INPUT;
          depth--;
          if (depth == 0) return i;
          expecting = (nesting[depth - 1] == '{') ? S_KEY : S_COMMA_OR_EOO;
        }
        break;
    }
    printf("%.*s\t%d %d\n", i - start + 1, &s[start], depth, expecting);
    // MJSONCALL(expecting);
  }
  return MJSON_ERROR_INVALID_INPUT;
}

#if 0
enum mjson_tok {
  MSJON_TOK_INVALID,
  MJSON_TOK_STR,
  MJSON_TOK_NUM,
  MJSON_TOK_TRUE,
  MJSON_TOK_FALSE,
  MJSON_TOK_NULL,
  MJSON_TOK_OBJ,
  MJSON_TOK_ARR,
  MJSON_TOK_COMMA,
  MJSON_TOK_COLON,
};

enum mjson_tok mjson_find(const char *s, int len, const char **ptr, int *n) {
}
#endif
