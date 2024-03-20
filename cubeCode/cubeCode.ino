boolean printDiagnostics = false;
#include <SPI.h>


union CubeData
{
  struct
  {
    int16_t state;
    int16_t watchdog;
    int16_t temp1;
    int16_t temp2;
    int16_t temp3;
  };
  byte buffer[10];
};
CubeData cubeData;

#include "BlinkyPicoWCube.h"


int commLEDPin = 2;
int commLEDBright = 255; 
int resetButtonPin = 3;
int csPin1 = 17;
int csPin2 = 20;
int csPin3 = 21;


unsigned long lastPublishTime;
unsigned long publishInterval = 2000;
SPISettings spiSettings;

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
  cubeData.temp1 = 0;
  cubeData.temp2 = 0;
  cubeData.temp3 = 0;

  pinMode(csPin1, OUTPUT);
  digitalWrite(csPin1, HIGH);
  pinMode(csPin2, OUTPUT);
  digitalWrite(csPin2, HIGH);
  pinMode(csPin3, OUTPUT);
  digitalWrite(csPin3, HIGH);

  spiSettings = SPISettings(2000000, MSBFIRST, SPI_MODE1);
  SPI.begin();

}

void cubeLoop()
{
  unsigned long nowTime = millis();
  
  if ((nowTime - lastPublishTime) > publishInterval)
  {
    lastPublishTime = nowTime;
    cubeData.watchdog = cubeData.watchdog + 1;
    if (cubeData.watchdog > 32760) cubeData.watchdog= 0 ;
    BlinkyPicoWCube.publishToServer();
    
    cubeData.temp1 = (int16_t) getMAX31855Temperature(csPin1, spiSettings);
    cubeData.temp2 = (int16_t) getMAX31855Temperature(csPin2, spiSettings);
    cubeData.temp3 = (int16_t) getMAX31855Temperature(csPin3, spiSettings);
    if (printDiagnostics)
    {
      Serial.print("Raw Temps: ");
      Serial.print(cubeData.temp1);
      Serial.print(",");
      Serial.print(cubeData.temp2);
      Serial.print(",");
      Serial.println(cubeData.temp3);
    }
    

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

int getMAX31855Temperature(int chipSelect, SPISettings spiSetup)
{
  uint8_t  dataBufRead[4];
  int bits[32];
  int iTemp = 0;
  int pow2 = 1; 

  SPI.beginTransaction(spiSetup);
  digitalWrite (chipSelect, LOW);
  SPI.transfer(&dataBufRead, 4);
  digitalWrite (chipSelect, HIGH);
  SPI.endTransaction();

  for (int ibyte = 0; ibyte < 4; ++ibyte)
  {
    for (int ibit = 0; ibit < 8; ++ibit)
    {
      bits[31 - (ibyte * 8 + 7 - ibit)] = ((dataBufRead[ibyte] >> ibit) % 2);
    }
  }
  iTemp = 0;
  pow2 = 1;
  for (int ibit = 18; ibit < 31; ++ibit)
  {
    iTemp = iTemp + pow2 * bits[ibit];
    pow2 = pow2 * 2;
  }
  if (bits[31] > 0)
  {
    iTemp = iTemp - 8192;
  }
  return iTemp;
}
