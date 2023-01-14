#include "EspSPIFFS.h"
// Initialize SPIFFS

void EspSPIFFS::initSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String EspSPIFFS::readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while (file.available())
  {
    fileContent += (char)file.read(); // file.readStringUntil('\n');
    // break;
  }
  file.close();
  return fileContent;
}

String EspSPIFFS::readWiFiConfig(fs::FS &fs)
{
  return readFile(fs, wifiConfigPath);
}
String EspSPIFFS::readStationConfig(fs::FS &fs)
{
  return readFile(fs, stationConfigPath);
}
// Write file to SPIFFS
void EspSPIFFS::writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("- file written");
  }
  else
  {
    Serial.println("- frite failed");
  }
  file.close();
}

void EspSPIFFS::writeWiFiConfig(fs::FS &fs, const char *message)
{
  writeFile(fs, wifiConfigPath, message);
};

void EspSPIFFS::writeStationConfig(fs::FS &fs, const char *message)
{
  writeFile(fs, stationConfigPath, message);
};