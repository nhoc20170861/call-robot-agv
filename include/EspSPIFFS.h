#ifndef __ESP_SPIFFS_H__
#define __ESP_SPIFFS_H__
#include "SPIFFS.h"
class EspSPIFFS
{
private:
    // File paths to save input values permanently
    const char *ssidPath = "/config/ssid.txt";
    const char *passPath = "/config/pass.txt";
    const char *ipPath = "/config/ip.txt";
    const char *gatewayPath = "/config/gateway.txt";
    const char *wifiConfigPath = "/config/wifiConfig.json";
    const char *stationConfigPath = "/config/stationConfig.json";

public:
    EspSPIFFS(){};
    ~EspSPIFFS();

    void initSPIFFS();
    String readFile(fs::FS &fs, const char *path);
    void writeFile(fs::FS &fs, const char *path, const char *message);

    String readStationConfig(fs::FS &fs);
    void writeStationConfig(fs::FS &fs, const char *message);

    String readWiFiConfig(fs::FS &fs);
    void writeWiFiConfig(fs::FS &fs, const char *message);
};

#endif