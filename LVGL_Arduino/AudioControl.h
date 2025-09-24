#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void playMusic();
void setVolume(int volume);
void playTextToSpeech(const char* text);
bool playHTTPStream(const char* url);

#ifdef __cplusplus
}
#endif

void AudioControl_updateDisplayInfo();
