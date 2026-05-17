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
  .speed = 250
};

struct JoyStick {
  int joyX = 0;
  int joyY = 0;
  bool joyBTN = false;
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
#define SCREEN_HEIGHT 32  
#define OLED_RESET -1   
#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);

//Buttons
#define BTN1 10

//Joystick
#define JOYX1 26
#define JOYY1 27
#define JOYSW1 22

//Data Variables
int pressed = 0;
bool newData = false;
JoyStick Joy1;


//Prototypes
void initScreen();
void displayText(const char *message);
void displayNum(int num);

bool BTNPressed(int BTN);
Direction nextDir(Direction d);

void readJoyStick(JoyStick *pJoy);
float limitRad(float rad);
Direction calculateDirection(JoyStick *pJoy);

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  pinMode(BTN1, INPUT_PULLDOWN);
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
  display.clearDisplay();
  display.setCursor(10, 15);

  readJoyStick(&Joy1);
  sendData.DIRECTION = calculateDirection(&Joy1);
  
  display.print((int) sendData.DIRECTION);

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
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  display.print("STARTING");
  display.display();
}

void displayText(const char *message) {
  display.setTextSize(1);
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
  int DEBOUNCE_MS = 50;

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

Direction calculateDirection(JoyStick *pJoy) {
  const int DEADZONE = 300; 

  if (abs(pJoy->joyX) < DEADZONE && abs(pJoy->joyY) < DEADZONE) {
    return Stop;
  }

  float angle = atan2(pJoy->joyY, pJoy->joyX); 
  float degrees = angle * RAD_TO_DEG; 

  if (degrees < 0) {
    degrees += 360;
  }
  if (degrees >= 22.5 && degrees <  67.5) return RightForward;
  else if (degrees >= 67.5 && degrees < 112.5) return Forward;
  else if (degrees >= 112.5 && degrees < 157.5) return LeftForward;
  else if (degrees >= 157.5 && degrees < 202.5) return Left;
  else if (degrees >= 202.5 && degrees < 247.5) return LeftBackward;
  else if (degrees >= 247.5 && degrees < 292.5) return Backward;
  else if (degrees >= 292.5 && degrees < 337.5) return RightBackward;
  else return Right; 
}