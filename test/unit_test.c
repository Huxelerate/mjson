#include <stdio.h>
#include <stdlib.h>

#define MJSON_ENABLE_PRETTY 1
#define MJSON_ENABLE_MERGE 1
#include "mjson.h"

static int s_num_tests = 0;
static int s_num_errors = 0;

#define ASSERT(expr)                                         \
  do {                                                       \
    s_num_tests++;                                           \
    if (!(expr)) {                                           \
      s_num_errors++;                                        \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
    }                                                        \
  } while (0)

static void test_cb(void) {
  const char *str;
  {
    const char *s = "{\"a\": true, \"b\": [ null, 3 ]}";
    ASSERT(mjson(s, strlen(s), NULL, NULL) == (int) strlen(s));
  }
  {
    const char *s = "[ 1, 2 ,  null, true,false,\"foo\"  ]";
    ASSERT(mjson(s, strlen(s), NULL, NULL) == (int) strlen(s));
  }
  {
    const char *s = "123";
    ASSERT(mjson(s, strlen(s), NULL, NULL) == (int) strlen(s));
  }
  {
    const char *s = "\"foo\"";
    ASSERT(mjson(s, strlen(s), NULL, NULL) == (int) strlen(s));
  }
  {
    const char *s = "123 ";  // Trailing space
    ASSERT(mjson(s, strlen(s), NULL, NULL) == (int) strlen(s) - 1);
  }
  {
    const char *s = "[[[[[[[[[[[[[[[[[[[[[";
    ASSERT(mjson(s, strlen(s), NULL, NULL) == MJSON_ERROR_TOO_DEEP);
  }

  str = "\"abc\"";
  ASSERT(mjson(str, 0, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  ASSERT(mjson(str, 1, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  ASSERT(mjson(str, 2, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  ASSERT(mjson(str, 3, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  ASSERT(mjson(str, 4, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  ASSERT(mjson(str, 5, NULL, NULL) == 5);

  str = "{\"a\":1}";
  ASSERT(mjson(str, 0, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  ASSERT(mjson(str, 1, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  ASSERT(mjson(str, 2, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  ASSERT(mjson(str, 3, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  ASSERT(mjson(str, 4, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  ASSERT(mjson(str, 5, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  ASSERT(mjson(str, 6, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
  ASSERT(mjson(str, 7, NULL, NULL) == 7);

  str = "{\"a\":[]}";
  ASSERT(mjson(str, 8, NULL, NULL) == 8);
  str = "{\"a\":{}}";
  ASSERT(mjson(str, 8, NULL, NULL) == 8);
  ASSERT(mjson("[]", 2, NULL, NULL) == 2);
  ASSERT(mjson("{}", 2, NULL, NULL) == 2);
  ASSERT(mjson("[[]]", 4, NULL, NULL) == 4);
  ASSERT(mjson("[[],[]]", 7, NULL, NULL) == 7);
  ASSERT(mjson("[{}]", 4, NULL, NULL) == 4);
  ASSERT(mjson("[{},{}]", 7, NULL, NULL) == 7);
  str = "{\"a\":[{}]}";
  ASSERT(mjson(str, 10, NULL, NULL) == 10);

  ASSERT(mjson("]", 1, NULL, NULL) == MJSON_ERROR_INVALID_INPUT);
}

static void test_find(void) {
  const char *p, *str;
  int n;
  ASSERT(mjson_find("", 0, "", &p, &n) == MJSON_TOK_INVALID);
  ASSERT(mjson_find("", 0, "$", &p, &n) == MJSON_TOK_INVALID);
  ASSERT(mjson_find("123", 3, "$", &p, &n) == MJSON_TOK_NUMBER);
  ASSERT(n == 3 && memcmp(p, "123", 3) == 0);
  str = "{\"a\":true}";
  ASSERT(mjson_find(str, 10, "$.a", &p, &n) == MJSON_TOK_TRUE);
  ASSERT(n == 4 && memcmp(p, "true", 4) == 0);
  str = "{\"a\":{\"c\":null},\"c\":2}";
  ASSERT(mjson_find(str, 22, "$.c", &p, &n) == MJSON_TOK_NUMBER);
  ASSERT(n == 1 && memcmp(p, "2", 1) == 0);
  str = "{\"a\":{\"c\":null},\"c\":2}";
  ASSERT(mjson_find(str, 22, "$.a.c", &p, &n) == MJSON_TOK_NULL);
  ASSERT(n == 4 && memcmp(p, "null", 4) == 0);
  str = "{\"a\":[1,null]}";
  ASSERT(mjson_find(str, 15, "$.a", &p, &n) == '[');
  ASSERT(n == 8 && memcmp(p, "[1,null]", 8) == 0);
  str = "{\"a\":{\"b\":7}}";
  ASSERT(mjson_find(str, 14, "$.a", &p, &n) == '{');
  str = "{\"b\":7}";
  ASSERT(n == 7 && memcmp(p, str, 7) == 0);

  // Test the shortcut: as soon as we find an element, stop the traversal
  str = "{\"a\":[1,2,garbage here!!";
  ASSERT(mjson_find(str, strlen(str), "$.a[0]", &p, &n) == MJSON_TOK_NUMBER);
  ASSERT(mjson_find(str, strlen(str), "$.a[1]", &p, &n) == MJSON_TOK_NUMBER);
  ASSERT(mjson_find(str, strlen(str), "$.a[2]", &p, &n) == MJSON_TOK_INVALID);
}

static void test_get_number(void) {
  const char *str;
  double v;
  ASSERT(mjson_get_number("", 0, "$", &v) == 0);
  ASSERT(mjson_get_number("234", 3, "$", &v) == 1 && v == 234);
  str = "{\"a\":-7}";
  ASSERT(mjson_get_number(str, 8, "$.a", &v) == 1 && v == -7);
  str = "{\"a\":1.2e3}";
  ASSERT(mjson_get_number(str, 11, "$.a", &v) == 1 && v == 1.2e3);
  ASSERT(mjson_get_number("[1.23,-43.47,17]", 16, "$", &v) == 0);
  ASSERT(mjson_get_number("[1.23,-43.47,17]", 16, "$[0]", &v) == 1 &&
         v == 1.23);
  ASSERT(mjson_get_number("[1.23,-43.47,17]", 16, "$[1]", &v) == 1 &&
         v == -43.47);
  ASSERT(mjson_get_number("[1.23,-43.47,17]", 16, "$[2]", &v) == 1 && v == 17);
  ASSERT(mjson_get_number("[1.23,-43.47,17]", 16, "$[3]", &v) == 0);
  {
    const char *s = "{\"a1\":[1,2,{\"a2\":4},[],{}],\"a\":3}";
    ASSERT(mjson_get_number(s, strlen(s), "$.a", &v) == 1 && v == 3);
  }
  {
    const char *s = "[1,{\"a\":2}]";
    ASSERT(mjson_get_number(s, strlen(s), "$[0]", &v) == v && v == 1);
    ASSERT(mjson_get_number(s, strlen(s), "$[1].a", &v) == 1 && v == 2);
  }
  ASSERT(mjson_get_number("[[2,1]]", 7, "$[0][1]", &v) == 1 && v == 1);
  ASSERT(mjson_get_number("[[2,1]]", 7, "$[0][0]", &v) == 1 && v == 2);
  ASSERT(mjson_get_number("[[2,[]]]", 8, "$[0][0]", &v) == 1 && v == 2);
  ASSERT(mjson_get_number("[1,[2,[]]]", 10, "$[1][0]", &v) == 1 && v == 2);
  ASSERT(mjson_get_number("[{},1]", 6, "$[1]", &v) == 1 && v == 1);
  ASSERT(mjson_get_number("[[],1]", 6, "$[1]", &v) == 1 && v == 1);
  ASSERT(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[0]", &v) == 1 && v == 1);
  ASSERT(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[1]", &v) == 0);
  ASSERT(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[1][0]", &v) == 1 &&
         v == 2);
  ASSERT(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[1][2]", &v) == 1 &&
         v == 3);
  ASSERT(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[1][3][0]", &v) == 1 &&
         v == 4);
  ASSERT(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[1][3][1]", &v) == 1 &&
         v == 5);
  ASSERT(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[1][3][2]", &v) == 0);
  ASSERT(mjson_get_number("[1,[2,[],3,[4,5]]]", 18, "$[1][3][2][0]", &v) == 0);

  str = "[1,2,{\"a\":[3,4]}]";
  ASSERT(mjson_get_number(str, 17, "$[1]", &v) == 1 && v == 2);
  str = "[1,2,{\"a\":[3,4]}]";
  ASSERT(mjson_get_number(str, 17, "$[2].a[0]", &v) == 1 && v == 3);
  str = "[1,2,{\"a\":[3,4]}]";
  ASSERT(mjson_get_number(str, 17, "$[2].a[1]", &v) == 1 && v == 4);
  str = "[1,2,{\"a\":[3,4]}]";
  ASSERT(mjson_get_number(str, 17, "$[2].a[2]", &v) == 0);
  str = "{\"a\":3,\"ab\":2}";
  ASSERT(mjson_get_number(str, 14, "$.ab", &v) == 1 && v == 2);
}

static void test_get_bool(void) {
  const char *s = "{\"state\":{\"lights\":true,\"version\":36,\"a\":false}}";
  double x;
  int v;
  ASSERT(mjson_get_bool("", 0, "$", &v) == 0);
  ASSERT(mjson_get_bool("true", 4, "$", &v) == 1 && v == 1);
  ASSERT(mjson_get_bool("false", 5, "$", &v) == 1 && v == 0);
  ASSERT(mjson_get_number(s, strlen(s), "$.state.version", &x) == 1 && x == 36);
  ASSERT(mjson_get_bool(s, strlen(s), "$.state.a", &v) == 1 && v == 0);
  ASSERT(mjson_get_bool(s, strlen(s), "$.state.lights", &v) == 1 && v == 1);
}

static void test_get_string(void) {
  char buf[100];
  {
    const char *s = "{\"a\":\"f\too\"}";
    ASSERT(mjson_get_string(s, strlen(s), "$.a", buf, sizeof(buf)) == 4);
    ASSERT(strcmp(buf, "f\too") == 0);
  }

  {
    const char *s = "{\"ы\":\"превед\"}";
    ASSERT(mjson_get_string(s, strlen(s), "$.ы", buf, sizeof(buf)) == 12);
    ASSERT(strcmp(buf, "превед") == 0);
  }

  {
    const char *s = "{\"a\":{\"x\":\"X\"},\"b\":{\"q\":\"Y\"}}";
    ASSERT(mjson_get_string(s, strlen(s), "$.a.x", buf, sizeof(buf)) == 1);
    ASSERT(strcmp(buf, "X") == 0);
    ASSERT(mjson_find(s, strlen(s), "$.a.q", NULL, NULL) == MJSON_TOK_INVALID);
    ASSERT(mjson_get_string(s, strlen(s), "$.a.q", buf, sizeof(buf)) < 0);
    // printf("-->[%s]\n", buf);
    ASSERT(mjson_get_string(s, strlen(s), "$.b.q", buf, sizeof(buf)) == 1);
    ASSERT(strcmp(buf, "Y") == 0);
  }

  {
    struct {
      char buf1[3], buf2[3];
    } foo;
    const char *s = "{\"a\":\"0123456789\"}";
    memset(&foo, 0, sizeof(foo));
    ASSERT(mjson_get_string(s, strlen(s), "$.a", foo.buf1, sizeof(foo.buf1)) ==
           -1);
    ASSERT(foo.buf1[0] == '0' && foo.buf1[2] == '2');
    ASSERT(foo.buf2[0] == '\0');
  }

  {
    const char *s = "[\"MA==\",\"MAo=\",\"MAr+\",\"MAr+Zw==\"]";
    ASSERT(mjson_get_base64(s, strlen(s), "$[0]", buf, sizeof(buf)) == 1);
    ASSERT(strcmp(buf, "0") == 0);
    ASSERT(mjson_get_base64(s, strlen(s), "$[1]", buf, sizeof(buf)) == 2);
    ASSERT(strcmp(buf, "0\n") == 0);
    ASSERT(mjson_get_base64(s, strlen(s), "$[2]", buf, sizeof(buf)) == 3);
    ASSERT(strcmp(buf, "0\n\xfe") == 0);
    ASSERT(mjson_get_base64(s, strlen(s), "$[3]", buf, sizeof(buf)) == 4);
    ASSERT(strcmp(buf, "0\n\xfeg") == 0);
  }

  {
    const char *s = "[\"200a\",\"fe31\",123]";
    ASSERT(mjson_get_hex(s, strlen(s), "$[0]", buf, sizeof(buf)) == 2);
    ASSERT(strcmp(buf, " \n") == 0);
    ASSERT(mjson_get_hex(s, strlen(s), "$[1]", buf, sizeof(buf)) == 2);
    ASSERT(strcmp(buf, "\xfe\x31") == 0);
    ASSERT(mjson_get_hex(s, strlen(s), "$[2]", buf, sizeof(buf)) < 0);
  }

  {
    const char *s = "[1,2]";
    double dv;
    ASSERT(mjson_get_number(s, strlen(s), "$[0]", &dv) == 1 && dv == 1);
    ASSERT(mjson_get_number(s, strlen(s), "$[1]", &dv) == 1 && dv == 2);
    ASSERT(mjson_get_number(s, strlen(s), "$[3]", &dv) == 0);
  }
}

static void test_print(void) {
  char tmp[100];
  const char *str;

  {
    struct mjson_fixedbuf fb = {tmp, sizeof(tmp), 0};
    ASSERT(mjson_print_int(&mjson_print_fixed_buf, &fb, -97, 1) == 3);
    ASSERT(memcmp(tmp, "-97", 3) == 0);
    ASSERT(fb.len < fb.size);
  }

  {
    struct mjson_fixedbuf fb = {tmp, sizeof(tmp), 0};
    ASSERT(mjson_print_int(&mjson_print_fixed_buf, &fb, -97, 0) == 10);
    ASSERT(memcmp(tmp, "4294967199", 10) == 0);
    ASSERT(fb.len < fb.size);
  }

  {
    struct mjson_fixedbuf fb = {tmp, sizeof(tmp), 0};
    ASSERT(mjson_print_int(&mjson_print_fixed_buf, &fb, -97, 1) == 3);
    ASSERT(memcmp(tmp, "-97", 3) == 0);
    ASSERT(fb.len < fb.size);
  }

  {
    struct mjson_fixedbuf fb = {tmp, 2, 0};
    ASSERT(mjson_print_int(&mjson_print_fixed_buf, &fb, -97, 1) == 1);
    ASSERT(fb.len == fb.size - 1);
  }

  {
    struct mjson_fixedbuf fb = {tmp, sizeof(tmp), 0};
    ASSERT(mjson_print_int(&mjson_print_fixed_buf, &fb, 0, 1) == 1);
    ASSERT(memcmp(tmp, "0", 1) == 0);
    ASSERT(fb.len < fb.size);
  }

  {
    struct mjson_fixedbuf fb = {tmp, sizeof(tmp), 0};
    ASSERT(mjson_print_int(&mjson_print_fixed_buf, &fb, 12345678, 1) == 8);
    ASSERT(memcmp(tmp, "12345678", 8) == 0);
    ASSERT(fb.len < fb.size);
  }

  {
    struct mjson_fixedbuf fb = {tmp, sizeof(tmp), 0};
    ASSERT(mjson_print_int(&mjson_print_fixed_buf, &fb, (int) 3456789012, 0) ==
           10);
    ASSERT(memcmp(tmp, "3456789012", 10) == 0);
    ASSERT(fb.len < fb.size);
  }

  {
    struct mjson_fixedbuf fb = {tmp, sizeof(tmp), 0};
    ASSERT(mjson_print_str(&mjson_print_fixed_buf, &fb, "a", 1) == 3);
    str = "\"a\"";
    ASSERT(memcmp(tmp, str, 3) == 0);
    ASSERT(fb.len < fb.size);
  }

  {
    struct mjson_fixedbuf fb = {tmp, sizeof(tmp), 0};
    const char *s = "a\b\n\f\r\t\"";
    ASSERT(mjson_print_str(&mjson_print_fixed_buf, &fb, s, 7) == 15);
    str = "\"a\\b\\n\\f\\r\\t\\\"\"";
    ASSERT(memcmp(tmp, str, 15) == 0);
    ASSERT(fb.len < fb.size);
  }
}

static int f1(mjson_print_fn_t fn, void *fndata, va_list *ap) {
  int value = va_arg(*ap, int);
  return mjson_printf(fn, fndata, "[%d]", value);
}

static void test_printf(void) {
  const char *str;
  {
    char tmp[20];
    struct mjson_fixedbuf fb = {tmp, sizeof(tmp), 0};
    ASSERT(mjson_printf(&mjson_print_fixed_buf, &fb, "%g", 0.123) == 5);
    ASSERT(memcmp(tmp, "0.123", 5) == 0);
    ASSERT(fb.len < fb.size);
  }

  {
    char tmp[20];
    struct mjson_fixedbuf fb = {tmp, sizeof(tmp), 0};
    ASSERT(mjson_printf(&mjson_print_fixed_buf, &fb, "{%Q:%B}", "a", 1) == 10);
    str = "{\"a\":true}";
    ASSERT(memcmp(tmp, str, 10) == 0);
    ASSERT(fb.len < fb.size);
  }

  {
    char tmp[20];
    struct mjson_fixedbuf fb = {tmp, sizeof(tmp), 0};
    str = "{\"a\":\"\"}";
    ASSERT(mjson_printf(&mjson_print_fixed_buf, &fb, "{%Q:%Q}", "a", NULL) ==
           (int) strlen(str));
    ASSERT(memcmp(tmp, str, strlen(str)) == 0);
    ASSERT(fb.len < fb.size);
  }

  {
    char *s = NULL;
    ASSERT(mjson_printf(&mjson_print_dynamic_buf, &s, "{%Q:%d, %Q:[%s]}", "a",
                        1, "b", "null") == 19);
    ASSERT(s != NULL);
    str = "{\"a\":1, \"b\":[null]}";
    ASSERT(memcmp(s, str, 19) == 0);
    free(s);
  }

  {
    char *s = NULL;
    const char *fmt = "{\"a\":%d, \"b\":%u, \"c\":%ld, \"d\":%lu, \"e\":%M}";
    ASSERT(mjson_printf(&mjson_print_dynamic_buf, &s, fmt, -1, 3456789012,
                        (long) -1, (unsigned long) 3456789012, f1, 1234) == 60);
    ASSERT(s != NULL);
    str =
        "{\"a\":-1, \"b\":3456789012, \"c\":-1, \"d\":3456789012, "
        "\"e\":[1234]}";
    ASSERT(memcmp(s, str, 60) == 0);
    free(s);
  }

  {
    char *s = NULL;
    ASSERT(mjson_printf(&mjson_print_dynamic_buf, &s, "[%.*Q,%.*s]", 2, "abc",
                        4, "truell") == 11);
    ASSERT(s != NULL);
    str = "[\"ab\",true]";
    ASSERT(memcmp(s, str, 11) == 0);
    free(s);
  }

  {
    char tmp[100], s[] = "0\n\xfeg";
    struct mjson_fixedbuf fb = {tmp, sizeof(tmp), 0};
    ASSERT(mjson_printf(&mjson_print_fixed_buf, &fb, "[%V,%V,%V,%V]", 1, s, 2,
                        s, 3, s, 4, s) == 33);
    str = "[\"MA==\",\"MAo=\",\"MAr+\",\"MAr+Zw==\"]";
    ASSERT(memcmp(tmp, str, 33) == 0);
    ASSERT(fb.len < fb.size);
    // printf("%d [%.*s]\n", n, n, tmp);
  }

  {
    int n = mjson_printf(&mjson_print_null, 0, "{%Q:%d}", "a", 1);
    ASSERT(n == 7);
  }

  {
    int n = mjson_printf(&mjson_print_file, stdout, "{%Q:%Q}\n", "message",
                         "well done, test passed");
    ASSERT(n == 37);
  }

  {
    char tmp[100] = "", s[] = "\"002001200220616263\"";
    struct mjson_fixedbuf fb = {tmp, sizeof(tmp), 0};
    ASSERT(mjson_printf(&mjson_print_fixed_buf, &fb, "%H", 9,
                        "\x00 \x01 \x02 abc") == 20);
    ASSERT(strcmp(tmp, s) == 0);
  }

  {
    char tmp[100] = "", s[] = "\"a/b\\nc\"";
    struct mjson_fixedbuf fb = {tmp, sizeof(tmp), 0};
    ASSERT(mjson_printf(&mjson_print_fixed_buf, &fb, "%Q", "a/b\nc") == 8);
    ASSERT(strcmp(tmp, s) == 0);
  }
}

static void foo(struct jsonrpc_request *r) {
  double v = 0;
  mjson_get_number(r->params, r->params_len, "$[1]", &v);
  jsonrpc_return_success(r, "{%Q:%g,%Q:%Q}", "x", v, "ud", r->userdata);
}

static void foo1(struct jsonrpc_request *r) {
  jsonrpc_return_error(r, 123, "", NULL);
}

static void foo2(struct jsonrpc_request *r) {
  jsonrpc_return_error(r, 456, "qwerty", "%.*s", r->params_len, r->params);
}

static void foo3(struct jsonrpc_request *r) {
  jsonrpc_return_success(r, "%.*s", r->params_len, r->params);
}

static int response_cb(const char *buf, int len, void *privdata) {
  return mjson_printf(mjson_print_fixed_buf, privdata, ">>%.*s<<", len, buf);
}

static void test_rpc(void) {
  // struct jsonrpc_ctx *ctx = &jsonrpc_default_context;
  const char *req = NULL, *res = NULL;
  char buf[200];
  struct mjson_fixedbuf fb = {buf, sizeof(buf), 0};

  // Init context
  jsonrpc_init(response_cb, &fb);

  // Call RPC.List
  req = "{\"id\": 1, \"method\": \"rpc.list\"}";
  res = "{\"id\":1,\"result\":[\"rpc.list\"]}\n";
  fb.len = 0;
  jsonrpc_process(req, strlen(req), mjson_print_fixed_buf, &fb);
  ASSERT(strcmp(buf, res) == 0);

  // Call non-existent method
  req = "{\"id\": 1, \"method\": \"foo\"}\n";
  res = "{\"id\":1,\"error\":{\"code\":-32601,\"message\":\"method not found\"}}\n";
  fb.len = 0;
  jsonrpc_process(req, strlen(req), mjson_print_fixed_buf, &fb);
  ASSERT(strcmp(buf, res) == 0);

  // Register our own function
  req = "{\"id\": 2, \"method\": \"foo\",\"params\":[0,1.23]}\n";
  res = "{\"id\":2,\"result\":{\"x\":1.23,\"ud\":\"hi\"}}\n";
  fb.len = 0;
  jsonrpc_export("foo", foo, (void *) "hi");
  jsonrpc_process(req, strlen(req), mjson_print_fixed_buf, &fb);
  ASSERT(strcmp(buf, res) == 0);

  // Test for bad frame
  req = "boo\n";
  res = "{\"error\":{\"code\":-32700,\"message\":\"boo\\n\"}}\n";
  fb.len = 0;
  jsonrpc_process(req, strlen(req), mjson_print_fixed_buf, &fb);
  ASSERT(strcmp(buf, res) == 0);

	// Test simple error response, without data
  req = "{\"id\": 3, \"method\": \"foo1\",\"params\":[1,true]}\n";
  res = "{\"id\":3,\"error\":{\"code\":123,\"message\":\"\"}}\n";
  jsonrpc_export("foo1", foo1, NULL);
  fb.len = 0;
  jsonrpc_process(req, strlen(req), mjson_print_fixed_buf, &fb);
  ASSERT(strcmp(buf, res) == 0);

	// Test more complex error response, with data
  req = "{\"id\": 4, \"method\": \"foo2\",\"params\":[1,true]}\n";
  res = "{\"id\":4,\"error\":{\"code\":456,\"message\":\"qwerty\",\"data\":[1,true]}}\n";
  jsonrpc_export("foo2", foo2, NULL);
  fb.len = 0;
  jsonrpc_process(req, strlen(req), mjson_print_fixed_buf, &fb);
  ASSERT(strcmp(buf, res) == 0);

  // Test notify - must not generate a response
  req = "{\"method\": \"ping\",\"params\":[1,true]}\n";
  fb.len = 0;
  jsonrpc_process(req, strlen(req), mjson_print_fixed_buf, &fb);
  ASSERT(fb.len == 0);

  // Test success response
  req = "{\"id\":123,\"result\":[1,2,3]}";
  res = ">>{\"id\":123,\"result\":[1,2,3]}<<";
  fb.len = 0;
  jsonrpc_process(req, strlen(req), mjson_print_fixed_buf, &fb);
  ASSERT(strcmp(buf, res) == 0);

  // Test error response
  req = "{\"id\":566,\"error\":{}}";
  res = ">>{\"id\":566,\"error\":{}}<<";
  fb.len = 0;
  jsonrpc_process(req, strlen(req), mjson_print_fixed_buf, &fb);
  ASSERT(strcmp(buf, res) == 0);

  // Test glob pattern in the RPC function name
  req = "{\"id\":777,\"method\":\"Bar.Baz\",\"params\":[true]}";
  res = "{\"id\":777,\"result\":[true]}\n";
  jsonrpc_export("Bar.*", foo3, NULL);
  fb.len = 0;
  jsonrpc_process(req, strlen(req), mjson_print_fixed_buf, &fb);
  ASSERT(strcmp(buf, res) == 0);
}

static void test_merge(void) {
  char buf[512];
  size_t i;
  const char *tests[] = {
      "",  // Empty string
      "",
      "",
      "{\"a\":1}",  // Simple replace
      "{\"a\":2}",
      "{\"a\":2}",
      "{\"a\":1}",  // Simple add
      "{\"b\":2}",
      "{\"a\":1,\"b\":2}",
      "{\"a\":{}}",  // Object -> scalar
      "{\"a\":1}",
      "{\"a\":1}",
      "{\"a\":{}}",  // Object -> object
      "{\"a\":{\"b\":1}}",
      "{\"a\":{\"b\":1}}",
      "{\"a\":{\"b\":1}}",  // Simple object
      "{\"a\":{\"c\":2}}",
      "{\"a\":{\"b\":1,\"c\":2}}",
      "{\"a\":{\"b\":1,\"c\":2}}",  // Simple object
      "{\"a\":{\"c\":null}}",
      "{\"a\":{\"b\":1}}",
      "{\"a\":[1,{\"b\":false}],\"c\":2}",  // Simple object
      "{\"a\":null,\"b\":[1]}",
      "{\"c\":2,\"b\":[1]}",
      "{\"a\":1}",  // Delete existing key
      "{\"a\":null}",
      "{}",
      "{\"a\":1}",  // Delete non-existing key
      "{\"b\":null}",
      "{\"a\":1}",
  };
  for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i += 3) {
    struct mjson_fixedbuf fb = {buf, sizeof(buf), 0};
    int n = mjson_merge(tests[i], strlen(tests[i]), tests[i + 1],
                        strlen(tests[i + 1]), mjson_print_fixed_buf, &fb);
    // printf("%s + %s = %.*s\n", tests[i], tests[i + 1], fb.len, fb.ptr);
    ASSERT(n == (int) strlen(tests[i + 2]));
    ASSERT(strncmp(fb.ptr, tests[i + 2], fb.len) == 0);
  }
}

static void test_pretty(void) {
  size_t i;
  const char *tests[] = {
      "{   }",  // Empty object
      "{}",
      "{}",
      "[   ]",  // Empty array
      "[]",
      "[]",
      "{ \"a\" :1    }",  // Simple object
      "{\n  \"a\": 1\n}",
      "{\"a\":1}",
      "{ \"a\" :1  ,\"b\":2}",  // Simple object, 2 keys
      "{\n  \"a\": 1,\n  \"b\": 2\n}",
      "{\"a\":1,\"b\":2}",
      "{ \"a\" :1  ,\"b\":2, \"c\":[1,2,{\"d\":3}]}",  // Complex object
      "{\n  \"a\": 1,\n  \"b\": 2,\n  \"c\": [\n"
      "    1,\n    2,\n    {\n      \"d\": 3\n    }\n  ]\n}",
      "{\"a\":1,\"b\":2,\"c\":[1,2,{\"d\":3}]}",

      "{ \"a\" :{\"b\"  :2},\"c\": {}    }",  // Nested object
      "{\n  \"a\": {\n    \"b\": 2\n  },\n  \"c\": {}\n}",
      "{\"a\":{\"b\":2},\"c\":{}}",
  };

  for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i += 3) {
    char buf[512];
    struct mjson_fixedbuf fb = {buf, sizeof(buf), 0};
    const char *s = tests[i];
    ASSERT(mjson_pretty(s, strlen(s), "  ", mjson_print_fixed_buf, &fb) > 0);
    ASSERT(fb.len == (int) strlen(tests[i + 1]));
    ASSERT(strncmp(fb.ptr, tests[i + 1], fb.len) == 0);
    // printf("==> %s\n", buf);

    // Terse print
    fb.len = 0;
    ASSERT(mjson_pretty(s, strlen(s), "", mjson_print_fixed_buf, &fb) > 0);
    ASSERT(fb.len == (int) strlen(tests[i + 2]));
    ASSERT(strncmp(fb.ptr, tests[i + 2], fb.len) == 0);
    // printf("--> %s\n", buf);
  }
}

static void test_next(void) {
  int a, b, c, d, t;

  {
    const char *s = "{}";
    ASSERT(mjson_next(s, strlen(s), 0, &a, &b, &c, &d, &t) == 0);
  }

  {
    const char *s = "{\"a\":1}";
    ASSERT(mjson_next(s, strlen(s), 0, &a, &b, &c, &d, &t) == 6);
    ASSERT(a == 1 && b == 3 && c == 5 && d == 1 && t == MJSON_TOK_NUMBER);
    ASSERT(mjson_next(s, strlen(s), 6, &a, &b, &c, &d, &t) == 0);
  }

  {
    const char *s = "{\"a\":123,\"b\":[1,2,3,{\"c\":1}],\"d\":null}";
    ASSERT(mjson_next(s, strlen(s), 0, &a, &b, &c, &d, &t) == 8);
    ASSERT(a == 1 && b == 3 && c == 5 && d == 3 && t == MJSON_TOK_NUMBER);
    ASSERT(mjson_next(s, strlen(s), 8, &a, &b, &c, &d, &t) == 28);
    ASSERT(a == 9 && b == 3 && c == 13 && d == 15 && t == MJSON_TOK_ARRAY);
    ASSERT(mjson_next(s, strlen(s), 28, &a, &b, &c, &d, &t) == 37);
    ASSERT(a == 29 && b == 3 && c == 33 && d == 4 && t == MJSON_TOK_NULL);
    ASSERT(mjson_next(s, strlen(s), 37, &a, &b, &c, &d, &t) == 0);
  }

  {
    const char *s = "[]";
    ASSERT(mjson_next(s, strlen(s), 0, &a, &b, &c, &d, &t) == 0);
  }

  {
    const char *s = "[3,null,{},[1,2],{\"x\":[3]},\"hi\"]";
    ASSERT(mjson_next(s, strlen(s), 0, &a, &b, &c, &d, &t) == 2);
    ASSERT(a == 0 && b == 0 && c == 1 && d == 1 && t == MJSON_TOK_NUMBER);
    ASSERT(mjson_next(s, strlen(s), 2, &a, &b, &c, &d, &t) == 7);
    ASSERT(a == 1 && b == 0 && c == 3 && d == 4 && t == MJSON_TOK_NULL);
    ASSERT(mjson_next(s, strlen(s), 7, &a, &b, &c, &d, &t) == 10);
    ASSERT(a == 2 && b == 0 && c == 8 && d == 2 && t == MJSON_TOK_OBJECT);
    ASSERT(mjson_next(s, strlen(s), 10, &a, &b, &c, &d, &t) == 16);
    ASSERT(a == 3 && b == 0 && c == 11 && d == 5 && t == MJSON_TOK_ARRAY);
    ASSERT(mjson_next(s, strlen(s), 16, &a, &b, &c, &d, &t) == 26);
    ASSERT(a == 4 && b == 0 && c == 17 && d == 9 && t == MJSON_TOK_OBJECT);
    ASSERT(mjson_next(s, strlen(s), 26, &a, &b, &c, &d, &t) == 31);
    ASSERT(a == 5 && b == 0 && c == 27 && d == 4 && t == MJSON_TOK_STRING);
    ASSERT(mjson_next(s, strlen(s), 31, &a, &b, &c, &d, &t) == 0);
  }
}

static void test_globmatch(void) {
  ASSERT(mjson_globmatch("", 0, "", 0) == 1);
  ASSERT(mjson_globmatch("*", 1, "a", 1) == 1);
  ASSERT(mjson_globmatch("*", 1, "ab", 2) == 1);
  ASSERT(mjson_globmatch("", 0, "a", 1) == 0);
  ASSERT(mjson_globmatch("/", 1, "/foo", 4) == 0);
  ASSERT(mjson_globmatch("/*/foo", 6, "/x/bar", 6) == 0);
  ASSERT(mjson_globmatch("/*/foo", 6, "/x/foo", 6) == 1);
  ASSERT(mjson_globmatch("/*/foo", 6, "/x/foox", 7) == 0);
  ASSERT(mjson_globmatch("/*/foo*", 7, "/x/foox", 7) == 1);
  ASSERT(mjson_globmatch("/*", 2, "/abc", 4) == 1);
  ASSERT(mjson_globmatch("/*", 2, "/ab/", 4) == 0);
  ASSERT(mjson_globmatch("/*", 2, "/", 1) == 1);
  ASSERT(mjson_globmatch("/x/*", 4, "/x/2", 4) == 1);
  ASSERT(mjson_globmatch("/x/*", 4, "/x/2/foo", 8) == 0);
  ASSERT(mjson_globmatch("/x/*/*", 6, "/x/2/foo", 8) == 1);
  ASSERT(mjson_globmatch("#", 1, "///", 3) == 1);
}

int main() {
  test_next();
  test_printf();
  test_cb();
  test_find();
  test_get_number();
  test_get_bool();
  test_get_string();
  test_print();
  test_rpc();
  test_merge();
  test_pretty();
  test_globmatch();
  printf("%s. Total tests: %d, failed: %d\n",
         s_num_errors ? "FAILURE" : "SUCCESS", s_num_tests, s_num_errors);
  return s_num_errors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
