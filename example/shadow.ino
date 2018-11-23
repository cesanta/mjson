#include "mjson.h"

static int ledOn = 0;  // Current LED status

static int sender(const char *buf, int len, void *privdata) {
  return Serial.write(buf, len);  // Send RPC reply frame
}

static void reportState(void) {
  jsonrpc_notify("{\"method\":%Q,\"params\":{\"on\":%s}}", "Shadow.Report",
                 ledOn ? "true" : "false");
}

static int on_delta(char *buf, int len, struct mjson_out *out, void *userdata) {
  ledOn = mjson_get_bool(buf, len, "$.on", 0);  // Fetch "on" shadow value
  digitalWrite(LED_BUILTIN, ledOn);             // Set LED to the "on" value
  reportState();  // Let shadow know our new state
  return 0;       // Signal sucess to the RPC engine
}

void setup() {
  jsonrpc_init(sender, NULL, "1.0");                // Init RPC library
  jsonrpc_export("Shadow.Delta", on_delta, NULL);   // Export "Shadow.Delta"
  pinMode(LED_BUILTIN, OUTPUT);   // Configure LED pin
  Serial.begin(9600);             // Init serial comms
  reportState();                  // Let shadow know our state
}

void loop() {
  if (Serial.available() > 0) jsonrpc_process_byte(Serial.read());
}
