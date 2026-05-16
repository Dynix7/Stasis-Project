#include <WiFi.h>
#include <WiFiUDP.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* WIFI_SSID = "Dynov2";
const char* WIFI_PASSWORD = "whatever";
const int UDP_PORT = 5005;

//Pinouts
#define BUZZER 10

//Screen
#define SDA_PIN 14
#define SCL_PIN 15

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64  
#define OLED_RESET -1   
#define SCREEN_ADDRESS 0x3C 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);

WiFiUDP udp;
char packet[64];

int pressed = 0;

//Prototypes
void initScreen();
void displayText(const char *message);
void displayNum(int num);


void setup() {
  Serial.begin(115200);

  pinMode(BUZZER, OUTPUT);
  
  Wire1.setSDA(SDA_PIN);
  Wire1.setSCL(SCL_PIN);
  Wire1.begin();
  initScreen();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }

  displayText("Connected to Wifi");

  Serial.println();
  Serial.println(WiFi.localIP());
  delay(5000);

  udp.begin(UDP_PORT);
  Serial.println("Listening for UDP packets...");
}

void loop() {
  display.clearDisplay();
  int packetSize = udp.parsePacket();
  Serial.println(packetSize);
  if (packetSize == sizeof(int)) {
    udp.read((uint8_t*)&pressed, sizeof(int));
  }
  displayNum(pressed);
  if (pressed) {
    analogWrite(BUZZER, 100);
  } else {
    analogWrite(BUZZER, LOW);
  }
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