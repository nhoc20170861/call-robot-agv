#ifndef __CALLROBOT__
#define __CALLROBOT__

#include <HTTPClient.h>
#include <Arduino_JSON.h>
enum StationStatus
{
    freeMission = 1, // Trạng thái ban đầu khi chưa gọi robot đến
    waiting,         // Trạm gọi đang đợi Robot đến
    readyGoHome,     // Robot đã đến, có thể gọi Robot từ vị trí trạm gọi di chuyển đến vị trí trả hàng hoặc về (home)
    unknown,         // Robot đang di chuyển đến trạm gọi nhưng bị mất tín hiệu
};

class CallRobot
{
private:
    // HTTPClient HttpClient;
    String _Ip = ""; // biến lưu địa chỉ của server Flet Manager
    String _Model = "";
    String _MapName = "";
    int _Port = 0;
    String RobotId = "";
    String RobotName = "";
    String StationName = "station-1";
    String LineName = "station-7";
    String TaskStationName = "NavigationTo";
    String TaskLineName = "NavigationLine";

    String StatusRobot = "StatusAGV";         // state robot type1
    String ProcessingRobot = "ProcessingAGV"; // state robot type2
    String RobotModel = "agv-500";
    // String RobotModel = "agv-line"; // simiulation
    String MapName = "demo-f1";

public:
    CallRobot(String ip, int port)
    {
        _Ip = ip;
        _Port = port;
        _Model = RobotModel;
        _MapName = MapName;
    };
    CallRobot(String ip, int port, String RobotModel, String MapName)
    {
        _Ip = ip;
        _Port = port;
        _Model = RobotModel;
        _MapName = MapName;
    };
    ~CallRobot(){};

    StationStatus StationStatusRuntime = StationStatus::freeMission;
    String HttpGet(String url);
    String HttpPost(String url, String data);
    String HttpDelete(String url);
    String getIdRobotForCall(String statusRobot, String taskName, String processRobot);
    String getState(String stateName, String robotId);

    int getTask(String taskName, String robotId);
    bool runTask(String taskName, String pointName, String robotId);
    bool CancelTask(String taskName, String robotId);

    String CallingRobot();
    String GoHomeRobot();
    bool CancelAction();

    void setStationName(const char *input);
    String getStationName(void);

    void setLineName(const char *input);
    String getLineName(void);

    String getRobotName(void);
    String getIdRobotCurrent(void);
    String getIdforAllRobot(void);
    int flagNavigationTo = 0;
};

#endif