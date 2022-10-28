#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.
const char* ssid = "";
const char* password = "";
//const char* ssid = "";
//const char* password = "";
const char* mqtt_server = "mqtt.tago.io";
#define mqtt_port 1883
#define MQTT_USER "esp8266"
#define MQTT_PASSWORD ""
#define MQTT_SERIAL_PUBLISH_CH "Area_1/semaforo_2/light_color"
#define MQTT_SERIAL_RECEIVER_CH "Area_1/semaforos/PGW"

//D0, D1, D2
int red_light=16, yellow_light=5, green_light=4; //pin definition for the traffic light

WiFiClient wifiClient;   

PubSubClient client(wifiClient);

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),MQTT_USER,MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      client.publish("Area_1/semaforo_2/light_color", "initializing...");
      // ... and resubscribe
      client.subscribe(MQTT_SERIAL_RECEIVER_CH);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void priorityGreenWave() {
  delay(1500);
  digitalWrite(red_light,LOW);
  digitalWrite(yellow_light,LOW);
  digitalWrite(green_light,HIGH);
  
  Serial.print("Priority Green Wave is starting!");
  Serial.println();
  publishSerialData("PGW!");
  delay(40000);
  
  Serial.print("End of Priority Green Wave");
  Serial.println();
}

void emergency() {
  digitalWrite(red_light,LOW);
  digitalWrite(yellow_light,HIGH);
  digitalWrite(green_light,LOW);
  delay(1000);

  digitalWrite(red_light,LOW);
  digitalWrite(yellow_light,LOW);
  digitalWrite(green_light,HIGH);
  delay(40000);
  
  Serial.print("Emergency");
  Serial.println(); 
  publishSerialData("Area_1 Emergency");
  delay(40000);
  
  Serial.print("End of Emergency");
  Serial.println(); 
}

void normalTrafficLigth() {
  digitalWrite(red_light,LOW);
  digitalWrite(yellow_light,HIGH);
  digitalWrite(green_light,LOW);
  Serial.print("Yellow");
  Serial.println(); 
  //publishSerialData("Yellow");
  delay(1000);
  
  digitalWrite(red_light,HIGH);
  digitalWrite(yellow_light,LOW);
  digitalWrite(green_light,LOW);
  Serial.print("Red");
  Serial.println(); 
  //publishSerialData("Red");
  delay(2000);
  
  digitalWrite(red_light,LOW);
  digitalWrite(yellow_light,LOW);
  digitalWrite(green_light,HIGH);
  Serial.print("Green");
  Serial.println(); 
  //publishSerialData("Green");
  delay(5000);
}

void callback(char* topic, byte *payload, unsigned int length) {
  Serial.println();
  Serial.println("-------new message from broker-----");
  Serial.print("channel:");
  Serial.println(topic);
  Serial.print("data:");  
  Serial.write(payload, length);
  Serial.println("!!!");
  if((char)payload[0] == '2'){
    Serial.println("PGW!");
    priorityGreenWave();
  }
  else{
    emergency();
  }
}


//---------------SETUP---------------
void setup() {
  Serial.begin(9600);
  Serial.setTimeout(500);// Set time out for 
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();
  
  //Set Pins as outputs
  pinMode(red_light,OUTPUT);
  pinMode(yellow_light,OUTPUT);
  pinMode(green_light,OUTPUT);

  //Start the program with all light off
  digitalWrite(red_light,LOW);
  digitalWrite(yellow_light,LOW);
  digitalWrite(green_light,LOW);
}

void publishSerialData(char *serialData){
  if (!client.connected()) {
    reconnect();
  }
  client.publish(MQTT_SERIAL_PUBLISH_CH, serialData);
}

void loop() {
   client.loop();
   if (!client.connected()) {
    reconnect();
   }
   normalTrafficLigth();
   client.loop();
}
