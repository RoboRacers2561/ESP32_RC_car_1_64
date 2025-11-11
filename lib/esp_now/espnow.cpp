#include <espnow.h>

int peer_count = 0;
struct_message incoming_message;
esp_now_peer_info_t peerInfo[MAX_PEERS];
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t baseAddress[] = {0x64, 0xB7, 0x08, 0x95, 0xF9, 0x40};
uint8_t b1Address[] = {0x64, 0xB7, 0x08, 0x9D, 0x68, 0x1C};
uint8_t b2Address[] = {0x64, 0xB7, 0x08, 0x9C, 0x64, 0xE8};

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  DEBUG_PRINTLN("\r\nLast Packet Send Status:\t");
  DEBUG_PRINTLN(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success"
                                               : "Delivery Fail");
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incoming_data, int len) {
  memcpy(&incoming_message, incoming_data, sizeof(incoming_message));
  DEBUG_PRINTLN("Bytes received: ");
  DEBUG_PRINT(len);
  DEBUG_PRINTLN("Data: ");
  DEBUG_PRINT(incoming_message.msg);
  DEBUG_PRINTLN(incoming_message.throttle);
  DEBUG_PRINTLN(incoming_message.steering);
  dacWrite(DAC_STEERING_PIN, incoming_message.throttle);
  dacWrite(DAC_THROTTLE_PIN, incoming_message.steering);
}

void addPeer(uint8_t *macAddress) {
  if (peer_count >= MAX_PEERS) {
    DEBUG_PRINTLN("Max peers reached!");
    return;
  }

  memcpy(peerInfo[peer_count].peer_addr, macAddress, 6);
  peerInfo[peer_count].channel = 0; // Use same WiFi channel
  peerInfo[peer_count].encrypt = false;

  if (esp_now_add_peer(&peerInfo[peer_count]) != ESP_OK) {
    DEBUG_PRINTLN("Failed to add peer");
    return;
  }

  char macStr[13];
  sprintf(macStr, "%02X%02X%02X%02X%02X%02X", macAddress[0], macAddress[1],
          macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
  DEBUG_PRINTLN("Added peer:");
  DEBUG_PRINTLN(macStr);
  peer_count++;
}
// Return 1 if peers exists 0 if not
bool checkPeer(uint8_t *macAddress) {
  for (int i = 0; i < peer_count; i++) {
    if (memcmp(peerInfo[i].peer_addr, macAddress, ESP_NOW_ETH_ALEN) == 0) {
      return 1;
    }
  }
  return 0;
}

void stringToBytes(const char *str, uint8_t *bytes, int byteCount) {
  for (int i = 0; i < byteCount; i++) {
    // Parse two hex characters at a time
    char byte[3] = {str[i * 2], str[i * 2 + 1], '\0'};
    bytes[i] = (uint8_t)strtol(byte, NULL, 16);
  }
}
