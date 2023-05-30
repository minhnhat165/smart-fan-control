#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>


// ANGLE
bool angle_auto = false;
int angle_current;
int angle_speed = 3;
// FAN
bool fan_enable;
bool fan_auto;

// TEMP
float temp_current;
bool temp_enable;
float temp_threshold;

// TIMER
String timer_start;
String timer_end;
String system_time;
bool isDuration = false;




String dataRead = "";
bool firstSendDataToUno = false;

// key for json data
//FAN
#define KEY_DATA_FAN_ENABLE "fe"

//ANGLE
#define KEY_DATA_ANGLE_AUTO "aa"
#define KEY_DATA_ANGLE_CURRENT "ac"
#define KEY_DATA_ANGLE_SPEED "as"

// TEMP
#define KEY_DATA_TEMP_ENABLE "te"
#define KEY_DATA_TEMP_CURRENT "tc"
#define KEY_DATA_TEMP_THRESHOLD "tt"



// FIREBASE DIR
// ANGLE
#define FIREBASE_ANGLE "/angle"
#define FIREBASE_ANGLE_AUTO FIREBASE_ANGLE + String("/auto")
#define FIREBASE_ANGLE_CURRENT FIREBASE_ANGLE + String("/current")
#define FIREBASE_ANGLE_SPEED FIREBASE_ANGLE + String("/speed")
// FAN
#define FIREBASE_FAN "/fan"
#define FIREBASE_FAN_AUTO FIREBASE_FAN + String("/auto")
#define FIREBASE_FAN_ENABLE FIREBASE_FAN + String("/enable")

// TEMP
#define FIREBASE_TEMP "/temp"
#define FIREBASE_TEMP_CURRENT FIREBASE_TEMP + String("/current")
#define FIREBASE_TEMP_ENABLE FIREBASE_TEMP + String("/auto")
#define FIREBASE_TEMP_THRESHOLD FIREBASE_TEMP + String("/threshold")

// timer
#define FIREBASE_TIMER "/timer"
#define FIREBASE_TIMER_START FIREBASE_TIMER + String("/start")
#define FIREBASE_TIMER_END FIREBASE_TIMER + String("/end")
#define FIREBASE_TIMER_IS_DURATION FIREBASE_TIMER + String("/isDuration")

#define RX D5
#define TX D6
SoftwareSerial unoEspSerial = SoftwareSerial(RX, TX);

#define FIREBASE_HOST "blog-nodv-default-rtdb.asia-southeast1.firebasedatabase.app"
#define FIREBASE_AUTH "LmIMgwMtJxBa6cUlhQGF14HnfC38F7rgpUGtcLb0"
#define WIFI_SSID "Mien Phi_2.4G"
#define WIFI_PASSWORD "nhapvodi"
// #define WIFI_SSID "PTIT.HCM_CanBo"
// #define WIFI_PASSWORD ""
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
  Serial.print("success");
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
  // FAN
  bool new_fan_enable = Firebase.getBool(FIREBASE_FAN_ENABLE);
  fan_auto = Firebase.getBool(FIREBASE_FAN_AUTO);
  // ANGLE
  bool new_angle_auto = Firebase.getBool(FIREBASE_ANGLE_AUTO);
  int new_angle_current = Firebase.getInt(FIREBASE_ANGLE_CURRENT);
  int new_angle_speed = Firebase.getInt(FIREBASE_ANGLE_SPEED);
  //TEMP
  bool new_temp_enable = Firebase.getBool(FIREBASE_TEMP_ENABLE);
  float new_temp_threshold = Firebase.getFloat(FIREBASE_TEMP_THRESHOLD);

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  bool isChange = false;

  if (!firstSendDataToUno) {

    // FAN
    fan_enable = new_fan_enable;
    root[KEY_DATA_FAN_ENABLE] = fan_enable;
    // ANGLE
    angle_auto = new_angle_auto;
    root[KEY_DATA_ANGLE_AUTO] = angle_auto;
    // angle_current = new_angle_current;
    // root[KEY_DATA_ANGLE_CURRENT] = angle_auto;
    // angle_speed = new_angle_speed;
    // root[KEY_DATA_ANGLE_SPEED] = angle_speed;

    // TEMP
    temp_enable = new_temp_enable;
    root[KEY_DATA_TEMP_ENABLE] = temp_enable;
    temp_threshold = new_temp_threshold;
    root[KEY_DATA_TEMP_THRESHOLD] = temp_threshold;
    fan_enable = new_fan_enable;

    isChange = true;
    firstSendDataToUno = true;
  } else {
    //FAN
    if (new_fan_enable != fan_enable) {
      fan_enable = new_fan_enable;
      root[KEY_DATA_FAN_ENABLE] = fan_enable;
      isChange = true;
    }
    //TEMP
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
    //ANGLE
    if (new_angle_auto != angle_auto) {
      angle_auto = new_angle_auto;
      root[KEY_DATA_ANGLE_AUTO] = angle_auto;
      isChange = true;
    }

    if (new_angle_current != angle_current) {
      angle_current = new_angle_current;
      root[KEY_DATA_ANGLE_CURRENT] = angle_current;
      isChange = true;
    }

    if (new_angle_speed != angle_speed) {
      angle_speed = new_angle_speed;
      root[KEY_DATA_ANGLE_SPEED] = angle_speed;
      isChange = true;
    }
  }

  if (!isChange) return "";
  String result;
  root.printTo(result);
  return result;
}

void handleJsonData(String data) {
  Serial.println("data handle" + data);
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(data);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }

  //FAN

  if (root.containsKey(KEY_DATA_FAN_ENABLE)) {
    String data = root[KEY_DATA_FAN_ENABLE];
    Firebase.setBool(FIREBASE_FAN_ENABLE, stringToBool(data));
  }

  // ANGLE

  if (root.containsKey(KEY_DATA_ANGLE_AUTO)) {
    String data = root[KEY_DATA_ANGLE_AUTO];
    Firebase.setBool(FIREBASE_ANGLE_AUTO, stringToBool(data));
  }

  if (root.containsKey(KEY_DATA_ANGLE_CURRENT)) {
    String data = root[KEY_DATA_ANGLE_CURRENT];
    Firebase.setInt(FIREBASE_ANGLE_CURRENT, data.toInt());
  }

  if (root.containsKey(KEY_DATA_ANGLE_SPEED)) {
    String data = root[KEY_DATA_ANGLE_SPEED];
    Firebase.setInt(FIREBASE_ANGLE_SPEED, data.toInt());
  }

  //TEMP
  if (root.containsKey(KEY_DATA_TEMP_CURRENT)) {
    String data = root[KEY_DATA_TEMP_CURRENT];
    Firebase.setFloat(FIREBASE_TEMP_CURRENT, data.toFloat());
  }
  if (root.containsKey(KEY_DATA_TEMP_ENABLE)) {
    String data = root[KEY_DATA_TEMP_ENABLE];
    Firebase.setBool(FIREBASE_TEMP_ENABLE, stringToBool(data));
  }
};


void setSystemTime() {
  timeClient.update();
  system_time = String(timeClient.getHours()) + ":" + String(timeClient.getMinutes());
}

void controlByTime() {
  if (fan_auto) {
    setSystemTime();
    timer_start = Firebase.getString(FIREBASE_TIMER_START);
    timer_end = Firebase.getString(FIREBASE_TIMER_END);
    Serial.println(system_time + " " + timer_start);
    if (system_time == timer_start) {
      Firebase.setBool(FIREBASE_FAN_ENABLE, true);
    } else if (system_time == timer_end) {
      Firebase.setBool(FIREBASE_FAN_ENABLE, false);
    }
    delay(500);
  }
}

bool stringToBool(String value) {
  if (value == "true") return true;
  return false;
}
