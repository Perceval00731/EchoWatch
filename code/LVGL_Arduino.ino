/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include "Display_SPD2010.h"
#include "Audio_PCM5101.h"
#include "RTC_PCF85063.h"
#include "Gyro_QMI8658.h"
#include "LVGL_Driver.h"
#include "MIC_MSM.h"
#include "PWR_Key.h"
#include "SD_Card.h"
#include "LVGL_Example.h"
#include "BAT_Driver.h"
#include "ui.h"
#include <WiFi.h>

const char* ssid = "iPhone de Melvin";
const char* password = "motdepasse2";


void Driver_Loop(void *parameter)
{
  Wireless_Test2();
  while(1)
  {
    PWR_Loop();
    QMI8658_Loop();
    PCF85063_Loop();
    BAT_Get_Volts();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
void Driver_Init()
{
  Flash_test();
  PWR_Init();
  BAT_Init();
  I2C_Init();
  TCA9554PWR_Init(0x00);   
  Backlight_Init();
  Set_Backlight(50);      //0~100 
  PCF85063_Init();
  QMI8658_Init(); 
  
}
void setup() {
  Serial.begin(115200);
  Serial.println("Début");

  // Init drivers qui ne consomment pas trop de RAM interne
  Driver_Init();
  Serial.println("Driver Init Done");

  // Ensuite init périphériques gourmands en RAM
  SD_Init();
  Serial.println("SD Init Done");
  Audio_Init();
  Serial.println("Audio Init Done");
  MIC_Init();
  Serial.println("MIC Init Done");
  LCD_Init();
  Serial.println("LCD Init Done");
  Lvgl_Init();
  Serial.println("LVGL Init Done");

  ui_init();
  Serial.println("UI Init Done");

  //WiFi.mode(WIFI_STA);
  //WiFi.disconnect(true);
  //delay(100);
  //Serial.println("Clean Wifi Done");

  //WiFi.begin(ssid, password);
  //Serial.println("\nConnecting");

  //unsigned long start = millis();
  //while(WiFi.status() != WL_CONNECTED && millis() - start < 10000) { // 10s timeout
      //Serial.print(".");
      //delay(100);
  //}

  //if(WiFi.status() == WL_CONNECTED){
      //Serial.println("\nConnected to WiFi");
      //Serial.print("Local IP: "); Serial.println(WiFi.localIP());
  //} else {
      //Serial.println("\nWiFi connection failed!");
  //}

  xTaskCreatePinnedToCore(
    Driver_Loop,
    "Other Driver task",
    4096,
    NULL,
    3,
    NULL,
    0
  );
}

void loop() {
  Lvgl_Loop();
  vTaskDelay(pdMS_TO_TICKS(5));

}
