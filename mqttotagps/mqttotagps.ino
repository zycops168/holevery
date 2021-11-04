#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#define RXPin (16)
#define TXPin (17)
////////////////////////////////////////////////////////////////////////////////////////
TaskHandle_t Task1;
TaskHandle_t Task2;
const int buttonPin = 2;
int buttonState = 0;
////////////////////////////////////////////////////////////////////////////////////////
unsigned long last_time = 0;
unsigned long period = 5000;
////////////////////////////////////////////////////////////////////////////////////////
const char* mqttServer = "203.150.107.212";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";
WiFiClient espClient;
PubSubClient client(espClient);
////////////////////////////////////////////////////////////////////////////////////////
//gps
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
HardwareSerial ss(2);
//led
int LED = 2;
int LEDWIFI = 19;
int LEDMQTT = 21;
////////////////////////////////////////////////////////////////////////////////////////
void displayInfo()
{
  if ( millis() - last_time > period) {
    last_time = millis();
    Serial.print(F("Location: "));
    if (gps.location.isValid())
    {
      Serial.print(gps.location.lat(), 6);
      Serial.print(F(","));
      Serial.print(gps.location.lng(), 6);
    }
    else
    {
      Serial.print(F("INVALID"));
    }
    Serial.println();
  }
}
////////////////////////////////////////////////////////////////////////////////////////
void pub_mqtt() {
  if ( millis() - last_time > period) {
    last_time = millis();
    StaticJsonBuffer<300> JSONbuffer;
    JsonObject& JSONencoder = JSONbuffer.createObject();

    JSONencoder["car_id"] = "26";
    JSONencoder["latitude"] = gps.location.lat(), 6;
    JSONencoder["longitude"] = gps.location.lng(), 6;


    char JSONmessageBuffer[100];
    JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    Serial.println("Sending message to MQTT topic..");
    Serial.println(JSONmessageBuffer);
    if (client.publish("excavator/location", JSONmessageBuffer) == true) {
      Serial.println("Success sending message");
    } else {
      Serial.println("Error sending message");
    }
  }
}
////////////////////////////////////////////////////////////////////////////////////////
void mqtt() {
  // mqtt
  client.setServer(mqttServer, mqttPort);
  if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
    Serial.println("Mqtt_connected");

  } else {
    Serial.print("failed with state ");
  }
}
////////////////////////////////////////////////////////////////////////////////////////
void ota() {
  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else          // แบบ U_SPIFFS
      type = "filesystem";
    Serial.println("Start updating " + type);

  })
  .onEnd([]() {
    Serial.println("\nEnd");
    ESP.restart();
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.println(WiFi.localIP());
}
////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  ss.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin, false);
  pinMode(LED, OUTPUT);
  pinMode(LEDWIFI, OUTPUT);
  pinMode(LEDMQTT, OUTPUT);
  pinMode(buttonPin, INPUT);
  //task 1
  xTaskCreatePinnedToCore(
    TaskA,   /* Task function. */
    "Task1",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task1,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
  delay(2000);
  //task 2
  xTaskCreatePinnedToCore(
    TaskB,   /* Task function. */
    "Task2",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task2,      /* Task handle to keep track of created task */
    1);          /* pin task to core 1 */
  delay(2000);
}
////////////////////////////////////////////////////////////////////////////////////////
void TaskA( void * pvParameters ) {
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  buttonState = digitalRead(buttonPin);
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  bool res;
  res = wm.autoConnect();
  if (!res) {
    Serial.println("Failed to connect");
    digitalWrite(LEDWIFI, LOW);
    ESP.restart();
  }
  else {
    Serial.println("connected...yeey :)");
    digitalWrite(LEDWIFI, HIGH);
  }
  ota();
  mqtt();
  while (true) {
    ArduinoOTA.handle();
    buttonState = digitalRead(buttonPin);
    if (buttonState == HIGH) {
      digitalWrite(LEDWIFI, LOW),
      wm.resetSettings();
    } else {
    }
    if (client.connect("ESP32Client", mqttUser, mqttPassword )) {
      //Led
      digitalWrite(LEDMQTT, HIGH);
      pub_mqtt();
    } else {
      //led
      digitalWrite(LEDMQTT, LOW);
    }
  }
}
////////////////////////////////////////////////////////////////////////////////////////
void TaskB( void * pvParameters ) {
  Serial.print("Task2 running on core ");
  Serial.println(xPortGetCoreID());

  while (true) {
    if (gps.encode(ss.read()))
      displayInfo();
  }
}
void loop() {


}
