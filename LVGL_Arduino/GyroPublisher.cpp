#include "GyroPublisher.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include "Gyro_QMI8658.h"
#include "NetworkManager.h"

// Configuration globale des capteurs
SensorConfig g_sensorConfig = {
    .periodicity_ms = 1000,    // 1 seconde par défaut
    .timeout_ms = 5000,        // 5 secondes de timeout par défaut
    .last_publish_time = 0,
    .enabled = true
};

static unsigned long g_last_request_time = 0;

void GyroPublisher_init() {
    g_sensorConfig.last_publish_time = millis();
}

void GyroPublisher_publish() {
    if (!NetworkManager_isMqttConnected()) {
        return;
    }

    getAccelerometer();
    getGyroscope();

    // Format JSON selon les spécifications du client
    char msg[256];
    snprintf(msg, sizeof(msg), 
        "{\"accelerometer\":{\"accel\":[%.2f,%.2f,%.2f],\"unit\":\"m.s-2\"},\"gyroscope\":{\"gyro\":[%.2f,%.2f,%.2f],\"unit\":\"deg/s\"}}",
        Accel.x, Accel.y, Accel.z,
        Gyro.x, Gyro.y, Gyro.z);

    if (NetworkManager_getClient().publish("esp32/sensors", msg)) {
        printf("\n[MQTT] Données des capteurs publiées sur esp32/sensors");
    } else {
        printf("\n[MQTT] Échec de la publication des données des capteurs");
    }
    
    g_sensorConfig.last_publish_time = millis();
}

void GyroPublisher_handleRequest(const char* json_payload) {
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, json_payload);
    
    if (error) {
        printf("\n[GyroPublisher] Erreur de parsing de la requête JSON");
        return;
    }
    
    // Mettre à jour le timestamp de la dernière requête
    g_last_request_time = millis();
    
    // Extraction des paramètres de la requête
    if (doc.containsKey("periodicity_ms")) {
        unsigned long new_periodicity = doc["periodicity_ms"];
        if (new_periodicity > 0) {
            g_sensorConfig.periodicity_ms = new_periodicity;
            printf("[GyroPublisher] Périodicité mise à jour: %lu ms\n", new_periodicity);
        }
    }
    
    if (doc.containsKey("timeout_ms")) {
        unsigned long new_timeout = doc["timeout_ms"];
        if (new_timeout > 0) {
            g_sensorConfig.timeout_ms = new_timeout;
            printf("[GyroPublisher] Timeout mis à jour: %lu ms\n", new_timeout);
        }
    }
    
    if (doc.containsKey("enabled")) {
        g_sensorConfig.enabled = doc["enabled"];
        printf("[GyroPublisher] État activé: %s\n", g_sensorConfig.enabled ? "true" : "false");
    }
    
    // Activer automatiquement la publication lors d'une requête
    g_sensorConfig.enabled = true;
    printf("\n[GyroPublisher] Publication activée suite à la requête");
}

void GyroPublisher_loop() {
    if (!NetworkManager_isMqttConnected()) {
        return;
    }
    
    unsigned long current_time = millis();
    
    // Vérifier le timeout : si une requête a été reçue et que le timeout est dépassé, arrêter la publication
    if (g_last_request_time > 0 && g_sensorConfig.enabled) {
        if (current_time - g_last_request_time >= g_sensorConfig.timeout_ms) {
            g_sensorConfig.enabled = false;
            printf("\n[GyroPublisher] Timeout dépassé, arrêt de la publication automatique");
            return;
        }
    }
    
    if (!g_sensorConfig.enabled) {
        return;
    }
    
    // Vérifier si il faut publier les données selon la périodicité
    if (current_time - g_sensorConfig.last_publish_time >= g_sensorConfig.periodicity_ms) {
        GyroPublisher_publish();
    }
}
