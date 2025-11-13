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
// Expose elapsed/duration in seconds for status decisions
unsigned long AudioControl_getElapsed();
unsigned long AudioControl_getDuration();

#ifdef __cplusplus
}
#endif

void AudioControl_updateDisplayInfo();
