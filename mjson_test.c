#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "mjson.h"

static void cb(int ev, char *s, int len, void *ud) {
  // const char *states[] = {"value", "literal", "string", "escape", "utf8"};
  // const char *actions[] = {"none", "start", "end", "start_s", "end_s"};
  // printf("%s\t%s\t[%.*s]\n", states[st], actions[ac], len, s);
  const char *names[] = {"value", "key", "colon", "comma"};
  printf("%d\t%s\t[%.*s]\n", ev, names[ev], len, s);
  (void) ud;
}

static void test_cb(void) {
  char s[] = "{\"a\": true, \"b\": [ null, 3 ]}";
  int n = mjson(s, sizeof(s) - 1, cb, NULL);
  printf("n = %d\n", n);
}

int main() {
  test_cb();
  return 0;
}
