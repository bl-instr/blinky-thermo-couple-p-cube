
#include "wifiCredentials.h"

#define BLINKYMQTTBUSBUFSIZE  5
union BlinkyBusUnion
{
  struct
  {
    int16_t state;
    int16_t watchdog;
    int16_t chipTemp;
    int16_t led1;
    int16_t led2;
  };
  int16_t buffer[BLINKYMQTTBUSBUFSIZE];
} blinkyBus;
void subscribeCallback(uint8_t address, int16_t value)
{
  if (address > 1) setLeds();
}

#include "blinky-wifiMqtt-cube.h"

int led1Pin = 10;
int led2Pin = 11;

void setup() 
{
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  Serial.begin(115200);
  delay(1000);
  initBlinkyBus(2000,true, LED_BUILTIN, 16);

  blinkyBus.led1 = 0;
  blinkyBus.led2 = 0;
  setLeds();


}

void loop() 
{
  blinkyBusLoop();
  blinkyBus.chipTemp = (int16_t) (analogReadTemp() * 100.0);
//  publishBlinkyBusNow(); 
}

void setLeds()
{
  analogWrite(led1Pin, blinkyBus.led1);    
  analogWrite(led2Pin, blinkyBus.led2);    
}
