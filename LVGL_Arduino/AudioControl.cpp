#include "AudioControl.h"

#include "Audio_PCM5101.h"
#include "ui.h"

extern Audio audio;

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
