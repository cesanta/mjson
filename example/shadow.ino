#include "mjson.h"  // Sketch -> Add File -> Add mjson.h

static int ledOn = 0;  // Current LED status

static void reportState(void) {
  jsonrpc_call("{\"method\":\"Shadow.Report\",\"params\":{\"on\":%s}}",
               ledOn ? "true" : "false");
}

void setup() {
  // Init RPC library. Pass a "sender" function that writes RPC reply frame.
  jsonrpc_init([](const char *frame, int frame_len, void *privdata) {
    return (int) Serial.write(frame, frame_len);
  }, NULL, NULL, "1.0");

  // Export "Shadow.Delta". Pass a callback that updates ledOn
  jsonrpc_export("Shadow.Delta", [](char *params, int params_len, struct mjson_out *out, void *ud) {
    ledOn = mjson_get_bool(params, params_len, "$.on", 0);  // Fetch "on" shadow value
    digitalWrite(LED_BUILTIN, ledOn);             // Set LED to the "on" value
    reportState();  // Let shadow know our new state
    return 0;       // Signal sucess to the RPC engine
  }, NULL);

  pinMode(LED_BUILTIN, OUTPUT);   // Configure LED pin
  Serial.begin(115200);           // Init serial comms
  reportState();                  // Let shadow know our state
}

void loop() {
  if (Serial.available() > 0) jsonrpc_process_byte(Serial.read());
}
