#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
bool fan_enable = false;
int fan_speed = 0;
bool temp_enable = false;
float temp_measure = 0;
bool timer_enable = false;
String timer_start = "00:00";
String timer_end = "00:00";
String system_time = "00:00";

String dataSend = "";
String dataRead = "";

bool isSending = true;
bool isReading = false;

// key for json data
#define KEY_DATA_FAN_ENABLE "fan_enable"
#define KEY_DATA_TEMP_ENABLE "temp_enable"

#define RX D5
#define TX D6
SoftwareSerial unoEspSerial = SoftwareSerial(RX, TX);

#define FIREBASE_HOST "blog-nodv-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "LmIMgwMtJxBa6cUlhQGF14HnfC38F7rgpUGtcLb0"
#define WIFI_SSID "Mien Phi_2.4G"
#define WIFI_PASSWORD "nhapvodi"
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

int last = 0;



void setup() {
  Serial.begin(9600);
  unoEspSerial.begin(9600);
  //connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  //connect filebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  timeClient.begin();
  timeClient.setTimeOffset(25200);  // pack time vn
}

void loop() {
  // handle error
  if (handleFirebaseError()) return;
  if (millis() - last >= 2000) {
    last = millis();
    sendDataToUno();
  }
  readDataFromUno();
  // setSystemTime();
  // controlByTime();
  // sendDataToUno();
  // readDataFromUno();
  // sendDataToFirebase();
}


bool handleFirebaseError() {
  if (Firebase.failed()) {
    Serial.print("setting /number failed:");
    Serial.println(Firebase.error());
    return true;
  }
  return false;
}

void readDataFromUno() {
  while (unoEspSerial.available()) {
    Serial.println("had data");
    char readChar = (char)unoEspSerial.read();
    if (readChar != '\n') {
      dataRead += readChar;
    } else {
      handleJsonData(dataRead);
      sendDataToFirebase();
      dataRead = "";
    }
  }
}

void sendDataToUno() {
  readDataFromFirebase();
  delay(500);
  setSystemTime();
  controlByTime();
  unoEspSerial.print(jsonWrite() + '\n');
  unoEspSerial.flush();  // wait end
}



void readDataFromFirebase() {
  fan_enable = Firebase.getBool("/fan/enable");
  fan_speed = Firebase.getInt("/fan/speed");
  temp_enable = Firebase.getBool("/temp/enable");
  timer_enable = Firebase.getBool("/timer/enable");
  timer_start = Firebase.getString("/timer/start");
  timer_end = Firebase.getString("/timer/end");
}

void sendDataToFirebase() {
  Firebase.setBool("/fan/enable", fan_enable);
  Firebase.setInt("/fan/speed", fan_speed);
  Firebase.setBool("/temp/enable", temp_enable);
  Firebase.setBool("/timer/enable", timer_enable);
}

void setSystemTime() {
  timeClient.update();
  system_time = String(timeClient.getHours()) + ":" + String(timeClient.getMinutes());
}

void controlByTime() {
  if (timer_enable) {
    if (system_time == timer_start) {
      fan_enable = true;
      Firebase.setBool("/fan/enable", fan_enable);

    } else if (system_time == timer_end) {
      fan_enable = false;
      Firebase.setBool("/fan/enable", fan_enable);
    }
  }
}

String jsonWrite() {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root[KEY_DATA_FAN_ENABLE] = fan_enable;
  root[KEY_DATA_TEMP_ENABLE] = temp_enable;
  String result;
  root.printTo(result);
  return result;
};

void handleJsonData(String data) {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(data);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  if (root.containsKey(KEY_DATA_FAN_ENABLE)) {
    String data = root[KEY_DATA_FAN_ENABLE];
    fan_enable = stringToBool(data);
    Serial.print(fan_enable);
  }
};

bool stringToBool(String value) {
  if (value == "true") return true;
  return false;
}
