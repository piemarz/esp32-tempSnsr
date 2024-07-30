/*
 * esp32TempSnsr.ino
 *
 *  Created on: 21 Apr 2024
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
#include <WiFi.h>
#include <DHTesp.h> /** Click here to get the library: http://librarymanager/All#DHTesp */
#include <MQTTClient.h>  /** Click here to get the library: http://librarymanager/All#MQTT */
#include <ArduinoJson.h> /** Click here to get the library: http://librarymanager/All#ArduinoJson */
#include <Preferences.h> /** Click here to get the library: http://librarymanager/All#Preferences */
#include "config.h"
#include "deepSleep.h"

/************************************
* Check configurations
 ************************************/
#ifndef ESP32
#pragma message(THIS EXAMPLE IS FOR ESP32 ONLY!)
#error Select ESP32 board.
#endif

#ifndef __CONFIG_H_
#pragma message(FILE CONFIG.H NOT ADDED TO THE PROJECT)
#error The file config.h needs to be added.
#endif

/************************************
 * EXTERN VARIABLES
 ************************************/

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/
#define UNCONFIGURED            "UNCONFIGURED\0            "
#define SERIAL_INPUT_TIMEOUT_S  10
#define WIFI_SSID_PSW_MA_LEN    64
#define INIT_STS_OK             true
#define INIT_STS_NOK            false

/************************************
 * PRIVATE TYPEDEFS
 ************************************/
typedef struct {
  wifi_mode_t wifiMode;
  char wifiSsid[WIFI_SSID_PSW_MA_LEN];
  char wifiPassword[WIFI_SSID_PSW_MA_LEN];
  bool config_init;
} wifiConfig_t;

typedef struct {
  wifiConfig_t wifiCfg;
} localConfig_t;

/************************************
 * STATIC VARIABLES
 ************************************/
static uint32_t chipId = 0;
static DHTesp dht;
static ComfortState cf;
static bool connectionUp = true;
static WiFiClient network;
static MQTTClient mqtt = MQTTClient(256);
static Preferences configuration;
static localConfig_t localCfg;

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
bool getTemperature();
void getChipId();
void connectToMQTT();
void sendToMQTT();
void messageHandler(String &topic, String &payload);
void startWifiConfig(void);
void readString(void);

/************************************
 * MAIN FUNCTION
 ************************************/
void setup()
{
  /* Initialize serial */
  Serial.begin(115200);
  delay(1000); /* wait 100 ms to let Serial monitor initialize */
  getChipId();
  configuration.begin("esp32TempSensor");

  log_i("Checking wifi configuration");
  strcpy(localCfg.wifiCfg.wifiSsid, UNCONFIGURED);
  strcpy(localCfg.wifiCfg.wifiPassword, UNCONFIGURED);
  localCfg.wifiCfg.wifiMode = (wifi_mode_t)configuration.getUChar("wifiMode", (uint8_t)WIFI_STA);
  configuration.getString("wifiSsid", localCfg.wifiCfg.wifiSsid, WIFI_SSID_PSW_MA_LEN);
  configuration.getString("wifiPassword", localCfg.wifiCfg.wifiPassword, WIFI_SSID_PSW_MA_LEN);
  localCfg.wifiCfg.config_init = INIT_STS_OK;

  Serial.print("wifiSsid: ");
  Serial.println(localCfg.wifiCfg.wifiSsid);

  if ((strcmp(localCfg.wifiCfg.wifiSsid, UNCONFIGURED) == 0) || (strcmp(localCfg.wifiCfg.wifiPassword, UNCONFIGURED) == 0)) {
    log_w("Wifi not configured");
    localCfg.wifiCfg.config_init = INIT_STS_NOK;
  }

  if (!localCfg.wifiCfg.config_init) {
    startWifiConfig();
  }

  Serial.printf("Define: %d \n", sizeof(WIFI_SSID));
  Serial.println(WIFI_SSID);
  Serial.printf("Config: %d \n", sizeof(localCfg.wifiCfg.wifiSsid));
  Serial.println(localCfg.wifiCfg.wifiSsid);

  WiFi.mode(localCfg.wifiCfg.wifiMode);
  WiFi.begin(localCfg.wifiCfg.wifiSsid, WIFI_PASSWORD);

  Serial.println("ESP32 - Connecting to Wi-Fi");

  uint8_t tentatives = ATTEMPING_CONNECTION;

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    tentatives--;
    Serial.print(".");
    if (tentatives == 0u)
    {
      Serial.println();
      Serial.println("Unable to connect to " + String(WIFI_SSID));
      connectionUp = false;
      break;
    }
  }
  Serial.println();

  if (connectionUp)
  {
    connectToMQTT();
  }
  Serial.println("Boot number: " + String(deepSleep_getbootCount()));

  /* Initialize temperature sensor */
  dht.setup(DHT_PIN, DHTesp::DHT11);
  Serial.println("DHT initiated");

  /* Configure the wake up source */
  deepSleep_Init(TIME_TO_SLEEP);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
}

/************************************
 * STATIC FUNCTIONS
 ************************************/

void startWifiConfig(void)
{
  Serial.println("***************************");
  Serial.println("Starting wifi configuration");
  Serial.println("");
  Serial.println("Insert settings in this format:");
  Serial.println("SSID: <Your WiFi name>");
  Serial.println("PSW: <Your password>");

  Serial.onReceive(readString);
  while (!localCfg.wifiCfg.config_init) {
    delay(500);
  }
  Serial.onReceive(NULL);
}

void readString(void)
{
  String serialRXString;
  size_t available = Serial.available();
  while (available --) {
    serialRXString += (char)Serial.read();
  }
  Serial.print("Received: ");
  Serial.println(serialRXString);
  if (serialRXString.startsWith("SSID: ")) {
    serialRXString.remove(0, (sizeof("SSID: ") - 1));
    serialRXString.toCharArray(localCfg.wifiCfg.wifiSsid, WIFI_SSID_PSW_MA_LEN);
  } else if (serialRXString.startsWith("PSW: ")) {
    serialRXString.remove(0, (sizeof("PSW: ") - 1));
    serialRXString.toCharArray(localCfg.wifiCfg.wifiPassword, WIFI_SSID_PSW_MA_LEN);
  } else {
    Serial.println("Wrong WiFi config input");
  }
  if ((strcmp(localCfg.wifiCfg.wifiSsid, UNCONFIGURED) != 0) && (strcmp(localCfg.wifiCfg.wifiPassword, UNCONFIGURED) != 0)) {
    log_i("Wifi configured");
    localCfg.wifiCfg.config_init = INIT_STS_OK;
  }
}

/*TEST*/
struct TEST
{
  float temperature;
  float humidity;
  float heatIndex;
  float dewPoint;
  String comfort;
};
struct TEST TESTT;

void loop()
{
  getTemperature();
  Serial.println("Going to sleep now");
  Serial.flush();
  deepSleep_goToSleep();
}

/**
 * getTemperature
 * Reads temperature from DHT11 sensor
 * @return bool
 *    true if temperature could be aquired
 *    false if aquisition failed
 */
bool getTemperature()
{
  // Reading temperature for humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
  TempAndHumidity newValues = dht.getTempAndHumidity();
  // Check if any reads failed and exit early (to try again).
  if (dht.getStatus() != 0)
  {
    Serial.println("DHT11 error status: " + String(dht.getStatusString()));
    return false;
  }

  float heatIndex = dht.computeHeatIndex(newValues.temperature, newValues.humidity);
  float dewPoint = dht.computeDewPoint(newValues.temperature, newValues.humidity);
  float cr = dht.getComfortRatio(cf, newValues.temperature, newValues.humidity);

  String comfortStatus;
  switch (cf)
  {
  case Comfort_OK:
    comfortStatus = "Comfort_OK";
    break;
  case Comfort_TooHot:
    comfortStatus = "Comfort_TooHot";
    break;
  case Comfort_TooCold:
    comfortStatus = "Comfort_TooCold";
    break;
  case Comfort_TooDry:
    comfortStatus = "Comfort_TooDry";
    break;
  case Comfort_TooHumid:
    comfortStatus = "Comfort_TooHumid";
    break;
  case Comfort_HotAndHumid:
    comfortStatus = "Comfort_HotAndHumid";
    break;
  case Comfort_HotAndDry:
    comfortStatus = "Comfort_HotAndDry";
    break;
  case Comfort_ColdAndHumid:
    comfortStatus = "Comfort_ColdAndHumid";
    break;
  case Comfort_ColdAndDry:
    comfortStatus = "Comfort_ColdAndDry";
    break;
  default:
    comfortStatus = "Unknown:";
    break;
  };

  Serial.println(" T:" + String(newValues.temperature) + " H:" + String(newValues.humidity) + " I:" + String(heatIndex) + " D:" + String(dewPoint) + " " + comfortStatus);
  TESTT.temperature = newValues.temperature;
  TESTT.humidity = newValues.humidity;
  TESTT.heatIndex = heatIndex;
  TESTT.dewPoint = dewPoint;
  TESTT.comfort = comfortStatus;

  if (connectionUp)
  {
    sendToMQTT();
  }

  return true;
}

void getChipId()
{
  log_i("Show chip ID");
  for (int i = 0; i < 17; i = i + 8)
  {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  Serial.print("Chip ID: ");
  Serial.println(chipId);
}

void connectToMQTT()
{
  // Connect to the MQTT broker
  mqtt.begin(MQTT_BROKER_ADRRESS, MQTT_PORT, network);

  // Create a handler for incoming messages
  mqtt.onMessage(messageHandler);

  Serial.print("ESP32 - Connecting to MQTT broker");

  while (!mqtt.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
  {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  if (!mqtt.connected())
  {
    Serial.println("ESP32 - MQTT broker Timeout!");
    return;
  }

  // Subscribe to a topic, the incoming messages are processed by messageHandler() function
  if (mqtt.subscribe(SUBSCRIBE_TOPIC))
    Serial.print("ESP32 - Subscribed to the topic: ");
  else
    Serial.print("ESP32 - Failed to subscribe to the topic: ");

  Serial.println(SUBSCRIBE_TOPIC);
  Serial.println("ESP32 - MQTT broker Connected!");
}

void sendToMQTT()
{
  JsonDocument message;
  message["timestamp"] = millis();
  message["Temperature"] = TESTT.temperature;
  message["humidity"] = TESTT.humidity;
  message["heatIndex"] = TESTT.heatIndex;
  message["dewPoint"] = TESTT.dewPoint;
  message["comfort"] = TESTT.comfort;
  char messageBuffer[1024];
  serializeJson(message, messageBuffer);

  mqtt.publish(PUBLISH_TOPIC, messageBuffer);

  Serial.println("ESP32 - sent to MQTT:");
  Serial.print("- topic: ");
  Serial.println(PUBLISH_TOPIC);
  Serial.print("- payload:");
  Serial.println(messageBuffer);
}

void messageHandler(String &topic, String &payload)
{
  Serial.println("ESP32 - received from MQTT:");
  Serial.println("- topic: " + topic);
  Serial.println("- payload:");
  Serial.println(payload);
}
