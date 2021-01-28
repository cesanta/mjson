#include "mjson.h"  // Sketch -> Add File -> Add mjson.h

// Gets called by the RPC engine to send a reply frame
static int wfn(const char *frame, int frame_len, void *privdata) {
  return Serial.write(frame, frame_len);
}

// Catch MQTT messages on topic "ccm/led", and switch LED on/off.
// Cloud connector forwards us MQTT messages as RPC frames like this:
// {"method": "MQTT.Message", "params": {"topic": "ccm/led", "message": "1"}}
static void mqtt_cb(struct jsonrpc_request *r) {
  char msg[100] = "";
  mjson_get_string(r->params, r->params_len, "$.message", msg, sizeof(msg));
  digitalWrite(LED_BUILTIN, atoi(msg));  // message is either "0" or "1"
}

// Each time Cloud Connector boots, it sends "Sys.Init" to the host controller.
// Host controller can use it to do run-time initialisation of Cloud Connector.
// In our case, we subscribe to the "ccm/led" MQTT topic.
static void sys_init_cb(struct jsonrpc_request *r) {
  mjson_printf(wfn, NULL, "{%Q: %Q, %Q: {%Q: %Q}}", "method", "MQTT.Sub",
               "params", "topic", "ccm/led");
}

void setup() {
  Serial.begin(115200);                     // Init serial comms
  pinMode(LED_BUILTIN, OUTPUT);             // Configure LED pin
  jsonrpc_init(NULL, NULL);                 // Init JSON-RPC engine
  jsonrpc_export("MQTT.Message", mqtt_cb);  // Set MQTT message callback
  jsonrpc_export("Sys.Init", sys_init_cb);  // Set init callback
  sys_init_cb(NULL);
}

void loop() {
  char buf[800];
  if (Serial.available() > 0) {
    int len = Serial.readBytes(buf, sizeof(buf));
    jsonrpc_process(buf, len, wfn, NULL, NULL);
  }

  // Publish to MQTT approximately every 5000 milliseconds.
  // Topic: "ccm/data", message: current uptime in milliseconds.
  static unsigned long old, now;
  now = millis();
  if (old > now || old + 5000 < now) {
    old = now;
    mjson_printf(wfn, NULL, "{%Q: %Q, %Q: {%Q: %Q, %Q: %lu}}", "method",
                 "MQTT.Pub", "params", "topic", "ccm/data", "message", now);
  }
}
