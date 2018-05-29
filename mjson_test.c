#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "mjson.h"

static void cb(int ev, const char *s, int len, void *ud) {
  // const char *states[] = {"value", "literal", "string", "escape", "utf8"};
  // const char *actions[] = {"none", "start", "end", "start_s", "end_s"};
  // printf("%s\t%s\t[%.*s]\n", states[st], actions[ac], len, s);
  // const char *names[] = {"value", "key", "colon", "comma"};
  // printf("%d\t[%.*s]\n", ev, len, s);
  (void) s;
  (void) len;
  (void) ev;
  (void) ud;
}

static void test_cb(void) {
  {
    const char *s = "{\"a\": true, \"b\": [ null, 3 ]}";
    assert(mjson(s, strlen(s), cb, NULL) == (int) strlen(s));
  }
  {
    const char *s = "[ 1, 2 ,  null, true,false,\"foo\"  ]";
    assert(mjson(s, strlen(s), cb, NULL) == (int) strlen(s));
  }
  {
    const char *s = "123";
    assert(mjson(s, strlen(s), cb, NULL) == (int) strlen(s));
  }
  {
    const char *s = "\"foo\"";
    assert(mjson(s, strlen(s), cb, NULL) == (int) strlen(s));
  }
  {
    const char *s = "123 ";  // Trailing space
    assert(mjson(s, strlen(s), cb, NULL) == (int) strlen(s) - 1);
  }
  {
    const char *s = "[[[[[[[[[[[[[[[[[[[[[";
    assert(mjson(s, strlen(s), cb, NULL) == MJSON_ERROR_TOO_DEEP);
  }
}

static void test_find(void) {
  const char *p;
  int n;
  assert(mjson_find("", 0, "", &p, &n) == MJSON_TOK_INVALID);
  assert(mjson_find("", 0, "$", &p, &n) == MJSON_TOK_INVALID);
  assert(mjson_find("123", 3, "$", &p, &n) == MJSON_TOK_NUMBER);
  assert(n == 3 && memcmp(p, "123", 3) == 0);
  assert(mjson_find("{\"a\":true}", 10, "$.a", &p, &n) == MJSON_TOK_TRUE);
  assert(n == 4 && memcmp(p, "true", 4) == 0);
  // assert(mjson_find("{\"a\":{\"b\":1},\"c\":2}", 19, "$.c", &p, &n) ==
  //        MJSON_TOK_NUMBER);
}

int main() {
  test_cb();
  test_find();
  return 0;
}
