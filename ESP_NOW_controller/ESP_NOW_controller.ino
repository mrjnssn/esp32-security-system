#include <WiFi.h>
#include <esp_now.h>
#include <protocol.h>

#define WIFI_CHANNEL 6

enum ConnectionState {
  DISCONNECTED,
  SYN_RECEIVED,
  CONNECTED,
  FIN_RECEIVED,
};

ConnectionState connectionState = DISCONNECTED;
uint16_t currentSequence = 42;

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

  switch (receivedHeader.type) {
    case MSG_SYN:
      if (connectionState != DISCONNECTED) {
        return;
      }
      
      handleSyn(info, receivedHeader);

      break;
    case MSG_ACK:
      if (connectionState != SYN_RECEIVED) {
        return;
      }

      handleAck(info, receivedHeader);

      break;
    case MSG_FIN:
      if (connectionState != CONNECTED) {
        return;
      }   

      handleFin(info, receivedHeader);

      break;
  }
}

void handleSyn(const esp_now_recv_info_t *info, const PacketHeader& receivedHeader) {
  // confirm reception of SYN
  printConfirmation(receivedHeader);
  
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

  // update sequence
  currentSequence = receivedHeader.sequence;

  // create response: SYN_ACK packet header
  PacketHeader synAckHeader;

  synAckHeader.version = PROTOCOL_VERSION;
  synAckHeader.source = NODE_CONTROLLER;
  synAckHeader.destination = receivedHeader.source;
  synAckHeader.type = MSG_SYN_ACK;
  synAckHeader.sequence = receivedHeader.sequence;
  synAckHeader.payloadLength = 0;

  // create space in memory for response
  uint8_t responseBuffer[HEADER_SIZE];

  // serialise and store response in buffer
  serializeHeader(synAckHeader, responseBuffer);

  // send response
  esp_now_send(
    info->src_addr,
    responseBuffer,
    HEADER_SIZE
  );

  // update state machine
  connectionState = SYN_RECEIVED;
}

void handleAck(const esp_now_recv_info_t *info, const PacketHeader& receivedHeader) {
  // confirm reception of SYN
  printConfirmation(receivedHeader);

  // check sequence
  if (receivedHeader.sequence != currentSequence) {
    return;
  }

  // update state machine
  connectionState = CONNECTED;
}

void handleFin(const esp_now_recv_info_t *info, const PacketHeader& receivedHeader) {
  // confirm reception of SYN
  printConfirmation(receivedHeader);

  // update sequence
  currentSequence = receivedHeader.sequence;

  // create response: FIN_ACK packet header
  PacketHeader finAckHeader;

  finAckHeader.version = PROTOCOL_VERSION;
  finAckHeader.source = NODE_CONTROLLER;
  finAckHeader.destination = receivedHeader.source;
  finAckHeader.type = MSG_FIN_ACK;
  finAckHeader.sequence = receivedHeader.sequence;
  finAckHeader.payloadLength = 0;

  // create space in memory for response
  uint8_t responseBuffer[HEADER_SIZE];

  // serialise and store response in buffer
  serializeHeader(finAckHeader, responseBuffer);

  // send response
  esp_now_send(
    info->src_addr,
    responseBuffer,
    HEADER_SIZE
  );

  // update state machine
  connectionState = DISCONNECTED;

  println("")
}

void printConfirmation(const PacketHeader& receivedHeader) {
  Serial.print("Valid SEC32 ");
  Serial.print(receivedHeader.type);
  Serial.println(" received");
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
