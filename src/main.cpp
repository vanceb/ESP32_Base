#include "Arduino.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h> 
#include <TimeLib.h>
#include <Timezone.h>
#include <NTPClient.h>
#include <Wire.h>

#include <keeptime.h>
#include <ota.h>
#include <serial_display.h>

// Uncomment to output the amount of spare task stack
//#define PRINT_SPARE_STACK

TaskHandle_t ntp_task;
TaskHandle_t display_task;

void setup()
{
  // Configure Serial port for debugging
  Serial.begin(115200);
  delay(2000);
  Serial.println("Setting up...");

  /* Populate the id */
  uint64_t raw_id = ESP.getEfuseMac();
  const char * chip = ESP.getChipModel();
  snprintf(id, CHIP_ID_LEN, "%04X%08X", (uint16_t)(raw_id>>32), (uint32_t)(raw_id));
  Serial.print("ID: ");
  Serial.println(id);

  /* Show the software version */
  Serial.print("Software Version: ");
  Serial.println(AUTO_VERSION);

  // Create a task to display time and other data
  xTaskCreate(
    display,
    "Display_Task",
    3000,
    NULL,
    0,
    &display_task
  );

  // Set up Wifi - Using WiFi Manager
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();
  
  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "ESP Base"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("ESP Base");
  
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  // Create a task to get time updates from NTP
  xTaskCreate(
    keeptime,
    "NTP_Task",
    1500,
    NULL,
    0,
    &ntp_task
  );

  /* Check OTA Firmware */
  ota_update_check();
}

void loop()
{
  delay(1000);
  
  /* Get the high watermark stack for each task */
  int spare_ntp     = uxTaskGetStackHighWaterMark(ntp_task);
  int spare_display = uxTaskGetStackHighWaterMark(display_task);

  /* Good for testing - print spare stack each loop */
  #ifdef PRINT_SPARE_STACK
  Serial.print("ntp spare stack: ");
  Serial.println(uxTaskGetStackHighWaterMark(ntp_task));
  Serial.print("display spare stack: ");
  Serial.println(uxTaskGetStackHighWaterMark(display_task));
  #endif

  /* Provide warnings of low stack space */
  if(spare_ntp < 100) {
    Serial.print("ntp_task low stack space: ");
    Serial.println(spare_ntp);
  }
  if(spare_display < 100) {
    Serial.print("display_task low stack space: ");
    Serial.println(spare_display);
  }
}