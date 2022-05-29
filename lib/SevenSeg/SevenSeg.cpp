#include <SevenSeg.h>
#include <keeptime.h>
#include <TideTimes.h>

#include <WiFi.h>
#include <stdio.h>

//#include <FastLED.h>

//#define PRINT_SPARE_STACK

//the opcodes for the MAX7221 and MAX7219
#define OP_NOOP   0
#define OP_DIGIT0 1
#define OP_DIGIT1 2
#define OP_DIGIT2 3
#define OP_DIGIT3 4
#define OP_DIGIT4 5
#define OP_DIGIT5 6
#define OP_DIGIT6 7
#define OP_DIGIT7 8
#define OP_DECODEMODE  9
#define OP_INTENSITY   10
#define OP_SCANLIMIT   11
#define OP_SHUTDOWN    12
#define OP_DISPLAYTEST 15

SevenSeg::SevenSeg(uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint16_t num_digits) {
    SPI_MOSI=dataPin;
    SPI_CLK=clkPin;
    SPI_CS=csPin;
    numDigits = num_digits;

    /* Make numDigits a multiple of 8 */
    if (numDigits == 0 || numDigits % 8 != 0) {
        numDigits = 8 * ((numDigits / 8) + 1);
    }
    numDrivers = numDigits / 8;

    writeBuffer = 1;  // Which buffer should get updates (non live buffer)?
    displayDirty = false;  // Does the display need updating?
    
    /* Create display buffers (double buffer) */
    szDispBuffer = sizeof(byte) * 2 * numDigits;
    pDispBuffer = (byte*) malloc(szDispBuffer);
    /* Initialise the buffer with 0s */
    fillBuffer(szDispBuffer, 0);

    /* Create the SPI Data transfer buffer */
    szSpiBuffer = sizeof(byte) * 2 * numDrivers;
    pSpiBuffer = (byte*) malloc(szSpiBuffer);

    /* Set pins to output and disable write with SPI_CS */
    pinMode(SPI_MOSI,OUTPUT);
    pinMode(SPI_CLK,OUTPUT);
    pinMode(SPI_CS,OUTPUT);
    digitalWrite(SPI_CS,HIGH);

    /* Initialise the display */
    setCmd(OP_DECODEMODE, 0x00);  // Dont decode, just display bits
    spiSend();
    setCmd(OP_INTENSITY, 0x07);   // Set the intensity to mid range
    spiSend();
    setCmd(OP_SCANLIMIT, 0x07);   // Enable all 8 digits
    spiSend();
    setCmd(OP_SHUTDOWN, 0x01);    // Come out of shutdown mode
    spiSend();
    setCmd(OP_DISPLAYTEST, 0x01); // Display test on
    spiSend();
    delay(1000);
    setCmd(OP_DISPLAYTEST, 0x00); // Display test off
    spiSend();

}

/* 
 * fillBuffer is the workhorse for filling/clearing the display buffer
 *
 * v         - Value to write
 * writeOnly - If true then only writes to the non-displayed buffer
 *             If false then writes to the entire double buffer
 */
uint16_t SevenSeg::fillBuffer(byte v, bool writeOnly) {
    /* Set the start point and size of the fill */
    byte *pBuff = pDispBuffer;
    uint16_t sz = szDispBuffer;
    if (writeOnly) {  // Only overwrite the writable buffer
        pBuff = &pBuff[(writeBuffer * numDigits)];
        sz = numDigits;
    }

    /* Fill buffer with value */
    for (int i=0; i<sz; i++) {
        pBuff[i] = v;
    }
    return sz;
}

/* Clear the writable buffer */
uint16_t SevenSeg::clearBuffer(bool writeOnly) {
   return fillBuffer(0, writeOnly); 
}

/* Flips to the alternative display buffer
 *
 * update - Should we write the contents to the display now,
 *          or flag the dusplay as dirty to be updated elsewhere?
 */
uint8_t SevenSeg::flipBuffer(bool update) {
    writeBuffer = writeBuffer == 0 ? 1 : 0;
    displayDirty = true;
    if (update == true) {
        updateDisplay();
    } 
}

uint8_t SevenSeg::clearSpiBuffer() {
    for (int i=0; i<szSpiBuffer; i++) {
        pSpiBuffer[i] = 0;
    }
}

uint8_t SevenSeg::setCmd(byte cmd, byte data, uint8_t addr) {
    if (addr == 0) {  // Set the same command for all drivers
        for (int i=0; i<numDrivers; i++) {
            pSpiBuffer[2*i]     = data;
            pSpiBuffer[(2*i)+1] = cmd;
        }
    } else {  // Set the command for the addressed driver
        pSpiBuffer[2*addr]     = data;
        pSpiBuffer[(2*addr)+1] = cmd;
    }
    return addr;
}

/* Workhorse SPI transfer function - Sends the contents of the spiBuffer */
uint8_t SevenSeg::spiSend() {
    //enable the line 
    digitalWrite(SPI_CS,LOW);
    //Now shift out the data 
    for(int i=szSpiBuffer; i>0; i--) {
        shiftOut(SPI_MOSI,SPI_CLK,MSBFIRST,pSpiBuffer[i-1]);
    }
    //latch the data onto the display
    digitalWrite(SPI_CS,HIGH);
}

/* Workhorse display update function */
uint8_t SevenSeg::updateDisplay() {
    if (displayDirty) {
        /* Go to the start of the buffer to be displayed */
        byte *pBuff = pDispBuffer;
        if (writeBuffer == 0) {
            pBuff += numDigits;
        }

        for (int i=0; i<8; i++) {  // Send each digit
            for (int j=0; j<numDrivers; j++) {  // To each driver
                pSpiBuffer[(2*j)+1] = i + 1;
                pSpiBuffer[2*j] = pBuff[(8*j)+i];
            }
            spiSend();
        }
        displayDirty = false;
        return true;
    }
    return false;
}

uint16_t SevenSeg::set(uint16_t addr, byte v) {
    if (addr > numDigits) {
        return 0;
    }

    /* Set the start point of the buffer that we are writing to */
    byte *pBuff = pDispBuffer + (writeBuffer * numDigits);
    pBuff[addr] = v;
    return addr;
}

uint16_t SevenSeg::overlay(uint16_t addr, byte v) {
    if (addr > numDigits) {
        return 0;
    }

    /* Set the start point of the buffer that we are writing to */
    byte *pBuff = pDispBuffer + (writeBuffer * numDigits);
    pBuff[addr] |= v;
    return addr;
}

uint16_t SevenSeg::exor(uint16_t addr, byte v) {
    if (addr > numDigits) {
        return 0;
    }

    /* Set the start point of the buffer that we are writing to */
    byte *pBuff = pDispBuffer + (writeBuffer * numDigits);
    pBuff[addr] ^= v;
    return addr;
}

uint16_t SevenSeg::print(char* msg, uint16_t addr) {
    uint16_t cursor = addr;
    /* Write to the display buffer */
    for(int i=0; i<strlen(msg); i++) {
        if (cursor < 0 || cursor > length()) {
            return cursor;
        }
        if (msg[i] == '.' || msg[i] == ':') {
            --cursor;
            overlay(cursor, 0x80);
        } else {
            set(cursor, charTable[msg[i]]);
        }
        ++cursor;
    }
}

uint16_t SevenSeg::length() {
    return numDigits;
}

Bullet::Bullet() {
    position = 0;
}

uint16_t Bullet::update(SevenSeg display) {
    display.set(position, 0x01);
    position = (position + 1) % display.length();
    return position;
}

DigitalClock::DigitalClock() {
    secondsMode = SM_DIGITAL;
    extraSpaces = false;
    position = 0;
}

bool DigitalClock::setMode(SevenSeg display, uint8_t mode, uint16_t pos, bool extra_spaces) {
    secondsMode = mode;
    switch (mode) {
        case SM_BORDER:
        case SM_BORDER_F:
        case SM_SIDES:
            if(display.length() != 32) 
                return false;
            position = 14;
            extraSpaces = false;
            break;
        default:
            extraSpaces = extra_spaces;
        /* Work out the length of the time string */
            uint8_t length = 4;
            if(secondsMode == SM_DIGITAL) {
                length += 2;
            }
            if(extraSpaces) {
                length += 1;
                if(secondsMode == SM_DIGITAL) {
                    length += 1;
                }
            }

            if(pos < display.length() - length) {
                position = pos;
            }
            break;
    }
    return true;
}

uint8_t DigitalClock::update(SevenSeg display, time_t t) {
    /* Deal with the printed digits */
    char charTime[9];

    switch (secondsMode) {
        case SM_DIGITAL:
            if (extraSpaces) {
                sprintf(charTime, "%02d %02d %02d", hour(t), minute(t), second(t));
            } else {
                sprintf(charTime, "%02d%02d%02d", hour(t), minute(t), second(t));
            }
            break;
        default:
            if (extraSpaces) {
                sprintf(charTime, "%02d %02d", hour(t), minute(t));
            } else {
                sprintf(charTime, "%02d%02d", hour(t), minute(t));
            }
            break;
    }          
    /* Write time to display buffer */
    display.print(charTime, position);

    /* Flash the decimal point */
    if (second(t) % 2 == 0) {
        display.exor(position + 1, 0x80);  // after the hours
        if (secondsMode == SM_DIGITAL) {
            if (extraSpaces) {
                display.exor(position + 4, 0x80);
            } else {
                display.exor(position + 3, 0x80);
            }
        }
    }

    /* Deal with the "seconds hand"
     *
     * These modes are only valid for a 32 digit display
     */
    switch (secondsMode) {
        case SM_BORDER:
            uint8_t sec = second(t);
            if (sec == 0) {
                display.exor(13, 0x40);
            } else if (sec > 0 && sec <= 14) {
                display.exor(17 + sec, 0x40);
            } else if(sec==15) {
                display.exor(31, 0x20);
            } else if(sec==16) {
                display.exor(31, 0x10);
            } else if(sec>16 && sec <=30) {
                display.exor(31-(sec-17), 0x08);
            } else if(sec>30 && sec<=44) {
                display.exor(13-(sec-31), 0x08);
            } else if(sec==45) {
                display.exor(0, 0x04);
            } else if(sec==46) {
                display.exor(0, 0x02);
            } else {
                display.exor(sec - 47, 0x40);
            }
    }
}

#include <uptime.h>

TimeUp::TimeUp(SevenSeg display, uint16_t pos) {
    position = 0;
    if ((pos + length <= display.length())) {
        position = pos;
    }
}

uint8_t TimeUp::update(SevenSeg display) {
    uptime::calculateUptime();
    char charUptime[16];
    snprintf(charUptime, 15, "%03d %02d %02d %02d.%d", 
            uptime::getDays(),
            uptime::getHours(),
            uptime::getMinutes(),
            uptime::getSeconds(),
            uptime::getMilliseconds()
    );
    /* Write to the display */
    display.print(charUptime, 0);
    return 1;
}

/* FreeRTOS Task for updating the display */
void display(void * parameters) {

    // Calculate the delay needed to meet the refresh rate (approx)
    int r = DISPLAY_REFRESH_RATE;
    r = r < MAX_REFRESH_RATE ? r : MAX_REFRESH_RATE;
    long d = 1000 / r;

    Serial.print("d: ");
    Serial.println(d);
    Serial.print("r: ");
    Serial.println(r);

    SevenSeg disp = SevenSeg(18, 5, 32, 32);
//    Bullet bullet = Bullet();
    DigitalClock digiclk = DigitalClock();
    digiclk.setMode(disp, SM_BORDER);
//    TimeUp timeup = TimeUp(disp, 19);
    TimeUp timeup = TimeUp(disp, 0);

    /* Create a Tides object and a task to keep it updated */
    /*
    Tides tides = Tides("0025", 2);
    TaskHandle_t tides_update_task;
    xTaskCreate(
                tidesTask,
                "Tides Update Task",
                5500,
                &tides,
                0,
                &tides_update_task
    );
    */

    int lastMinute = -1;
    for(;;) {
        // Manage loop delay, looking out for overruns
        // Works with the delay at the bottom of this loop
        unsigned long m = millis();
        unsigned long refresh = m + d;
        if(refresh < m) {
            // we have overrun the long counter
            refresh = d;
        }

        // Get the fixed time for this loop
        time_t t = UK.toLocal(now());
    
        /* Display a test pattern on the 7-Seg Display */
        disp.clearBuffer();
        /* Update tide data display */
        //tides.update_display(disp, 20, tides.next_tide());

//        bullet.update(disp);        
        timeup.update(disp);
        digiclk.update(disp, t);
        disp.flipBuffer(true);

        // Account for loop processing time
        // Software I2C for OLED slow
        if(millis() < refresh) {
            delay(refresh - millis());
        }

        #ifdef PRINT_SPARE_STACK
        Serial.print("Tides update spare stack: ");
        Serial.println(uxTaskGetStackHighWaterMark(tides_update_task));
        #endif

        /* Ouput loop processing marker for debugging */
        Serial.print("d");
    }
}