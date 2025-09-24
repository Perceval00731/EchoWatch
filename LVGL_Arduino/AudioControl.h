#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void playMusic();
void setVolume(int volume);
bool playTextToSpeech(const char* text, const char* lang);
bool playHTTPStream(const char* url);
void AudioControl_stop();
bool AudioControl_isRunning();
bool AudioControl_consumeStreamFinished(char* infoBuffer, size_t bufferLen);

#ifdef __cplusplus
}
#endif

void AudioControl_updateDisplayInfo();
