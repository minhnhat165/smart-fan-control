#include <ArduinoJson.h>
#include <SoftwareSerial.h>
// defaul config
bool fan_enable = false;
int fan_speed = 0;
bool temp_enable = false;
float temp_measure = 0;
int last = 0;

String dataSend = "";
String dataRead = "";

// key for json data
#define KEY_DATA_FAN_ENABLE "fan_enable"
#define KEY_DATA_TEMP_ENABLE "temp_enable"
#define KEY_DATA_TEMP_MEASURE "temp_measure"  

// function declare
void readDataFromEsp();
void sendDataToEsp();
void handleFanState();
String jsonWrite();  // convert to json data to send
void handleJsonData(String data);  // convert json data to data can handle

// define uart
#define RX 8
#define TX 9
SoftwareSerial unoEspSerial = SoftwareSerial(RX, TX);

// declare pinmode
#define FAN_CONTROL_PIN 3

// key for json data
#define KEY_DATA_FAN_STATE "isOnFan"

// control fan
#define OFF_FAN digitalWrite(FAN_CONTROL_PIN, LOW)
#define ON_FAN digitalWrite(FAN_CONTROL_PIN, HIGH)

void setup() {
  Serial.begin(9600);
  unoEspSerial.begin(9600);
}

void loop() {
  readDataFromEsp();
    if (millis() - last >= 1000) {
    last = millis();
    sendDataToEsp();
  }
  handleFanState();
}


void readDataFromEsp() {
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

void sendDataToEsp() {
  String dataSend = jsonWrite();
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
    // fan_enable = data.toInt();
    Serial.println(data);
  }
  if (root.containsKey(KEY_DATA_TEMP_ENABLE)) {
    String data = root[KEY_DATA_TEMP_ENABLE];
    // temp_enable = data.toInt();
  
    Serial.println(data);
  }
};

void handleFanState() {
  if (fan_enable) ON_FAN;
  else OFF_FAN;
}

void handleControlByTemp() {
}

void handleControlByTime() {
}

void handleControlByRemote() {
}
