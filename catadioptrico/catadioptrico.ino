#include <MFRC522.h> //biblioteca responsável pela comunicação com o módulo RFID-RC522
#include <MFRC522Extended.h>
#include <SPI.h> //biblioteca para comunicação do barramento SPI
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>

// Update these with values suitable for your network.
const char* ssid = "3bits_203-2.4Ghz";
const char* password = "Apto!203@)#";
//const char* ssid = "Network_Mustard";
//const char* password = "458@33ns!";
const char* mqtt_server = "mqtt.tago.io";
#define mqtt_port 1883
#define MQTT_USER "esp32"
#define MQTT_PASSWORD "e8e008b2-edba-4954-bd58-6111f26c8d41"
#define MQTT_SERIAL_PUBLISH_PGW "Area_1/semaforos/PGW"
#define MQTT_SERIAL_PUBLISH_LTR "Area_1/catadioptrico_1/lastTagRead"
#define MQTT_SERIAL_RECEIVER_CH "Area_1/tagEmEmergencia"
#define SS_PIN    21
#define RST_PIN   22

//esse objeto 'chave' é utilizado para autenticação
MFRC522::MIFARE_Key key;
//código de status de retorno da autenticação
MFRC522::StatusCode status;

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

char* lastTagRead = "";
String emergencyTag = "";

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
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),MQTT_USER,MQTT_PASSWORD)) {
      Serial.println("connected");
      //Once connected, publish an announcement...
      client.publish("Area_1/catadioptrico_1/lastTagRead", "00 00 00 00");
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


void callback(char* topic, byte *payload, unsigned int length) {
    char buff[length+1];
    for(int i = 0; i < length; i++){
      buff[i] = payload[i];
    }
    buff[length] = '\0';
    emergencyTag = String((char*)buff);

    Serial.println("O veículo dessa TAG está em uma emergência:");
    Serial.println(emergencyTag);
    
    Serial.println();
    if (strncmp((char *) payload, lastTagRead, length) == 0) {
      publishPriorityGreenWave("1");
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
  
  // Inicia MFRC522
  SPI.begin();                                                  // Init SPI bus
  mfrc522.PCD_Init();                                           // Init MFRC522 card
  Serial.println(F("Read personal data on a MIFARE PICC:"));    //shows in serial that it is ready to read
}


void publishPriorityGreenWave(char *serialData){
  if (!client.connected()) {
    reconnect();
  }
  client.publish(MQTT_SERIAL_PUBLISH_PGW, serialData);
}

void publishLTR(char *serialData){
  if (!client.connected()) {
    reconnect();
  }
  client.publish(MQTT_SERIAL_PUBLISH_LTR, serialData);
}

void loop() {
  
  client.loop();
  lastTagRead = "";
  
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  //some variables we need
  byte block;
  byte len;
  MFRC522::StatusCode status;

  //-------------------------------------------

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  
  Serial.println();
  Serial.println(F("**Card Detected:**"));

  //-------------------------------------------

  //mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); //dump some details about the card

  //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));      //uncomment this to see all blocks in hex

  String tag = "";
  
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    tag.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    tag.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  tag.toUpperCase();
  
  Serial.println("Id do cartão");
  tag = tag.substring(1);
  Serial.println(tag);

  if(tag == emergencyTag){
    Serial.println("Priority Green Wave!");
    publishPriorityGreenWave("1");
  }
  
  int tagLength = tag.length(); //obtem o tamanho do Id da tag lida
  char lastTagRead[tagLength+1]; // variavel temporaria do tipo char para conter a ulitma tag lida
  tag.toCharArray(lastTagRead, tagLength+1);

  publishLTR(lastTagRead); //publica a tag sem o espaco em branco no inicio

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
