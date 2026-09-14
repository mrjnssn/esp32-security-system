#include <WiFi.h>
#include <esp_now.h>
#include <protocol.h>

#define WIFI_CHANNEL 6

constexpr unsigned long RESPONSE_TIMEOUT_MS = 3000;
constexpr uint8_t MAX_RETRIES = 3;

unsigned long lastSendAt = 0;
uint8_t retryCount = 0;

uint8_t controllerMac[] = {0x5C, 0x01, 0x3B, 0xBF, 0x87, 0x78};

enum ConnectionState {
  DISCONNECTED,
  SYN_SENT,
  CONNECTED,
  FIN_SENT,
  CONNECTION_FAILED
};

ConnectionState connectionState = DISCONNECTED;
uint8_t buffer[HEADER_SIZE];
uint16_t currentSequence = 42;


bool sendSyn() {
  // create MSG_SYN
  PacketHeader synHeader;

  synHeader.version = PROTOCOL_VERSION;
  synHeader.source = NODE_SENSOR_1;
  synHeader.destination = NODE_CONTROLLER;
  synHeader.type = MSG_SYN;
  synHeader.sequence = currentSequence;
  synHeader.payloadLength = 0;

  // serialize message
  serializeHeader(synHeader, buffer);
  // send message
  esp_err_t message = esp_now_send(
    controllerMac,
    buffer,
    HEADER_SIZE
  );

  if (result == ESP_OK) {
    return true;
  } else {
    return false;
  }

}


void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  PacketHeader receivedHeader; 

  if (len < HEADER_SIZE) {
    return;
  }

  deserializeHeader(data, receivedHeader);

  if (receivedHeader.version != PROTOCOL_VERSION) {
    return;
  }

  if (receivedHeader.destination != NODE_SENSOR_1) {
    return;
  }

  switch (receivedHeader.type) {
    case MSG_SYN_ACK:
      if (connectionState != SYN_SENT) {
        return;
      }
      
      handleSynAck(info, receivedHeader);

      break;

    case MSG_FIN_ACK:
      if (connectionState != CONNECTED) {
        return;
      }   

      // TODO: handleFinAck(info, receivedHeader);

      connectionState = DISCONNECTED;
      Serial.println("Sensor: disconnected.");

      break;
  }

  if (receivedHeader.type != MSG_SYN_ACK) {
    return;
  }


}

void handleSynAck(const esp_now_recv_info_t *info, const PacketHeader& receivedHeader) {
  if (receivedHeader.sequence != currentSequence) {
    return;
  }

  printConfirmation(receivedHeader);

  PacketHeader ackHeader;

  ackHeader.version = PROTOCOL_VERSION;
  ackHeader.source = NODE_SENSOR_1;
  ackHeader.destination = receivedHeader.source;
  ackHeader.type = MSG_ACK;
  ackHeader.sequence = receivedHeader.sequence;
  ackHeader.payloadLength = 0;

  uint8_t responseBuffer[HEADER_SIZE];

  serializeHeader(ackHeader, responseBuffer);

  esp_now_send(
    info->src_addr,
    responseBuffer,
    HEADER_SIZE
  );

  connectionState = CONNECTED;
  Serial.println("Sensor: connection established.");

}

void printConfirmation(const PacketHeader& receivedHeader) {
  Serial.print("Valid SEC32 type ");
  Serial.print(receivedHeader.type);
  Serial.println(" message received");
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

  WiFi.mode(WIFI_STA);
  WiFi.setChannel(WIFI_CHANNEL);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  // FOR DEBUGGING
  delay(3000);

  esp_now_peer_info_t peerInfo = {};

  memcpy(peerInfo.peer_addr, controllerMac, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_recv_cb(onDataReceived);

  Serial.println("ESP-NOW ready");

  // FOR DEBUGGING
  Serial.println("Starting handshake in 3 seconds...");
  delay(3000);

}

void loop() {
  static ConnectionState lastState = DISCONNECTED;
  if (connectionState != lastState) {
    Serial.print("Sensor state changed to: ");
    Serial.println(connectionState);
    
    lastState = connectionState;
  }

  switch (connectionState) {
    case DISCONNECTED:
      if (sendSyn()) {
        Serial.println("SYN sent");
        connectionState = SYN_SENT;
        lastSendAt = millis();
      }
      retryCount = 0;
      break;
    case SYN_SENT:
      if (millis() - lastSendAt >= RESPONSE_TIMEOUT_MS) {
        if (retryCount < MAX_RETRIES) {
          Serial.println("SYN wasn't sent. Retrying...")
          sendSyn();
          lastSendAt = millis();
          retryCount++;
          Serial.print(retryCount);
          Serial.print("/");
          Serial.println(MAX_RETRIES);
        } else {
          Serial.println("Handshake failed: maximum retries reached.")
          connectionState = CONNECTION_FAILED;
        }
      }
      break;
    case FIN_SENT:
      // TODO: create retry for FIN
      break;
  }
}
