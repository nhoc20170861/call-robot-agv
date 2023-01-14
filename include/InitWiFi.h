#ifndef __INITWIFI_H__
#define __INITWIFI_H__
#include <WiFi.h>

class InitWiFi
{
private:
    unsigned long previousMillis_cnWifi = 0;

public:
    InitWiFi(){};

    // Variables to save values from HTML form

    String ssid;
    String pass;
    String ip;
    String gateway;

    bool init();
};
#endif