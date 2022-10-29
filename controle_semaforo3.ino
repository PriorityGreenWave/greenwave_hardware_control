#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.
const char* ssid = "3bits_203-2.4Ghz";
const char* password = "Apto!203@)#";
//const char* ssid = "Network_Mustard";
//const char* password = "458@33ns!";
const char* mqtt_server = "mqtt.tago.io";
#define mqtt_port 1883
#define MQTT_USER "esp8266"
#define MQTT_PASSWORD "e8e008b2-edba-4954-bd58-6111f26c8d41"
#define MQTT_SERIAL_PUBLISH_CH "Area_1/semaforo_3/light_color"
#define MQTT_SERIAL_RECEIVER_CH "Area_1/semaforos/PGW"

//D5, D6, D7
int red_light=14, yellow_light=12, green_light=13; //pin definition for the traffic light

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
      client.publish("Area_1/semaforo_3/light_color", "initializing...");
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
  delay(1000);
  digitalWrite(red_light,LOW);
  digitalWrite(yellow_light,LOW);
  digitalWrite(green_light,HIGH);
  
  Serial.print("Priority Green Wave is starting!");
  Serial.println();
  publishSerialData("PGW!");
  delay(10000);
  
  Serial.print("End of Priority Green Wave");
  Serial.println();
}

void emergency() {
  Serial.print("Emergency");
  Serial.println(); 
  publishSerialData("Area_1 Emergency");
  
  digitalWrite(red_light,LOW);
  digitalWrite(yellow_light,HIGH);
  digitalWrite(green_light,LOW);
  delay(1000);

  digitalWrite(red_light,HIGH);
  digitalWrite(yellow_light,LOW);
  digitalWrite(green_light,LOW);
  delay(10000);
  
  Serial.print("End of Emergency");
  Serial.println(); 
}

void normalTrafficLigth() {
  digitalWrite(red_light,LOW);
  digitalWrite(yellow_light,HIGH);
  digitalWrite(green_light,LOW);
  Serial.print("Yellow");
  Serial.println(); 
  publishSerialData("Yellow");
  delay(1000);
  
  digitalWrite(red_light,HIGH);
  digitalWrite(yellow_light,LOW);
  digitalWrite(green_light,LOW);
  Serial.print("Red");
  Serial.println(); 
  publishSerialData("Red");
  delay(3000);
  
  digitalWrite(red_light,LOW);
  digitalWrite(yellow_light,LOW);
  digitalWrite(green_light,HIGH);
  Serial.print("Green");
  Serial.println(); 
  publishSerialData("Green");
  delay(7000);
}

void callback(char* topic, byte *payload, unsigned int length) {
  Serial.println();
  Serial.println("-------new message from broker-----");
  Serial.print("channel:");
  Serial.println(topic);
  Serial.print("data:");  
  Serial.write(payload, length);
  Serial.println("!!!");
  if((char)payload[0] == '1' or (char)payload[0] == '2' or (char)payload[0] == '3'){
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
