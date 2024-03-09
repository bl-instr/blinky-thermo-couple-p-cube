#include <SPI.h>

int csPin1 = 17;
int csPin2 = 20;
int csPin3 = 21;

unsigned long lastPublishTime;
unsigned long publishInterval = 3000;
SPISettings spiSettings;

void setup() 
{
  Serial.begin(115200);
  lastPublishTime = millis();
  pinMode(csPin1, OUTPUT);
  digitalWrite(csPin1, HIGH);
  pinMode(csPin2, OUTPUT);
  digitalWrite(csPin2, HIGH);
  pinMode(csPin3, OUTPUT);
  digitalWrite(csPin3, HIGH);

  spiSettings = SPISettings(2000000, MSBFIRST, SPI_MODE1);
  SPI.begin();

}
int itemp;
void loop() 
{
  unsigned long nowTime = millis();
  
  if ((nowTime - lastPublishTime) > publishInterval)
  {

    lastPublishTime = nowTime;
    
    itemp = getMAX31855Temperature(csPin1, spiSettings);
    Serial.print(itemp);
    Serial.print(",");
    itemp = getMAX31855Temperature(csPin2, spiSettings);
    Serial.print(itemp);
    Serial.print(",");
    itemp  = getMAX31855Temperature(csPin3, spiSettings);
    Serial.println(itemp);

  }  
}
// 4 * temperature
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
