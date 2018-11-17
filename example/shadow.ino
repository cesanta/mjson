#include "mjson.h"

static char in[200];
static int in_len = 0;
static int ledOn = 0;

// Gets called by the RPC engine to send a reply frame
static int sender(char *buf, int len, void *privdata) {
  return Serial.write(buf, len) + Serial.write('\n');
}

static void reportState(void) {
  jsonrpc_notify("{\"method\":%Q,\"params\":{\"on\":%s}}", "Shadow.Report",
                 ledOn ? "true" : "false");
}

// Handle device shadow delta
static int on_shadow_delta(char *buf, int len, struct mjson_out *out,
                           void *userdata) {
  ledOn = mjson_get_bool(buf, len, "$.on", 0);
  digitalWrite(LED_BUILTIN, ledOn);
  reportState();
  return 0;  // Success
}

void setup() {
  jsonrpc_init(sender, NULL, "1.0");
  jsonrpc_export("Shadow.Delta", on_shadow_delta, NULL);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  reportState();
}

void loop() {
  if (Serial.available() > 0) {
    if (in_len >= sizeof(in)) in_len = 0;  // On overflow, wrap
    in[in_len++] = Serial.read();  // Read from the UART, append to the buffer
    if (in[in_len - 1] == '\n') {  // If that was a new line, parse frame
      if (in_len > 1) jsonrpc_process(in, in_len);  // Handle frame
      in_len = 0;                                   // Reset input buffer
    }
  }
}
