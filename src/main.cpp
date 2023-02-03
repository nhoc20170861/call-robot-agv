#include "main.h"

// Callback
String processorCallRobot(const String &var)
{
  if (var == "RSSI")
  {
    return String(WiFi.RSSI());
  }
  else if (var == "IP")
  {
    return initWiFi->ip;
  }
  else if (var == "StationID")
  {
    return CallRobotObject->getStationID();
  }
  else if (var == "StateStation")
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

// Task used to process logic Call Robot AGV
void Task1code(void *pvParameters)
{
  unsigned long timeRequestButtonCurent = millis();
  unsigned long timeGetStatusTask = millis(); // time wil Check Status Task NavigationTo
  Serial.println("task1 start on core:" + String(xPortGetCoreID()));
  Serial.println("status station init: " + String(CallRobotObject->StationStatusRuntime));

  int SignalInput = 0;
  while (1)
  {

    if ((unsigned long)(millis() - timeGetStatusTask) > 2000 && (CallRobotObject->flagNavigationTo == 1))
    {
      int statusTask = CallRobotObject->getTask("NavigationTo", CallRobotObject->getIdRobotCurrent());
      String robotName = CallRobotObject->getRobotName();
      // 2: task is running, 3: task is runcomplete
      if (statusTask == 0)
      {
        // Serial.println(robotName + " disconnected Flet manager then reconnected!! ");
        String message = robotName + " disconnects to Fleet Manager!!!";
        events.send(message.c_str(), "callRobotRun", millis());

        CallRobotObject->StationStatusRuntime = StationStatus::unknown;
        CallRobotObject->flagNavigationTo = 0;
        taskControlWs2812 = 4;

        vTaskDelay(50 / portTICK_PERIOD_MS);
        String stateStation = "unknow";
        events.send(stateStation.c_str(), "stateStationESP", millis());
      }
      else if (statusTask == 2)
      {
        Serial.println(robotName + " is running task NavigationTo");
      }
      else if (statusTask == 3)
      {
        Serial.println(robotName + " runs Task NavigationTo is completion");
        String message = robotName + " has arrived!!! ";
        events.send(message.c_str(), "callRobotRun", millis());

        CallRobotObject->flagNavigationTo = false;
        CallRobotObject->StationStatusRuntime = StationStatus::readyGoHome;
        taskControlWs2812 = 6;

        vTaskDelay(50 / portTICK_PERIOD_MS);
        String stateStation = "readyGoHome";
        events.send(stateStation.c_str(), "stateStationESP", millis());
      }
      // Serial.println("statusTask"+String(statusTask));
      timeGetStatusTask = millis();
    }
    // checking button press
    // if ((unsigned long)(millis() - timeRequestButtonCurent) > 200)
    // {
    //   timeRequestButtonCurent = millis();

    //   if (digitalRead(BUTTON_INPUT) == SignalInput)
    //   {
    //     int count = 0;
    //     while (digitalRead(BUTTON_INPUT) == SignalInput)
    //     {
    //       count++;
    //       delay(1);
    //     }
    //     if (count > 3000)
    //     {
    //       // cancel task
    //       bool result = CallRobotObject->CancelAction();
    //       Serial.println("Robot CancelAction, then reset stateStation");
    //       vTaskDelay(50 / portTICK_PERIOD_MS);
    //       String message = " _Reset stateStation, please reload page_ ";
    //       events.send(message.c_str(), "callRobotRun", millis());
    //       BUTTON_LED_OFF;
    //       taskControlWs2812 = 4;

    //       // Reset station
    //     }
    //     else if (count > 99)
    //     {
    //       taskMission = 1;
    //       Serial.println("ButtonGreen Click");
    //     }
    //   }
    // }

    switch (taskMission)
    {
    case 1: // Call Robot
    {
      digitalWrite(BUTTON_LED, HIGH);
      String message = "";
      String result = "e";
      String stateStation = "...";
      // thuc hien 1 nhiem vu gi do

      switch (CallRobotObject->StationStatusRuntime)
      {

      case StationStatus::freeMission:
      {

        result = CallRobotObject->CallingRobot();
        // After call Robot to station success, StationStatus will change to wating
        vTaskDelay(50 / portTICK_PERIOD_MS);
        message = "PrevState(1): " + result;
        events.send(message.c_str(), "callRobotRun", millis());

        vTaskDelay(50 / portTICK_PERIOD_MS);

        // Send notify to Web server to inform state of station changed
        if (CallRobotObject->StationStatusRuntime == StationStatus::waiting)
        {
          stateStation = "waiting";
          events.send(stateStation.c_str(), "stateStationESP", millis());
          taskControlWs2812 = 5;
        }
        break;
      }
        // if robot's in callstation (station-1), call robot readyGoHome (station-7)
      case StationStatus::waiting:
      {

        String robotName = CallRobotObject->getRobotName();
        vTaskDelay(50 / portTICK_PERIOD_MS);
        // Send Events to the Web Server with the Sensor Readings
        message = "PrevState(2): Please, wait " + robotName + " go to station before call again!!";
        events.send(message.c_str(), "callRobotRun", millis());

        break;
      }
        // if robot's in home (station-7), call robot come to callstation(station-1)
      case StationStatus::readyGoHome:
      {
        result = CallRobotObject->GoHomeRobot();
        // After call Robot to gohome success, StationStatus will change to freemission
        vTaskDelay(50 / portTICK_PERIOD_MS);

        message = "PrevState(3): " + result;
        events.send(message.c_str(), "callRobotRun", millis());

        vTaskDelay(50 / portTICK_PERIOD_MS);

        if (CallRobotObject->StationStatusRuntime == StationStatus::freeMission)
        {
          taskControlWs2812 = 4;
          stateStation = "freeMission";
          events.send(stateStation.c_str(), "stateStationESP", millis());
        }
        break;
      }
      case StationStatus::unknown:
      {
        result = CallRobotObject->CallingRobot();
        String robotName = CallRobotObject->getRobotName();
        if (result == "Waiting until Robot reconnects")
        {
          vTaskDelay(50 / portTICK_PERIOD_MS);
          message = "Waiting until " + robotName + "reconnects";
          events.send(message.c_str(), "callRobotRun", millis());
        }
        else
        {
          // After call Robot to station success, StationStatus will change to wating
          vTaskDelay(50 / portTICK_PERIOD_MS);
          message = "PrevState(4): " + result;
          events.send(message.c_str(), "callRobotRun", millis());

          vTaskDelay(50 / portTICK_PERIOD_MS);

          // Send notify to Web server to inform state of station changed
          if (CallRobotObject->StationStatusRuntime == StationStatus::waiting)
          {
            stateStation = "waiting";
            events.send(stateStation.c_str(), "stateStationESP", millis());
            taskControlWs2812 = 5;
          }
        }
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
      // notify to user with signal light in button
      BUTTON_LED_OFF;
      taskMission = 0;
      break;
    }
    case 2: // Get Id Robot
    {
      String response = CallRobotObject->getIdforAllRobot();
      if (response == "error")
      {
        response = "{\"result\":\"error\"}";
      }

      // send response to client broser
      events.send(response.c_str(), "httpGetURL", millis());
      vTaskDelay(50 / portTICK_PERIOD_MS);
      taskMission = 0;
      break;
    }
    case 3: // Reset State station and CancelTask
    {
      // Reset station
      bool result = CallRobotObject->CancelAction();
      Serial.println("Robot CancelAction, then reset stateStation");
      vTaskDelay(50 / portTICK_PERIOD_MS);
      String message = " Reset state of Station success, please reload page ";
      events.send(message.c_str(), "callRobotRun", millis());
      taskControlWs2812 = 4;
      taskMission = 0;
      break;
    }
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void TaskReconnectWiFi(void *pvParameters)
{

  // unsigned long previousMillis_reconnectWifi = 0;
  bool task1_suspend = false;
  while (1)
  {

    // if WiFi is down, tryreconnecting
    wl_status_t wifi_status = WiFi.status();
    // Serial.println("TaskReconnectWiFi" + String(wifimanager_status));
    if ((wifi_status != WL_CONNECTED) && (wifimanager_status == false))
    {

      Serial.print(millis());
      Serial.println(" Reconnecting to WiFi...");
      if (task1_suspend == false)
      {
        task1_suspend = true;
        vTaskSuspend(Task1); // suspend Task1code
        // detachInterrupt(BUTTON_INPUT); // detachInterrupt when reconnect wifi
      }

      WiFi.disconnect();
      WiFi.reconnect();
      // previousMillis_reconnectWifi = currentMillis;
      taskControlWs2812 = 2;

      count_retry_connect++;

      // if (count_retry_connect > 20)
      // {

      //   Serial.println("Unable to reconnect to WiFi -> Start AP again");
      //   wifimanager_status = true;

      //   wifimanager_start();
      // }
    }
    if ((wifi_status == WL_CONNECTED) && (wifimanager_status == false))
    {

      if (task1_suspend == true)
      {
        Serial.println("Reconnect to WiFi Success -> Task1code resum");

        vTaskResume(Task1); // resume Task1code
        count_retry_connect = 0;
        // If ESP32 inits successfully in station mode light up all pixels in a green color
        taskControlWs2812 = 1;
        task1_suspend = false;
        // attachInterrupt(BUTTON_INPUT, ISR, FALLING); // attachInterrupt again
        taskMission = 0;
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
      colorWipe(strip1.Color(0, 255, 0), 0);
      break;
    }
    case 2: // Indicator WiFi reconnecting
    {
      colorWipe(strip1.Color(205, 200, 0), 50);
      break;
    }
    case 3: // Indicator Esp32 start WiFi manager
    {
      colorWipe(strip1.Color(250, 0, 0), 0); // Red light

      break;
    }
    case 4: // statusStation == freeMission
    {

      strip1.clear();
      for (int i = 0; i < 1; i++)
      { // For each pixel...
        strip1.setPixelColor(i, strip1.Color(255, 105, 180));
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

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(BUTTON_INPUT, INPUT_PULLUP);
  pinMode(BUTTON_LED, OUTPUT);
  BUTTON_LED_OFF;
  attachInterrupt(BUTTON_INPUT, ISR, FALLING);

  // Initialize strips
  strip1.begin(); // Set brightness
  strip1.setBrightness(BRIGHTNESS);
  colorWipe(strip1.Color(0, 0, 250), 0);

  // delete old config
  WiFi.disconnect(true);
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  espSPIFFS->initSPIFFS();

  // Read wifi config from SPIFFS
  String configWiFi = espSPIFFS->readWiFiConfig(SPIFFS);

  Serial.println(configWiFi);
  initWiFi->setPram(configWiFi);

  // Read SPIFFS in oder to update StationName and LineName
  String stationConfig = espSPIFFS->readStationConfig(SPIFFS);
  Serial.println(stationConfig);
  JSONVar myObject = JSON.parse(stationConfig);
  String stationName = myObject["StationName"];
  String lineName = myObject["LineName"];

  if (stationName.length() > 0 && lineName.length() > 0)
  {
    CallRobotObject->setStationName(stationName.c_str());
    CallRobotObject->setLineName(lineName.c_str());
  }

  // Start Webserver
  if (initWiFi->connectWiFi())
  {
    taskControlWs2812 = 1; // WiFi connected
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
    server.on("/resetStateStation", HTTP_POST, [](AsyncWebServerRequest *request)
              {
     
      taskMission = 3;
      String response = "Waiting for Station reset";
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

                    // update new StationName and LineName into SPIFFS
                  String stationConfig =  "{\"StationName\":\""+ inputMessage1 + "\",\"LineName\":\""+inputMessage2+"\"}";
                  espSPIFFS->writeStationConfig(SPIFFS,stationConfig.c_str());

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
    AsyncElegantOTA.begin(&server, http_username, http_password);

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
  if (wifimanager_status == true)
  {
    vTaskSuspend(Task1);
    vTaskSuspend(Task2);
    Serial.println("Wait for completing WiFiManager");
  } /* Run on one core*/
  else
    Serial.println("Setup Done!");
}

void loop()
{
}
