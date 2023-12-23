#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <mjson.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size == 0) {
    return 0;
  }
  uint8_t decider = data[0];
  size -= 1;
  data++;

  if ((decider % 2) == 0) {
    mjson((const char *)data, size, NULL, NULL);
  } else {
    mjson_find((const char *)data, size, "$.a", NULL, NULL);
  }

  return 0;
}