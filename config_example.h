
/*
 * config.h
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

#ifndef CONFIG_H_
#define CONFIG_H_

#define TIME_TO_SLEEP   30        /* Time ESP32 will go to sleep (in seconds) */
#define DHT_PIN         27        /* ESP32 communication pin with DHT11/DHT22 */

const char WIFI_SSID[] = "YOUR_WIFI_SSID";     // CHANGE TO YOUR WIFI SSID
const char WIFI_PASSWORD[] = "YOUR_WIFI_PASSWORD";  // CHANGE TO YOUR WIFI PASSWORD

const char MQTT_BROKER_ADRRESS[] = "test.mosquitto.org";  // CHANGE TO MQTT BROKER'S ADDRESS
const int MQTT_PORT = 1883;
const char MQTT_CLIENT_ID[] = "YOUR-NAME-esp32-001";  // CHANGE IT AS YOU DESIRE
const char MQTT_USERNAME[] = "";                        // CHANGE IT IF REQUIRED, empty if not required
const char MQTT_PASSWORD[] = "";                        // CHANGE IT IF REQUIRED, empty if not required

// The MQTT topics that ESP32 should publish/subscribe
const char PUBLISH_TOPIC[] = "YOUR-NAME-esp32-001/loopback";    // CHANGE IT AS YOU DESIRE
const char SUBSCRIBE_TOPIC[] = "YOUR-NAME-esp32-001/loopback";  // CHANGE IT AS YOU DESIRE

#endif /* CONFIG_H_ */
