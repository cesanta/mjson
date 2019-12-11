#include "mjson.h" // Sketch -> Add File -> Add mjson.h

// Gets called by the RPC engine to send a reply frame
static int wfn(const char *frame, int frame_len, void *privdata) {
  return Serial.write(frame, frame_len);
}

// Catch MQTT messages on topic "ccm/led", and switch LED on/off
static void mqtt_cb(struct jsonrpc_request *r) {
  char topic[100] = "", message[100] = ""; 
  mjson_get_string(r->params, r->params_len, "$.topic", topic, sizeof(topic));
  mjson_get_string(r->params, r->params_len, "$.message", message, sizeof(message));
  if (strcmp(topic, "ccm/led") == 0) pinMode(LED_BUILTIN, atoi(message));
}

void setup() {
  Serial.begin(115200);          // Init serial comms
	pinMode(LED_BUILTIN, OUTPUT);  // Configure LED pin
  jsonrpc_init(NULL, NULL);      // Init JSON-RPC engine
  jsonrpc_export("MQTT.Message", mqtt_cb, NULL); // Set MQTT message callback
  jsonrpc_call(wfn, NULL,
               "{\"method\":\"MQTT.Sub\",\"params\":{\"topic\":\"ccm/#\"}}");
}

void loop() {
  while (Serial.available() > 0) jsonrpc_process_byte(Serial.read(), wfn, NULL);

  // Publish to MQTT periodically
  static unsigned long old, now;
  now = millis();
  if (old > now || old + 5000 < now) {
    old = now;
    jsonrpc_call(wfn, NULL, "{\"method\":\"MQTT.Pub\",\"params\":"
                 "{\"topic\":\"ccm/data\",\"message\":%lu}}", now);
  }
}
