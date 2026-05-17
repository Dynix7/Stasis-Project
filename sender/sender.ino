#include <WiFi.h>
#include <WiFiUDP.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


// Communications
//johnny's wifi ssid "Dynov2"
//johnny's password "whatever"
//hira's wifi ssid "Hifo Hira #1!!"
//hira's wifi password "Hira1234"
const char *WIFI_SSID = "Hifo Hira #1!!";
const char *WIFI_PASSWORD = "Hira1234";
const char *PICO2_IP = "172.20.10.8";  // Reciever Pico IP
const int UDP_PORT = 5005;
WiFiUDP udp;

const int pinX = 26;
const int pinY = 27;
const int pinSW = 22;
int x_val;
int y_val;
int sw_val;
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

//Data
int pressed = 0;
//Prototypes
void initScreen();
void displayText(const char *message);
void displayNum(int num);

bool BTNPressed(int BTN);


void setup() {
  Serial.begin(115200);

  analogReadResolution(12);

  //Setup the pin mode for the joystick
  pinMode(pinSW, INPUT_PULLUP);
  pinMode(pinX, INPUT);
  pinMode(pinY, INPUT);

  pinMode(BTN1, INPUT_PULLDOWN);
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
  //setup for values of joystick, SW is R3 like PS4, x and y is self explantory.
  int x_val = analogRead(pinX);
  int y_val = analogRead(pinY);
  int sw_pin = analogRead(pinSW);
  display.clearDisplay();
  udp.beginPacket(PICO2_IP, UDP_PORT);
  udp.write((uint8_t*) &pressed, sizeof(pressed));
  udp.write((uint8_t*) &x_val, sizeof(x_val));



  udp.endPacket();
  

  displayNum((int) millis() / 1000);

  display.setCursor(10, 15);
  if (BTNPressed(BTN1)) {
    display.print("pressed!");
    pressed = 1;
  } else {
    display.print("!pressed");
    pressed = 0;
  }
  display.print(", ");
  display.print(x_val);

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