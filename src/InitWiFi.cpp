
#include "InitWiFi.h"
IPAddress localIP;
// IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
IPAddress localGateway;

// IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 254, 0);

IPAddress primaryDNS(8, 8, 8, 8);   // optional
IPAddress secondaryDNS(8, 8, 4, 4); // optional
bool InitWiFi::init()
{
    Serial.println("WiFi config with:");
    Serial.println(ssid);
    Serial.println(pass);
    Serial.println(ip);
    Serial.println(gateway);
    const long interval = 10000; // interval to wait for Wi-Fi connection (milliseconds)

    if (ssid == "" || ip == "")
    {
        Serial.println("Undefined SSID or IP address.");
        return false;
    }

    WiFi.mode(WIFI_STA);
    char Host_Name[11];
    byte mac[6];
    WiFi.macAddress(mac);
    sprintf(Host_Name, "ESP_%02X%02X%02X", mac[3], mac[4], mac[5]);
    WiFi.setHostname(Host_Name);

    // // -------------- Config ESP Wifi_sta with ip static--------
    localIP.fromString(ip.c_str());
    localGateway.fromString(gateway.c_str());

    if (!WiFi.config(localIP, localGateway, subnet, primaryDNS, secondaryDNS))
    {
        Serial.println("STA Failed to configure");
        return false;
    }

    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.println("Connecting to WiFi...");

    unsigned long currentMillis = millis();
    previousMillis_cnWifi = currentMillis;

    while (WiFi.status() != WL_CONNECTED)
    {
        currentMillis = millis();
        if (currentMillis - previousMillis_cnWifi >= interval)
        {
            Serial.println("Failed to connect.");
            return false;
        }
    }
    Serial.println("Successfully connected to Access Point:  " + ssid);
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    long rssi = WiFi.RSSI();
    Serial.print("RSSI:");
    Serial.println(rssi);

    return true;
}
