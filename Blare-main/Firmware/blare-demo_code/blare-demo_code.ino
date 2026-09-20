#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <WiFi.h>
#include <time.h>

#define TFT_SCLK 0
#define TFT_MOSI 1
#define TFT_RST 2
#define TFT_DC 3
#define TFT_CS 4
#define TFT_BL 5

#define BTN_INCREMENT 8
#define BTN_DECREMENT 9
#define BTN_CANCEL 10
#define BTN_ENTER 11
#define BUZZER_PIN 6

const char* ssid = "Airtel_kuma_7191";
const char* password = "my_password_is_a_secret_you_cant_hack_me:)";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

class MyST7789 : public Adafruit_ST7789 {
public:
  MyST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst)
    : Adafruit_ST7789(cs, dc, mosi, sclk, rst) {}
  void setOffsets(uint8_t col, uint8_t row) {
    _colstart = _colstart2 = col;
    _rowstart = _rowstart2 = row;
  }
};

MyST7789 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

uint8_t alarmHour = 7;
uint8_t alarmMin = 0;
bool alarmEnabled = true;
bool alarmTriggered = false;
unsigned long baseMillis = 0;
time_t baseTime = 0;
int settingState = 0;

bool connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("\nWiFi Connection Failed!");
    return false;
  }
}

bool buttonPressed(int pin) {
  if (digitalRead(pin) == LOW) {
    delay(50);
    if (digitalRead(pin) == LOW) {
      while (digitalRead(pin) == LOW) {
        delay(10);
      }
      delay(50);
      return true;
    }
  }
  return false;
}

void displayClock(uint8_t hour, uint8_t minute, uint8_t second) {
  tft.fillScreen(ST77XX_BLACK);
  
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(7);
  
  char timeStr[20];
  sprintf(timeStr, "%02d:%02d", hour, minute);
  
  int16_t x = 30;
  int16_t y = 40;
  tft.setCursor(x, y);
  tft.println(timeStr);

  tft.setTextSize(3);
  tft.setCursor(120, 60);
  sprintf(timeStr, "%02d", second);
  tft.println(timeStr);

  tft.setTextSize(2);
  tft.setCursor(10, 120);
  
  char alarmStr[30];
  sprintf(alarmStr, "Alarm: %02d:%02d", alarmHour, alarmMin);
  
  if (alarmEnabled) {
    tft.setTextColor(ST77XX_GREEN);
  } else {
    tft.setTextColor(ST77XX_RED);
  }
  tft.println(alarmStr);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 145);

  if (alarmEnabled) {
    tft.println("ENTER: Set  3: Toggle OFF");
  } else {
    tft.println("ENTER: Set  3: Toggle ON");
  }
}

void displaySetHour(uint8_t hour) {
  tft.fillScreen(ST77XX_BLUE);
  
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(6);
  tft.setCursor(50, 50);
  
  char hourStr[5];
  sprintf(hourStr, "%02d", hour);
  tft.println(hourStr);

  tft.setTextSize(2);
  tft.setCursor(20, 130);
  tft.println("Set Hour");
  tft.setCursor(10, 150);
  tft.println("1:Up  2:Down  4:OK");
}

void displaySetMin(uint8_t minute) {
  tft.fillScreen(ST77XX_CYAN);
  
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(6);
  tft.setCursor(50, 50);
  
  char minStr[5];
  sprintf(minStr, "%02d", minute);
  tft.println(minStr);

  tft.setTextSize(2);
  tft.setCursor(20, 130);
  tft.println("Set Minute");
  tft.setCursor(10, 150);
  tft.println("1:Up  2:Down  4:OK");
}

void displayAlarmTrigger() {
  tft.fillScreen(ST77XX_RED);
  
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(7);
  tft.setCursor(30, 40);
  tft.println("ALARM!");

  tft.setTextSize(3);
  tft.setCursor(20, 120);
  tft.println("PRESS TO");
  tft.setCursor(40, 160);
  tft.println("DISMISS");
}

void buzzerBeep() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(300);
  digitalWrite(BUZZER_PIN, LOW);
  delay(300);
}

void setup() {
  Serial.begin(115200);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW);
  
  pinMode(BTN_INCREMENT, INPUT_PULLUP);
  pinMode(BTN_DECREMENT, INPUT_PULLUP);
  pinMode(BTN_CANCEL, INPUT_PULLUP);
  pinMode(BTN_ENTER, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  tft.init(76, 284);
  tft.setOffsets(82, 18);
  tft.invertDisplay(false);
  tft.setRotation(1);
  Serial.println("Display ready!");

  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(4);
  tft.setCursor(20, 40);
  tft.println("Blare-Honey clock");

  tft.setTextSize(2);
  tft.setCursor(10, 90);
  tft.println("Connecting WiFi...");

  if (connectToWiFi()) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    delay(2000);
    
    baseTime = time(nullptr);
    baseMillis = millis();

    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(3);
    tft.setCursor(40, 80);
    tft.println("Time Synced!");
    delay(2000);
  } else {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(20, 80);
    tft.println("WiFi Failed");
    tft.setCursor(10, 110);
    tft.println("Check credentials");
    baseTime = time(nullptr);
    baseMillis = millis();
    delay(3000);
  }

  tft.fillScreen(ST77XX_BLACK);
}

void loop() {
  unsigned long currentMillis = millis();
  unsigned long elapsedSeconds = (currentMillis - baseMillis) / 1000;
  
  time_t currentTime = baseTime + elapsedSeconds;
  struct tm* timeinfo = localtime(&currentTime);
  
  uint8_t hour = timeinfo->tm_hour;
  uint8_t minute = timeinfo->tm_min;
  uint8_t second = timeinfo->tm_sec;

  if (settingState == 0) {
    displayClock(hour, minute, second);
    
    if (buttonPressed(BTN_CANCEL)) {
      alarmEnabled = !alarmEnabled;
    }
    
    if (buttonPressed(BTN_ENTER)) {
      settingState = 1;
    }
    
    if (alarmEnabled && !alarmTriggered && hour == alarmHour && minute == alarmMin && second == 0) {
      alarmTriggered = true;
      settingState = 3;
    }
    
  } else if (settingState == 1) {
    displaySetHour(alarmHour);
    
    if (buttonPressed(BTN_INCREMENT)) {
      alarmHour = (alarmHour + 1) % 24;
    }
    
    if (buttonPressed(BTN_DECREMENT)) {
      alarmHour = (alarmHour - 1 + 24) % 24;
    }
    
    if (buttonPressed(BTN_ENTER)) {
      settingState = 2;
    }
    
  } else if (settingState == 2) {
    displaySetMin(alarmMin);
    
    if (buttonPressed(BTN_INCREMENT)) {
      alarmMin = (alarmMin + 1) % 60;
    }
    
    if (buttonPressed(BTN_DECREMENT)) {
      alarmMin = (alarmMin - 1 + 60) % 60;
    }
    
    if (buttonPressed(BTN_ENTER)) {
      settingState = 0;
    }
    
  } else if (settingState == 3) {
    displayAlarmTrigger();
    buzzerBeep();
    
    if (buttonPressed(BTN_ENTER) || buttonPressed(BTN_INCREMENT) || buttonPressed(BTN_DECREMENT)) {
      alarmTriggered = false;
      settingState = 0;
    }
  }
  
  delay(100);
}