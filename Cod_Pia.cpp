#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <BluetoothSerial.h>

String teamId;

const long connection_timeout = 15000;//time until connection failed
long startConnection = 0;//time until connection starts

//Bluetooth
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
 #error Bluetooth is not enabled!
#endif

String nume_esp="ESP Alex-Radu";

BluetoothSerial SerialBT;
// Define a variable for the connection status
bool connected = false;

// Callback function for connection events
void deviceConnected(esp_spp_cb_event_t event,esp_spp_cb_param_t *param){
  if(event == ESP_SPP_SRV_OPEN_EVT){
  Serial.println("Device Connected");
  connected = true;
  }
  if(event == ESP_SPP_CLOSE_EVT ){
  Serial.println("Device disconnected");
  connected = false;
  }
}

void setup(){
  Serial.begin(115200);//Pentru arduino, rata de transfer de date
  
  SerialBT.begin(nume_esp);//Initializeaza comunicarea Bluethooth

  SerialBT.register_callback(deviceConnected);
  Serial.println("");
  Serial.println("Dispozitiv gata de conectare!");
}

void get_networks(){
  StaticJsonDocument <1000> Wifidoc; ///Pentru memorie suficienta
  
  Serial.println("Start scan");
  
  int networksFound = WiFi.scanNetworks();
  
  Serial.println("Scan complete");
  
  for (int i = 0; i < networksFound; i++){
    Wifidoc["ssid"] = WiFi.SSID(i);
    Wifidoc["strength"] = WiFi.RSSI(i);
    Wifidoc["encryption"] = WiFi.encryptionType(i);
    Wifidoc["teamId"] = teamId;
    String output;
    serializeJson(Wifidoc,output);
    
    Serial.println(output);
    SerialBT.println(output);
    delay(20);
  }
}

void connect_wifi(String ssid,String password){
  //WIFI CONNECT
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  delay(100);
  
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("Connecting");
  startConnection = millis(); ///Salveaza timpoul actual
  
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(500);
    if (millis() - startConnection > connection_timeout){
        break;
    }
  }
  
  StaticJsonDocument <1000> jdoc;
  Serial.println("");
  
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Connection failed");
    jdoc["ssid"] = ssid;
    jdoc["connected"] = false;
    jdoc["teamId"] = teamId;
  }
  else{
    jdoc["ssid"] = ssid;
    jdoc["connected"] = true;
    jdoc["teamId"] = teamId;

    Serial.print("Connected to network: ");
    Serial.println(WiFi.SSID());
    Serial.print("Local IP address: ");
    Serial.println(WiFi.localIP());
    
  }
  
  String output;
  serializeJson(jdoc,output);
  Serial.println(output);
  SerialBT.println(output);
  delay(20);
}

void get_data(){
  HTTPClient http;
  String url = "http://proiectia.bogdanflorea.ro/api/south-park-episodes/episodes";
  
  http.begin(url);
  http.setConnectTimeout(30000); // connect timeout
  http.setTimeout(30000); // response timeout
  int httpResponseCode = http.GET();//requesting info from the url (aka informational code) 
  
  // Check response code
  if (httpResponseCode<=0){
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  else{
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    // Get response data
    String payload = http.getString();

    //Process payload
    StaticJsonDocument <3500> episod;
    DeserializationError error = deserializeJson(episod, payload);
    if(error){
      Serial.print("JSON deserialization failed: ");
      Serial.println(error.c_str());
    }
    else{
      JsonObject data = episod["current_episode"].as<JsonObject>();
      
      for (JsonObject data : episod.as<JsonArray>()){
        String id = data["id"].as<String>();
        String name = data["name"].as<String>();
        String thumbnail_url = data["thumbnail_url"].as<String>();
        StaticJsonDocument <1000> trimite_episod;
        trimite_episod["id"] = id;
        trimite_episod["name"] = name;
        trimite_episod["image"] = thumbnail_url;
        trimite_episod["teamId"] = teamId;
        
        String output;
        serializeJson(trimite_episod,output);
        Serial.println(output);
        SerialBT.println(output);
      }         
    }
    
  }
  http.end();
    
}

void get_details(String id){
  HTTPClient http;
  String url = "http://proiectia.bogdanflorea.ro/api/south-park-episodes/episode?id=" + id;
  http.begin(url);
  http.setConnectTimeout(30000); // connect timeout
  http.setTimeout(30000); // response timeout
  int httpResponseCode = http.GET();//requesting info from the url (aka informational code) 

  if (httpResponseCode <= 0){
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  else{
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    
    String payload = http.getString();
    StaticJsonDocument <1000> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, payload);
    
    if(error){
      Serial.print("JSON deserialization failed: ");
      Serial.println(error.c_str());
    }
    else{
      StaticJsonDocument <1000> trimite_episod;
      String description = "";
      String add = jsonDoc["season"];
      description += "season: " + add + '\n';
      String add1 = jsonDoc["episode"];
      description += "episode: " + add1 + '\n';
      String add2 = jsonDoc["air_date"];
      description += "air_date: " + add2 + '\n';
      String add3 = jsonDoc["wiki_url"];
      description += "wiki_url: " + add3 + '\n';
      
      trimite_episod["id"] = id;
      trimite_episod["name"] = jsonDoc["name"];
      trimite_episod["image"] = jsonDoc["thumbnail_url"];
      trimite_episod["description"] = description;
      trimite_episod["teamId"] = teamId;
      
      String output;
      serializeJson(trimite_episod,output);
      Serial.println(output);
      SerialBT.println(output);
    }
    
  }
  http.end();
}

void loop() {
  if(SerialBT.available()){
    String DATA = SerialBT.readString();
    String action;
    Serial.print ("Received:");
    Serial.println(DATA);

    StaticJsonDocument <800> doc; ///Pentru a preveni erori de memorie sau trunchieri de date
    const char * json = DATA.c_str();
    deserializeJson(doc, json);
    action = doc["action"].as<String>();
    Serial.println(action);

    if(action == "getNetworks"){
      teamId = doc["teamId"].as<String>();
      get_networks();
      
    }
    else if(action == "connect"){
      String ssid_1,pass;
      ssid_1 = doc["ssid"].as<String>();
      pass = doc["password"].as<String>();
      connect_wifi(ssid_1,pass);
    }
    else if(action == "getData"){
        get_data();
    }
    else if(action == "getDetails"){
      String idEpisod = doc["id"]; //id=id episod
      get_details(idEpisod);
    }
  }
}
