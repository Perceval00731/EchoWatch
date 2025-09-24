#include "AppInit.h"

#include "Audio_PCM5101.h"
#include "Display_SPD2010.h"
#include "Gyro_QMI8658.h"
#include "LVGL_Driver.h"
#include "RTC_PCF85063.h"
#include "SD_Card.h"
#include "Touch_SPD2010.h"
#include "ui.h"

void AppInit() {
  I2C_Init();
  Backlight_Init();
  Set_Backlight(50);
  LCD_Init();
  Lvgl_Init();
  SD_Init();
  Touch_Init();
  Audio_Init();
  PCF85063_Init();
  QMI8658_Init();
  ui_init();
}
