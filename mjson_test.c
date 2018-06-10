#include <assert.h>
#include <stdio.h>

#include "mjson.h"

static void test_cb(void) {
  {
    const char *s = "{\"a\": true, \"b\": [ null, 3 ]}";
    assert(mjson(s, strlen(s), NULL, NULL) == (int) strlen(s));
  }
  {
    const char *s = "[ 1, 2 ,  null, true,false,\"foo\"  ]";
    assert(mjson(s, strlen(s), NULL, NULL) == (int) strlen(s));
  }
  {
    const char *s = "123";
    assert(mjson(s, strlen(s), NULL, NULL) == (int) strlen(s));
  }
  {
    const char *s = "\"foo\"";
    assert(mjson(s, strlen(s), NULL, NULL) == (int) strlen(s));
  }
  {
    const char *s = "123 ";  // Trailing space
    assert(mjson(s, strlen(s), NULL, NULL) == (int) strlen(s) - 1);
  }
  {
    const char *s = "[[[[[[[[[[[[[[[[[[[[[";
    assert(mjson(s, strlen(s), NULL, NULL) == MJSON_ERROR_TOO_DEEP);
  }

  assert(mjson("\"abc\"", 0, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson("\"abc\"", 1, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson("\"abc\"", 2, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson("\"abc\"", 3, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson("\"abc\"", 4, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson("\"abc\"", 5, NULL, NULL) == 5);

  assert(mjson("{\"a\":1}", 0, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson("{\"a\":1}", 1, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson("{\"a\":1}", 2, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson("{\"a\":1}", 3, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson("{\"a\":1}", 4, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson("{\"a\":1}", 5, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson("{\"a\":1}", 6, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson("{\"a\":1}", 7, NULL, NULL) == 7);

  assert(mjson("{\"a\":[]}", 8, NULL, NULL) == 8);
  assert(mjson("{\"a\":{}}", 8, NULL, NULL) == 8);
  assert(mjson("[]", 2, NULL, NULL) == 2);
  assert(mjson("{}", 2, NULL, NULL) == 2);
  assert(mjson("[[]]", 4, NULL, NULL) == 4);
  assert(mjson("[[],[]]", 7, NULL, NULL) == 7);
  assert(mjson("[{}]", 4, NULL, NULL) == 4);
  assert(mjson("[{},{}]", 7, NULL, NULL) == 7);
  assert(mjson("{\"a\":[{}]}", 10, NULL, NULL) == 10);
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
  assert(mjson_find("{\"a\":{\"c\":null},\"c\":2}", 22, "$.c", &p, &n) ==
         MJSON_TOK_NUMBER);
  assert(n == 1 && memcmp(p, "2", 1) == 0);
  assert(mjson_find("{\"a\":{\"c\":null},\"c\":2}", 22, "$.a.c", &p, &n) ==
         MJSON_TOK_NULL);
  assert(n == 4 && memcmp(p, "null", 4) == 0);
  assert(mjson_find("{\"a\":[1,null]}", 15, "$.a", &p, &n) == '[');
  assert(n == 8 && memcmp(p, "[1,null]", 8) == 0);
  assert(mjson_find("{\"a\":{\"b\":7}}", 14, "$.a", &p, &n) == '{');
  assert(n == 7 && memcmp(p, "{\"b\":7}", 7) == 0);
}

static void test_find_number(void) {
  assert(mjson_find_number("", 0, "$", 123) == 123);
  assert(mjson_find_number("234", 3, "$", 123) == 234);
  assert(mjson_find_number("{\"a\":-7}", 8, "$.a", 123) == -7);
  assert(mjson_find_number("{\"a\":1.2e3}", 11, "$.a", 123) == 1.2e3);
  assert(mjson_find_number("[1.23,-43.47,17]", 16, "$", 42) == 42);
  assert(mjson_find_number("[1.23,-43.47,17]", 16, "$[0]", 42) == 1.23);
  assert(mjson_find_number("[1.23,-43.47,17]", 16, "$[1]", 42) == -43.47);
  assert(mjson_find_number("[1.23,-43.47,17]", 16, "$[2]", 42) == 17);
  assert(mjson_find_number("[1.23,-43.47,17]", 16, "$[3]", 42) == 42);
  {
    const char *s = "{\"a1\":[1,2,{\"a2\":4},[],{}],\"a\":3}";
    assert(mjson_find_number(s, strlen(s), "$.a", 0) == 3);
  }
  {
    const char *s = "[1,{\"a\":2}]";
    assert(mjson_find_number(s, strlen(s), "$[0]", 0) == 1);
    assert(mjson_find_number(s, strlen(s), "$[1].a", 0) == 2);
  }
  assert(mjson_find_number("[[2,1]]", 7, "$[0][1]", 42) == 1);
  assert(mjson_find_number("[[2,1]]", 7, "$[0][0]", 42) == 2);
  assert(mjson_find_number("[[2,[]]]", 8, "$[0][0]", 42) == 2);
  assert(mjson_find_number("[1,[2,[]]]", 10, "$[1][0]", 42) == 2);
  assert(mjson_find_number("[{},1]", 6, "$[1]", 42) == 1);
  assert(mjson_find_number("[[],1]", 6, "$[1]", 42) == 1);
  assert(mjson_find_number("[1,[2,[],3,[4,5]]]", 18, "$[0]", 42) == 1);
  assert(mjson_find_number("[1,[2,[],3,[4,5]]]", 18, "$[1]", 42) == 42);
  assert(mjson_find_number("[1,[2,[],3,[4,5]]]", 18, "$[1][0]", 42) == 2);
  assert(mjson_find_number("[1,[2,[],3,[4,5]]]", 18, "$[1][2]", 42) == 3);
  assert(mjson_find_number("[1,[2,[],3,[4,5]]]", 18, "$[1][3][0]", 42) == 4);
  assert(mjson_find_number("[1,[2,[],3,[4,5]]]", 18, "$[1][3][1]", 42) == 5);
  assert(mjson_find_number("[1,[2,[],3,[4,5]]]", 18, "$[1][3][2]", 42) == 42);
  assert(mjson_find_number("[1,[2,[],3,[4,5]]]", 18, "$[1][3][2][0]", 3) == 3);

  assert(mjson_find_number("[1,2,{\"a\":[3,4]}]", 17, "$[1]", 3) == 2);
  assert(mjson_find_number("[1,2,{\"a\":[3,4]}]", 17, "$[2].a[0]", 11) == 3);
  assert(mjson_find_number("[1,2,{\"a\":[3,4]}]", 17, "$[2].a[1]", 11) == 4);
  assert(mjson_find_number("[1,2,{\"a\":[3,4]}]", 17, "$[2].a[2]", 11) == 11);
}

static void test_find_bool(void) {
  assert(mjson_find_bool("", 0, "$", 1) == 1);
  assert(mjson_find_bool("", 0, "$", 0) == 0);
  assert(mjson_find_bool("true", 4, "$", 0) == 1);
  assert(mjson_find_bool("false", 5, "$", 1) == 0);
}

static void test_find_string(void) {
  char buf[100];
  {
    const char *s = "{\"a\":\"f\too\"}";
    assert(mjson_find_string(s, strlen(s), "$.a", buf, sizeof(buf)) == 4);
    assert(strcmp(buf, "f\too") == 0);
  }

  {
    const char *s = "{\"ы\":\"превед\"}";
    assert(mjson_find_string(s, strlen(s), "$.ы", buf, sizeof(buf)) == 12);
    assert(strcmp(buf, "превед") == 0);
  }

  {
    const char *s = "[\"MA==\",\"MAo=\",\"MAr+\",\"MAr+Zw==\"]";
    assert(mjson_find_base64(s, strlen(s), "$[0]", buf, sizeof(buf)) == 1);
    assert(strcmp(buf, "0") == 0);
    assert(mjson_find_base64(s, strlen(s), "$[1]", buf, sizeof(buf)) == 2);
    assert(strcmp(buf, "0\n") == 0);
    assert(mjson_find_base64(s, strlen(s), "$[2]", buf, sizeof(buf)) == 3);
    assert(strcmp(buf, "0\n\xfe") == 0);
    assert(mjson_find_base64(s, strlen(s), "$[3]", buf, sizeof(buf)) == 4);
    assert(strcmp(buf, "0\n\xfeg") == 0);
  }
}

static void test_print(void) {
  char tmp[100];

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_print_int(&out, -97) == 3);
    assert(memcmp(tmp, "-97", 3) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, 3);
    assert(mjson_print_int(&out, -97) == 3);
    assert(memcmp(tmp, "-97", 3) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, 2);
    assert(mjson_print_int(&out, -97) == 2);
    assert(out.u.fixed_buf.overflow == 1);
  }

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_print_int(&out, 0) == 1);
    assert(memcmp(tmp, "0", 1) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_print_int(&out, 12345678) == 8);
    assert(memcmp(tmp, "12345678", 8) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_print_str(&out, "a", 1) == 3);
    assert(memcmp(tmp, "\"a\"", 3) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    const char *s = "a\b\n\f\r\t\"";
    assert(mjson_print_str(&out, s, 7) == 15);
    assert(memcmp(tmp, "\"a\\b\\n\\f\\r\\t\\\"\"", 15) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }
}

static int f1(struct mjson_out *out, va_list *ap) {
  int value = va_arg(*ap, int);
  return mjson_printf(out, "[%d]", value);
}

static void test_printf(void) {
  {
    char tmp[20];
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    // int n = mjson_printf(&out, "%f", 0.123);
    // printf("%d [%.*s]\n", n, n, tmp);
    assert(mjson_printf(&out, "%f", 0.123) == 5);
    assert(memcmp(tmp, "0.123", 5) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    char tmp[20];
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_printf(&out, "{%Q:%B}", "a", 1) == 10);
    assert(memcmp(tmp, "{\"a\":true}", 10) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    char *s = NULL;
    struct mjson_out out = MJSON_OUT_DYNAMIC_BUF(&s);
    assert(mjson_printf(&out, "{%Q:%d, %Q:[%s]}", "a", 1, "b", "null") == 19);
    assert(s != NULL);
    assert(memcmp(s, "{\"a\":1, \"b\":[null]}", 19) == 0);
    free(s);
  }

  {
    char *s = NULL;
    struct mjson_out out = MJSON_OUT_DYNAMIC_BUF(&s);
    assert(mjson_printf(&out, "{%Q:%d, %Q:%M}", "a", 1, "b", f1, 1234) == 19);
    assert(s != NULL);
    assert(memcmp(s, "{\"a\":1, \"b\":[1234]}", 19) == 0);
    free(s);
  }

  {
    char *s = NULL;
    struct mjson_out out = MJSON_OUT_DYNAMIC_BUF(&s);
    assert(mjson_printf(&out, "[%.*Q,%.*s]", 2, "abc", 4, "truell") == 11);
    assert(s != NULL);
    assert(memcmp(s, "[\"ab\",true]", 11) == 0);
    free(s);
  }

  {
    char tmp[100], s[] = "0\n\xfeg";
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_printf(&out, "[%V,%V,%V,%V]", 1, s, 2, s, 3, s, 4, s) == 33);
    assert(memcmp(tmp, "[\"MA==\",\"MAo=\",\"MAr+\",\"MAr+Zw==\"]", 33) == 0);
    assert(out.u.fixed_buf.overflow == 0);
    // printf("%d [%.*s]\n", n, n, tmp);
  }

  {
    struct mjson_out out = MJSON_OUT_FILE(stdout);
    mjson_printf(&out, "{%Q:%Q}\n", "message", "well done, test passed");
  }
}

int main() {
  test_cb();
  test_find();
  test_find_number();
  test_find_bool();
  test_find_string();
  test_print();
  test_printf();
  return 0;
}
