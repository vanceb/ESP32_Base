/*
 *    SevenSeg.h - A library to control a strip of 7-Segment LEDs
 *    Copyright (c) 2021 Vance Briggs
 * 
 *    Based upon:
 * 
 *    LedControl.h - A library for controling Leds with a MAX7219/MAX7221
 *    Copyright (c) 2007 Eberhard Fahle
 * 
 *    Permission is hereby granted, free of charge, to any person
 *    obtaining a copy of this software and associated documentation
 *    files (the "Software"), to deal in the Software without
 *    restriction, including without limitation the rights to use,
 *    copy, modify, merge, publish, distribute, sublicense, and/or sell
 *    copies of the Software, and to permit persons to whom the
 *    Software is furnished to do so, subject to the following
 *    conditions:
 * 
 *    This permission notice shall be included in all copies or 
 *    substantial portions of the Software.
 * 
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *    OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SevenSeg_h
#define SevenSeg_h

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

/*
 * Segments to be switched on for characters and digits on
 * 7-Segment Displays
 * 
 * Bit numbers for each segment
 * 
 *     666
 *    1   5
 *    1   5
 *     000
 *    2   4
 *    2   4
 *     333  7
 * 
 * So: 0x01 - Would light the central segment
 * and 0x14 - Would light the 2 rightmost segments giving a digit 1
 * 
 */
const static byte charTable [] PROGMEM  = {
    B01111110,B00110000,B01101101,B01111001,B00110011,B01011011,B01011111,B01110000,
    B01111111,B01111011,B01110111,B00011111,B00001101,B00111101,B01001111,B01000111,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B10000000,B00000001,B10000000,B00000000,
    B01111110,B00110000,B01101101,B01111001,B00110011,B01011011,B01011111,B01110000,
    B01111111,B01111011,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B01110111,B00011111,B00001101,B00111101,B01001111,B01000111,B00000000,
    B00110111,B00000000,B00000000,B00000000,B00001110,B00000000,B00000000,B00000000,
    B01100111,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00001000,
    B00000000,B01110111,B00011111,B00001101,B00111101,B01001111,B01000111,B00000000,
    B00110111,B00000100,B00000000,B00000000,B00001110,B00000000,B00010101,B00011101,
    B01100111,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,
    B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000
};

class SevenSeg {
    private:
        /* Send out a single command to the device */

        /* Send the data in the spiBuffer */
        uint8_t spiSend();

        /* Clear the spiBuffer */
        uint8_t clearSpiBuffer();

        /* Load a command into the spiBuffer
         * If addr=0 then load the same command to all drivers
         * Otherwise just the single driver indexed from 1
         * Needs a call to spiSend() to actually send the command
         */
        uint8_t setCmd(byte cmd, byte data, uint8_t addr=0);

        /* Number of digits in the display */
        /* Should be a multiple of 8 for MAX7219/MAX7221 */
        uint16_t numDigits;   // Number of 7-segment digits
        uint8_t  numDrivers;  // Number of MAX72XX devices, should be numDigits/8

        /* Pointers to display buffers */
        byte *pDispBuffer;
        uint16_t szDispBuffer;
        uint8_t writeBuffer;  // Buffer 0 or Buffer 1 to be written to - display the other

        /* Pointer to SPI data to be transferred */
        uint8_t szSpiBuffer;
        byte *pSpiBuffer;

        /* Should we update the display? - Gets set to True after we flip the buffer 
         * Allows for asynchronous transfers, but by default a call to flipBuffer
         * will send the data to the drivers
        */
        bool displayDirty;

        /* Data is shifted out of this pin*/
        uint8_t SPI_MOSI;
        /* The clock is signaled on this pin */
        uint8_t SPI_CLK;
        /* Chip Select is driven LOW for data transfer */
        uint8_t SPI_CS;

    public:
        /* 
         * Create a new controler 
         * Params :
         * dataPin		pin on the Arduino where data gets shifted out
         * clockPin		pin for the clock
         * csPin		pin for selecting the device 
         * numDigits	maximum number of digits in the display
         */
        SevenSeg(uint8_t dataPin, uint8_t clkPin, uint8_t csPin, uint16_t numDigits=8);

        /* Fill the displayBuffer with a value 
         * 
         * v         = The value to be written
         * writeOnly = If true only fill the buffer which is being written to
         *             If false then fill the entire double buffer
        */
        uint16_t fillBuffer(byte v=0, bool writeOnly=true);

        /* Clear the display buffer (i.e. set all values to 0)
         * 
         * writeOnly = If true only fill the buffer which is being written to
         *             If false then fill the entire double buffer
         */
        uint16_t clearBuffer(bool writeOnly=true);

        /* Flip the display buffer and optionally update the display
         *
         * update = If true then update the display synchronously
         *          If not then mark the buffer as dirty ro allow
         *          asynchronous updaye of the display
         */
        uint8_t  flipBuffer(bool update=true);

        /* Set a display digit to show a raw value
         *
         * addr = The index of the digit to set, starting at 0
         * v    = The raw value
         */
        uint16_t   set(uint16_t addr, byte v);

        /* Overlay (OR) a display digit with a raw value
         *
         * addr = The index of the digit to set, starting at 0
         * v    = The raw value
         */
        uint16_t   overlay(uint16_t addr, byte v);

        /* Exclusive Or (XOR) a display digit with a raw value
         *
         * addr = The index of the digit to set, starting at 0
         * v    = The raw value
         */
        uint16_t   exor(uint16_t addr, byte v);

        /* Best Effort output string to display buffer 
         *
         * addr = Index of start position
         * msg  = char* containing the string to be output
         */
        uint16_t   print(char* msg, uint16_t addr);

        /* Update the display
         *
         * Transfer the live display buffer data to the drivers using SPI
         */
        uint8_t  updateDisplay();

        /* Get the number of digits in the display */
        uint16_t length();
};

/* A simple display of a line passing across the display fropm left to right */
class Bullet {
    private:
        uint16_t position;

    public:
        Bullet();
        uint16_t update(SevenSeg display);
};


/* A digital clock for the display */
class DigitalClock {
    private:
        bool extraSpaces;       // Put extra spaces between the hours, minutes and seconds?
        uint8_t secondsMode;    // How are we showing the seconds?
        uint16_t position;


    public:
        /* Define the seconds modes */
        #define SM_NONE     0   // Don't display seconds at all
        #define SM_DIGITAL  1   // Show seconds digitally as part of the clock
        #define SM_BORDER   2   // Show seconds using the entire border like a second hand
        #define SM_BORDER_F 3   // Show seconds using the entire border cumulatively
        #define SM_SIDES    4   // Show the seconds using space at both sides

        DigitalClock();
        bool setMode(SevenSeg display, uint8_t mode=SM_DIGITAL, uint16_t pos=0, bool extra_spaces=false);
        uint8_t set_24h(bool tf=true);
        uint8_t update(SevenSeg display, time_t t);
};


class TimeUp {
    private:
        static const uint8_t length = 13;
        uint16_t position;

    public:
        TimeUp(SevenSeg display, uint16_t pos=0);
        uint8_t update(SevenSeg display);
};




/* General */
#define MAX_REFRESH_RATE        50
#define DISPLAY_REFRESH_RATE    40

/* Neopixel */
//#define DATA_PIN    33
//#define NUM_LEDS    1

/* FreeRTOS Task to create and update the display 
 * 
 * Started from the main function
 */
void display(void * parameters);

#endif