#include "mjson.h"

static char in[200];
static int in_len = 0;

// Gets called by the RPC engine to send a reply frame
static int sender(char *buf, int len, void *privdata) {
  return Serial.write(buf, len) + Serial.write('\n');
}

// Handle device shadow delta
static int on_shadow_delta(char *buf, int len, struct mjson_out *out,
                           void *userdata) {
  int on = mjson_find_bool(buf, len, "$.on", 0);  // Get {"on": true/false}
  digitalWrite(LED_BUILTIN, on);  // Turn the LED on/off, according to delta
  jsonrpc_notify(("{\"method\":%Q,\"params\":{\"on\":%s}}",
                  "Shadow.Update", on ? "true" : "false"));
  return 0;  // Success
}

void setup() {
  jsonrpc_init(sender, NULL, "1.0");
  jsonrpc_export("Shadow.Delta", on_shadow_delta, NULL);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
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
