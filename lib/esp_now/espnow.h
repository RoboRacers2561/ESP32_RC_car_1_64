#pragma once
#include "debug.h"
#include "pins.h"
#include <Arduino.h>
#include <esp_mac.h>
#include <esp_now.h>

#define MAX_PEERS 10

extern uint8_t broadcastAddress[6];
extern uint8_t board1Address[6];
extern esp_now_peer_info_t peerInfo[MAX_PEERS];

typedef struct struct_message {
  int value;
  bool throttle;
} struct_message;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

void OnDataRecv(const uint8_t *mac, const uint8_t *incoming_data, int len);

void addPeer(uint8_t *macAddress);

bool checkPeer(uint8_t *macAddress);

void stringToBytes(const char *str, uint8_t *bytes, int byteCount);
