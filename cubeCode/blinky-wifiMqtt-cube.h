#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient    g_wifiClient;
PubSubClient  g_mqttClient(g_wifiClient);
WiFiServer    g_wifiServer(80);
unsigned long g_lastMsgTime = 0;
unsigned long g_lastWirelessBlink;
int           g_wifiStatus = 0;
unsigned long g_wifiTimeout = 25000;
unsigned long g_wifiRetry = 30000;
unsigned long g_wifiLastTry = 0;
int           g_publishInterval  = 2000;
boolean       g_publishOnInterval = true;
boolean       g_publishNow = false;
int           g_commLEDPin = LED_BUILTIN;
boolean       g_commLEDState = false;
int           g_resetButtonPin = -1;
boolean       g_wifiAccessPointMode = false;
boolean       g_webPageServed = false;
boolean       g_webPageRead = false;
volatile unsigned long g_resetButtonDownTime = 0;

union SubscribeData
{
  struct
  {
      uint8_t command;
      uint8_t address;
      int16_t value;
  };
  byte buffer[4];
} g_subscribeData;



void setupWifiAp()
{
  g_wifiServer = WiFiServer(80);
  Serial.println("Reset");
  WiFi.disconnect();
  g_wifiAccessPointMode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("picoTest");
  Serial.print("[+] AP Created with IP Gateway ");
  Serial.println(WiFi.softAPIP());
  g_wifiServer.begin();
  Serial.print("Connected to wifi. My address:");
  IPAddress myAddress = WiFi.localIP();
  Serial.println(myAddress);
  g_webPageServed = false;
  g_webPageRead = false;
}
void serveWebPage()
{
  if (g_webPageServed) return;
  WiFiClient client = g_wifiServer.available();

  if (client) 
  {
    Serial.println("new client");
    String header = "";
    while (client.connected() && !g_webPageServed) 
    {
      if (client.available()) 
      {
        char c = client.read();
        header.concat(c);
      }
      else
      {
        break;
      }
    }
    Serial.println(header);
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");  // the connection will be closed after completion of the response
    client.println();
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<div>");
    client.println("<form action=\"/disconnected\" method=\"POST\">");
    client.println("<table align=\"center\">");
    client.println("<tr>");
    client.println("<td><label for=\"ssid\">SSID</label></td>");
    client.println("<td><input name=\"ssid\" id=\"ssid\" type=\"text\"/></td>");
    client.println("</tr>");
    client.println("<tr>");
    client.println("<td><label for=\"password\">Password</label></td>");
    client.println("<td><input name=\"password\" id=\"password\" type=\"password\"/></td>");
    client.println("</tr>");
    client.println("<tr>");
    client.println("<td></td>");
    client.println("<td><input type=\"submit\"/></td>");
    client.println("</tr>");
    client.println("</table>");
    client.println("</form>");
    client.println("</div>");
    client.println("</html>");
    g_webPageServed = true;
    delay(1);
    client.stop();
    Serial.println("client disconnected");
  }
}
void readWebPage()
{
  if (!g_webPageServed) return;
  if (g_webPageRead) return;
  WiFiClient client = g_wifiServer.available();

  if (client) 
  {
    String header = "";
    while (client.connected() && !g_webPageRead) 
    {
      if (client.available()) 
      {
        char c = client.read();
        header.concat(c);
      }
      else
      {
        break;
      }
    }
    if (header.indexOf("POST") >= 0)
    {
      Serial.println(header);
      String data = header.substring(header.indexOf("ssid="),header.length());
      g_ssid = data.substring(5,data.indexOf("&password="));
      g_wifiPassword = data.substring(data.indexOf("&password=") + 10,data.length());
      Serial.println(header);
      Serial.println(data);
      Serial.print("X");
      Serial.print(g_ssid);
      Serial.print("X");
      Serial.print(g_wifiPassword);
      Serial.println("X");
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");  // the connection will be closed after completion of the response
      client.println();
      client.println("<!DOCTYPE HTML>");
      client.println("<html>");
      client.println("<div>");
      client.println("</div>");
      client.println("</html>");
      g_webPageRead = true;
    }
    delay(1);
    client.stop();
    if (g_webPageRead)
    {
      g_wifiAccessPointMode = false;
      WiFi.disconnect();
    }
  }
}
void resetButtonHandler()
{
  if (digitalRead(g_resetButtonPin)) 
  {
    Serial.println("Button Down");
    g_resetButtonDownTime = millis();
  }
  else
  {
    Serial.println("Button up");
    if ((millis() - g_resetButtonDownTime) > 10000)
    {
      setupWifiAp();
    }
  }
}

void setup_wifi() 
{
  if ((millis() - g_wifiLastTry) < g_wifiRetry) return;
  g_commLEDState = 1;
  digitalWrite(g_commLEDPin, g_commLEDState);
  delay(10);
  g_wifiLastTry = millis();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(g_ssid);
  char ssidBuffer[40];
  char pwBuffer[40];
  g_ssid.toCharArray(ssidBuffer,40);
  g_wifiPassword.toCharArray(pwBuffer,40);
  
  Serial.print("SSID: ");
  Serial.println(ssidBuffer);
  Serial.print("Wifi Password: ");
  Serial.println(pwBuffer);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidBuffer, pwBuffer);
  
  while ((g_wifiStatus != WL_CONNECTED) && ((millis() - g_wifiLastTry) < g_wifiTimeout))
  {
    g_wifiStatus = WiFi.status();
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  if (g_wifiStatus == WL_CONNECTED)
  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("");
    Serial.println("WiFi not connected");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] {command: ");
  for (int i = 0; i < length; i++) 
  {
    g_subscribeData.buffer[i] = payload[i];
  }
  Serial.print(g_subscribeData.command);
  Serial.print(", address: ");
  Serial.print(g_subscribeData.address);
  Serial.print(", value: ");
  Serial.print(g_subscribeData.value);
  Serial.println("}");
  if (g_subscribeData.command == 1)
  {
      blinkyBus.buffer[g_subscribeData.address] = g_subscribeData.value;
      subscribeCallback(g_subscribeData.address, g_subscribeData.value);
  }

}

void reconnect() 
{
  // Loop until we're reconnected
  while (!g_mqttClient.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String mqttClientId = "PicoWClient-";
    mqttClientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (g_mqttClient.connect(mqttClientId.c_str(),mqttUsername, mqttPassword)) 
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
//      g_mqttClient.publish(mqttPublishTopic, "hello world");
      // ... and resubscribe
      g_mqttClient.subscribe(mqttSubscribeTopic);
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(g_mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void initBlinkyBus(int publishInterval, boolean publishOnInterval, int commLEDPin, int resetButtonPin)
{
  Serial.println("Starting Communications");
  g_publishInterval = publishInterval;
  g_publishOnInterval = publishOnInterval;
  setup_wifi();
  g_mqttClient.setServer(mqttServer, 1883);
  g_mqttClient.setCallback(mqttCallback);
  blinkyBus.state = 1; //init
  blinkyBus.watchdog = 0;
  g_commLEDPin = commLEDPin;
  pinMode(g_commLEDPin, OUTPUT);
  digitalWrite(g_commLEDPin, g_commLEDState);
  g_resetButtonPin = resetButtonPin;
  if (g_resetButtonPin > 0 )
  { 
    attachInterrupt(digitalPinToInterrupt(g_resetButtonPin), resetButtonHandler, CHANGE);
    pinMode(g_resetButtonPin, INPUT);
  }

  
}
void blinkyBusLoop()
{
  if (g_wifiAccessPointMode)
  {
    serveWebPage();
    readWebPage();
  }
  else
  {
    g_wifiStatus = WiFi.status();
    unsigned long now = millis();
    if (g_wifiStatus == WL_CONNECTED)
    {
      if (!g_mqttClient.connected()) 
      {
        reconnect();
      }
      g_mqttClient.loop();
    
      if (g_publishOnInterval)
      {
        if (g_commLEDState > 0)
        {
          if (now - g_lastMsgTime > 10) 
          {
            g_commLEDState = 0;
            digitalWrite(g_commLEDPin, g_commLEDState);
          }
        }
        if (now - g_lastMsgTime > g_publishInterval) 
        {
          g_lastMsgTime = now;
          g_commLEDState = 1;
          digitalWrite(g_commLEDPin, g_commLEDState);
          blinkyBus.watchdog = blinkyBus.watchdog + 1;
          if (blinkyBus.watchdog > 32760) blinkyBus.watchdog = 0 ;
          g_mqttClient.publish(mqttPublishTopic, (uint8_t*)blinkyBus.buffer, BLINKYMQTTBUSBUFSIZE * 2);
        }
      }
      if (g_publishNow)
      {
          g_lastMsgTime = now;
          g_commLEDState = 1;
          digitalWrite(g_commLEDPin, g_commLEDState);
          g_publishNow = false;
          blinkyBus.watchdog = blinkyBus.watchdog + 1;
          if (blinkyBus.watchdog > 32760) blinkyBus.watchdog = 0 ;
          g_mqttClient.publish(mqttPublishTopic, (uint8_t*)blinkyBus.buffer, BLINKYMQTTBUSBUFSIZE * 2);
      }
    }
    else
    {
      setup_wifi();
      if ((now - g_lastWirelessBlink) > 100)
      {
        g_commLEDState = !g_commLEDState;
        g_lastWirelessBlink = now;
        digitalWrite(g_commLEDPin, g_commLEDState);
      }
    }
  }

}
void publishBlinkyBusNow()
{
  g_publishNow = true;
}
