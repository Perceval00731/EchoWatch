#pragma once

// Structure pour gérer la configuration des capteurs
typedef struct {
    unsigned long periodicity_ms;
    unsigned long timeout_ms;
    unsigned long last_publish_time;
    bool enabled;
} SensorConfig;

extern SensorConfig g_sensorConfig;

void GyroPublisher_init();
void GyroPublisher_publish();
void GyroPublisher_handleRequest(const char* json_payload);
void GyroPublisher_loop();
