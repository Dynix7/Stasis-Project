#include <SoftwareSerial.h>
#include "DeviceDriverSet_xxx0.h"
#include "ApplicationFunctionSet_xxx0.cpp"

DeviceDriverSet_Motor AppMotor;
Application_xxx Controller;

struct velocity {
  int32_t direction;
  int32_t speed;
};

struct velocity controlData = {stop_it, 0};
SoftwareSerial picoSerial(12, 13);

void setup() {
  Serial.begin(115200);
  AppMotor.DeviceDriverSet_Motor_Init();
  picoSerial.begin(9600);
}

void loop() {
  if (picoSerial.available() >= sizeof(controlData)) {
    picoSerial.readBytes((uint8_t*)&controlData, sizeof(controlData));
    Controller.Motion_Control = (ConquerorCarMotionControl)controlData.direction;
    Serial.println(controlData.direction);
    ApplicationFunctionSet_ConquerorCarMotionControl(Controller.Motion_Control, (uint8_t)controlData.speed);
  }
}