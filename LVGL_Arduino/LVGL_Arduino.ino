#include "AppInit.h"
#include "GyroPublisher.h"
#include "LampControl.h"
#include "LVGL_Driver.h"
#include "NetworkManager.h"
#include "RTC_PCF85063.h"
#include "StatusDisplay.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

unsigned long lastUpdateTime = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("===== Démarrage =====");

  AppInit();
  NetworkManager_begin();
  LampControl_setClient(&NetworkManager_getClient());
  LampControl_onMqttDisconnected();
  GyroPublisher_init();
}

void loop() {
  Lvgl_Loop();
  NetworkManager_loop();
  LampControl_loop();
  GyroPublisher_loop();

  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateTime >= 1000UL) {
    lastUpdateTime = currentMillis;
    PCF85063_Loop();
    StatusDisplay_update();
  }

  vTaskDelay(pdMS_TO_TICKS(5));
}
