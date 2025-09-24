#include "AudioControl.h"

#include <string.h>

#include "Audio_PCM5101.h"
#include "ui.h"

extern Audio audio;

namespace {
volatile bool g_streamFinished = false;
char g_lastStreamInfo[64] = {0};
}

void audio_info(const char* info) {
  if (!info) {
    return;
  }
  Serial.print("[Audio] ");
  Serial.println(info);
}

void audio_eof_stream(const char* info) {
  Serial.println("Lecture du flux HTTP terminée");
  if (info) {
    Serial.print("[Audio] Source: ");
    Serial.println(info);
    strncpy(g_lastStreamInfo, info, sizeof(g_lastStreamInfo) - 1);
    g_lastStreamInfo[sizeof(g_lastStreamInfo) - 1] = '\0';
  } else {
    g_lastStreamInfo[0] = '\0';
  }
  g_streamFinished = true;
}

extern "C" void playMusic() {
  if (audio.isRunning()) {
    printf("Le son en cours, pause.\n");
    Music_pause();
  } else {
    printf("Lecture du son.\n");
    Volume_adjustment(21);
    Play_Music("/", "ptitsondetest.mp3");
  }
}

extern "C" void setVolume(int volume) {
  if (volume < 0) {
    volume = 0;
  }
  if (volume > 21) {
    volume = 21;
  }
  Volume_adjustment(volume);
}

void AudioControl_updateDisplayInfo() {
  if (ui_DurationSlider && ui_DurationLabel && audio.isRunning()) {
    uint32_t musicDuration = audio.getAudioFileDuration();
    uint32_t musicElapsed = audio.getAudioCurrentTime();

    if (musicDuration > 0) {
      int sliderValue = (musicElapsed * 100) / musicDuration;
      lv_slider_set_value(ui_DurationSlider, sliderValue, LV_ANIM_OFF);

      char buf[16];
      int elapsedMin = musicElapsed / 60;
      int elapsedSec = musicElapsed % 60;
      int durationMin = musicDuration / 60;
      int durationSec = musicDuration % 60;

      snprintf(buf, sizeof(buf), "%02d:%02d / %02d:%02d",
               elapsedMin, elapsedSec, durationMin, durationSec);
      lv_label_set_text(ui_DurationLabel, buf);
    }
  } else if (ui_DurationLabel) {
    lv_label_set_text(ui_DurationLabel, "00:00 / 00:00");
    if (ui_DurationSlider) {
      lv_slider_set_value(ui_DurationSlider, 0, LV_ANIM_OFF);
    }
  }
}

bool playTextToSpeech(const char* text, const char* lang) {
  if (audio.isRunning()) {
    audio.stopSong();
  }
  return audio.connecttospeech(text, lang);
}

bool playHTTPStream(const char* url) {
  if (audio.isRunning()) {
    audio.stopSong();
  }

  bool started = audio.connecttohost(url);
  if (!started) {
    Serial.println("Impossible de démarrer le flux HTTP");
  }

  return started;
}

void AudioControl_stop() {
  if (audio.isRunning()) {
    audio.stopSong();
  }
}

bool AudioControl_isRunning() {
  return audio.isRunning();
}

bool AudioControl_consumeStreamFinished(char* infoBuffer, size_t bufferLen) {
  if (!g_streamFinished) {
    return false;
  }

  g_streamFinished = false;
  if (infoBuffer && bufferLen > 0) {
    if (g_lastStreamInfo[0] != '\0') {
      strncpy(infoBuffer, g_lastStreamInfo, bufferLen - 1);
      infoBuffer[bufferLen - 1] = '\0';
    } else {
      infoBuffer[0] = '\0';
    }
  }
  return true;
}
