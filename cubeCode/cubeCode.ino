boolean printDiagnostics = false;

union CubeData
{
  struct
  {
    int16_t state;
    int16_t watchdog;
    int16_t chipTemp;
    int16_t tempA;
    int16_t tempB;
  };
  byte buffer[10];
};
CubeData cubeData;

#include "BlinkyPicoWCube.h"
#include "one_wire.h" 
// from https://github.com/adamboardman/pico-onewire


int commLEDPin = 11;
int commLEDBright = 255; 
int resetButtonPin = 15;

unsigned long lastPublishTime;
unsigned long publishInterval = 3000;

One_wire tempAOneWire(16);
One_wire tempBOneWire(18);
rom_address_t tempAaddress{};
rom_address_t tempBaddress{};
int g_tempCount = 0;

void setupServerComm()
{
  // Optional setup to overide defaults
  if (printDiagnostics)
  {
    Serial.begin(115200);
    delay(10000);
  }
  BlinkyPicoWCube.setChattyCathy(printDiagnostics);
  BlinkyPicoWCube.setWifiTimeoutMs(20000);
  BlinkyPicoWCube.setWifiRetryMs(20000);
  BlinkyPicoWCube.setMqttRetryMs(3000);
  BlinkyPicoWCube.setResetTimeoutMs(10000);
  BlinkyPicoWCube.setHdwrWatchdogMs(8000);
  BlinkyPicoWCube.setBlMqttKeepAlive(8);
  BlinkyPicoWCube.setBlMqttSocketTimeout(4);
  BlinkyPicoWCube.setMqttLedFlashMs(10);
  BlinkyPicoWCube.setWirelesBlinkMs(100);
  BlinkyPicoWCube.setMaxNoMqttErrors(5);
  
  // Must be included
  BlinkyPicoWCube.init(commLEDPin, commLEDBright, resetButtonPin);
}

void setupCube()
{
  lastPublishTime = millis();
  cubeData.state = 1;
  cubeData.watchdog = 0;

  tempAOneWire.init();
  tempBOneWire.init();
  tempAOneWire.single_device_read_rom(tempAaddress);
  tempBOneWire.single_device_read_rom(tempBaddress);
  cubeData.tempA = -100;
  cubeData.tempB = -100;
  g_tempCount = 0;
}

void cubeLoop()
{
  unsigned long nowTime = millis();
  
  if ((nowTime - lastPublishTime) > publishInterval)
  {
    cubeData.chipTemp = (int16_t) (analogReadTemp() * 100.0);
    switch (g_tempCount) 
    {
      case 0:
        tempAOneWire.convert_temperature(tempAaddress, true, false);
        cubeData.tempA = (int16_t) (tempAOneWire.temperature(tempAaddress) * 100.0);
//        if (printDiagnostics) Serial.print("Temp A: ");
//        if (printDiagnostics) Serial.println(cubeData.tempA);
        break;
      case 1:
        tempBOneWire.convert_temperature(tempBaddress, true, false);
        cubeData.tempB = (int16_t) (tempBOneWire.temperature(tempBaddress) * 100.0);
//        if (printDiagnostics) Serial.print("Temp B: ");
//        if (printDiagnostics) Serial.println(cubeData.tempB);
        break;
      default:
        break;
    }
    g_tempCount = g_tempCount + 1;
    if (g_tempCount > 1) g_tempCount = 0;

    lastPublishTime = nowTime;
    cubeData.watchdog = cubeData.watchdog + 1;
    if (cubeData.watchdog > 32760) cubeData.watchdog= 0 ;
    BlinkyPicoWCube::publishToServer();
  }  
  
}


void handleNewSettingFromServer(uint8_t address)
{
  switch(address)
  {
    case 0:
      break;
    case 1:
      break;
    case 2:
      break;
    default:
      break;
  }
}
