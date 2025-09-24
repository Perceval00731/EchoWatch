#include "LampControl.h"

#include "NetworkManager.h"
#include "ui.h"

namespace {
constexpr unsigned long LAMP_ACK_TIMEOUT_MS = 5000;

PubSubClient* g_client = nullptr;
bool g_lampStateAck = false;
bool g_lampPending = false;
int g_lampDesired = 0;
unsigned long g_lampPendingSince = 0;

void applyAckStateToSwitch() {
  if (!ui_LightSwitch1) {
    return;
  }

  if (g_lampStateAck) {
    lv_obj_add_state(ui_LightSwitch1, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(ui_LightSwitch1, LV_STATE_CHECKED);
  }
}

void setSwitchDisabled(bool disabled) {
  if (!ui_LightSwitch1) {
    return;
  }

  if (disabled) {
    lv_obj_add_state(ui_LightSwitch1, LV_STATE_DISABLED);
  } else {
    lv_obj_clear_state(ui_LightSwitch1, LV_STATE_DISABLED);
  }
}

void cancelPendingAndRevert() {
  g_lampPending = false;
  applyAckStateToSwitch();
  setSwitchDisabled(false);
}

void requestLampStateChange(bool desiredOn) {
  if (!g_client || !NetworkManager_isMqttConnected() || !g_client->connected()) {
    applyAckStateToSwitch();
    return;
  }

  g_lampDesired = desiredOn ? 1 : 0;
  g_lampPending = true;
  g_lampPendingSince = millis();

  applyAckStateToSwitch();
  setSwitchDisabled(true);

  const char* payload = desiredOn ? "ON" : "OFF";
  if (!g_client->publish("esp32/lampe", payload)) {
    cancelPendingAndRevert();
  }
}
}

void LampControl_setClient(PubSubClient* client) {
  g_client = client;
}

void LampControl_onMqttConnected() {
  setSwitchDisabled(false);
  applyAckStateToSwitch();
}

void LampControl_onMqttDisconnected() {
  cancelPendingAndRevert();
}

void LampControl_handleAck(bool on) {
  g_lampStateAck = on;
  g_lampDesired = on ? 1 : 0;
  cancelPendingAndRevert();
}

void LampControl_loop() {
  if (g_lampPending && (millis() - g_lampPendingSince > LAMP_ACK_TIMEOUT_MS)) {
    cancelPendingAndRevert();
  }
}

extern "C" void ui_request_lamp_set(int desiredOn) {
  requestLampStateChange(desiredOn != 0);
}

extern "C" void ui_revert_lamp_visual_to_ack(void) {
  applyAckStateToSwitch();
}
