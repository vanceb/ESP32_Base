#include <serial_display.h>
#include <WiFi.h>
#include <keeptime.h>
#include <uptime_formatter.h>

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
    
    /* Infinite loop needed for FreeRTOS Task */
    while (true) {
        Serial.print(getEpochStringByParams(UK.toLocal(now())));
        Serial.println((" up " + uptime_formatter::getUptime()));
        delay(1000);
    }
}