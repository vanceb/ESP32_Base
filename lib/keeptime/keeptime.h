#ifndef KEEPTIME_H
#define KEEPTIME_H

#include <Timezone.h>

#define NTP_SERVER "pool.ntp.org"
#define NTP_UPDATE_INTERVAL 15  // minutes
#define NTP_LOOP_DELAY 30000    // ms

/* Timezones with Daylight Saving */
extern TimeChangeRule BST;
extern TimeChangeRule GMT;
extern Timezone UK;

/* FreeRTOS Task to keep time updated by NTP */
void keeptime(void * parameter);

/* Static functions */

/**
 * Input time in epoch format and return tm time format
 * by Renzo Mischianti <www.mischianti.org> 
 */
static tm getDateTimeByParams(long time){
    struct tm *newtime;
    const time_t tim = time;
    newtime = localtime(&tim);
    return *newtime;
}


/**
 * Input tm time format and return String with format pattern
 * by Renzo Mischianti <www.mischianti.org>
 */
static String getDateTimeStringByParams(tm *newtime, char* pattern = (char *)"%d/%m/%Y %H:%M:%S"){
    char buffer[30];
    strftime(buffer, 30, pattern, newtime);
    return buffer;
}


/**
 * Input time in epoch format and return String with format pattern
 * by Renzo Mischianti <www.mischianti.org> 
 */
static String getEpochStringByParams(long time, char* pattern = (char *)"%d/%m/%Y %H:%M:%S"){
//    struct tm *newtime;
    tm newtime;
    newtime = getDateTimeByParams(time);
    return getDateTimeStringByParams(&newtime, pattern);
}

#endif