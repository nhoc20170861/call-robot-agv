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

public:
    EspSPIFFS(){};
    ~EspSPIFFS();

    void initSPIFFS();
    String readFile(fs::FS &fs, const char *path);

    String readIP(fs::FS &fs);
    String readSSID(fs::FS &fs);
    String readPASS(fs::FS &fs);
    String readGATEWAY(fs::FS &fs);

    void writeFile(fs::FS &fs, const char *path, const char *message);
    void writeIP(fs::FS &fs, const char *message);
    void writePASS(fs::FS &fs, const char *message);
    void writeSSID(fs::FS &fs, const char *message);
    void writeGATEWAY(fs::FS &fs, const char *message);
};

#endif