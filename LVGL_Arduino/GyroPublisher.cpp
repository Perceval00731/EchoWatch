#include "GyroPublisher.h"

#include <Arduino.h>

#include "Gyro_QMI8658.h"
#include "NetworkManager.h"

void GyroPublisher_publish() {
  if (!NetworkManager_isMqttConnected()) {
    return;
  }

  getAccelerometer();
  getGyroscope();

  char msg[128];
  snprintf(msg, sizeof(msg), "{\"accel\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f},\"gyro\":{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f}}",
           Accel.x, Accel.y, Accel.z,
           Gyro.x, Gyro.y, Gyro.z);

  if (NetworkManager_getClient().publish("esp32/gyro", msg)) {
    Serial.println("[MQTT] Données gyroscopiques publiées");
  } else {
    Serial.println("[MQTT] Échec de la publication des données gyroscopiques");
  }
}
