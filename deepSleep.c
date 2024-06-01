/*
 * deepSleep.c
 *
 *  Created on: 5 Mar 2024
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

/************************************
 * INCLUDES
 ************************************/
#include "deepSleep.h"

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/
#define IS_WAKEUP_TIME_VALID(timer) (timer > 0)

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/
RTC_DATA_ATTR int bootCount = 0;

deepSleep_cfg_t cfg;

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
void printWakeupReason();

/************************************
 * GLOBAL FUNCTIONS
 ************************************/
bool deepSleep_Init(int wakeupTimer_s)
{
    esp_err_t ok = ESP_OK;

    /* Init config */
    cfg.wakeTime = -1;
    cfg.setupStatusOk = false;

    /* Increment boot number */
    ++bootCount;
    /* Print wakeup reason if log level is selected */
    printWakeupReason();
    if (IS_WAKEUP_TIME_VALID(wakeupTimer_s))
    {
        /* Configure wakeup timer */
        ok = esp_sleep_enable_timer_wakeup(wakeupTimer_s * uS_TO_S_FACTOR);
    }
    else
    {
        ok = ESP_FAIL;
    }

    if (ESP_OK == ok)
    {
        cfg.wakeTime = wakeupTimer_s;
    }

    return (ESP_OK == ok);
}

/*
Get the boot count number
*/
int deepSleep_getbootCount()
{
    return bootCount;
}

void deepSleep_goToSleep()
{
    esp_deep_sleep_start();
}

deepSleep_cfg_t deepSleep_getCfg()
{
    return cfg;
}

bool deepSleep_setCfg(deepSleep_cfg_t new_cfg)
{
    esp_err_t ok = ESP_FAIL;
    if (!IS_WAKEUP_TIME_VALID(new_cfg.wakeTime))
    {
        log_e("Wrong configuration time %d", new_cfg.wakeTime);
        return false;
    }

    ok = esp_sleep_enable_timer_wakeup(new_cfg.wakeTime * uS_TO_S_FACTOR);

    if (ESP_OK == ok)
    {
        cfg.wakeTime = new_cfg.wakeTime;
    }

    log_d("new cfg return code %s", esp_err_to_name(ok));
    return (ESP_OK == ok);
}

/************************************
 * STATIC FUNCTIONS
 ************************************/
/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void printWakeupReason()
{
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
        log_d("Wakeup caused by external signal using RTC_IO");
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        log_d("Wakeup caused by external signal using RTC_CNTL");
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        log_d("Wakeup caused by timer");
        break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        log_d("Wakeup caused by touchpad");
        break;
    case ESP_SLEEP_WAKEUP_ULP:
        log_d("Wakeup caused by ULP program");
        break;
    default:
        log_d("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
        break;
    }
}

/*EOF*/
