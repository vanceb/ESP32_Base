#ifndef TIDE_TIMES_H
#define TIDE_TIMES_H

#include <time.h>
#include <Arduino.h>

#include <SevenSeg.h>

#define ADMIRALTY_URL_TEMPLATE  "https://admiraltyapi.azure-api.net/uktidalapi/api/V1/Stations/%s/TidalEvents?duration=%d"
#define MAX_URL_LENGTH          128
#define MAX_EVENTS              10  // Maximum number of tide events to track

struct tide_event {
    char    event[3];  // "Hi" or "Lo"
    time_t  when;
    float   height;
};

/* Tides class gets tide times from the Admiralty web service */

class Tides {
    private:
        char    admiralty_url[MAX_URL_LENGTH];
        time_t  next_update;
        struct tide_event tides[MAX_EVENTS];
        uint8_t num_tide_events;

        time_t getTime(const char* timestring);

    public:
        /* Create a Tides instance by providing a station_id and a duration */
        Tides(char* station_id, uint8_t duration=2);
        /* Update data from the Admiralty API - optionally force the update */
        int                 update(boolean force=false);
        /* Get the ID of the next tide so we can get the data */
        int8_t              next_tide();
        /* Get the tide event by ID, starting at 0 for first tide of the day */
        struct tide_event   tide(uint8_t id);

        void update_display(SevenSeg display, uint16_t position, uint8_t tide_id);
};

/* FreeRTOS Task to update the tides data 
 * 
 * Needs a pointer to a Tides object passed in as a parameter */
void tidesTask(void * parameters);

#endif