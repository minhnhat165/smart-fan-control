#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

bool fan_enable;
int fan_speed;
bool temp_enable;
float temp_measure;
float temp_threshold;
bool timer_enable;
String timer_start;
String timer_end;
String system_time;

String dataRead = "";

bool firstSendDataToUno = false;

// key for json data
#define KEY_DATA_FAN_ENABLE "fanEnable"
#define KEY_DATA_TEMP_ENABLE "tempEnable"
#define KEY_DATA_TEMP_MEASURE "tempMeasure"
#define KEY_DATA_FAN_SPEED "fanSpeed"
#define KEY_DATA_TEMP_THRESHOLD "tempTh"

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
  if (millis() - last >= 1000) {
    last = millis();
    sendDataToUno();
  }
  readDataFromUno();
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
    char readChar = (char)unoEspSerial.read();
    if (readChar != '\n') {
      dataRead += readChar;
    } else {
      handleJsonData(dataRead);
      dataRead = "";
    }
  }
}

void sendDataToUno() {
  String dataSend = hasChangeFirebaseJson();
  controlByTime();
  if (dataSend != "") {
    Serial.println("send " + dataSend);
    unoEspSerial.print(dataSend + '\n');
    unoEspSerial.flush();  // wait end
  }
}



String hasChangeFirebaseJson() {
  bool new_fan_enable = Firebase.getBool("/fan/enable");
  int new_fan_speed = Firebase.getInt("/fan/speed");
  bool new_temp_enable = Firebase.getBool("/temp/enable");
  float new_temp_threshold = Firebase.getFloat("temp/threshold");
  timer_enable = Firebase.getBool("/timer/enable");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  bool isChange = false;

  if (!firstSendDataToUno) {
    fan_speed = new_fan_speed;
    root[KEY_DATA_FAN_SPEED] = fan_speed;
    temp_enable = new_temp_enable;
    root[KEY_DATA_TEMP_ENABLE] = temp_enable;
    temp_threshold = new_temp_threshold;
    root[KEY_DATA_TEMP_THRESHOLD] = temp_threshold;
    fan_enable = new_fan_enable;
    root[KEY_DATA_FAN_ENABLE] = fan_enable;
    isChange = true;
    firstSendDataToUno = true;
  } else {
    if (new_fan_enable != fan_enable) {
      fan_enable = new_fan_enable;
      root[KEY_DATA_FAN_ENABLE] = fan_enable;
      isChange = true;
    }
    if (new_fan_speed != fan_speed) {
      fan_speed = new_fan_speed;
      root[KEY_DATA_FAN_SPEED] = fan_speed;
      isChange = true;
    }
    if (new_temp_enable != temp_enable) {
      temp_enable = new_temp_enable;
      root[KEY_DATA_TEMP_ENABLE] = temp_enable;
      isChange = true;
    }

    if (new_temp_threshold != temp_threshold) {
      temp_threshold = new_temp_threshold;
      root[KEY_DATA_TEMP_THRESHOLD] = temp_threshold;
      isChange = true;
    }
  }

  if (!isChange) return "";
  String result;
  root.printTo(result);
  return result;
}


void setSystemTime() {
  timeClient.update();
  system_time = String(timeClient.getHours()) + ":" + String(timeClient.getMinutes());
}

void controlByTime() {
  if (timer_enable) {
    setSystemTime();
    timer_start = Firebase.getString("/timer/start");
    timer_end = Firebase.getString("/timer/end");
    if (system_time == timer_start) {
      Firebase.setBool("/fan/enable", true);

    } else if (system_time == timer_end) {
      Firebase.setBool("/fan/enable", false);
    }
    delay(500);
  }
}


void handleJsonData(String data) {
  Serial.println("data handle" + data);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(data);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  if (root.containsKey(KEY_DATA_FAN_ENABLE)) {
    String data = root[KEY_DATA_FAN_ENABLE];
    Firebase.setBool("/fan/enable", stringToBool(data));
  }
  if (root.containsKey(KEY_DATA_FAN_SPEED)) {
    String data = root[KEY_DATA_FAN_SPEED];
    Firebase.setBool("/fan/speed", data.toInt());
  }
  if (root.containsKey(KEY_DATA_TEMP_ENABLE)) {
    String data = root[KEY_DATA_TEMP_ENABLE];
    Firebase.setBool("/temp/enable", stringToBool(data));
  }

  if (root.containsKey(KEY_DATA_TEMP_MEASURE)) {
    String data = root[KEY_DATA_TEMP_MEASURE];
    Firebase.setFloat("/temp/measure", data.toFloat());
  }
};


bool stringToBool(String value) {
  if (value == "true") return true;
  return false;
}
