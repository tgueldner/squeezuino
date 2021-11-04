#include <SPI.h>
#include <MFRC522.h>

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <MqttClient.h>

#define LOG_PRINTFLN(fmt, ...)  logfln(fmt, ##__VA_ARGS__)
#define LOG_SIZE_MAX 128
void logfln(const char *fmt, ...) {
  char buf[LOG_SIZE_MAX];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, LOG_SIZE_MAX, fmt, ap);
  va_end(ap);
  Serial.println(buf);
}

const int ledPin = LED_BUILTIN;
// https://github.com/nodemcu/nodemcu-devkit-v1.0/issues/16
#define turnOn 0
#define turnOff 1

// https://www.smarthome-tricks.de/esp8266/rfid-reader-rc522/
// define ports for RFID 
#define SS_PIN D8
#define RST_PIN D0

byte regVal = 0x7F;
void activateRec(MFRC522 mfrc522);
void clearInt(MFRC522 mfrc522);
 
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;

// MQTT
#define MQTT_ID "squeezuino"
const char* MQTT_TOPIC_SUB = "radio/" MQTT_ID "/sub";
const char* MQTT_TOPIC_PUB = "radio/" MQTT_ID "/tag";

static MqttClient *mqtt = NULL;
static WiFiClient network;

// ============== Object to supply system functions ============================
class System: public MqttClient::System {
public:

  unsigned long millis() const {
    return ::millis();
  }

  void yield(void) {
    ::yield();
  }
};
 
void setup(void){
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("Boot RFID-Reader..."));

  delay(10);
  SPI.begin();
  rfid.PCD_Init();
  delay(4); 
  rfid.PCD_DumpVersionToSerial();

  Serial.println();
  Serial.print(F("Connecting to "));
  WiFi.mode(WIFI_STA);
  WiFi.begin("ssid", "pass");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());

  // Setup MqttClient
  MqttClient::System *mqttSystem = new System;
  MqttClient::Logger *mqttLogger = new MqttClient::LoggerImpl<HardwareSerial>(Serial);
  MqttClient::Network * mqttNetwork = new MqttClient::NetworkClientImpl<WiFiClient>(network, *mqttSystem);
  //// Make 128 bytes send buffer
  MqttClient::Buffer *mqttSendBuffer = new MqttClient::ArrayBuffer<128>();
  //// Make 128 bytes receive buffer
  MqttClient::Buffer *mqttRecvBuffer = new MqttClient::ArrayBuffer<128>();
  //// Allow up to 2 subscriptions simultaneously
  MqttClient::MessageHandlers *mqttMessageHandlers = new MqttClient::MessageHandlersImpl<2>();
  //// Configure client options
  MqttClient::Options mqttOptions;
  ////// Set command timeout to 10 seconds
  mqttOptions.commandTimeoutMs = 10000;
  //// Make client object
  mqtt = new MqttClient(
    mqttOptions, *mqttLogger, *mqttSystem, *mqttNetwork, *mqttSendBuffer,
    *mqttRecvBuffer, *mqttMessageHandlers
  );

  pinMode(ledPin, OUTPUT);

  Serial.println(F("End setup"));
}
 
void loop(void){
  // Check connection status
  if (!mqtt->isConnected()) {
    digitalWrite(ledPin, turnOn);
    // Close connection if exists
    network.stop();
    // Re-establish TCP connection with MQTT broker
    Serial.print("Connecting to MQTT...");
    network.connect("192.168.1.1", 1883);
    if (!network.connected()) {
      LOG_PRINTFLN("Can't establish the TCP connection");
      delay(5000);
      ESP.reset();
    } else {
      Serial.println("OK");
      digitalWrite(ledPin, turnOff);
    }
    // Start new MQTT connection
    MqttClient::ConnectResult connectResult;
    // Connect
    {
      MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
      options.MQTTVersion = 4;
      options.clientID.cstring = (char*)MQTT_ID;
      options.cleansession = true;
      options.keepAliveInterval = 15; // 15 seconds
      MqttClient::Error::type rc = mqtt->connect(options, connectResult);
      if (rc != MqttClient::Error::SUCCESS) {
        LOG_PRINTFLN("Connection error: %i", rc);
        return;
      }
    }
    {
      // Add subscribe here if required
    }
  } else {
    {
      // Publish uid
      handleRFID();
    }
    // Idle for some seconds
    mqtt->yield(2000L);

    //digitalWrite(ledPin, turnOn);
    //delay(100);
    //digitalWrite(ledPin, turnOff);
  }
}
 
void handleRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  Serial.println(F("Card Detected!"));

  //rfid.PICC_DumpToSerial(&(rfid.uid));
  
  Serial.println(printHex(rfid.uid.uidByte, rfid.uid.size));
  char buf[rfid.uid.size+1];
  memset(buf, 0, sizeof(buf));
  for (int cnt = 0; cnt < rfid.uid.size; cnt++) {
    // convert byte to its ascii representation
    sprintf(&buf[cnt * 2], "%02X", rfid.uid.uidByte[cnt]);
  }
  //const char* buf = myNewArray;
  MqttClient::Message message;
  message.qos = MqttClient::QOS0;
  message.retained = false;
  message.dup = false;
  message.payload = (void*) buf;
  message.payloadLen = strlen(buf);
  mqtt->publish(MQTT_TOPIC_PUB, message);
   
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  digitalWrite(ledPin, turnOn);
  delay(1000);
  digitalWrite(ledPin, turnOff);
}
 
String printHex(byte *buffer, byte bufferSize) {
  String id = "";
  for (byte i = 0; i < bufferSize; i++) {
    id += buffer[i] < 0x10 ? "0" : "";
    id += String(buffer[i], HEX);
  }
  return id;
}
