/*
 * deepSleep.h
 *
 *  Created on: 01 Jun 2024
 *      Author: piemarz
 *
 *  MIT License
 *
 * Copyright (c) 2024 piemarz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef __DEEPSLEEP_H_
#define __DEEPSLEEP_H_

#ifdef __cplusplus
extern "C"
{
#endif

/************************************
 * INCLUDES
 ************************************/
#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

/************************************
 * MACROS AND DEFINES
 ************************************/
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */

/************************************
 * TYPEDEFS
 ************************************/
typedef struct {
    int wakeTime;
    bool setupStatusOk;
} deepSleep_cfg_t;

/************************************
 * EXPORTED VARIABLES
 ************************************/

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/
bool deepSleep_Init(int wakeupTimer_s);
int deepSleep_getbootCount();
void deepSleep_goToSleep();
deepSleep_cfg_t deepSleep_getCfg();
bool deepSleep_setCfg(deepSleep_cfg_t new_cfg);

#ifdef __cplusplus
}
#endif

#endif /* __DEEPSLEEP_H_ */
