
const char* ssid = "xxxxx";
const char* wifiPassword = "xxxxx";
const char* mqttServer = "xxxxx";
const char* mqttUsername = "xxxxx";
const char* mqttPassword = "xxxxx";
const char* mqttPublishTopic = "blinky-mqtt-led/XX/reading";
const char* mqttSubscribeTopic = "blinky-mqtt-led/XX/setting";


#define BLINKYMQTTBUSBUFSIZE  5
union BlinkyBusUnion
{
  struct
  {
    int16_t state;
    int16_t watchdog;
    int16_t led1;
    int16_t led2;
    int16_t led3;
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
int led3Pin = 12;

void setup() 
{
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(led3Pin, OUTPUT);
  Serial.begin(115200);
  delay(5000);
  initBlinkyBus(2000,true, LED_BUILTIN);

  blinkyBus.led1 = 0;
  blinkyBus.led2 = 0;
  blinkyBus.led3 = 0;
  setLeds();
}

void loop() 
{
  blinkyBusLoop();
//  publishBlinkyBusNow(); 
}

void setLeds()
{
  digitalWrite(led1Pin, blinkyBus.led1);    
  digitalWrite(led2Pin, blinkyBus.led2);    
  digitalWrite(led3Pin, blinkyBus.led3);    
}
