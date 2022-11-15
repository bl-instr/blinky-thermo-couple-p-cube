#include "BlinkyMqttCube.h"

struct
{
  int16_t state;
  int16_t watchdog;
  int16_t chipTemp;
  int16_t led1;
  int16_t led2;
} cubeData;

void subscribeCallback(uint8_t address, int16_t value)
{
  if (address > 1) setLeds();
}


int led1Pin = 10;
int led2Pin = 11;

void setup() 
{
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  delay(1000);
//  Serial.begin(115200);
//  while (!Serial) {;}

  BlinkyMqttCube.setChattyCathy(false);
  BlinkyMqttCube.init(2000,true, 17, 255, 16, (int16_t*)& cubeData,  sizeof(cubeData), subscribeCallback);

  cubeData.led1 = 0;
  cubeData.led2 = 0;
  setLeds();
}

void loop() 
{
  BlinkyMqttCube.loop();
  cubeData.chipTemp = (int16_t) (analogReadTemp() * 100.0);
//  publishBlinkyBusNow(); 
}

void setLeds()
{
  analogWrite(led1Pin, cubeData.led1);    
  analogWrite(led2Pin, cubeData.led2);    
}
