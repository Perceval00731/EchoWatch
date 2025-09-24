#include "StatusDisplay.h"

#include <Arduino.h>
#include <math.h>
#include <time.h>

#include "AudioControl.h"
#include "BAT_Driver.h"
#include "NetworkManager.h"
#include "RTC_PCF85063.h"
#include "ui.h"

namespace {
unsigned long g_lastBatteryUpdateTime = 0;

int computeBatteryPercent(float volts) {
  if (volts >= 4.10f) return 100;
  if (volts >= 3.70f) {
    float ratio = (volts - 3.70f) / (4.10f - 3.70f);
    int pct = 50 + static_cast<int>(roundf(ratio * 49.0f));
    if (pct > 99) pct = 99;
    return pct;
  }
  if (volts >= 3.50f) {
    float ratio = (volts - 3.50f) / (3.70f - 3.50f);
    int pct = 20 + static_cast<int>(roundf(ratio * 29.0f));
    if (pct > 49) pct = 49;
    return pct;
  }

  constexpr float LOW_MIN = 3.20f;
  constexpr float LOW_MAX = 3.50f;
  if (volts <= LOW_MIN) return 0;
  if (volts >= LOW_MAX) return 19;
  float ratio = (volts - LOW_MIN) / (LOW_MAX - LOW_MIN);
  int pct = static_cast<int>(roundf(ratio * 19.0f));
  if (pct > 19) pct = 19;
  if (pct < 0) pct = 0;
  return pct;
}

void updateBatteryPanels(float volts) {
  int level = 0;
  if (volts > 4.1f) level = 3;
  else if (volts >= 3.7f) level = 2;
  else if (volts >= 3.5f) level = 1;
  else level = 0;

  constexpr int OPA_ON = 255;
  constexpr int OPA_OFF = 40;

  if (ui_EnergyPanel1) lv_obj_set_style_bg_opa(ui_EnergyPanel1, (level >= 1) ? OPA_ON : OPA_OFF, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (ui_EnergyPanel2) lv_obj_set_style_bg_opa(ui_EnergyPanel2, (level >= 2) ? OPA_ON : OPA_OFF, LV_PART_MAIN | LV_STATE_DEFAULT);
  if (ui_EnergyPanel3) lv_obj_set_style_bg_opa(ui_EnergyPanel3, (level >= 3) ? OPA_ON : OPA_OFF, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void updateBatteryDisplay() {
  if (!ui_BatteryLabel) {
    return;
  }

  unsigned long now = millis();
  if (now - g_lastBatteryUpdateTime < 60000UL && g_lastBatteryUpdateTime != 0) {
    return;
  }

  // float volts = BAT_Get_Volts();
  float volts = 3.62f;  // TODO: remplacer par la lecture réelle lorsque disponible
  int pct = computeBatteryPercent(volts);
  lv_label_set_text(ui_BatteryLabel, (String(pct) + "%").c_str());
  updateBatteryPanels(volts);
  g_lastBatteryUpdateTime = now;
}

void updateTimeDisplay() {
  struct tm timeinfo;
  bool updated = false;

  if (NetworkManager_isWifiConnected() && NetworkManager_isNtpConfigured() && getLocalTime(&timeinfo, 0)) {
    updated = true;
  }

  if (!updated) {
    timeinfo.tm_hour = datetime.hour;
    timeinfo.tm_min = datetime.minute;
  }

  if (ui_Hour) {
    char hourStr[6];
    snprintf(hourStr, sizeof(hourStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    lv_label_set_text(ui_Hour, hourStr);
  }
}
}

void StatusDisplay_update() {
  updateBatteryDisplay();
  updateTimeDisplay();
  AudioControl_updateDisplayInfo();
}
