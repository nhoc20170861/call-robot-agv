#include "CallRobot.h"
String CallRobot::getRobotName(void)
{
  return RobotName;
}
String CallRobot::getIdRobotCurrent(void)
{
  return RobotId;
}
String CallRobot::getStationName(void)
{
  return StationName;
}

void CallRobot::setStationName(const char *input)
{
  StationName = input;
  Serial.println("StationName: " + StationName);
}

String CallRobot::getLineName(void)
{
  return LineName;
}

void CallRobot::setLineName(const char *input)
{
  LineName = input;
  Serial.println("LineName: " + LineName);
}

String CallRobot::HttpGet(String url)
{
  String Json = "";

  if ((WiFi.status() == WL_CONNECTED))
  {
    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpCode);
      Json = http.getString();
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpCode);
      Json = "error";
    }
    http.end();
    return Json;
  }
  return "WiFiLost";
}

String CallRobot::HttpPost(String url, String data)
{
  String Json = "";
  if ((WiFi.status() == WL_CONNECTED))
  {
    HTTPClient http;
    http.begin(url);
    int httpCode = http.POST(data);

    if (httpCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpCode);
      Json = http.getString();
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpCode);
      Json = "error";
    }
    http.end();
    return Json;
  }
  return "WiFiLost";
}

String CallRobot::HttpDelete(String url)
{
  if ((WiFi.status() == WL_CONNECTED))
  {
    HTTPClient http;
    http.begin(url);
    int httpCode = http.sendRequest("Delete");
    String Json = "";
    if (httpCode > 0)
    {
      Json = http.getString();
    }
    http.end();
    return Json;
  }
  return "WiFiLost";
}

String CallRobot::getIdRobotForCall(String statusRobot, String taskName, String processRobot)
{
  String url = "http://" + _Ip + ":" + _Port + "/api/Remote/Robots?model=" + _Model + "&map=" + _MapName;
  Serial.println(url);
  String DataResult = HttpGet(url);

  if (DataResult == "error")
    return "Get Id error";
  else if (DataResult == "WiFiLost")
  {
    return "[GET] WiFiLost";
  }
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, DataResult);

  // Get size array
  size_t length = doc.size();
  // Serial.println("Length: " + String(length));
  int indexForCallRobot = 0;
  char message[50];
  for (int i = 0; i < length; i++)
  {
    const char *id = doc[i]["id"];
    const char *name = doc[i]["name"];
    if (id == "")
      break;

    String StatusAGV = getState(statusRobot, id);

    sprintf(message, "StatusAGV of: %s is %s", name, StatusAGV);
    Serial.println(message);
    if (StatusAGV != "Stopping") // Find robot has state (StatusAGV) = Stopping
    {

      continue;
    }
    int ProcessTask = getTask(taskName, id);
    sprintf(message, "%s of %s is: %d", taskName, name, ProcessTask);
    Serial.println(message);
    if (ProcessTask != 1 && ProcessTask != 3)
    // Find robot has state of Task = RunComplete ( 1: Created, 2: Running, 3:RanToCompletion)
    {
      continue;
    }
    String ProcessingAGV = getState(processRobot, id);
    sprintf(message, "ProcessAGV of: %s is %s", name, ProcessingAGV);
    Serial.println(message);
    if (ProcessingAGV != "Done" && ProcessingAGV != "UnKnown") // Find robot has state (ProcessingsAGV) = Done or UnKnow
      continue;

    indexForCallRobot = i;
    const char *robotname = doc[indexForCallRobot]["name"];
    sprintf(message, "%s", robotname);
    RobotName = String(message);

    // break;
    return id;

    // return id;
  }
  return "";
}

String CallRobot::getState(String stateName, String robotId)
{
  String url = "http://" + _Ip + ":" + _Port + "/api/Remote/Robot/" + robotId + "/State/" + stateName;
  String DataResult = HttpGet(url);
  if (DataResult == "error")
  {
    return "Get state error";
  }

  DynamicJsonDocument doc(1024);
  deserializeJson(doc, DataResult);
  const char *result = doc["result"];
  bool isError = doc["isError"];
  if (isError)
  {
    return "error";
  }

  return result;
}

int CallRobot::getTask(String taskName, String robotId)
{
  String url = "http://" + _Ip + ":" + _Port + "/api/Remote/Robot/" + robotId + "/Task/" + taskName;
  String DataResult = HttpGet(url);
  if (DataResult == "error")
  {
    return 0;
  }
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, DataResult);
  int result = doc["result"]["status"];
  bool isError = doc["isError"];
  if (isError)
    return 0;
  return result;
}

bool CallRobot::runTask(String taskName, String pointName, String robotId)
{
  String url = "http://" + _Ip + ":" + _Port + "/api/Remote/Robot/" + robotId + "/Task/" + taskName + "?args=" + pointName;
  String args = "args=" + pointName;
  Serial.println(url);
  String DataResult = HttpPost(url, args);
  if (DataResult == "error")
    return NULL;
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, DataResult);
  bool isError = doc["isError"];
  return ~isError;
}

bool CallRobot::CancelTask(String taskName, String robotId)
{
  String url = "http://" + _Ip + ":" + _Port + "/api/Remote/Robot/" + robotId + "/Task/" + taskName;
  Serial.println(url);
  String DataResult = HttpDelete(url);
  Serial.println(DataResult);
  if (DataResult == "")
    return NULL;
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, DataResult);
  bool isError = doc["isError"];
  return isError;
}

String CallRobot::CallingRobot()
{
  RobotId = getIdRobotForCall(StatusRobot, TaskStationName, ProcessingRobot);
  if (RobotId == "Get Id error" || RobotId == "")
    return "Get RobotId for Calling error";
  bool result = runTask(TaskStationName, StationName, RobotId); // runTask NavigationTo on robot agv, and register Station-1

  if (!result)
    return "Can't call robot " + RobotName + " run NavigationTo";
  StationStatusRuntime = StationStatus::waiting; // Waitting for robot come
  flagNavigationTo = 1;                          // Robot begin run task NavigationTo
  return "Waiting " + RobotName + " come to Station " + StationName;
}

String CallRobot::GoHomeRobot()
{
  if (RobotId == "" || RobotId == "Get Id error")
  {

    RobotId = getIdRobotForCall(StatusRobot, TaskStationName, ProcessingRobot);

    return "Get Id error";
  }
  int NavigationToStatus = getTask(TaskStationName, RobotId);
  // 2: task is running, 3: task is runcomplete
  if (NavigationToStatus == 2)
  {
    return RobotName + " is running task NavigationTo";
  }
  String ProcessingAGV = getState(ProcessingRobot, RobotId);
  Serial.println("ProcessingAGV of " + RobotName + ": " + ProcessingAGV);
  bool result;
  if (ProcessingAGV == "UnKnown" || ProcessingAGV == "Done")
  {
    result = runTask(TaskLineName, LineName, RobotId); // runTask NavigationLine on robot agv, and register Station-7
    // Serial.println("The state of NavigationTo is unknown ");
  }
  else
  {
    return "ProcessingAGV of " + RobotName + " is " + ProcessingAGV;
    result = false;
  }
  // if (ProcessingAGV != "Done")
  // {
  //   return "The state of NavigationTo is not done ";
  // }

  if (!result)
    return "Can't call robot " + RobotName + " run NavigationLine";

  StationStatusRuntime = StationStatus::freeMission;
  RobotId = "";
  return RobotName + " run task NavigationLine";
}

bool CallRobot::CancelAction()
{
  if (StationStatusRuntime == StationStatus::waiting)
  {
    bool result = CancelTask(TaskStationName, RobotId);
  }
  if (StationStatusRuntime == StationStatus::readyGoHome)
  {
    bool result = CancelTask(TaskLineName, RobotId);
  }
  StationStatusRuntime = StationStatus::freeMission;
  RobotId = "";
  return true;
}