#include <MFRC522.h> //biblioteca responsável pela comunicação com o módulo RFID-RC522
#include <MFRC522Extended.h>
#include <SPI.h> //biblioteca para comunicação do barramento SPI
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>

// Update these with values suitable for your network.
const char* ssid = "3bits_203-2.4Ghz";
const char* password = "Apto!203@)#";
//const char* ssid = "Network_Mustard";
//const char* password = "458@33ns!";
const char* mqtt_server = "mqtt.tago.io";

const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIEODCCAyCgAwIBAgIQOl/rsaTgFhSdiHmbGtv3zzANBgkqhkiG9w0BAQsFADBI\n" \
"MRswGQYDVQQDDBJFU0VUIFNTTCBGaWx0ZXIgQ0ExHDAaBgNVBAoME0VTRVQsIHNw\n" \
"b2wuIHMgci4gby4xCzAJBgNVBAYTAlNLMB4XDTIyMDMxNDE4Mzk1NVoXDTIzMDMw\n" \
"OTE4Mzk1NVowajELMAkGA1UEBhMCVVMxCzAJBgNVBAgTAldBMRAwDgYDVQQHEwdS\n" \
"ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xHDAaBgNVBAMM\n" \
"EyouYXp1cmV3ZWJzaXRlcy5uZXQwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEK\n" \
"AoIBAQCWBfNR9xJ+SzUD+xa1hXIa7iqt72vwahUBfb47n645T+L5mClNOrY7auMw\n" \
"KfkTni2nPyphilQdGskUTY0S5z72sEmBjo9Ar4UQ47ULf11BoRmU+8Diz0zw2x/i\n" \
"DdTAoDI3QTC2rW/t2vOORwLPG4aBvcCwxMJ47Vve8HmwP8DyFy5OBvrmqC2RFyQi\n" \
"UPt6cN4i7Wa84CSHcuV9FrpyvHY3Ic/+ZfIfkWIoG67nugrEP/ACsnDLks1Wzupf\n" \
"h2448AU7T2ZNJJrymOEC4T7aorVfDOMpsFu8D5irRV0VtpM8pw3ybIQEHcXkj6Pa\n" \
"nUl1MO/z6NUNgoQwsWDSUmcHd9bvAgMBAAGjgfswgfgwCwYDVR0PBAQDAgWgMB0G\n" \
"A1UdDgQWBBRpRVLk5gIOwW1YTJVsX2xc/cOekjAfBgNVHSMEGDAWgBSBo/xRTxp4\n" \
"/lXUTU0kZgJc92EXZzB8BgNVHREEdTBzghMqLmF6dXJld2Vic2l0ZXMubmV0ghcq\n" \
"LnNjbS5henVyZXdlYnNpdGVzLm5ldIISKi5henVyZS1tb2JpbGUubmV0ghYqLnNj\n" \
"bS5henVyZS1tb2JpbGUubmV0ghcqLnNzby5henVyZXdlYnNpdGVzLm5ldDAMBgNV\n" \
"HRMBAf8EAjAAMB0GA1UdJQQWMBQGCCsGAQUFBwMCBggrBgEFBQcDATANBgkqhkiG\n" \
"9w0BAQsFAAOCAQEAo46rwB7edJUcQgZlHNwuj2y4Lb3bvfvvtAwtU7iR0flMxpED\n" \
"vYwSGvNqd4suRgl3m6qpoCo1k38GtF7YOGxVKtO0k4VRMXt9vG8Yrb0L6KeGz+pf\n" \
"EnVOWaz3ILCatD0pXa/0xnN/RMAqd4Ra/iF9BEH/Ah3MgvqKqHN6K0JjzmoVJwV+\n" \
"polnEQI2stPH8t8hLoPmf7/RJ1gidWefCHyGJROz9Hb4iIqBQthswenzxD/0ySbV\n" \
"ePtYVtTiP0jlTNQxqXTLLt4CLD1qQEqqzc3qwX790teopyRCfpNB309L8zpb87q0\n" \
"5+9QKoA8WaR+xhnuP0CvNey2Vqn86vsKTK3q2g==\n" \
"-----END CERTIFICATE-----\n";

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
String urlEmergency = "https://greeenwaveapi.azurewebsites.net/api/Veiculo/EstadoEmergencia?Rfid=";

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

  tag.replace(" ", "%20");
  Serial.println(tag);

  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;
    String urlEmergencyTag = urlEmergency+tag;
    Serial.println("Requisicao");
    Serial.println(urlEmergencyTag);
    http.begin(urlEmergencyTag); //URL de requisição para api
    http.addHeader("Connection", "keep-alive");
    http.addHeader("Accept", "*/*");
    int httpCode = http.GET();

    if (httpCode > 0){
      String payload = http.getString();
      Serial.println(httpCode);
      Serial.println(payload);
      if (payload == "true"){
        publishPriorityGreenWave("1");
      }
    }
    else {
      Serial.println("Error on HTTP rquest");
      Serial.println(httpCode);
    }
    http.end();
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
