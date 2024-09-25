#include "Arduino.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h> 
#include <TimeLib.h>
#include <Timezone.h>
#include <NTPClient.h>
#include <Wire.h>

#include <etask_wifi.h>
#include <etask_ota.h>
#include <etask_serial_clock.h>

// Uncomment to output the amount of spare task stack
//#define PRINT_SPARE_STACK

TaskHandle_t wifi_task;
TaskHandle_t ota_task;
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
  /* Show the project name */
  Serial.printf("Project Name: %s\n", PROJECT_NAME);

  // Create a task to display time and other data
  xTaskCreate(
    etask_serial_clock,
    "Display_Task",
    3000,
    NULL,
    0,
    &display_task
  );

  // Create a task to get time updates from NTP
  xTaskCreate(
    etask_wifi,
    "WiFi_Task",
    5000,
    NULL,
    0,
    &wifi_task
  );

  // Create a task to get OTA updates
  xTaskCreate(
    etask_ota,
    "OTA_Task",
    8000,
    NULL,
    0,
    &ota_task
  );
}

void loop()
{
  delay(1000);
  
  /* Get the high watermark stack for each task */
  int spare_wifi    = uxTaskGetStackHighWaterMark(wifi_task);
  int spare_ota     = uxTaskGetStackHighWaterMark(ota_task);
  int spare_display = uxTaskGetStackHighWaterMark(display_task);

  /* Good for testing - print spare stack each loop */
  #ifdef PRINT_SPARE_STACK
  Serial.print("wifi spare stack: ");
  Serial.println(uxTaskGetStackHighWaterMark(wifi_task));
  Serial.print("ota spare stack: ");
  Serial.println(uxTaskGetStackHighWaterMark(ota_task));
  Serial.print("display spare stack: ");
  Serial.println(uxTaskGetStackHighWaterMark(display_task));
  #endif

  /* Provide warnings of low stack space */
  if(spare_wifi < 100) {
    Serial.print("wifi_task low stack space: ");
    Serial.println(spare_wifi);
  }
  if(spare_ota < 100) {
    Serial.print("ota_task low stack space: ");
    Serial.println(spare_ota);
  }
  if(spare_display < 100) {
    Serial.print("display_task low stack space: ");
    Serial.println(spare_display);
  }
}