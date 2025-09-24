#pragma once

#include <Arduino.h>
#include <PubSubClient.h>

void LampControl_setClient(PubSubClient* client);
void LampControl_onMqttConnected();
void LampControl_onMqttDisconnected();
void LampControl_handleAck(bool on);
void LampControl_loop();

#ifdef __cplusplus
extern "C" {
#endif

void ui_request_lamp_set(int desiredOn);
void ui_revert_lamp_visual_to_ack(void);

#ifdef __cplusplus
}
#endif
