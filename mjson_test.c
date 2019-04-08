#include <assert.h>
#include <stdio.h>

#ifdef INCLUDE_MJSON_C
#include "mjson.c"
#else
#include "mjson.h"
#endif

static void test_cb(void) {
  const char *str;
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

  str = "\"abc\"";
  assert(mjson(str, 0, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson(str, 1, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson(str, 2, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson(str, 3, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson(str, 4, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson(str, 5, NULL, NULL) == 5);

  str = "{\"a\":1}";
  assert(mjson(str, 0, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson(str, 1, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson(str, 2, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson(str, 3, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson(str, 4, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson(str, 5, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson(str, 6, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  assert(mjson(str, 7, NULL, NULL) == 7);

  str = "{\"a\":[]}";
  assert(mjson(str, 8, NULL, NULL) == 8);
  str = "{\"a\":{}}";
  assert(mjson(str, 8, NULL, NULL) == 8);
  assert(mjson("[]", 2, NULL, NULL) == 2);
  assert(mjson("{}", 2, NULL, NULL) == 2);
  assert(mjson("[[]]", 4, NULL, NULL) == 4);
  assert(mjson("[[],[]]", 7, NULL, NULL) == 7);
  assert(mjson("[{}]", 4, NULL, NULL) == 4);
  assert(mjson("[{},{}]", 7, NULL, NULL) == 7);
  str = "{\"a\":[{}]}";
  assert(mjson(str, 10, NULL, NULL) == 10);

  assert(mjson("]", 1, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
}

static void test_find(void) {
  const char *p, *str;
  int n;
  assert(mjson_find("", 0, "", &p, &n) == MJSON_TOK_INVALID);
  assert(mjson_find("", 0, "$", &p, &n) == MJSON_TOK_INVALID);
  assert(mjson_find("123", 3, "$", &p, &n) == MJSON_TOK_NUMBER);
  assert(n == 3 && memcmp(p, "123", 3) == 0);
  str = "{\"a\":true}";
  assert(mjson_find(str, 10, "$.a", &p, &n) == MJSON_TOK_TRUE);
  assert(n == 4 && memcmp(p, "true", 4) == 0);
  str = "{\"a\":{\"c\":null},\"c\":2}";
  assert(mjson_find(str, 22, "$.c", &p, &n) == MJSON_TOK_NUMBER);
  assert(n == 1 && memcmp(p, "2", 1) == 0);
  str = "{\"a\":{\"c\":null},\"c\":2}";
  assert(mjson_find(str, 22, "$.a.c", &p, &n) == MJSON_TOK_NULL);
  assert(n == 4 && memcmp(p, "null", 4) == 0);
  str = "{\"a\":[1,null]}";
  assert(mjson_find(str, 15, "$.a", &p, &n) == '[');
  assert(n == 8 && memcmp(p, "[1,null]", 8) == 0);
  str = "{\"a\":{\"b\":7}}";
  assert(mjson_find(str, 14, "$.a", &p, &n) == '{');
  str = "{\"b\":7}";
  assert(n == 7 && memcmp(p, str, 7) == 0);
}

static void test_get_number(void) {
  const char *str;
  assert(mjson_get_number("", 0, "$", 123) == 123);
  assert(mjson_get_number("234", 3, "$", 123) == 234);
  str = "{\"a\":-7}";
  assert(mjson_get_number(str, 8, "$.a", 123) == -7);
  str = "{\"a\":1.2e3}";
  assert(mjson_get_number(str, 11, "$.a", 123) == 1.2e3);
  assert(mjson_get_number("[1.23,-43.47,17]", 16, "$", 42) == 42);
  assert(mjson_get_number("[1.23,-43.47,17]", 16, "$[0]", 42) == 1.23);
  assert(mjson_get_number("[1.23,-43.47,17]", 16, "$[1]", 42) == -43.47);
  assert(mjson_get_number("[1.23,-43.47,17]", 16, "$[2]", 42) == 17);
  assert(mjson_get_number("[1.23,-43.47,17]", 16, "$[3]", 42) == 42);
  {
    const char *s = "{\"a1\":[1,2,{\"a2\":4},[],{}],\"a\":3}";
    assert(mjson_get_number(s, strlen(s), "$.a", 0) == 3);
  }
  {
    const char *s = "[1,{\"a\":2}]";
    assert(mjson_get_number(s, strlen(s), "$[0]", 0) == 1);
    assert(mjson_get_number(s, strlen(s), "$[1].a", 0) == 2);
  }
  assert(mjson_get_number("[[2,1]]", 7, "$[0][1]", 42) == 1);
  assert(mjson_get_number("[[2,1]]", 7, "$[0][0]", 42) == 2);
  assert(mjson_get_number("[[2,[]]]", 8, "$[0][0]", 42) == 2);
  assert(mjson_get_number("[1,[2,[]]]", 10, "$[1][0]", 42) == 2);
  assert(mjson_get_number("[{},1]", 6, "$[1]", 42) == 1);
  assert(mjson_get_number("[[],1]", 6, "$[1]", 42) == 1);
  assert(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[0]", 42) == 1);
  assert(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[1]", 42) == 42);
  assert(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[1][0]", 42) == 2);
  assert(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[1][2]", 42) == 3);
  assert(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[1][3][0]", 42) == 4);
  assert(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[1][3][1]", 42) == 5);
  assert(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[1][3][2]", 42) == 42);
  assert(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[1][3][2][0]", 3) == 3);

  str = "[1,2,{\"a\":[3,4]}]";
  assert(mjson_get_number(str, 17, "$[1]", 3) == 2);
  str = "[1,2,{\"a\":[3,4]}]";
  assert(mjson_get_number(str, 17, "$[2].a[0]", 11) == 3);
  str = "[1,2,{\"a\":[3,4]}]";
  assert(mjson_get_number(str, 17, "$[2].a[1]", 11) == 4);
  str = "[1,2,{\"a\":[3,4]}]";
  assert(mjson_get_number(str, 17, "$[2].a[2]", 11) == 11);
  str = "{\"a\":3,\"ab\":2}";
  assert(mjson_get_number(str, 14, "$.ab", 0) == 2);
}

static void test_get_bool(void) {
  assert(mjson_get_bool("", 0, "$", 1) == 1);
  assert(mjson_get_bool("", 0, "$", 0) == 0);
  assert(mjson_get_bool("true", 4, "$", 0) == 1);
  assert(mjson_get_bool("false", 5, "$", 1) == 0);
}

static void test_get_string(void) {
  char buf[100];
  {
    const char *s = "{\"a\":\"f\too\"}";
    assert(mjson_get_string(s, strlen(s), "$.a", buf, sizeof(buf)) == 4);
    assert(strcmp(buf, "f\too") == 0);
  }

  {
    const char *s = "{\"ы\":\"превед\"}";
    assert(mjson_get_string(s, strlen(s), "$.ы", buf, sizeof(buf)) == 12);
    assert(strcmp(buf, "превед") == 0);
  }

  {
    const char *s = "[\"MA==\",\"MAo=\",\"MAr+\",\"MAr+Zw==\"]";
    assert(mjson_get_base64(s, strlen(s), "$[0]", buf, sizeof(buf)) == 1);
    assert(strcmp(buf, "0") == 0);
    assert(mjson_get_base64(s, strlen(s), "$[1]", buf, sizeof(buf)) == 2);
    assert(strcmp(buf, "0\n") == 0);
    assert(mjson_get_base64(s, strlen(s), "$[2]", buf, sizeof(buf)) == 3);
    assert(strcmp(buf, "0\n\xfe") == 0);
    assert(mjson_get_base64(s, strlen(s), "$[3]", buf, sizeof(buf)) == 4);
    assert(strcmp(buf, "0\n\xfeg") == 0);
  }
}

static void test_print(void) {
  char tmp[100];
  const char *str;

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_print_int(&out, -97, 1) == 3);
    assert(memcmp(tmp, "-97", 3) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_print_int(&out, -97, 0) == 10);
    assert(memcmp(tmp, "4294967199", 10) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, 3);
    assert(mjson_print_int(&out, -97, 1) == 3);
    assert(memcmp(tmp, "-97", 3) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, 2);
    assert(mjson_print_int(&out, -97, 1) == 2);
    assert(out.u.fixed_buf.overflow == 1);
  }

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_print_int(&out, 0, 1) == 1);
    assert(memcmp(tmp, "0", 1) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_print_int(&out, 12345678, 1) == 8);
    assert(memcmp(tmp, "12345678", 8) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_print_int(&out, (int) 3456789012, 0) == 10);
    assert(memcmp(tmp, "3456789012", 10) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_print_str(&out, "a", 1) == 3);
    str = "\"a\"";
    assert(memcmp(tmp, str, 3) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    const char *s = "a\b\n\f\r\t\"";
    assert(mjson_print_str(&out, s, 7) == 15);
    str = "\"a\\b\\n\\f\\r\\t\\\"\"";
    assert(memcmp(tmp, str, 15) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }
}

static int f1(struct mjson_out *out, va_list *ap) {
  int value = va_arg(*ap, int);
  return mjson_printf(out, "[%d]", value);
}

static void test_printf(void) {
  const char *str;
  {
    char tmp[20];
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_printf(&out, "%g", 0.123) == 5);
    assert(memcmp(tmp, "0.123", 5) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    char tmp[20];
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_printf(&out, "{%Q:%B}", "a", 1) == 10);
    str = "{\"a\":true}";
    assert(memcmp(tmp, str, 10) == 0);
    assert(out.u.fixed_buf.overflow == 0);
  }

  {
    char *s = NULL;
    struct mjson_out out = MJSON_OUT_DYNAMIC_BUF(&s);
    assert(mjson_printf(&out, "{%Q:%d, %Q:[%s]}", "a", 1, "b", "null") == 19);
    assert(s != NULL);
    str = "{\"a\":1, \"b\":[null]}";
    assert(memcmp(s, str, 19) == 0);
    free(s);
  }

  {
    char *s = NULL;
    struct mjson_out out = MJSON_OUT_DYNAMIC_BUF(&s);
    const char *fmt = "{\"a\":%d, \"b\":%u, \"c\":%ld, \"d\":%lu, \"e\":%M}";
    assert(mjson_printf(&out, fmt, -1, 3456789012, (long) -1,
                        (unsigned long) 3456789012, f1, 1234) == 60);
    assert(s != NULL);
    str =
        "{\"a\":-1, \"b\":3456789012, \"c\":-1, \"d\":3456789012, "
        "\"e\":[1234]}";
    assert(memcmp(s, str, 60) == 0);
    free(s);
  }

  {
    char *s = NULL;
    struct mjson_out out = MJSON_OUT_DYNAMIC_BUF(&s);
    assert(mjson_printf(&out, "[%.*Q,%.*s]", 2, "abc", 4, "truell") == 11);
    assert(s != NULL);
    str = "[\"ab\",true]";
    assert(memcmp(s, str, 11) == 0);
    free(s);
  }

  {
    char tmp[100], s[] = "0\n\xfeg";
    struct mjson_out out = MJSON_OUT_FIXED_BUF(tmp, sizeof(tmp));
    assert(mjson_printf(&out, "[%V,%V,%V,%V]", 1, s, 2, s, 3, s, 4, s) == 33);
    str = "[\"MA==\",\"MAo=\",\"MAr+\",\"MAr+Zw==\"]";
    assert(memcmp(tmp, str, 33) == 0);
    assert(out.u.fixed_buf.overflow == 0);
    // printf("%d [%.*s]\n", n, n, tmp);
  }

  {
    struct mjson_out out = MJSON_OUT_FILE(stdout);
    mjson_printf(&out, "{%Q:%Q}\n", "message", "well done, test passed");
  }
}

static void foo(struct jsonrpc_request *r) {
  double x = mjson_get_number(r->params, r->params_len, "$[1]", 0);
  jsonrpc_return_success(r, "{%Q:%g,%Q:%Q}", "x", x, "ud", r->userdata);
}

#define OUTLEN 200
static int sender(const char *buf, int len, void *privdata) {
  char *dst = (char *) privdata;
  snprintf(dst + strlen(dst), OUTLEN - strlen(dst), "%.*s", len, buf);
  return len;
}

static void process_str(struct jsonrpc_ctx *ctx, const char *str) {
  while (str && *str != '\0') jsonrpc_ctx_process_byte(ctx, *str++);
}

static void response_cb(const char *buf, int len, void *privdata) {
  snprintf((char *) privdata, OUTLEN, ">>%.*s<<", len, buf);
}

static void test_rpc(void) {
  struct jsonrpc_ctx *ctx = &jsonrpc_default_context;
  char out[OUTLEN + 1];

  // Init context
  jsonrpc_ctx_init(ctx, sender, response_cb, out);

  {
    // Call RPC.List
    out[0] = '\0';
    process_str(ctx, "{\"id\": 1, \"method\": \"RPC.List\"}\n");
    assert(strstr(out, "RPC.List") != NULL);
  }

  {
    // Call non-existent method
    out[0] = '\0';
    process_str(ctx, "{\"id\": 1, \"method\": \"foo\"}\n");
    assert(strstr(out, "-32601") != NULL);
  }

  {
    // Register our own function
    const char *req = "{\"id\": 2, \"method\": \"foo\",\"params\":[0,1.23]}\n";
    const char *reply = "{\"id\":2,\"result\":{\"x\":1.23,\"ud\":\"hi\"}}\n";
    jsonrpc_ctx_export(ctx, "foo", foo, (void *) "hi");
    out[0] = '\0';
    process_str(ctx, req);
    assert(strcmp(out, reply) == 0);
  }

  {
    // Test for bad frame
    char request[] = "boo\n";
    const char *reply = "{\"error\":{\"code\":-32700,\"message\":\"boo\"}}\n";
    out[0] = '\0';
    process_str(ctx, request);
    // printf("--> [%s]\n", out);
    assert(strcmp(out, reply) == 0);
  }

  {
    // Test notify
    const char *reply = "{\"method\":\"ping\"}\n";
    out[0] = '\0';
    jsonrpc_ctx_call(ctx, "{%Q:%Q}", "method", "ping");
    // printf("--> [%s]\n", out);
    assert(strcmp(out, reply) == 0);
  }

  {
    // Test call / response
    const char *expected = ">>{\"id\":123,\"result\":777}<<";
    out[0] = '\0';
    process_str(ctx, "{\"id\":123,\"result\":777}\n");
    // printf("--> [%s]\n", out);
    assert(strcmp(out, expected) == 0);
  }
}

int main() {
  test_printf();
  test_cb();
  test_find();
  test_get_number();
  test_get_bool();
  test_get_string();
  test_print();
  test_rpc();
  return 0;
}
