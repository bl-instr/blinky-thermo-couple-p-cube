#include "BlinkyMqttCube.h"
#include "one_wire.h" 
// from https://github.com/adamboardman/pico-onewire
struct
{
  int16_t state;
  int16_t watchdog;
  int16_t chipTemp;
  int16_t tempA;
  int16_t tempB;
} cubeData;

void subscribeCallback(uint8_t address, int16_t value)
{
  if (address > 1) return;
  return;
}
unsigned long g_measInterval = 2000;
unsigned long g_lastMeasTime = 0;
unsigned long g_now = 0;
int g_tempCount = 0;

One_wire tempAOneWire(12);
One_wire tempBOneWire(19);
rom_address_t tempAaddress{};
rom_address_t tempBaddress{};

void setup() 
{
  pinMode(10,OUTPUT);
  pinMode(21,OUTPUT);
  digitalWrite(10,HIGH);
  digitalWrite(21,HIGH);
  delay(1000);
//  Serial.begin(9600);
//  while (!Serial) {;}

  tempAOneWire.init();
  tempBOneWire.init();
  tempAOneWire.single_device_read_rom(tempAaddress);
  tempBOneWire.single_device_read_rom(tempBaddress);

  BlinkyMqttCube.setChattyCathy(false);
  BlinkyMqttCube.init(2000,true, 15, 255, 6, (int16_t*)& cubeData,  10, subscribeCallback);
  cubeData.tempA = -100;
  cubeData.tempB = -100;
  g_lastMeasTime = millis();
  g_tempCount = 0;
}

void loop() 
{
  cubeData.chipTemp = (int16_t) (analogReadTemp() * 100.0);
  BlinkyMqttCube.loop();
  g_now = millis();
  if ((g_now - g_lastMeasTime) > g_measInterval)
  {
    switch (g_tempCount) 
    {
      case 0:
        tempAOneWire.convert_temperature(tempAaddress, true, false);
        cubeData.tempA = (int16_t) (tempAOneWire.temperature(tempAaddress) * 100.0);
//        Serial.print("Temp A: ");
//        Serial.println(cubeData.tempA);
        break;
      case 1:
        tempBOneWire.convert_temperature(tempBaddress, true, false);
        cubeData.tempB = (int16_t) (tempBOneWire.temperature(tempBaddress) * 100.0);
//        Serial.print("Temp B: ");
//        Serial.println(cubeData.tempB);
        break;
      default:
        break;
    }
    g_tempCount = g_tempCount + 1;
    if (g_tempCount > 1) g_tempCount = 0;
    g_lastMeasTime = g_now;  
  }
}
