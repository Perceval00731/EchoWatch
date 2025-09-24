#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void playMusic();
void setVolume(int volume);
bool playTextToSpeech(const char* text, const char* lang);
bool playHTTPStream(const char* url);

#ifdef __cplusplus
}
#endif

void AudioControl_updateDisplayInfo();
