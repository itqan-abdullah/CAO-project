#include <ESPAsyncWebServer.h>
#include <WiFI.h>
#include <ESPmDNS.h>
#include <AsyncJson.h>
#include <SPIFFS.h>
#include "esp_wpa2.h" //wpa2 library for connections to Enterprise networks

#define EAP_IDENTITY "itqan.bscs20seecs" //if connecting from another corporation, use identity@organisation.domain in Eduroam 
#define EAP_PASSWORD "Student.123" //your Eduroam password
const char* ssid = "eduroam"; // Eduroam SSID
int counter = 0;

#define BUILTIN_LED 2

AsyncWebServer server(80);

void setup()
{
  Serial.begin(115200);
   delay(10);
  Serial.println();
  Serial.print("Connecting to network: ");
  Serial.println(ssid);
  WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
  WiFi.mode(WIFI_STA); //init wifi mode
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY)); //provide identity
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY)); //provide username --> identity and username is same
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD)); //provide password
  esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT(); //set config settings to default
  esp_wifi_sta_wpa2_ent_enable(&config); //set config settings to enable function
  WiFi.begin(ssid); //connect to wifi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    counter++;
    if(counter>=60){ //after 30 seconds timeout - reset board
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address set: "); 
  Serial.println(WiFi.localIP()); //print LAN IP


  SPIFFS.begin();

  MDNS.begin("demo-server");

  pinMode(BUILTIN_LED, OUTPUT);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, PUT");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "*");

  server.addHandler(new AsyncCallbackJsonWebHandler("/led", [](AsyncWebServerRequest *request, JsonVariant &json) {
    const JsonObject &jsonObj = json.as<JsonObject>();
    if (jsonObj["on"])
    {
      Serial.println("Turn on LED");
      digitalWrite(BUILTIN_LED, HIGH);
    }
    else
    {
      Serial.println("Turn off LED");
      digitalWrite(BUILTIN_LED, LOW);
    }
    request->send(200, "OK");
  }));

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS)
    {
      request->send(200);
    }
    else
    {
      Serial.println("Not found");
      request->send(404, "Not found");
    }
  });

  server.begin();
}

void loop()
{
  // put your main code here, to run repeatedly:
}
