
#include <LCDWIKI_GUI.h>
#include <LCDWIKI_KBV.h>
#include <SoftwareSerial.h>

// ========== TFT ==========
LCDWIKI_KBV mylcd(240, 320, A3, A2, A1, A0, A4);

// ESP32 GPIO17 TX -> UNO D10 RX
SoftwareSerial espSerial(10, 11);

// ========== COLORS ==========
#define BLACK  0x0000
#define WHITE  0xFFFF
#define RED    0xF800
#define GREEN  0x07E0
#define BLUE   0x001F
#define CYAN   0x07FF
#define YELLOW 0xFFE0

// ========== RECEIVED VALUES ==========
float voltage = 0.0;
float temperature = 0.0;
float current_mA = 0.0;

int suitable = 0;
int solar = 0;
int charging = 0;
int screenState = 0;

int displayedState = -1;
String packet = "";

// ========== PARSING ==========
float getFloatValue(String data, String key) {
  int start = data.indexOf(key);
  if (start < 0) return 0;

  start += key.length();

  int end = data.indexOf(',', start);
  if (end < 0) end = data.length();

  return data.substring(start, end).toFloat();
}

int getIntValue(String data, String key) {
  int start = data.indexOf(key);
  if (start < 0) return 0;

  start += key.length();

  int end = data.indexOf(',', start);
  if (end < 0) end = data.length();

  return data.substring(start, end).toInt();
}

// ========== TEXT ==========
void textStyle(uint16_t fg, uint16_t bg, uint8_t size) {
  mylcd.Set_Text_Mode(0);
  mylcd.Set_Text_colour(fg);
  mylcd.Set_Text_Back_colour(bg);
  mylcd.Set_Text_Size(size);
}

void header(String title) {
  mylcd.Fill_Rectangle(0, 0, 319, 35);

  textStyle(WHITE, BLUE, 2);
  mylcd.Print_String(title, 10, 8);
}

// ========== VALUE UPDATE HELPERS ==========
void drawVoltage(int x, int y) {
  mylcd.Fill_Rectangle(x, y, x + 85, y + 25);

  textStyle(CYAN, BLACK, 2);
  mylcd.Print_Number_Float(
      voltage, 2, x, y, '.', 3, ' '
  );
}

void drawTemperature(int x, int y) {
  mylcd.Fill_Rectangle(x, y, x + 85, y + 25);

  textStyle(YELLOW, BLACK, 2);
  mylcd.Print_Number_Float(
      temperature, 1, x, y, '.', 3, ' '
  );
}

void drawCurrent(int x, int y) {
  mylcd.Fill_Rectangle(x, y, x + 85, y + 25);

  textStyle(WHITE, BLACK, 2);
  mylcd.Print_Number_Int(
      (int)current_mA, x, y, 3, ' ', 10
  );
}

// ========== CHARGING STATUS ==========
// Used ONLY on the Solar Storage screen.
void drawChargingStatus(int y) {
  mylcd.Fill_Rectangle(35, y - 5, 285, y + 30);

  if (charging == 1 && solar == 1) {
    textStyle(GREEN, BLACK, 3);
    mylcd.Print_String("CHARGING", 65, y);
  } else {
    textStyle(YELLOW, BLACK, 2);
    mylcd.Print_String("STANDBY", 90, y);
  }
}

// ========== HOME STATIC LAYOUT ==========
void drawHomeLayout() {
  mylcd.Fill_Screen(BLACK);
  header("RE-VOLT");

  textStyle(WHITE, BLACK, 2);
  mylcd.Print_String("BATTERY SCREENING", 15, 55);

  mylcd.Print_String("Voltage:", 10, 100);
  mylcd.Print_String("V", 235, 100);

  mylcd.Print_String("Temp:", 10, 140);
  mylcd.Print_String("C", 235, 140);

  mylcd.Print_String("Current:", 10, 180);
  mylcd.Print_String("mA", 235, 180);

  textStyle(YELLOW, BLACK, 1);
  mylcd.Print_String("CURRENT IS SIMULATED", 10, 215);

  textStyle(GREEN, BLACK, 2);
  mylcd.Print_String("SYSTEM READY", 45, 250);

  textStyle(WHITE, BLACK, 1);
  mylcd.Print_String("Press button to start", 45, 290);
}

// ========== SCREENING STATIC LAYOUT ==========
void drawScreeningLayout() {
  mylcd.Fill_Screen(BLACK);
  header("SCREENING");

  textStyle(YELLOW, BLACK, 2);
  mylcd.Print_String("TEST IN PROGRESS", 25, 60);

  textStyle(CYAN, BLACK, 2);
  mylcd.Print_String("Voltage:", 15, 110);
  mylcd.Print_String("V", 240, 110);

  textStyle(YELLOW, BLACK, 2);
  mylcd.Print_String("Temp:", 15, 150);
  mylcd.Print_String("C", 240, 150);

  textStyle(WHITE, BLACK, 2);
  mylcd.Print_String("Current:", 15, 190);
  mylcd.Print_String("mA", 240, 190);

  textStyle(GREEN, BLACK, 2);
  mylcd.Print_String("ANALYZING...", 55, 250);
}

// ========== RESULT STATIC LAYOUT ==========
// No charging status on this page.
void drawResultLayout() {
  mylcd.Fill_Screen(BLACK);
  header("RESULT");

  if (suitable == 1) {
    textStyle(GREEN, BLACK, 3);
    mylcd.Print_String("SUITABLE", 55, 70);

    textStyle(WHITE, BLACK, 2);
    mylcd.Print_String("SECOND-LIFE", 55, 125);
    mylcd.Print_String("READY", 95, 160);

    textStyle(GREEN, BLACK, 2);
    mylcd.Print_String("Voltage / Temp PASS", 20, 220);

  } else {
    textStyle(RED, BLACK, 3);
    mylcd.Print_String("NOT SUITABLE", 20, 75);

    textStyle(WHITE, BLACK, 2);
    mylcd.Print_String("Further evaluation", 20, 145);
    mylcd.Print_String("or recycling", 55, 185);
  }

  textStyle(YELLOW, BLACK, 1);
  mylcd.Print_String("Prototype screening only", 55, 265);
  mylcd.Print_String("Press for solar demo", 55, 290);
}

// ========== SOLAR STATIC LAYOUT ==========
void drawSolarLayout() {
  mylcd.Fill_Screen(BLACK);
  header("SOLAR STORAGE");

  textStyle(CYAN, BLACK, 2);
  mylcd.Print_String("Battery:", 15, 75);
  mylcd.Print_String("V", 240, 75);

  // Charging/standby appears ONLY on this screen.
  drawChargingStatus(145);

  textStyle(YELLOW, BLACK, 1);
  mylcd.Print_String("DEMO / SIMULATED CHARGING", 25, 215);
  mylcd.Print_String("Solar panel is a demo input", 25, 245);
  mylcd.Print_String("Press to return HOME", 55, 290);
}

// ========== LIVE VALUE UPDATES ==========
void updateLiveValues() {
  if (screenState == 0) {
    drawVoltage(145, 100);
    drawTemperature(145, 140);
    drawCurrent(145, 180);
  }
  else if (screenState == 1) {
    drawVoltage(150, 110);
    drawTemperature(150, 150);
    drawCurrent(150, 190);
  }
  else if (screenState == 3) {
    // Only the Solar Storage screen updates charging status.
    drawVoltage(155, 75);
    drawChargingStatus(145);
  }
}

// ========== DRAW STATE ==========
void drawCurrentState() {
  if (screenState == 0) {
    drawHomeLayout();
  }
  else if (screenState == 1) {
    drawScreeningLayout();
  }
  else if (screenState == 2) {
    drawResultLayout();
  }
  else if (screenState == 3) {
    drawSolarLayout();
  }

  displayedState = screenState;
  updateLiveValues();
}

// ========== PARSE COMPLETE PACKET ==========
void parsePacket() {
  int oldState = screenState;
  int oldSuitable = suitable;
  int oldCharging = charging;
  int oldSolar = solar;

  voltage = getFloatValue(packet, "V:");
  temperature = getFloatValue(packet, "T:");
  current_mA = getFloatValue(packet, "I:");

  suitable = getIntValue(packet, "S:");
  solar = getIntValue(packet, "SOL:");
  charging = getIntValue(packet, "CHG:");
  screenState = getIntValue(packet, "SCR:");

  Serial.print("S=");
  Serial.print(suitable);
  Serial.print(" SOL=");
  Serial.print(solar);
  Serial.print(" CHG=");
  Serial.print(charging);
  Serial.print(" SCR=");
  Serial.println(screenState);

  bool screenChanged =
      screenState != oldState ||
      screenState != displayedState;

  bool solarStatusChanged =
      screenState == 3 &&
      (charging != oldCharging || solar != oldSolar);

  bool resultChanged =
      screenState == 2 &&
      suitable != oldSuitable;

  if (screenChanged || solarStatusChanged || resultChanged) {
    drawCurrentState();
  } else {
    updateLiveValues();
  }

  packet = "";
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  espSerial.begin(9600);

  mylcd.Init_LCD();
  mylcd.Set_Rotation(1);
  mylcd.Fill_Screen(BLACK);

  textStyle(WHITE, BLACK, 3);
  mylcd.Print_String("RE-VOLT", 100, 130);

  Serial.println("RE-VOLT UNO TFT READY");
}

// ========== LOOP ==========
void loop() {
  while (espSerial.available()) {
    char c = espSerial.read();

    if (c == '\n') {
      if (packet.length() > 0) {
        parsePacket();
      }
    }
    else if (c != '\r') {
      if (packet.length() < 100) {
        packet += c;
      } else {
        // Discard malformed/oversized packet.
        packet = "";
      }
    }
  }
}
