#define BLINKY_DIAG        0
#define CUBE_DIAG          0
#define COMM_LED_PIN       2
#define RST_BUTTON_PIN     3
#include <BlinkyPicoW.h>
#include <SPI.h>

struct CubeSetting
{
  uint16_t publishInterval;
};
CubeSetting setting;

struct CubeReading
{
  int16_t tempA;
  int16_t tempB;
  int16_t tempC;
};
CubeReading reading;

unsigned long lastPublishTime;

int csPin1 = 17;
int csPin2 = 20;
int csPin3 = 21;


SPISettings spiSettings;

void setupBlinky()
{
  if (BLINKY_DIAG > 0) Serial.begin(9600);

  BlinkyPicoW.setMqttKeepAlive(15);
  BlinkyPicoW.setMqttSocketTimeout(4);
  BlinkyPicoW.setMqttPort(1883);
  BlinkyPicoW.setMqttLedFlashMs(100);
  BlinkyPicoW.setHdwrWatchdogMs(8000);

  BlinkyPicoW.begin(BLINKY_DIAG, COMM_LED_PIN, RST_BUTTON_PIN, true, sizeof(setting), sizeof(reading));
}

void setupCube()
{
  if (CUBE_DIAG > 0) Serial.begin(9600);
  setting.publishInterval = 4000;
  lastPublishTime = millis();

  reading.tempA = 0;
  reading.tempB = 0;
  reading.tempC = 0;

  pinMode(csPin1, OUTPUT);
  digitalWrite(csPin1, HIGH);
  pinMode(csPin2, OUTPUT);
  digitalWrite(csPin2, HIGH);
  pinMode(csPin3, OUTPUT);
  digitalWrite(csPin3, HIGH);

  spiSettings = SPISettings(2000000, MSBFIRST, SPI_MODE1);
  SPI.begin();

}

void loopCube()
{
  unsigned long now = millis();
  if ((now - lastPublishTime) > setting.publishInterval)
  {
    
    reading.tempA = (int16_t) getMAX31855Temperature(csPin1, spiSettings);
    reading.tempB = (int16_t) getMAX31855Temperature(csPin2, spiSettings);
    reading.tempC = (int16_t) getMAX31855Temperature(csPin3, spiSettings);
    if (CUBE_DIAG > 0)
    {
      Serial.print("Raw Temps: ");
      Serial.print(reading.tempA);
      Serial.print(",");
      Serial.print(reading.tempB);
      Serial.print(",");
      Serial.println(reading.tempC);
    }
    lastPublishTime = now;
    boolean successful = BlinkyPicoW.publishCubeData((uint8_t*) &setting, (uint8_t*) &reading, false);
  }  
  boolean newSettings = BlinkyPicoW.retrieveCubeSetting((uint8_t*) &setting);
  if (newSettings)
  {
    if (setting.publishInterval < 1000) setting.publishInterval = 1000;
  }
  
}


int getMAX31855Temperature(int chipSelect, SPISettings spiSetup)
{
  uint8_t  dataBufRead[4];
  int bits[32];
  int iTemp = 0;
  int pow2 = 1; 

  SPI.beginTransaction(spiSetup);
  delay(10);
  digitalWrite (chipSelect, LOW);
  delay(10);
  SPI.transfer(&dataBufRead, 4);
  delay(10);
  digitalWrite (chipSelect, HIGH);
  delay(10);
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
