#include <WiFi.h>
#include <PubSubClient.h>
#include "LittleFS.h"

String        g_ssid;
String        g_wifiPassword;
String        g_mqttServer;
String        g_mqttUsername;
String        g_mqttPassword;
String        g_mqttPublishTopic;
String        g_mqttSubscribeTopic;
WiFiClient    g_wifiClient;
PubSubClient  g_mqttClient(g_wifiClient);
WiFiServer    g_wifiServer(80);
unsigned long g_lastMsgTime = 0;
unsigned long g_lastWirelessBlink;
int           g_wifiStatus = 0;
unsigned long g_wifiTimeout = 25000;
unsigned long g_wifiRetry = 30000;
unsigned long g_wifiLastTry = 0;
unsigned long g_resetTimeout = 10000;
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
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(g_mqttUsername);
  Serial.print("[+] AP Created with IP Gateway ");
  Serial.println(WiFi.softAPIP());
  g_wifiServer = WiFiServer(80);
  g_wifiAccessPointMode = true;
  g_wifiServer.begin();
  Serial.print("Connected to wifi. My address:");
  IPAddress myAddress = WiFi.localIP();
  Serial.println(myAddress);
  g_webPageServed = false;
  g_webPageRead = false;
}
String replaceHtmlEscapeChar(String inString)
{
  String outString = "";
  char hexBuff[3];
  char *hexptr;
  for (int ii = 0; ii < inString.length(); ++ii)
  {
    char testChar = inString.charAt(ii);
    if (testChar == '%')
    {
      inString.substring(ii + 1,ii + 3).toCharArray(hexBuff, 3);
      char specialChar = (char) strtoul(hexBuff, &hexptr, 16);
      outString += specialChar;
      ii = ii + 2;
    }
    else
    {
      outString += testChar;
    }
  }
  return outString;
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
    client.println("<html><head><title>Blinky-Lite Credentials</title><style>");
    client.println("body{background-color: #083357 !important;font-family: Arial;}");
    client.println(".labeltext{color: white;font-size:250%;}");
    client.println(".formtext{color: black;font-size:250%;}");
    client.println(".cell{padding-bottom:25px;}");
    client.println("</style></head><body>");
    client.println("<h1 style=\"color:white;font-size:300%;text-align: center;\">Blinky-Lite Credentials</h1><hr><div>");
    client.println("<form action=\"/disconnected\" method=\"POST\">");
    client.println("<table align=\"center\">");
    
    client.println("<tr>");
    client.println("<td class=\"cell\"><label for=\"ssid\" class=\"labeltext\">SSID</label></td>");
    String tag = "<td class=\"cell\"><input name=\"ssid\" id=\"ssid\" type=\"text\" value=\"" + g_ssid + "\" class=\"formtext\"/></td>";
    client.println(tag.c_str());
    client.println("</tr>");
    
    client.println("<tr>");
    client.println("<td class=\"cell\"><label for=\"pass\" class=\"labeltext\">Wifi Password</label></td>");
    tag = "<td class=\"cell\"><input name=\"pass\" id=\"pass\" type=\"password\" value=\"" + g_wifiPassword + "\" class=\"formtext\"/></td>";
    client.println(tag.c_str());
    client.println("</tr>");
    
    client.println("<tr>");
    client.println("<td class=\"cell\"><label for=\"serv\" class=\"labeltext\">MQTT Server</label></td>");
    tag = "<td class=\"cell\"><input name=\"serv\" id=\"serv\" type=\"text\" value=\"" + g_mqttServer + "\" class=\"formtext\"/></td>";
    client.println(tag.c_str());
    client.println("</tr>");
  
    client.println("<tr>");
    client.println("<td class=\"cell\"><label for=\"unam\" class=\"labeltext\">MQTT Username</label></td>");
    tag = "<td class=\"cell\"><input name=\"unam\" id=\"unam\" type=\"text\" value=\"" + g_mqttUsername + "\" class=\"formtext\"/></td>";
    client.println(tag.c_str());
    client.println("</tr>");
    
    client.println("<tr>");
    client.println("<td class=\"cell\"><label for=\"mpass\" class=\"labeltext\">MQTT Password</label></td>");
    tag = "<td class=\"cell\"><input name=\"mpass\" id=\"mpass\" type=\"password\" value=\"" + g_mqttPassword + "\" class=\"formtext\"/></td>";
    client.println(tag.c_str());
    client.println("</tr>");
   
    client.println("<tr>");
    client.println("<td class=\"cell\"><label for=\"pubt\" class=\"labeltext\">MQTT Pub. Topic</label></td>");
    tag = "<td class=\"cell\"><input name=\"pubt\" id=\"pubt\" type=\"text\" value=\"" + g_mqttPublishTopic + "\" class=\"formtext\"/></td>";
    client.println(tag.c_str());
    client.println("</tr>");
      
    client.println("<tr>");
    client.println("<td class=\"cell\"><label for=\"subt\" class=\"labeltext\">MQTT Sub. Topic</label></td>");
    tag = "<td class=\"cell\"><input name=\"subt\" id=\"subt\" type=\"text\" value=\"" + g_mqttSubscribeTopic + "\" class=\"formtext\"/></td>";
    client.println(tag.c_str());
    client.println("</tr>");
    
    client.println("<tr>");
    client.println("<td></td>");
    client.println("<td><input type=\"submit\" class=\"formtext\"/></td>");
    client.println("</tr>");
    client.println("</table>");
    client.println("</form>");
    client.println("</div>");
    client.println("</body></html>");
    g_webPageServed = true;
    delay(100);
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
      g_ssid               = replaceHtmlEscapeChar(data.substring(5,data.indexOf("&pass=")));
      g_wifiPassword       = replaceHtmlEscapeChar(data.substring(data.indexOf("&pass=")  + 6,data.indexOf("&serv=")));
      g_mqttServer         = replaceHtmlEscapeChar(data.substring(data.indexOf("&serv=")  + 6,data.indexOf("&unam=")));
      g_mqttUsername       = replaceHtmlEscapeChar(data.substring(data.indexOf("&unam=")  + 6,data.indexOf("&mpass=")));
      g_mqttPassword       = replaceHtmlEscapeChar(data.substring(data.indexOf("&mpass=") + 7,data.indexOf("&pubt=")));
      g_mqttPublishTopic   = replaceHtmlEscapeChar(data.substring(data.indexOf("&pubt=")  + 6,data.indexOf("&subt=")));
      g_mqttSubscribeTopic = replaceHtmlEscapeChar(data.substring(data.indexOf("&subt=")  + 6,data.length()));
      Serial.println(header);
      Serial.println(data);
      Serial.print("X");
      Serial.print(g_ssid);
      Serial.print("X");
      Serial.print(g_wifiPassword);
      Serial.print("X");
      Serial.print(g_mqttServer);
      Serial.print("X");
      Serial.print(g_mqttUsername);
      Serial.print("X");
      Serial.print(g_mqttPassword);
      Serial.print("X");
      Serial.print(g_mqttPublishTopic);
      Serial.print("X");
      Serial.print(g_mqttSubscribeTopic);
      Serial.println("X");
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");  // the connection will be closed after completion of the response
      client.println();
      client.println("<!DOCTYPE HTML>");
      client.println("<html><head><title>Blinky-Lite Credentials</title><style>");
      client.println("body{background-color: #083357 !important;font-family: Arial;}");
      client.println(".labeltext{color: white;font-size:250%;}");
      client.println(".formtext{color: black;font-size:250%;}");
      client.println(".cell{padding-bottom:25px;}");
      client.println("</style></head><body>");
      client.println("<h1 style=\"color:white;font-size:300%;text-align: center;\">Blinky-Lite Credentials</h1><hr><div>");
      client.println("<div>");
      client.println("<h1 style=\"color:yellow;font-size:300%;text-align: center;\">Accepted</h1><div>");
      client.println("</div>");
      client.println("</body></html>");
      g_webPageRead = true;
    }
    delay(100);
    client.stop();
    if (g_webPageRead)
    {
      g_wifiAccessPointMode = false;
      WiFi.disconnect();
      Serial.println("Writing creds.txt file");
      File file = LittleFS.open("/creds.txt", "w");
      if (file) 
      {
        String line = "";
        line = "{" + g_ssid + "}";
        file.println(line);
        line = "{" + g_wifiPassword + "}";
        file.println(line);
        line = "{" + g_mqttServer + "}";
        file.println(line);
        line = "{" + g_mqttUsername + "}";
        file.println(line);
        line = "{" + g_mqttPassword + "}";
        file.println(line);
        line = "{" + g_mqttPublishTopic + "}";
        file.println(line);
        line = "{" + g_mqttSubscribeTopic + "}";
        file.println(line);
        file.println("");
        file.println("");
        file.println("");
        file.close();
      }
      else
      {
        Serial.println("file open failed");
      }

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
    if ((millis() - g_resetButtonDownTime) > g_resetTimeout)
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
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(g_ssid);
  
  Serial.print("SSID: ");
  Serial.println(g_ssid);
  Serial.print("Wifi Password: ");
  Serial.println(g_wifiPassword);

  WiFi.mode(WIFI_STA);
  WiFi.begin(g_ssid.c_str(), g_wifiPassword.c_str());
  
  g_wifiLastTry = millis();
  g_wifiStatus = WiFi.status();
  while ((g_wifiStatus != WL_CONNECTED) && ((millis() - g_wifiLastTry) < g_wifiTimeout))
  {
    g_wifiStatus = WiFi.status();
    delay(500);
    Serial.print(".");
  }
  g_wifiLastTry = millis();

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
  if (!g_mqttClient.connected()) 
  {
    Serial.print("Attempting MQTT connection using ID...");
    // Create a random client ID
    String mqttClientId = g_mqttUsername + "-";
    mqttClientId += String(random(0xffff), HEX);
    Serial.print(mqttClientId);
    // Attempt to connect
    if (g_mqttClient.connect(mqttClientId.c_str(),g_mqttUsername.c_str(), g_mqttPassword.c_str())) 
    {
      Serial.println("...connected");
      // Once connected, publish an announcement...
//      g_mqttClient.publish(g_mqttPublishTopic, "hello world");
      // ... and resubscribe
      g_mqttClient.subscribe(g_mqttSubscribeTopic.c_str());
    } 
    else 
    {
      int mqttState = g_mqttClient.state();
      Serial.print("failed, rc=");
      Serial.print(mqttState);
      Serial.print(": ");
      switch (mqttState) 
      {
        case -4:
          Serial.println("MQTT_CONNECTION_TIMEOUT");
          break;
        case -3:
          Serial.println("MQTT_CONNECTION_LOST");
          break;
        case -2:
          Serial.println("MQTT_CONNECT_FAILED");
          break;
        case -1:
          Serial.println("MQTT_DISCONNECTED");
          break;
        case 0:
          Serial.println("MQTT_CONNECTED");
          break;
        case 1:
          Serial.println("MQTT_CONNECT_BAD_PROTOCOL");
          break;
        case 2:
          Serial.println("MQTT_CONNECT_BAD_CLIENT_ID");
          break;
        case 3:
          Serial.println("MQTT_CONNECT_UNAVAILABLE");
          break;
        case 4:
          Serial.println("MQTT_CONNECT_BAD_CREDENTIALS");
          break;
        case 5:
          Serial.println("MQTT_CONNECT_UNAUTHORIZED");
          break;
        default:
          break;
      }
    }
  }
}
void initBlinkyBus(int publishInterval, boolean publishOnInterval, int commLEDPin, int resetButtonPin)
{
  Serial.println("Reading creds.txt file");
  LittleFS.begin();
  File file = LittleFS.open("/creds.txt", "r");
  if (file) 
  {
      String lines[7];
      String data = file.readString();
      int startPos = 0;
      int stopPos = 0;
      for (int ii = 0; ii < 7; ++ii)
      {
        startPos = data.indexOf("{") + 1;
        stopPos = data.indexOf("}");
        lines[ii] = data.substring(startPos,stopPos);
        Serial.println(lines[ii]);
        data = data.substring(stopPos + 1);
      }
      g_ssid               = lines[0];
      g_wifiPassword       = lines[1];
      g_mqttServer         = lines[2];
      g_mqttUsername       = lines[3];
      g_mqttPassword       = lines[4];
      g_mqttPublishTopic   = lines[5];
      g_mqttSubscribeTopic = lines[6];
      file.close();
      g_mqttClient.setKeepAlive(60);
      g_mqttClient.setSocketTimeout(60);
  }
  else
  {
    Serial.println("file open failed");
  }

  Serial.println("Starting Communications");
  g_publishInterval = publishInterval;
  g_publishOnInterval = publishOnInterval;
  setup_wifi();
  g_mqttClient.setServer(g_mqttServer.c_str(), 1883);
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
    if (g_resetButtonPin > 0 )
    {
      if (digitalRead(g_resetButtonPin))
      {
        if ((millis() - g_resetButtonDownTime) < g_resetTimeout)
        {
          g_commLEDState = 0;
          digitalWrite(g_commLEDPin, g_commLEDState);
          return;
        }
        else
        {
          g_commLEDState = 1;
          digitalWrite(g_commLEDPin, g_commLEDState);
          return;
        }
      }
    }
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
          g_mqttClient.publish(g_mqttPublishTopic.c_str(), (uint8_t*)blinkyBus.buffer, BLINKYMQTTBUSBUFSIZE * 2);
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
          g_mqttClient.publish(g_mqttPublishTopic.c_str(), (uint8_t*)blinkyBus.buffer, BLINKYMQTTBUSBUFSIZE * 2);
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
