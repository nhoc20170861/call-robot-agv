#ifndef __MAIN_H__
#define __MAIN_H__
// Include library
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <AsyncElegantOTA.h> // Library for Async OTA
#include "SPIFFS.h"
#include "./EspSPIFFS.h"
#include "./CallRobot.h"
#include "./InitWiFi.h"
#include <Arduino_JSON.h>
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Create pointer object for class CallRobot and pass in 2 arguments: IP address of Robot and port
// CallRobot *CallRobotObject = new CallRobot("172.20.0.201", 7245); // simiulation
CallRobot *CallRobotObject = new CallRobot("172.20.2.66", 8080);

// Create pointer object for class EspSPIFFS
EspSPIFFS *espSPIFFS = new EspSPIFFS();

// Create pointer object for class InitWiFi
InitWiFi *initWiFi = new InitWiFi();

// Declare our NeoPixel strip object:
#define STRIP_1_PIN 14 // GPIO the LEDs are connected to
#define LED_COUNT 8    // Number of LEDs
#define BRIGHTNESS 50  // NeoPixel brightness, 0 (min) to 255 (max)
// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip1(LED_COUNT, STRIP_1_PIN, NEO_GRB + NEO_KHZ800);

// Define Button
#define BUTTON_INPUT 18
#define BUTTON_LED 19
#define BUTTON_LED_ON digitalWrite(BUTTON_LED, HIGH);
#define BUTTON_LED_OFF digitalWrite(BUTTON_LED, LOW);

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_SSID = "ssid";
const char *PARAM_INPUT_PASS = "pass";
const char *PARAM_INPUT_IP = "ip";
const char *PARAM_INPUT_GATEWAY = "gateway";

// http authentication
const char *http_username = "robotnet";
const char *http_password = "Robotnet@2022";

// Pram input for requset, example : ?StationNameL=<inputMessage>
const char *PARAM_INPUT_StationName = "StationName";
const char *PARAM_INPUT_LineName = "LineName";

bool wifimanager_status = false; // variable use to active wifmanager

int taskMission = 0; // use for Task1code to Call Api robot

int taskControlWs2812 = 0; // use for TaskControlWs2812 to control led status

int count_retry_connect = 0; // variable used to count number of retry connect WiFi

// declare object for taskhandle
static TaskHandle_t Task1 = NULL; // control Task1code
static TaskHandle_t Task2 = NULL; // control TaskReconnectWiFi
static TaskHandle_t Task3 = NULL; // control TaskControlWs2812b

// function control colorWipe of led Ws2812b
void colorWipe(uint32_t color, int wait)
{
    strip1.clear();
    for (int i = 0; i < strip1.numPixels(); i++)
    {                                   // For each pixel in strip...
        strip1.setPixelColor(i, color); //  Set pixel's color (in RAM)
        strip1.show();                  //  Update strip to match
        if (wait > 0)
            delay(wait); //  Pause for a moment
    }
}

// function handle when user get unknow router
void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

void IRAM_ATTR ISR()
{
    taskMission = 1;
}

JSONVar wifiScanList;
String processorWiFiManager(const String &var)
{
    if (var == "wifiScanList")
    {

        int size = wifiScanList.length();

        String html = "";
        for (int i = 0; i < size; i++)
        {

            const char *SSID = (const char *)(wifiScanList[i]["SSID"]);
            int RSSI = (int)(wifiScanList[i]["RSSI"]);

            html +=
                String("<tr onclick=\"add(this)\">") +
                String("<td id=\"td-id\">") + String(i + 1) + String("</td>") +
                String("<td id=\"td-ssid\">") + SSID + String("</td>") +
                String("<td id=\"td-rssi\">") + String(RSSI) + String("</td>") +
                String("</tr>");
        }

        return String(html);
    }

    return String();
}

void wifimanager_start()
{

    WiFi.disconnect(true);
    // light up all pixels in a red color
    taskControlWs2812 = 3;

    // WiFi.scanNetworks will return the number of networks found.

    JSONVar wifiScan = JSON.parse("{\"SSID\":\"NaN\",\"RSSI\":\"NaN\"}");
    int n = WiFi.scanNetworks();
    if (n == 0)
    {
        Serial.println("no networks found");
    }
    else
    {
        Serial.print(n);
        Serial.println(" networks found");
        Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
        if (n > 6)
            n = 6;
        for (int i = 0; i < n; ++i)
        {
            wifiScan["SSID"] = WiFi.SSID(i);
            wifiScan["RSSI"] = WiFi.RSSI(i);
            wifiScanList[i] = wifiScan;

            // Print SSID and RSSI for each network found
            Serial.printf("%2d", i + 1);
            Serial.print(" | ");
            Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
            Serial.print(" | ");
            Serial.printf("%4d", WiFi.RSSI(i));
            Serial.print(" | ");
            Serial.printf("%2d", WiFi.channel(i));
            Serial.print(" | ");

            Serial.println();
        }
        String message = JSON.stringify(wifiScanList);
        Serial.println(message);
    }

    // Delete the scan result to free memory for code below.
    WiFi.scanDelete();

    Serial.println("Setting AP (Access Point)");
    // initialize the ESP32 in Access Point mode
    WiFi.softAP("ESP01-WIFI-MANAGER", "22446688");

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    server.serveStatic("/", SPIFFS, "/");
    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/views/wifimanager.html", "text/html", false, processorWiFiManager); });

    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
              {
      const char input[] = "{\"SSID\":\"NaN\",\"Password\":\"NaN\",\"IP\":\"NaN\",\"Gateway\":\"NaN\"}";
      JSONVar myObject = JSON.parse(input);
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_SSID) {
            String ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
           
            myObject["SSID"] = ssid;
          }
          // HTTP POST password value
          if (p->name() == PARAM_INPUT_PASS) {
            String pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
           
             myObject["Password"] = pass;
            
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_IP) {
            String ip = p->value().c_str();
           
            Serial.print("IP Address set to: ");
            Serial.println(initWiFi->ip);
            
               myObject["IP"] = ip;
            
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_GATEWAY) {
            String gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
          
               myObject["Gateway"] = gateway;
            
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      
      espSPIFFS->writeWiFiConfig(SPIFFS, JSON.stringify(myObject).c_str());
      
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + initWiFi->ip);
      //wifimanager_status = false;
      delay(3000);
      ESP.restart(); });
    server.begin();
}

#endif