#include <WiFi.h>
#include <esp_now.h>
#include <protocol.h>

#define WIFI_CHANNEL 6

enum connectionState {
  DISCONNECTED,
  SYN_RECEIVED,
  CONNECTED,
  FIN_RECEIVED,
};

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

  // add source as peer

  if (!esp_now_is_peer_exist(info->src_addr)) {

    esp_now_peer_info_t peerInfo = {}; // initialize peerInfo struct

    memcpy(peerInfo.peer_addr, info->src_addr, 6);
    peerInfo.channel = WIFI_CHANNEL;
    peerInfo.encrypt = false;

      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
      }
  }

  PacketHeader synAckHeader;

  synAckHeader.version = PROTOCOL_VERSION;
  synAckHeader.source = NODE_CONTROLLER;
  synAckHeader.destination = receivedHeader.source;
  synAckHeader.type = MSG_SYN_ACK;
  synAckHeader.sequence = receivedHeader.sequence;
  synAckHeader.payloadLength = 0;

  uint8_t responseBuffer[HEADER_SIZE];

  serializeHeader(synAckHeader, responseBuffer);

  esp_now_send(
    info->src_addr,
    responseBuffer,
    HEADER_SIZE
  );

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
