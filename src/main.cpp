#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
// #include <AsyncElegantOTA.h> // Library for Async OTA
#include "SPIFFS.h"
#include "EEPROM.h"
#include "../include/EspSPIFFS.h"
#include "../include/CallRobot.h"
#include "../include/InitWiFi.h"

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

// Create pointer object for class CallRobot and pass in 2 arguments: IP address of Robot and port
// CallRobot *CallRobotObject = new CallRobot("172.20.0.201", 7245);
CallRobot *CallRobotObject = new CallRobot("172.20.2.66", 8080);

// Create pointer object for class EspSPIFFS
EspSPIFFS *espSPIFFS = new EspSPIFFS();

// Declare our NeoPixel strip object:
#define STRIP_1_PIN 14 // GPIO the LEDs are connected to
#define LED_COUNT 8    // Number of LEDs
#define BRIGHTNESS 50  // NeoPixel brightness, 0 (min) to 255 (max)
Adafruit_NeoPixel strip1(LED_COUNT, STRIP_1_PIN, NEO_GRB + NEO_KHZ800);

// Define Button
#define BUTTON_INPUT 18
#define BUTTON_LED 19
#define BUTTON_LED_ON digitalWrite(BUTTON_LED, HIGH);
#define BUTTON_LED_OFF digitalWrite(BUTTON_LED, LOW);

// define the number of bytes you want to access
#define EEPROM_SIZE 4

// Search for parameter in HTTP POST request
const char *PARAM_INPUT_SSID = "ssid";
const char *PARAM_INPUT_PASS = "pass";
const char *PARAM_INPUT_IP = "ip";
const char *PARAM_INPUT_GATEWAY = "gateway";

// http authentication
const char *http_username = "robotnet";
const char *http_password = "Robotnet@2022";

void colorWipe(uint32_t color, int wait)
{
  strip1.clear();
  for (int i = 0; i < strip1.numPixels(); i++)
  {                                 // For each pixel in strip...
    strip1.setPixelColor(i, color); //  Set pixel's color (in RAM)
    strip1.show();                  //  Update strip to match
    delay(wait);                    //  Pause for a moment
  }
}
void wifimanager_start();

// Replaces placeholder
String processorCallRobot(const String &var)
{
  if (var == "RSSI")
  {
    return String(WiFi.RSSI());
  }
  else if (var == "IP")
  {
    return ip;
  }
  else if (var == "stateStation")
  {
    switch (CallRobotObject->StationStatusRuntime)
    {
    case 1:
      return "freeMission";

    case 2:
      return "waiting";
    case 3:
      return "readyGoHome";
    default:
      break;
    }
    return ip;
  }
  else if (var == "StationName")
  {
    return CallRobotObject->getStationName();
  }
  else if (var == "LineName")
  {
    return CallRobotObject->getLineName();
  }
  return String();
}

// function handle when user get unknow router
void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

// Pram input for requset, example : ?inputURL=<inputMessage>
const char *PARAM_INPUT_URL = "inputURL";
const char *PARAM_INPUT_StationName = "StationName";
const char *PARAM_INPUT_LineName = "LineName";

// declare object for taskhandle
static TaskHandle_t Task1 = NULL;
static TaskHandle_t Task2 = NULL;

// variable use to active wifmanager
bool wifimanager_status = false;

int taskMission = 0; // use for Task1code to Call Api robot
String httpURL = "";
int taskControlWs2812 = 0; // use for TaskControlWs2812 to control led status

void Task1code(void *pvParameters)
{
  unsigned long timeRequestButtonCurent = millis();
  unsigned long timeGetStatusTask = millis(); // time wil Check Status Task NavigationTo
  Serial.println("task1 start on core:" + String(xPortGetCoreID()));
  // EEPROM.begin(EEPROM_SIZE);
  // EEPROM.write(0, 1); đồng bộ lại trạng thái ban đầu cho trạm gọi
  // EEPROM.commit();
  // int stionStatus = EEPROM.read(0);
  // stationStatus = (stationStatus == 255) ? 1 : (stationStatus);

  // if (StationStatus == 1)
  // {
  //   CallRobotObject->stationStatusRuntime = StationStatus::freeMission;
  // }
  // else if (StationStatus == 2)
  // {
  //   CallRobotObject->stationStatusRuntime = stationStatus::waitting;
  // }
  // else if (StationStatus == 3)
  // {
  //   CallRobotObject->stationStatusRuntime = stationStatus::readyreadyGoHome;
  // }
  Serial.println("status station init: " + String(CallRobotObject->StationStatusRuntime));
  // BUTTON_LED_OFF;

  int SignalInput = 0;
  while (1)
  {
    String result = "e";

    if ((unsigned long)(millis() - timeGetStatusTask) > 2000 && (CallRobotObject->flagNavigationTo == 1))
    {
      int statusTask = CallRobotObject->getTask("NavigationTo", CallRobotObject->getIdRobotCurrent());
      String robotName = CallRobotObject->getRobotName();
      taskControlWs2812 = 4;
      if (statusTask == 2)
      {
        Serial.println(robotName + " is running task NavigationTo");
      }
      else if (statusTask == 3)
      {
        Serial.println(robotName + "runs Task NavigationTo is completion");
        String message = robotName + " has arrived!!! ";
        events.send(message.c_str(), "callRobotRun", millis());
        CallRobotObject->flagNavigationTo = false;
        CallRobotObject->StationStatusRuntime = StationStatus::readyGoHome;
        vTaskDelay(50 / portTICK_PERIOD_MS);
        String stateStation = "readyGoHome";
        events.send(stateStation.c_str(), "stateStationESP", millis());

        taskControlWs2812 = 5;
      }
      // Serial.println("statusTask"+String(statusTask));
      timeGetStatusTask = millis();
    }
    // checking button press
    if ((unsigned long)(millis() - timeRequestButtonCurent) > 200)
    {
      timeRequestButtonCurent = millis();

      if (digitalRead(BUTTON_INPUT) == SignalInput)
      {
        int count = 0;
        while (digitalRead(BUTTON_INPUT) == SignalInput)
        {
          count++;
          delay(1);
        }
        if (count > 3000)
        {
          // cancel programing
          bool result = CallRobotObject->CancelAction();
          Serial.println("Robot CancelAction");
          BUTTON_LED_OFF;
        }
        else if (count > 99)
        {
          taskMission = 1;
          Serial.println("ButtonGreen Click");
        }
      }
    }

    switch (taskMission)
    {
    case 1: // Call Robot
    {
      digitalWrite(BUTTON_LED, HIGH);
      String message = "";
      // thuc hien 1 nhiem vu gi do

      switch (CallRobotObject->StationStatusRuntime)
      {

      case StationStatus::freeMission:
      {

        result = CallRobotObject->CallingRobot();
        vTaskDelay(50 / portTICK_PERIOD_MS);

        message = "PrevState(1): " + String(result);
        events.send(message.c_str(), "callRobotRun", millis());

        vTaskDelay(50 / portTICK_PERIOD_MS);
        // Send notify to Web server to inform state of station changed
        message = "waiting";
        events.send(message.c_str(), "stateStationESP", millis());

        break;
      }
        // if robot's in callstation (station-1), call robot readyGoHome (station-7)
      case StationStatus::waiting:
      {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        // Send Events to the Web Server with the Sensor Readings
        message = "PrevState(2): Station is busy";
        events.send(message.c_str(), "callRobotRun", millis());

        break;
      }
        // if robot's in home (station-7), call robot come to callstation(station-1)
      case StationStatus::readyGoHome:
      {
        result = CallRobotObject->GoHomeRobot();
        vTaskDelay(100 / portTICK_PERIOD_MS);
        message = "PrevState(3): " + String(result);
        events.send(message.c_str(), "callRobotRun", millis());

        break;
      }
      default:
      {
        Serial.println("defaults");
      }
      }
      Serial.println(result);
      // Control ws2812 follow stationStatus
      vTaskDelay(50 / portTICK_PERIOD_MS);
      int stationStatus = CallRobotObject->StationStatusRuntime;
      String stateStation = "h";

      if (stationStatus == 1)
      {
        stateStation = "freeMission";
        events.send(stateStation.c_str(), "stateStationESP", millis());
        String stateStation = "hello";
        taskControlWs2812 = 4;
      }
      else if (stationStatus == 3)
      {
        stateStation = "readyGoHome";
        events.send(stateStation.c_str(), "stateStationESP", millis());

        taskControlWs2812 = 6;
      }
      // stationStatus = CallRobotObject->StationStatusRuntime;
      // EEPROM.write(0, stationStatus);
      // EEPROM.commit();
      // delay(10);
      // Serial.println("StationStatus:  " + String(EEPROM.read(0)));

      // notify to user with signal light

      BUTTON_LED_OFF;

      taskMission = 0;
      break;
    }

    case 2: // Get Id Robot
    {
      // String response = CallRobotObject->HttpGet("http://172.20.2.50:8080/api/Remote/Robots?model=agv-500&map=demo-f1");
      String response = CallRobotObject->HttpGet("http://172.20.2.66:8080/api/Remote/Robots?model=agv-line&map=demo-f1");
      if (response == "error")
      {
        response = "{\"result\":\"error\"}";
      }
      // events.send(response.c_str(), "getIdRobot", millis());
      events.send(response.c_str(), "httpGetURL", millis());
      vTaskDelay(50 / portTICK_PERIOD_MS);
      taskMission = 0;
      break;
    }
    case 3: // Call API server with method GET
    {
      String response = CallRobotObject->HttpGet(httpURL.c_str());
      // Serial.println(response);
      if (response == "error")
      {
        response = "{\"result\":\"error\"}";
      }
      events.send(response.c_str(), "httpGetURL", millis());
      vTaskDelay(100 / portTICK_PERIOD_MS);
      taskMission = 0;
    }
    }

    // Serial.print("HighWaterMark:");
    // Serial.println(uxTaskGetStackHighWaterMark(NULL));
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

int count_retry_connect = 0;
void TaskReconnectWiFi(void *pvParameters)
{

  // unsigned long previousMillis_reconnectWifi = 0;
  bool task1_suspend = false;
  while (1)
  {
    // put your main code here, to run repeatedly:
    // unsigned long currentMillis = millis();

    // if WiFi is down, tryreconnecting
    wl_status_t wifi_status = WiFi.status();
    // Serial.println("TaskReconnectWiFi" + String(wifimanager_status));
    if ((wifi_status != WL_CONNECTED) && (wifimanager_status == false))
    {

      Serial.print(millis());
      Serial.println(" Reconnecting to WiFi...");

      task1_suspend = true;
      vTaskSuspend(Task1); // suspend Task1code

      WiFi.disconnect();
      WiFi.reconnect();
      // previousMillis_reconnectWifi = currentMillis;
      taskControlWs2812 = 2;

      count_retry_connect++;

      if (count_retry_connect > 20)
      {

        Serial.println("Unable to reconnect to WiFi -> Start AP again");
        wifimanager_status = true;

        wifimanager_start();
      }
    }
    if ((wifi_status == WL_CONNECTED) && (wifimanager_status == false))
    {

      if (task1_suspend == true)
      {
        Serial.println("Reconnect to WiFi Success -> Task1code resum");
        taskControlWs2812 = 1;
        vTaskResume(Task1); // resume Task1code
        task1_suspend = false;
      }
    }

    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void TaskControlWs2812(void *pvParameters)
{
  while (1)
  {
    switch (taskControlWs2812)
    {
    case 1: // Indicator WiFi connected
    {
      /* code */
      strip1.clear();
      for (int i = 0; i < LED_COUNT; i++)
      { // For each pixel...
        strip1.setPixelColor(i, strip1.Color(0, 255, 0));

        strip1.show(); // Send the updated pixel colors to the hardware.
      }
      break;
    }
    case 2: // Indicator WiFi reconnecting
    {
      colorWipe(strip1.Color(205, 200, 0), 50);
      break;
    }
    case 3: // Indicator Esp32 start WiFi manager
    {
      for (int i = 0; i < LED_COUNT; i++)
      { // For each pixel...
        strip1.setPixelColor(i, strip1.Color(250, 0, 0));

        strip1.show(); // Send the updated pixel colors to the hardware.
      }
      break;
    }
    case 4: // statusStation == freeMission
    {

      strip1.clear();
      for (int i = 0; i < 1; i++)
      { // For each pixel...
        strip1.setPixelColor(i, strip1.Color(255, 105, 180));
      }
      String robotNameCurrent = CallRobotObject->getRobotName();

      if (robotNameCurrent == "Nagase")
      {
        strip1.setPixelColor(5, strip1.Color(250, 0, 0));
      }
      else
      {
        strip1.setPixelColor(5, strip1.Color(0, 250, 0));
      }
      strip1.show(); // Send the updated pixel colors to the hardware.
      break;
    }
    case 5: // statusStation == waiting
    {
      strip1.clear();
      for (int i = 0; i < 2; i++)
      { // For each pixel...
        strip1.setPixelColor(i, strip1.Color(255, 105, 180));
      }
      String robotNameCurrent = CallRobotObject->getRobotName();

      if (robotNameCurrent == "Nagase")
      {
        strip1.setPixelColor(5, strip1.Color(250, 0, 0));
      }
      else
      {
        strip1.setPixelColor(5, strip1.Color(0, 250, 0));
      }
      strip1.show(); // Send the updated pixel colors to the hardware.
      break;
    }
    case 6: // statusStation == readyreadyGoHome
    {
      strip1.clear();
      for (int i = 0; i < 3; i++)
      { // For each pixel...
        strip1.setPixelColor(i, strip1.Color(255, 105, 180));
      }
      String robotNameCurrent = CallRobotObject->getRobotName();

      if (robotNameCurrent == "Nagase")
      {
        strip1.setPixelColor(5, strip1.Color(250, 0, 0));
      }
      else
      {
        strip1.setPixelColor(5, strip1.Color(0, 250, 0));
      }
      strip1.show(); // Send the updated pixel colors to the hardware.
      break;
    }

    default:
      break;
    }
    taskControlWs2812 = 0;
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}
void Wifi_connected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Successfully connected to Access Point:  " + String(ssid));

  count_retry_connect = 0;
  // If ESP32 inits successfully in station mode light up all pixels in a teal color
  taskControlWs2812 = 1;
}

void Get_IPAddress_RSSI(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("WIFI is connected!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  long rssi = WiFi.RSSI();
  Serial.print("RSSI:");
  Serial.println(rssi);
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(BUTTON_INPUT, INPUT_PULLUP);
  pinMode(BUTTON_LED, OUTPUT);
  BUTTON_LED_OFF;

  // Initialize strips
  strip1.begin(); // Set brightness
  strip1.setBrightness(BRIGHTNESS);
  for (int i = 0; i < LED_COUNT; i++)
  { // For each pixel...
    strip1.setPixelColor(i, strip1.Color(0, 0, 250));
    strip1.show();
  }

  // delete old config
  WiFi.disconnect(true);
  WiFi.onEvent(Wifi_connected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(Get_IPAddress_RSSI, ARDUINO_EVENT_WIFI_STA_GOT_IP);

  vTaskDelay(2000 / portTICK_PERIOD_MS);

  espSPIFFS->initSPIFFS();

  // Read field ssid, pass, ip , gateway
  ssid = espSPIFFS->readSSID(SPIFFS);
  pass = espSPIFFS->readPASS(SPIFFS);
  ip = espSPIFFS->readIP(SPIFFS);
  gateway = espSPIFFS->readGATEWAY(SPIFFS);

  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);

  // Create mutex before starting tasks
  // mutex = xSemaphoreCreateMutex();
  if (initWiFi())
  {
    // Handle the Web Server in Station Mode
    // Route for root / web page
    server.serveStatic("/", SPIFFS, "/");

    // Route for CallRobot web page
    server.on("/CallRobot", HTTP_GET, [](AsyncWebServerRequest *request)
              {   
                if(!request->authenticate(http_username, http_password))
                    return request->requestAuthentication();
                request->send(SPIFFS, "/views/CallRobot.html", "text/html", false, processorCallRobot); });

    // [POST] /callRobotRun
    server.on("/callRobotRun", HTTP_POST, [](AsyncWebServerRequest *request)
              {
          taskMission = 1; 
    

          request->send(200, "text/html", "Set callRobot true success"); });

    server.on("/getIdRobot", HTTP_POST, [](AsyncWebServerRequest *request)
              {
                taskMission = 2;
               
                Serial.println("getidRobot");
                request->send(200, "text/html", "Set getIdRobot success!"); });
    // Send a GET request to <ESP_IP>/httpGetURL?inputURL=<inputMessage>
    server.on("/httpGetURL", HTTP_GET, [](AsyncWebServerRequest *request)
              {
      String inputMessage = request->getParam(PARAM_INPUT_URL)->value();

      taskMission = 3;
      httpURL = inputMessage;

      String response = "Waiting for Esp Call URL";
      request->send(200, "text/html", response.c_str()); });

    // Send a GET request to <ESP_IP>/httpSetPram?StationName=<inputMessage1>&LineName=<inputMessage2>
    server.on("/httpSetPram", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                String inputMessage1 = "";
                String inputMessage2 = "";
                if (request->hasParam(PARAM_INPUT_StationName) && request->hasParam(PARAM_INPUT_LineName))
                {
                  inputMessage1 = request->getParam(PARAM_INPUT_StationName)->value();
                  inputMessage2 = request->getParam(PARAM_INPUT_LineName)->value();

                  CallRobotObject->setStationName(inputMessage1.c_str());
                  CallRobotObject->setLineName(inputMessage2.c_str());
                  request->send(200, "text/html", "Set Pram success");
                }
                else
                {
                  
                  request->send(200, "text/html", "Set Pram error");
                } });

    server.onNotFound(notFound);

    events.onConnect([](AsyncEventSourceClient *client)
                     {
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
                } });

    // Start ElegantOTA
    // AsyncElegantOTA.begin(&server, http_username, http_password);

    server.addHandler(&events);
    server.begin();
  }
  else
  {

    wifimanager_status = true;
    wifimanager_start();
  }

  xTaskCreatePinnedToCore(
      TaskReconnectWiFi,   /* Task function. */
      "TaskReconnectWiFi", /* name of task. */
      1024 * 4,            /* Stack size of task (byte in ESP32) */
      NULL,                /* parameter of the task */
      2,                   /* priority of the task */
      &Task2,              /* Task handle */
      0);                  /* Run on one core*/
  xTaskCreatePinnedToCore(
      Task1code, /* Task function. */
      "Task1",   /* name of task. */
      2048 * 2,  /* Stack size of task (byte in ESP32) */
      NULL,      /* parameter of the task */
      3,         /* priority of the task */
      &Task1,    /* Task handle */
      1);        /* Run on one core*/
  xTaskCreatePinnedToCore(
      TaskControlWs2812, /* Task function. */
      "Task2",           /* name of task. */
      2048,              /* Stack size of task (byte in ESP32) */
      NULL,              /* parameter of the task */
      2,                 /* priority of the task */
      NULL,              /* Task handle */
      1);                /* Run on one core*/

  Serial.println("Done!");
}

void loop()
{
}

void wifimanager_start()
{
  // stop task until esp32 config wifi done

  WiFi.disconnect(true);
  // else initialize the ESP32 in Access Point mode
  // light up all pixels in a red color
  taskControlWs2812 = 3;
  // Connect to Wi-Fi network with SSID and password
  Serial.println("Setting AP (Access Point)");
  // NULL sets an open Access Point
  WiFi.softAP("ESP-WIFI-MANAGER", "22446688");

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.serveStatic("/", SPIFFS, "/");
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/views/wifimanager.html", "text/html"); });

  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_SSID) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            espSPIFFS->writeSSID(SPIFFS,ssid.c_str());
          }
          // HTTP POST password value
          if (p->name() == PARAM_INPUT_PASS) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save password
            espSPIFFS->writePASS(SPIFFS, pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_IP) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            espSPIFFS->writeIP(SPIFFS, ip.c_str()); 
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_GATEWAY) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            espSPIFFS->writeGATEWAY(SPIFFS, gateway.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      //wifimanager_status = false;
      delay(3000);
      ESP.restart(); });
  server.begin();
}
