#include "CallRobot.h"
String CallRobot::getRobotName(void)
{
  return RobotName;
}
String CallRobot::getStationID(void)
{
  return StationID;
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

String CallRobot::getIdforAllRobot()
{
  String url = "http://" + _Ip + ":" + _Port + "/api/Remote/Robots?model=" + _Model + "&map=" + _MapName;
  Serial.println(url);
  String result = HttpGet(url);
  return result;
}
String CallRobot::getIdRobotForCall(String statusRobot, String taskName, String processRobot)
{

  String DataResult = getIdforAllRobot();

  if (DataResult == "error")
    return "Get Id error";
  else if (DataResult == "WiFiLost")
  {
    return "[GET] WiFiLost";
  }
  JSONVar json = JSON.parse(DataResult);
  // Get size array
  size_t length = json.length();
  // Serial.println("Length: " + String(length));
  int indexForCallRobot = 0;
  char message[50];
  for (int i = 0; i < length; i++)
  {
    const char *id = (const char *)json[i]["id"];
    const char *name = (const char *)json[i]["name"];
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
    const char *robotname = (const char *)json[indexForCallRobot]["name"];
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

  JSONVar json = JSON.parse(DataResult);
  const char *result = json["result"];
  bool isError = json["isError"];
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
  JSONVar json = JSON.parse(DataResult);
  int result = json["result"]["status"];
  bool isError = json["isError"];
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
  JSONVar json = JSON.parse(DataResult);
  bool isError = json["isError"];
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
  JSONVar json = JSON.parse(DataResult);
  bool isError = json["isError"];
  return isError;
}

String CallRobot::CallingRobot()
{
  if (StationStatusRuntime != StationStatus::unknown)
  {
    RobotId = getIdRobotForCall(StatusRobot, TaskStationName, ProcessingRobot);
    if (RobotId == "Get Id error" || RobotId == "")
      return "Get RobotId for Calling error";
  }
  else
  {
    int ProcessTask = getTask(TaskStationName, RobotId);
    if (ProcessTask == 0) // RobotId current no valiable connection
    {
      return "Waiting until Robot reconnects";
    }
  }

  bool result = runTask(TaskStationName, StationName, RobotId); // runTask NavigationTo on robot agv, and register Station-1

  if (!result)
    return "Can't call robot " + RobotName + " run task NavigationTo, try again!!";
  StationStatusRuntime = StationStatus::waiting; // Waitting for robot come
  flagNavigationTo = 1;                          // Robot begin run task NavigationTo
  return RobotName + " will come to Station " + StationName;
}

String CallRobot::GoHomeRobot()
{
  if (RobotId == "" || RobotId == "Get Id error")
  {
    // RobotId = getIdRobotForCall(StatusRobot, TaskStationName, ProcessingRobot);
    return "Can't call Robot go home because missing current Robotid!!";
  }

  bool result = runTask(TaskLineName, LineName, RobotId); // runTask NavigationLine on robot agv, and register Station-7

  if (!result)
    return "Can't call robot " + RobotName + " run NavigationLine";

  StationStatusRuntime = StationStatus::freeMission;
  RobotId = "";
  return RobotName + " run task NavigationLine";
}

bool CallRobot::CancelAction()
{
  if (flagNavigationTo == 1)
  {
    bool result = CancelTask(TaskStationName, RobotId);
    flagNavigationTo = 0;
  }

  StationStatusRuntime = StationStatus::freeMission;
  RobotId = "";
  return true;
}