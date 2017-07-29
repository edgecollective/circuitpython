/*
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Craig Versek and Don Blair for EdgeCollective
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "sleepydog.h"
#include "asf/common/services/sleepmgr/sleepmgr.h"
#include <power.h>
//#include <sam.h>

volatile bool _sleepydog_initialized = false;

int sleepydog_enable(int maxPeriodMS, bool isForSleep) {
    // Enable the watchdog with a period up to the specified max period in
    // milliseconds.

    // Review the watchdog section from the SAMD21 datasheet section 17:
    //   http://www.atmel.com/images/atmel-42181-sam-d21_datasheet.pdf

    int     cycles, actualMS;
    uint8_t bits;

    if(!_sleepydog_initialized) sleepydog_initialize_wdt();

    WDT->CTRL.reg = 0; // Disable watchdog for config
    while(WDT->STATUS.bit.SYNCBUSY);

    if((maxPeriodMS >= 16000) || !maxPeriodMS) {
        cycles = 16384;
        bits   = 0xB;
    } else {
        cycles = (maxPeriodMS * 1024L + 500) / 1000; // ms -> WDT cycles
        if(cycles >= 8192) {
            cycles = 8192;
            bits   = 0xA;
        } else if(cycles >= 4096) {
            cycles = 4096;
            bits   = 0x9;
        } else if(cycles >= 2048) {
            cycles = 2048;
            bits   = 0x8;
        } else if(cycles >= 1024) {
            cycles = 1024;
            bits   = 0x7;
        } else if(cycles >= 512) {
            cycles = 512;
            bits   = 0x6;
        } else if(cycles >= 256) {
            cycles = 256;
            bits   = 0x5;
        } else if(cycles >= 128) {
            cycles = 128;
            bits   = 0x4;
        } else if(cycles >= 64) {
            cycles = 64;
            bits   = 0x3;
        } else if(cycles >= 32) {
            cycles = 32;
            bits   = 0x2;
        } else if(cycles >= 16) {
            cycles = 16;
            bits   = 0x1;
        } else {
            cycles = 8;
            bits   = 0x0;
        }
    }

    if(isForSleep) {
        WDT->INTENSET.bit.EW   = 1;      // Enable early warning interrupt
        WDT->CONFIG.bit.PER    = 0xB;    // Period = max
        WDT->CONFIG.bit.WINDOW = bits;   // Set time of interrupt
        WDT->CTRL.bit.WEN      = 1;      // Enable window mode
        while(WDT->STATUS.bit.SYNCBUSY); // Sync CTRL write
    } else {
        WDT->INTENCLR.bit.EW   = 1;      // Disable early warning interrupt
        WDT->CONFIG.bit.PER    = bits;   // Set period for chip reset
        WDT->CTRL.bit.WEN      = 0;      // Disable window mode
        while(WDT->STATUS.bit.SYNCBUSY); // Sync CTRL write
    }

    actualMS = (cycles * 1000L + 512) / 1024; // WDT cycles -> ms

    sleepydog_reset();                  // Clear watchdog interval
    //ugly hack to prevent clash with ATMEL's stupid def
    #undef ENABLE
    WDT->CTRL.bit.ENABLE = 1; // Start watchdog now!
    #define ENABLE 1
    while(WDT->STATUS.bit.SYNCBUSY);

    return actualMS;
}

void sleepydog_reset() {
// Write the watchdog clear key value (0xA5) to the watchdog
    // clear register to clear the watchdog timer and reset it.
    WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
    while(WDT->STATUS.bit.SYNCBUSY);
}

void sleepydog_disable() {
    //ugly hack to prevent clash with ATMEL's stupid def
    #undef ENABLE
    WDT->CTRL.bit.ENABLE = 0;
    #define ENABLE 1
    while(WDT->STATUS.bit.SYNCBUSY);
}

void WDT_Handler(void) {
    // ISR for watchdog early warning, DO NOT RENAME!
    //ugly hack to prevent clash with ATMEL's stupid def
    #undef ENABLE
    WDT->CTRL.bit.ENABLE = 0;        // Disable watchdog
    #define ENABLE 1
    while(WDT->STATUS.bit.SYNCBUSY); // Sync CTRL write
    //WDT->INTFLAG.bit.EW  = 1;        // Clear interrupt flag
}

int sleepydog_sleep(int maxPeriodMS) {

    int actualPeriodMS = sleepydog_enable(maxPeriodMS, true); // true = for sleep

    system_set_sleepmode(SYSTEM_SLEEPMODE_STANDBY); // Deepest sleep
    system_sleep();
    // Code resumes here on wake (WDT early warning interrupt)

    return actualPeriodMS;
}

void sleepydog_initialize_wdt() {

    // One-time initialization of watchdog timer.
    // Insights from rickrlh and rbrucemtl in Arduino forum!

    // Generic clock generator 2, divisor = 32 (2^(DIV+1))
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(2) | GCLK_GENDIV_DIV(4);
    // Enable clock generator 2 using low-power 32KHz oscillator.
    // With /32 divisor above, this yields 1024Hz(ish) clock.
    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(2) |
                        GCLK_GENCTRL_GENEN |
                        GCLK_GENCTRL_SRC_OSCULP32K |
                        GCLK_GENCTRL_DIVSEL;
    while(GCLK->STATUS.bit.SYNCBUSY);
    // WDT clock = clock gen 2
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_WDT |
                        GCLK_CLKCTRL_CLKEN |
                        GCLK_CLKCTRL_GEN_GCLK2;

    // Enable WDT early-warning interrupt
    NVIC_DisableIRQ(WDT_IRQn);
    NVIC_ClearPendingIRQ(WDT_IRQn);
    NVIC_SetPriority(WDT_IRQn, 0); // Top priority
    NVIC_EnableIRQ(WDT_IRQn);

    _sleepydog_initialized = true;
}
