#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
//#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include <ExtSoftwareSerial.h>

#include <ESP8266HTTPUpdateServer.h>

#include <ESP8266mDNS.h>

#include <Hash.h>
#include <PubSubClient.h>

#include "Dali.h"

#include <ArduinoOTA.h>

//define your default values here, if there are different values in config.json, they are overwritten.
char node_name[64] = "dali";
char mqtt_server[64] = "home.local";
char mqtt_port[6] = "1883";


//ESP MQTT
WiFiClient espClient;
PubSubClient mqttClient(espClient);

//Webserver
ESP8266WebServer webServer(80);
//DNSServer         dnsServer;

//flag for saving data
bool shouldSaveConfig = false;

Dali dali(13,15,14);

String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}

String getContentType(String filename){
  if(webServer.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  Serial.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    Serial.println("File opened: " + path);
    size_t sent = webServer.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
        char buffer[64];
        char ans_string[128] =  {0};
        memcpy(ans_string, payload, length);
        sprintf(buffer, "%s/result", node_name);
        Serial.println("Callback");

        payload[length] = 0;

        Serial.println((const char*)payload);

        int result = dali.parse_execute((const char*) ans_string);

        if(result == ACK)
        {
                mqttClient.publish(buffer, "ACK");
        }
        else if(result == NACK)
        {
                mqttClient.publish(buffer, "NACK");
        }
        else if((result & 0xFF00) == RESULT)
        {
                sprintf(ans_string, "ANS: %d", result & 0xFF);
                mqttClient.publish(buffer, ans_string);
        }
        else
        {
            mqttClient.publish(buffer, "FATAL ERROR");
        }



}

void filldynamicdata()
{
    String values ="";
    values += "mydynamicdata|" + (String) + "This is my Dynamic Value" + "|div\n";   // Build a string, like this:  ID|VALUE|TYPE
    webServer.send ( 200, "text/plain", values);
}

//callback notifying us of the need to save config
void saveConfigCallback () {
        Serial.println("Should save config");
        shouldSaveConfig = true;
}

void reset_wifi_settings()
{
        webServer.send(200, "text/plain", "Restoring factory settings");

        SPIFFS.remove("/config.json");
        WiFiManager wifiManager;

        wifiManager.resetSettings();

        delay(3000);

        ESP.reset();

        delay(5000);
}

void send_direct()
{
  //webServer.send(200, "text/plain", "Send");
  Serial.println("Send");
  dali.sendDirect(BROADCAST, 0, 199);
}

void handleRoot() {
        handleFileRead("/index.html");
}

#define wakeup_pin 12

void setup() {
        pinMode(wakeup_pin, OUTPUT);
        digitalWrite(wakeup_pin, HIGH);
        // put your setup code here, to run once:
        Serial.begin(115200);
        Serial.println();

        //clean FS, for testing
        //SPIFFS.format();

        //read configuration from FS json
        Serial.println("mounting FS...");

        if (SPIFFS.begin()) {
                Serial.println("mounted file system");
                if (SPIFFS.exists("/config.json")) {
                        //file exists, reading and loading
                        Serial.println("reading config file");
                        File configFile = SPIFFS.open("/config.json", "r");
                        if (configFile) {
                                Serial.println("opened config file");
                                size_t size = configFile.size();
                                // Allocate a buffer to store contents of the file.
                                std::unique_ptr<char[]> buf(new char[size]);

                                configFile.readBytes(buf.get(), size);
                                DynamicJsonBuffer jsonBuffer;
                                JsonObject& json = jsonBuffer.parseObject(buf.get());
                                json.printTo(Serial);
                                if (json.success()) {
                                        Serial.println("\nparsed json");
                                        if(json.containsKey("node_name"))
                                          strcpy(node_name, json["node_name"]);
                                        if(json.containsKey("mqtt_server"))
                                          strcpy(mqtt_server, json["mqtt_server"]);
                                        if(json.containsKey("mqtt_port"))
                                          strcpy(mqtt_port, json["mqtt_port"]);

                                } else {
                                        Serial.println("failed to load json config");
                                }
                        }
                }
        } else {
                Serial.println("failed to mount FS");
        }
        //end read
        {
          Dir dir = SPIFFS.openDir("/");
              while (dir.next()) {
                String fileName = dir.fileName();
                size_t fileSize = dir.fileSize();
                Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
          }
        }

        // The extra parameters to be configured (can be either global or just in the setup)
        // After connecting, parameter.getValue() will get you the configured value
        // id/name placeholder/prompt default length
        WiFiManagerParameter custom_node_name("name", "node name", node_name, 40);
        WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
        WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);

        //WiFiManager
        //Local intialization. Once its business is done, there is no need to keep it around
        WiFiManager wifiManager;

        //set config save notify callback
        wifiManager.setSaveConfigCallback(saveConfigCallback);

        //set static ip
        //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

        //add all your parameters here
        wifiManager.addParameter(&custom_node_name);
        wifiManager.addParameter(&custom_mqtt_server);
        wifiManager.addParameter(&custom_mqtt_port);

        //reset settings - for testing
        //wifiManager.resetSettings();

        //set minimu quality of signal so it ignores AP's under that quality
        //defaults to 8%
        //wifiManager.setMinimumSignalQuality();

        //sets timeout until configuration portal gets turned off
        //useful to make it all retry or go to sleep
        //in seconds
        //wifiManager.setTimeout(120);

        //fetches ssid and pass and tries to connect
        //if it does not connect it starts an access point with the specified name
        //here  "AutoConnectAP"
        //and goes into a blocking loop awaiting configuration
        if (!wifiManager.autoConnect("DaliMasterSettings", "password")) {
                Serial.println("failed to connect and hit timeout");
                delay(3000);
                //reset and try again, or maybe put it to deep sleep
                ESP.reset();
                delay(5000);
        }

        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");

        //read updated parameters
        strcpy(node_name, custom_node_name.getValue());
        strcpy(mqtt_server, custom_mqtt_server.getValue());
        strcpy(mqtt_port, custom_mqtt_port.getValue());

        for(int i =  0; i < 6; i++)
        {
                if(*(i + mqtt_port) == 0)
                        break;
                else if((*(i + mqtt_port) < '0') || (*(i + mqtt_port) > '9') )
                {
                        strcpy(mqtt_port, "1883");
                        break;
                }
        }

        //save the custom parameters to FS
        if (shouldSaveConfig) {
                Serial.println("saving config");
                DynamicJsonBuffer jsonBuffer;
                JsonObject& json = jsonBuffer.createObject();
                json["mqtt_server"] = mqtt_server;
                json["mqtt_port"] = mqtt_port;
                json["node_name"] = node_name;

                File configFile = SPIFFS.open("/config.json", "w");
                if (!configFile) {
                        Serial.println("failed to open config file for writing");
                }

                json.printTo(Serial);
                json.printTo(configFile);
                configFile.close();
                //end save
        }

        Serial.println("local ip");
        Serial.println(WiFi.localIP());

        ArduinoOTA.begin();

        //httpUpdater.setup(&webServer);
        mqttClient.setServer(mqtt_server, atoi(mqtt_port));
        //mqttClient.setServer("192.168.0.2", 1883);
        mqttClient.setCallback(mqtt_callback);

        char buffer[64];
        sprintf(buffer, "%s.local", node_name);
        Serial.print("DNS name: ");
        Serial.println(buffer);
        MDNS.begin(node_name);

        webServer.on("/reset_settings", reset_wifi_settings);
        webServer.on("/send", send_direct);
        webServer.on("/", handleRoot);

        webServer.on ( "/dynamic", filldynamicdata );

        webServer.onNotFound([](){
            if(!handleFileRead(webServer.uri()))
              webServer.send(404, "text/plain", "FileNotFound");
        });

        webServer.begin();

        MDNS.addService("http", "tcp", 80);
}

void mqtt_reconnect()
{
        Serial.println("Reconnect MQTT");
        char buffer[64];

        if(mqttClient.connect(node_name))
        {
                Serial.println("MQTT connected");

                sprintf(buffer, "%s/command", node_name);
                mqttClient.subscribe(buffer);
        }
        else
        {
                Serial.println("MQTT not connected");
        }


        sprintf(buffer, "MQTT state is: %d", mqttClient.state());
        Serial.println(buffer);
}


void loop() {
        if (!mqttClient.connected()) {
                mqtt_reconnect();
        }
        mqttClient.loop();
        ArduinoOTA.handle();
        webServer.handleClient();
}
