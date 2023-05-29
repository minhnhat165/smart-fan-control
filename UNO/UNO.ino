#include <ArduinoJson.h>
#include <SoftwareSerial.h>
#include "DHT.h"
#include <IRremote.h>
#include <Servo.h>

// Default configuration
const int DHTPIN = A0;
const int DHTTYPE = DHT11;
DHT dht(DHTPIN, DHTTYPE);
float temp_current = 33.0;
float temp_prev = 00.0;
float temp_threshold = 33.0;
bool temp_enable = false;

// Fan configuration
bool fan_enable = false;
bool prev_fan_enable = true;

// Servo configuration
Servo servo;
int servoPin = A4;
#define SERVO_SPEED_ONE 20
#define SERVO_SPEED_TWO 40
#define SERVO_SPEED_THREE 60
#define ANGLE_MAX 180
#define ANGLE_MIN 0
bool servo_auto = false;
int servo_speed = 3;
int angle = 90;
int prev_angle = 0;
bool isPlus = true;

// Speed configuration
#define SPEED_ONE 300
#define SPEED_TWO 700
#define SPEED_THREE 1023

bool speed_auto;
int speed_current;
int speed_prev = 0;
float speed_one_max;
float speed_two_max;

// IR receiver configuration
const int receiverPin = 5;
IRrecv irrecv(receiverPin);
decode_results results;

// JSON key constants
#define KEY_DATA_FAN_ENABLE "fe"
#define KEY_DATA_ANGLE_AUTO "aa"
#define KEY_DATA_ANGLE_CURRENT "ac"
#define KEY_DATA_ANGLE_SPEED "as"
#define KEY_DATA_SPEED_AUTO "sa"
#define KEY_DATA_SPEED_CURRENT "sc"
#define KEY_DATA_SPEED_ONE_MAX "som"
#define KEY_DATA_SPEED_TWO_MAX "stm"
#define KEY_DATA_TEMP_CURRENT "tc"
#define KEY_DATA_TEMP_ENABLE "te"
#define KEY_DATA_TEMP_THRESHOLD "tt"


// UART configuration
#define RX 8
#define TX 9
SoftwareSerial unoEspSerial(RX, TX);
bool espConnected = false;

// Pin definitions
#define FAN_CONTROL_PIN 6


// control fan
#define OFF_FAN digitalWrite(FAN_CONTROL_PIN, LOW)
#define ON_FAN digitalWrite(FAN_CONTROL_PIN, HIGH)

void setup() {
  Serial.begin(9600);
  unoEspSerial.begin(9600);
  irrecv.enableIRIn();
  pinMode(FAN_CONTROL_PIN, OUTPUT);
  OFF_FAN;
  servo.attach(servoPin);
  dht.begin();
}




void loop() {
  readDataFromEsp();
  readTemp();
  controlByRemote();
  controlFanState();
  controlServo();
  controlByTemp();
}


void readDataFromEsp() {

  if (unoEspSerial.available()) {
    String dataRead = unoEspSerial.readString();
    Serial.print(dataRead);
    handleJsonData(dataRead);
    espConnected = true;
  }
}





void sendUart(String data) {
  Serial.println("Send " + data);
  unoEspSerial.print(data + '\n');
  unoEspSerial.flush();  // wait end
}


String jsonWriteOne(String key, String value) {

  String result = String("{\"") + key + String("\":\"") + value + String("\"}");
  return result;
}


void handleJsonData(String data) {
  Serial.println("read " + data);
  DynamicJsonBuffer jsonBuffer(200);
  JsonObject& root = jsonBuffer.parseObject(data);

  if (!root.success()) {
    Serial.println("parseObject() failed");
    return;
  }


  //FAN
  if (root.containsKey(KEY_DATA_FAN_ENABLE)) {
    String data = root[KEY_DATA_FAN_ENABLE];
    fan_enable = stringToBool(data);
    Serial.println("fan enable " + data);
  }
  //TEMP
   if (root.containsKey(KEY_DATA_TEMP_ENABLE)) {
    String data = root[KEY_DATA_TEMP_ENABLE];
    temp_enable = stringToBool(data);
    Serial.println("temp enable " + data);
  }
  if (root.containsKey(KEY_DATA_TEMP_THRESHOLD)) {
    String data = root[KEY_DATA_TEMP_THRESHOLD];
    temp_threshold = data.toFloat();
    Serial.println("temp threshold " + data);
  }
  //SPEED
  if (root.containsKey(KEY_DATA_SPEED_CURRENT)) {
    String data = root[KEY_DATA_SPEED_CURRENT];
    speed_current = data.toInt();
    Serial.println("speed " + data);
  }
  if (root.containsKey(KEY_DATA_SPEED_AUTO)) {
    String data = root[KEY_DATA_SPEED_AUTO];
    speed_auto = stringToBool(data);
    Serial.println("auto " + data);
  }
  if (root.containsKey(KEY_DATA_SPEED_ONE_MAX)) {
    String data = root[KEY_DATA_SPEED_ONE_MAX];
    speed_one_max = data.toFloat();
    Serial.println("speed one max" + data);
  }
  if (root.containsKey(KEY_DATA_SPEED_TWO_MAX)) {
    String data = root[KEY_DATA_SPEED_TWO_MAX];
    speed_two_max = data.toFloat();
    Serial.println("speed two max" + data);
  }

  // ANGLE
  if (root.containsKey(KEY_DATA_ANGLE_CURRENT)) {
    String data = root[KEY_DATA_ANGLE_CURRENT];
    angle = data.toInt();
    Serial.println("angle " + data);
  }

  if (root.containsKey(KEY_DATA_ANGLE_AUTO)) {
    String data = root[KEY_DATA_ANGLE_AUTO];
    servo_auto = stringToBool(data);
    Serial.println("angle auto mode " + data);
  }

  if (root.containsKey(KEY_DATA_ANGLE_SPEED)) {
    String data = root[KEY_DATA_ANGLE_SPEED];
    servo_speed = data.toInt();
    Serial.println("angle speed " + data);
  }
};

void controlFanState() {
  if (fan_enable != prev_fan_enable) {
    Serial.println("call");
    if (fan_enable) {
      ON_FAN;
    } else {
      OFF_FAN;
    };
    prev_fan_enable = fan_enable;
  }
}

void controlServo() {
  if (fan_enable) {
    servo.write(angle);
    if (servo_auto) {
      if (angle == ANGLE_MAX) {
        isPlus = false;
      } else if (angle == ANGLE_MIN) {
        isPlus = true;
      }
      if (isPlus) {
        angle += 1;
      } else {
        angle -= 1;
      }

      // Calculate delay based on speed
      int delayValue;
      switch (servo_speed) {
        case 1:
          delayValue = SERVO_SPEED_ONE;
          break;
        case 2:
          delayValue = SERVO_SPEED_TWO;
          break;
        case 3:
          delayValue = SERVO_SPEED_THREE;
          break;
        default:
          delayValue = SERVO_SPEED_THREE;
          break;
      }
      delay(delayValue);
    } else {
      if (angle != prev_angle) {
        servo.write(angle);
      }
      // Fixed delay when not in auto mode
      delay(50);
    }
    prev_angle = angle;
  }
}

void controlByTemp() {
  if (!temp_enable) return;
  if (temp_current > temp_threshold) {
    if (fan_enable) return;
    fan_enable = true;
    String dataSend = jsonWriteOne(KEY_DATA_FAN_ENABLE, boolToString(fan_enable));  // send new value
    sendUart(dataSend);
  } else {
    if (!fan_enable) return;
    fan_enable = false;
    Serial.println("off");
    String dataSend = jsonWriteOne(KEY_DATA_FAN_ENABLE, boolToString(fan_enable));  // send new value
    sendUart(dataSend);
  }
}


// void controlFanSpeed() {
//   if (speed_auto) {
//     if (temp_current <= speed_one_max) {
//       speed_current = 1;
//     } else if (temp_current <= speed_two_max) {
//       speed_current = 2;
//     } else {
//       speed_current = 3;

//     }
//   }
//   if (speed_prev != speed_current) {
//     switch (speed_current) {
//       case 1:
//         // analogWrite(FAN_CONTROL_PIN, SPEED_ONE);
//         break;
//       case 2:
//         // analogWrite(FAN_CONTROL_PIN, SPEED_TWO);
//         break;
//       case 3:
//         // analogWrite(FAN_CONTROL_PIN, SPEED_THREE);
//         break;
//     }
//     speed_prev = speed_current;
//     Serial.println(speed_current);
//     sendUart(jsonWriteOne(KEY_DATA_SPEED_CURRENT, String(speed_current)));
//   }
// }


void readTemp() {
  temp_current = dht.readTemperature();
  if (espConnected && abs(temp_current - temp_prev) >= 0.1) {
    Serial.println(temp_current);
    temp_prev = temp_current;
    sendUart(jsonWriteOne(KEY_DATA_TEMP_CURRENT, String(temp_current)));
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
        fan_enable = !fan_enable;
        sendUart(jsonWriteOne(KEY_DATA_FAN_ENABLE, boolToString(fan_enable)));
        delay(500);
        break;
      case 4335:
        speed_auto = !speed_auto;
        sendUart(jsonWriteOne(KEY_DATA_SPEED_AUTO, boolToString(speed_auto)));
        delay(500);
        break;
      case 17085:
        speed_auto = false;
        sendUart(jsonWriteOne(KEY_DATA_SPEED_AUTO, boolToString(speed_auto)));
        speed_current = 1;
        break;
      case 19125:
        speed_auto = false;
        sendUart(jsonWriteOne(KEY_DATA_SPEED_AUTO, boolToString(speed_auto)));
        speed_current = 2;
        break;
      case 21165:
        speed_auto = false;
        sendUart(jsonWriteOne(KEY_DATA_SPEED_AUTO, boolToString(speed_auto)));
        speed_current = 3;
        break;
      case 39015:
        servo_auto = !servo_auto;
        sendUart(jsonWriteOne(KEY_DATA_ANGLE_AUTO, boolToString(servo_auto)));
        sendUart(jsonWriteOne(KEY_DATA_ANGLE_CURRENT, String(angle)));
        prev_angle = angle;
        delay(500);
        break;
    }
  }
  irrecv.resume();  // Receive the next value
}


// helper
bool stringToBool(String value) {
  if (value == "true") return true;
  return false;
}

String boolToString(bool value) {
  if (value) return "true";
  return "false";
}
