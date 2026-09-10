#include <WiFi.h>
#include <esp_now.h>
#include <protocol.h>

#define WIFI_CHANNEL 6

uint8_t controllerMac[] = {0x5C, 0x01, 0x3B, 0xBF, 0x87, 0x78};

enum connectionState {
  DISCONNECTED,
  SYN_SENT,
  CONNECTED,
  FIN_SENT,
};

PacketHeader synHeader;

uint8_t buffer[HEADER_SIZE];

void setup() {
  synHeader.version = PROTOCOL_VERSION;
  synHeader.source = NODE_SENSOR_1;
  synHeader.destination = NODE_CONTROLLER;
  synHeader.type = MSG_SYN;
  synHeader.sequence = 42;
  synHeader.payloadLength = 0;

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.setChannel(WIFI_CHANNEL);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  esp_now_peer_info_t peerInfo = {};

  memcpy(peerInfo.peer_addr, controllerMac, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("ESP-NOW ready");

}

void loop() {

  serializeHeader(synHeader, buffer);

  esp_err_t message = esp_now_send(
    controllerMac,
    buffer,
    HEADER_SIZE
  );

  if (message == ESP_OK) {
    Serial.println("SYN queued for sending");
  } else {
    Serial.println("Send failed");
  }

  delay(2000);
}
