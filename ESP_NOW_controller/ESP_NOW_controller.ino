#include <WiFi.h>
#include <esp_now.h>
#include <protocol.h>

#define WIFI_CHANNEL 6

void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  PacketHeader receivedHeader; 

  if (len < HEADER_SIZE) {
    return;
  }

  deserializeHeader(data, receivedHeader);

  if (receivedHeader.version != PROTOCOL_VERSION) {
    return;
  }

  if (receivedHeader.destination != NODE_CONTROLLER) {
    return;
  }

  if (receivedHeader.type != MSG_SYN) {
    return;
  }
  
  Serial.println("Valid SEC32 SYN received");
  Serial.print("version: ");
  Serial.println(receivedHeader.version);
  Serial.print("source: ");
  Serial.println(receivedHeader.source);
  Serial.print("destination: ");
  Serial.println(receivedHeader.destination);
  Serial.print("sequence: ");
  Serial.println(receivedHeader.sequence);
  Serial.print("payloadLength: ");
  Serial.println(receivedHeader.payloadLength);

}

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA); // stattion mode
  WiFi.setChannel(WIFI_CHANNEL);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_register_recv_cb(onDataReceived);

  Serial.println("Controller ready");

}

void loop() {
  Serial.println("Controller is running");
  Serial.println();
  
  delay(2000); 

}
