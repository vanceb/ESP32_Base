#include <TideTimes.h>
#include <ArduinoJson.h>
#include <admiralty_api.h>

#include <HTTPClient.h>

#include <time.h>
#include <TimeLib.h>


Tides::Tides(char* station_id, uint8_t duration) {
    snprintf(admiralty_url, MAX_URL_LENGTH, ADMIRALTY_URL_TEMPLATE, station_id, duration);
    next_update = now();
    num_tide_events = 0;
    //update(true);
}

/* Needed as Arduino seems to have lost strpdate() function */
time_t Tides::getTime(const char* timestring) {
    int year, month, date, hour, minute, second;
    sscanf(timestring,"%d-%d-%dT%d:%d:%d", &year, &month, &date, &hour, &minute, &second);
    struct tm t;
    t.tm_year = year - 1900;
    t.tm_mon  = month - 1;
    t.tm_mday = date;
    t.tm_hour = hour;
    t.tm_min  = minute;
    t.tm_sec  = second;

    //Serial.printf("Date: %4d-%02d-%02dT%02d:%02d:%02d\n", year, month, date, hour, minute, second);
    return mktime(&t); 
}

int Tides::update(boolean force) {
    if (force || difftime(next_update, now()) < 0) {
        /* Get the tide data from the Admiralty API */
        Serial.println("Getting Admiralty Data");
        Serial.println(admiralty_url);
        HTTPClient https;
        https.begin(admiralty_url, root_ca);
        https.addHeader("Ocp-Apim-Subscription-Key", API_KEY);
        int httpCode = https.GET();

        /* Check that we were successful */
        if (httpCode > 0) {
            String payload = https.getString();
            Serial.println(httpCode);
            Serial.println(payload);

            /* Parse the JSON data */
            const int capacity = JSON_ARRAY_SIZE(10) + 10 * JSON_OBJECT_SIZE(7) + 70 * JSON_OBJECT_SIZE(1);
            DynamicJsonDocument doc(capacity);
            DeserializationError err = deserializeJson(doc, payload);

            /* In case of error have an exponential backoff */
            uint16_t error_backoff = 15;  // Exponential backoff (seconds) for querying API
            const uint16_t max_error_backoff = 480;  // Max backoff

            /* Did we get an error? */
            if(err) {
                Serial.print(F("deserialization of JSON failed with code "));
                Serial.println(err.f_str());
                /* Exponential backoff for getting data from the API in case of error */
                next_update = now() + error_backoff;
                error_backoff = error_backoff > max_error_backoff / 2 ? max_error_backoff : 2 * error_backoff;
            } else {
                /* Parse the JSON objects into tide_events */
                num_tide_events = 0;
                int i = 0;
                char future;
                JsonArray arr = doc.as<JsonArray>();
                for (JsonObject ev : arr) {
                    if (i >= MAX_EVENTS) {
                        /* Exit loop if about to overrun tide event array */
                        break;
                    }
                    /* Set parsed tide information */
                    memset(tides[i].event, '\0', sizeof(tides[i].event));
                    strncpy(tides[i].event, ev["EventType"], 2);
                    tides[i].height = ev["Height"];
                    tides[i].when = getTime(ev["DateTime"]);

                    /* Output the parsed tide data */
                    if (difftime(tides[i].when, now()) > 0) {
                        future = '*';
                    } else {
                        future = ' ';
                    }
                    Serial.printf("Event: %s When: %d Height %f %c\n", tides[i].event, tides[i].when, tides[i].height, future);

                    i++;
                }
                num_tide_events = i;
                next_update = tides[next_tide()].when;
                /* Reset error backoff after successful update */
                error_backoff = 15;
                return num_tide_events;
            }
        }
    }
    return 0;
}

int8_t Tides::next_tide() {
    for (int i=0; i<num_tide_events; i++) {
        if (difftime(tides[i].when, now()) > 0) {
            return i;
        }
    }
    return -1;
}

struct tide_event Tides::tide(uint8_t id) {
    if(id >= 0 && id < num_tide_events) {
        return tides[id];
    }
    struct tide_event t = {.event="bd", .when=now(), .height=0.0};
    return t;
}

void Tides::update_display(SevenSeg display, uint16_t position, uint8_t tide_id) {
    if (position + 11 > display.length()) {
        return;
    }
    char charTide[16];
    snprintf(charTide, 14, "%s %02d.%02d %0.3f", 
                    tides[tide_id].event,
                    hour(tides[tide_id].when),
                    minute(tides[tide_id].when),
                    tides[tide_id].height
    );
    /* Write to the display buffer */
    display.print(charTide, position);
}


/* FreeRTOS task to update the tide data 
 * 
 * A pointer to a Tides object must be passed in as a parameter
 */
void tidesTask(void* parameters) {
    Tides *pTides = (Tides*) parameters;
    for (;;) {
        pTides->update();
        delay(1000);
        Serial.print('a');
    }
}