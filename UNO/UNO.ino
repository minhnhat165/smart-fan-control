#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include "DHT.h"
#include <IRremote.h>  // thư viện hỗ trợ IR remote
// default config
// Cam bien nhiet do & do am
const int DHTPIN = A0;
const int DHTTYPE = DHT11;
DHT dht(DHTPIN, DHTTYPE);
//
bool fan_enable = false;
int fan_speed = 0;
bool temp_enable = false;
float temp_measure = 33.0;
int last = 0;
float TEMP_THRESHOLD = 33.0;

// Cam bien hong ngoai
const int receiverPin = A2;  // chân digital 8 dùng để đọc tín hiệu
IRrecv irrecv(receiverPin);  // tạo đối tượng IRrecv mới
decode_results results;      // lưu giữ kết quả giải mã tín hiệu

String dataSend = "";
String dataRead = "";
bool isSending = true;
bool isReading = true;

// key for json data
#define KEY_DATA_FAN_ENABLE "fan_enable"
#define KEY_DATA_TEMP_ENABLE "temp_enable"
#define KEY_DATA_TEMP_MEASURE "temp_measure"

// function declare
void readDataFromEsp();
void sendDataToEsp();
void handleFanState();
String jsonWrite();                // convert to json data to send
void handleJsonData(String data);  // convert json data to data can handle

// define uart
#define RX 8
#define TX 9
SoftwareSerial unoEspSerial = SoftwareSerial(RX, TX);

// declare pinmode
#define FAN_CONTROL_PIN 3

// control fan
#define OFF_FAN digitalWrite(FAN_CONTROL_PIN, LOW)
#define ON_FAN digitalWrite(FAN_CONTROL_PIN, HIGH)

void setup() {
  Serial.begin(9600);
  unoEspSerial.begin(9600);
  irrecv.enableIRIn();
  pinMode(FAN_CONTROL_PIN, OUTPUT);

  // analogWrite(FAN_CONTROL_PIN, 0);
}

void loop() {
  readDataFromEsp();
  // delay(5000);
  // Serial.println("1");
  // analogWrite(FAN_CONTROL_PIN, 300);
  // digitalWrite(FAN_CONTROL_PIN, LOW);
  // delay(5000);
  // Serial.println("2");
  // digitalWrite(FAN_CONTROL_PIN, HIGH);

  // analogWrite(FAN_CONTROL_PIN, 1000);
  // Serial.println("3");
  // readTemp();
  controlByRemote();
  // controlByTemp();
  controlFanState();
  // sendDataToEsp();
  // delay(1000);
  // if (millis() - last >= 10000) {
  //   last = millis();
  //   Serial.println("send");
  //   sendDataToEsp();
  // }
}


void readDataFromEsp() {
  ;
  while (unoEspSerial.available()) {
    char readChar = (char)unoEspSerial.read();
    if (readChar != '\n') {
      dataRead += readChar;
    } else {
      // Serial.println("read data form esp: " + dataRead);
      handleJsonData(dataRead);
      dataRead = "";
      // isReading = false;
    }
  }
}

void sendDataToEsp() {
  Serial.println("send data");
  String dataSend = jsonWrite();
  Serial.println("send data" + dataSend);
  unoEspSerial.print(dataSend + '\n');
  unoEspSerial.flush();  // wait end
}

String jsonWrite() {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root[KEY_DATA_FAN_ENABLE] = fan_enable;
  root[KEY_DATA_TEMP_ENABLE] = temp_enable;
  root[KEY_DATA_TEMP_MEASURE] = temp_measure;
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
    Serial.println(fan_enable);
  }
  if (root.containsKey(KEY_DATA_TEMP_ENABLE)) {
    String data = root[KEY_DATA_TEMP_ENABLE];
    temp_enable = stringToBool(data);
    Serial.println(temp_enable);
  }
};

void controlFanState() {
  if (fan_enable) ON_FAN;
  else OFF_FAN;
}

void readTemp() {
  temp_measure = dht.readTemperature();
}

void controlByTemp() {
  if (!temp_enable) return;

  if (temp_measure > TEMP_THRESHOLD) {
    fan_enable = true;
  } else {
    fan_enable = false;
  }
}

void controlByRemote() {
  if (irrecv.decode(&results))  // nếu nhận được tín hiệu
  {
    unsigned int value = results.value;
    Serial.println(value);
    switch (value) {
      case 26775:
      case 65535:
        fan_enable = false;
        Serial.println("tat");
        delay(500);
        break;
      case 12495:
      case 32255:
      case 255:
        fan_enable = true;
        Serial.println("bat");
        delay(500);
        break;
    }
    sendDataToEsp();
  }

  irrecv.resume();  // Receive the next value
}

bool stringToBool(String value) {
  if (value == "true") return true;
  return false;
}
