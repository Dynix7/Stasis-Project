#include <WiFi.h>
#include <WiFiUDP.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

enum Direction { 
  Forward, //0    
  Backward, //1  
  Left, //2
  Right, //3
  LeftForward, //4
  LeftBackward, //5
  RightForward, //6
  RightBackward, //7
  Stop //8   
};

struct velocity {
  Direction DIRECTION;
  int speed;
};

struct velocity sendData = {
  .DIRECTION = Stop,
  .speed = 0
};

struct JoyStick {
  int joyX = 0;
  int joyY = 0;
  bool joyBTN = false;
};

struct GearStates {
  bool Gear1 = true; // 100
  bool Gear2 = false; // 175
  bool Gear3 = false; //250
  int currentGear = 1;
  int maxSpeed = 100;
};

struct PedalStates {
  bool Clutch = false;
  bool Brake = false;
  bool Gas = false;
};

// Communications
const char *WIFI_SSID = "Dynov2";
const char *WIFI_PASSWORD = "whatever";
const char *PICO2_IP = "172.20.10.9";  // Reciever Pico IP
const int UDP_PORT = 5005;
WiFiUDP udp;

//Screen
#define SDA_PIN 14
#define SCL_PIN 15

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64  
#define OLED_RESET -1   
#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);

//Buttons
#define BTN1 10
#define BTN2 1
#define BTN3 2
//Joystick
#define JOYX1 26
#define JOYY1 27
#define JOYSW1 22

#define BUZZER 17
//Data Variables
int pressed = 0;
bool newData = false;
String msg = "";
JoyStick Joy1;
GearStates Gears;
PedalStates Pedals;

//Prototypes
void initScreen();
void displayText(const char *message);
void displayNum(int num);

bool BTNPressed(int BTN);
Direction nextDir(Direction d);

void readJoyStick(JoyStick *pJoy);
float limitRad(float rad);
Direction calculateDirection(JoyStick *pJoy);

void updateButtons();
void updatePedals(String msg);

void accelerate();
void slowDown();
void switchGears();
void setGearSpeed();
void Stall();
void changeGear(int newGear);


void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  digitalWrite(BUZZER, HIGH);


  pinMode(BTN1, INPUT_PULLDOWN);
  pinMode(BTN2, INPUT_PULLDOWN);
  pinMode(BTN3, INPUT_PULLDOWN);
  pinMode(JOYX1, INPUT);
  pinMode(JOYY1, INPUT);
  pinMode(JOYSW1, INPUT_PULLUP);

  Wire1.setSDA(SDA_PIN);
  Wire1.setSCL(SCL_PIN);
  Wire1.begin();

  initScreen();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  displayText("Connected to Wifi");

  Serial.println();
  Serial.println(WiFi.localIP());


}

void loop() {
  newData = false;
  msg = "";
  if (Serial.available()) {
    msg = Serial.readStringUntil('\n');
    msg.trim();
  }

  display.clearDisplay();
  display.setCursor(10, 15);

  updateButtons();
  updatePedals(msg);

  readJoyStick(&Joy1);
  sendData.DIRECTION = calculateDirection(&Joy1);

  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Current Gear: ");
  display.println(Gears.currentGear);

  display.print("Current Speed: ");
  display.println(sendData.speed);

  display.print("Max Speed: ");
  display.println(Gears.maxSpeed);

  switchGears();
  if (Pedals.Gas) accelerate();
  if (!Pedals.Gas) gradualSlowDown();
  if (Pedals.Brake || (sendData.speed > Gears.maxSpeed)) slowDown();
  
  udp.beginPacket(PICO2_IP, UDP_PORT);
  udp.write((uint8_t*) &sendData, sizeof(sendData));
  udp.endPacket();

  display.display();

}



void initScreen() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("Dispaly start erorr");
  }
  display.clearDisplay();
  display.ssd1306_command(0xA1);
  display.ssd1306_command(0xC8);
  display.setRotation(2);
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  display.print("STARTING");
  display.display();
}

void displayText(const char *message) {
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(message);
}

void displayNum(int num) {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(num);
}
/*
bool BTNPressed(int BTN) {
  static bool lastState[30] = {false};
  static unsigned long lastChangeTime[30] = {0};
  int DEBOUNCE_MS = 50;

  bool reading = digitalRead(BTN) == HIGH;
  unsigned long now = millis();

  if (reading != lastState[BTN] && (now - lastChangeTime[BTN] > DEBOUNCE_MS)) {
    lastState[BTN] = reading;
    lastChangeTime[BTN] = now;
  }

  return lastState[BTN];
}
*/

//Rising Edge Ver. 
bool BTNPressed(int BTN) {
  static bool lastState[30] = {false};
  static bool lastReturned[30] = {false};
  static unsigned long lastChangeTime[30] = {0};
  int DEBOUNCE_MS = 300;

  bool reading = digitalRead(BTN) == HIGH;
  unsigned long now = millis();

  if (reading != lastState[BTN] && (now - lastChangeTime[BTN] > DEBOUNCE_MS)) {
    lastState[BTN] = reading;
    lastChangeTime[BTN] = now;
  }

  // Rising edge: currently pressed, wasn't pressed last call
  bool risingEdge = lastState[BTN] && !lastReturned[BTN];
  lastReturned[BTN] = lastState[BTN];

  return risingEdge;
}

Direction nextDir(Direction d) {
  if (d == Stop) {
    return Forward;
  }
  return (Direction) ((int) d + 1);
  
}

void readJoyStick(JoyStick *pJoy) {
  int XVal = analogRead(JOYX1) - (4095/2) - 50;
  int YVal = -(analogRead(JOYY1) - (4095/2)) + 50;
  bool SW = digitalRead(JOYSW1);

  pJoy->joyX = XVal;
  pJoy->joyY =  YVal;
  pJoy->joyBTN = SW;
}

float limitRad(float rad) {
  if (rad > TWO_PI) {
    rad -= TWO_PI;
  }
  if (rad < -TWO_PI) {
    rad += TWO_PI;
  }
  return rad;
}

void updateButtons() {
  Gears.Gear1 = BTNPressed(BTN1);
  Gears.Gear2 = BTNPressed(BTN2);
  Gears.Gear3 = BTNPressed(BTN3); // i think this button is broken
  //Serial.println(Gears.Gear1);
  //Serial.println(Gears.Gear2);
  //Serial.println(Gears.Gear3);

}

void updatePedals(String msg) {
  static unsigned long lastATime = 0;
  static unsigned long lastBTime = 0;
  static unsigned long lastCTime = 0;
  const unsigned long HOLD_TIMEOUT = 400; // ms, adjust to be > your send interval

  unsigned long now = millis();

  if (msg == "a") lastATime = now;
  else if (msg == "b") lastBTime = now;
  else if (msg == "c") lastCTime = now;

  Pedals.Clutch = (now - lastATime) < HOLD_TIMEOUT;
  Pedals.Brake  = (now - lastBTime) < HOLD_TIMEOUT;
  Pedals.Gas = (now - lastCTime) < HOLD_TIMEOUT;
}

void accelerate() {
  int calcSpeed = sendData.speed;
  if (!Pedals.Clutch) {
    calcSpeed += 2;
  }
  if (calcSpeed > Gears.maxSpeed) {
    calcSpeed = Gears.maxSpeed; 
  }

  sendData.speed = calcSpeed;
}

void slowDown() {
  int calcSpeed = sendData.speed;
  calcSpeed -= 2;
  if (calcSpeed < 0) {
    calcSpeed = 0;
  }
  sendData.speed = calcSpeed;
}
//Hira implemented this. son are we fr?
void gradualSlowDown() {
  int calcSpeed = sendData.speed;
  calcSpeed -= 0.01;
  if (calcSpeed < 0) {
    calcSpeed = 0;
  }
  sendData.speed = calcSpeed;
}

void switchGears() {
  if (Pedals.Clutch) {
    int current = Gears.currentGear;
    // Pressing 1st Button
    if (Gears.Gear1) {
      //Stall
      if (current == 3) {
        Stall();
      }
      if (current == 2) 
        changeGear(1);
    }
    // Pressing 2nd Button
    if (Gears.Gear2) {
      if (current == 1)
        changeGear(2);
      if (current == 3)
        changeGear(2);
    }

    // Pressing 3rd Button
    if (Gears.Gear3) {
      if (current == 1)
        Stall();
      if (current == 2) 
        changeGear(3);
    }
  }

}

void setGearSpeed() {
  int current = Gears.currentGear;
  switch (current) {
    //Gear 1
    case 1:
      Gears.maxSpeed = 125;
      break;
    case 2:
      Gears.maxSpeed = 175;
      break;
    case 3:
      Gears.maxSpeed = 255;
      break;
  }
}

//Private
void Stall() {
  Gears.currentGear = 1;
  setGearSpeed();
  sendData.speed = 50;
  display.setTextSize(2);
  display.println("STALL!");
  delay(100);
}

//Hira did this work
void changeGear(int newGear) {
  int currentGear = Gears.currentGear;
  int amountBetween = abs(newGear-currentGear);
  if(amountBetween > 1) {
    Stall();
  }
  else if (amountBetween <= 1) {
    Gears.currentGear = newGear;
    setGearSpeed();
    }
  }


Direction calculateDirection(JoyStick *pJoy) {
  const int DEADZONE = 300; 

    
  if (abs(pJoy->joyX) < DEADZONE && abs(pJoy->joyY) < DEADZONE && Pedals.Gas) {
    return Forward;
  }

  if (abs(pJoy->joyX) < DEADZONE && abs(pJoy->joyY) < DEADZONE && !Pedals.Gas) {
    return Stop;
  }

  float angle = atan2(pJoy->joyY, pJoy->joyX); 
  float degrees = angle * RAD_TO_DEG; 

//Code for movement and acceleration. Main source of problem is that the pedals dont actually dicate the movement, its the joysticks that do.
// Hira's revisions**
  if (degrees < 0) {
    degrees += 360;
  }
  if ((degrees >= 22.5 && degrees <  67.5) && Pedals.Gas) return RightForward;
  else if ((degrees >= 67.5 && degrees < 112.5) && Pedals.Gas) return Forward;
  else if ((degrees >= 112.5 && degrees < 157.5) && Pedals.Gas) return LeftForward;
  else if ((degrees >= 157.5 && degrees < 202.5) && Pedals.Gas) return Left;
  else if ((degrees >= 202.5 && degrees < 247.5) && Pedals.Gas) return LeftBackward;
  else if ((degrees >= 247.5 && degrees < 292.5) && Pedals.Gas) return Backward; 
  else if ((degrees >= 292.5 && degrees < 337.5) && Pedals.Gas) return RightBackward;
  else if ((degrees >= 337.5 || degrees < 22.5) && Pedals.Gas) return Right;
  else return Stop;

}
