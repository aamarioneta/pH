#include <PubSubClient.h>
#include <WiFi.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "credentials.h"
#include "mqtt.h"
#include <OneWire.h>
#include <DallasTemperature.h>

const int pinTemp = 25;
const int pinPh = 35;
OneWire oneWire(pinTemp);
DallasTemperature sensors(&oneWire);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void connectWifi() {
  WiFi.begin(STASSID, STAPSK);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to ");
  Serial.println(STASSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void mqttPublishConfig() {
  Serial.println("MQTT publish config");
  
  const size_t bufferSize = JSON_OBJECT_SIZE(8);
  DynamicJsonDocument root(1024);
  root["name"] = "uhu3";
  root["stat_t"] = MQTT_TOPIC_STATE;
  root["avty_t"] = "house/pH1/LWT";\
  //root["unique_id"] = "pH1";
  root["pl_avail"] = MQTT_TOPIC_LWT_ONLINE;
  root["pl_not_avail"] = "Offline";
  root["val_tpl"] = "{{value_json.POWER}}";
  root["pl_off"] = "OFF";
  root["pl_on"] = "ON";
  root["uniq_id"] = "pH1";
  JsonArray ids = root.createNestedArray();
  ids.add("pH1");
  root["dev"] = ids;
  char message[1024];
  serializeJson(root, message);
  Serial.println(message);
  
  if(!mqttClient.publish(MQTT_TOPIC_DISCOVERY, MQTT_FOR_HOMEASSISTANT)) {
    Serial.println("MQTT publish config failed");
  }
  
  Serial.println("MQTT publish lwt");
  mqttClient.publish(MQTT_TOPIC_LWT, MQTT_TOPIC_LWT_ONLINE);
  
  Serial.println("pH1 online");
}

void mqttPublishState(char* pH,char* temp) {
  //Serial.print("mqtt pH ");
  //Serial.println(pH);
  //Serial.print(" temp ");
  //Serial.println(temp);
  mqttClient.publish(MQTT_TOPIC_PH, pH);
  mqttClient.publish(MQTT_TOPIC_TEMP, temp);
}

void mqttReconnect() {
  Serial.println("MQTT connecting.");
  int i = 0;
  while (i< 10 && !mqttClient.connected()) {
    i++;
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("pH1")) {
      Serial.println("MQTT connected.");
      mqttClient.setBufferSize(1024);
      mqttPublishConfig();
      mqttClient.subscribe(MQTT_TOPIC_STATE);
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

void setup() 
{ 
  Serial.begin(115200);
  connectWifi();
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  //pinMode(pinPh, INPUT); 
} 

float ph (float voltage) {
  return 7 + ((2.48 - voltage) / 0.18);
}

char* stringToChar(String s) {
  char charBuf[50];
  s.toCharArray(charBuf, 50);
  return charBuf;
}
 
void loop() 
{ 
  if(!mqttClient.loop()) {
    Serial.println("MQTT Client not connected.");
    mqttReconnect();
  }
  int analogPH = analogRead(pinPh); 
  float vPh = analogPH * (3.3 / 4095.0); 
  float phValue = ph(vPh);
  sensors.requestTemperatures(); 
  float temperatureC = sensors.getTempCByIndex(0);  
  Serial.print("analog pH: "); 
  Serial.print(analogPH); 
  Serial.print(" V: "); 
  Serial.print(vPh); 
  Serial.print(" pH: "); 
  Serial.print(phValue);
  Serial.print(" temp: "); 
  Serial.print(temperatureC);
  Serial.println(""); 
  char char_ph[16];
  char char_t[16];
  mqttPublishState(dtostrf(phValue, 6, 2, char_ph),dtostrf(temperatureC, 6, 2, char_t));
  delay(500); 
}
