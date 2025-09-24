#pragma once

#include <PubSubClient.h>

void NetworkManager_begin();
void NetworkManager_loop();

bool NetworkManager_isWifiConnected();
bool NetworkManager_isMqttConnected();
bool NetworkManager_isNtpConfigured();

PubSubClient& NetworkManager_getClient();
