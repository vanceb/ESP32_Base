#include <Arduino.h>
#include <serial_display.h>
#include <WiFi.h>
#include <etask_wifi.h>
#include <uptime_formatter.h>

#define LED_PIN 2
#define LED_CHANNEL 0
#define FREQ 5000
#define RESOLUTION 8

void display(void * parameters) {

    // Make sure WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Waiting for wifi connection");
        while (WiFi.status() != WL_CONNECTED) {
            Serial.print(".");
            delay(1000);
        }
        Serial.println("\nConnected!");
    }

    /* Configure onboard LED */
    ledcSetup(LED_CHANNEL, FREQ, RESOLUTION);
    ledcAttachPin(LED_PIN,LED_CHANNEL);
    
    
    /* Infinite loop needed for FreeRTOS Task */
    unsigned long counter = 0;
    while (true) {
        if (counter % 100 == 0) {
            /* Do something every second */
            Serial.print(getEpochStringByParams(UK.toLocal(now())));
            Serial.println((" up " + uptime_formatter::getUptime()));
        }
        uint8_t brightness = (counter % 128) > 64 ? 128 - counter % 128 : counter % 128;
        ledcWrite(LED_CHANNEL, brightness);
        ++counter;
        delay(10);
    }
}