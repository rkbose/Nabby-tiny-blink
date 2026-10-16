//
// Nabby-tiny-blink ESP32 based by RKB

//
// Copyright 2022, Raj Bose
//
// This file is part of Nabby-tiny.
// Nabby-tiny is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
// Nabby-tiny is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
// You should have received a copy of the GNU General Public License along with Nabby-tiny. If not, see <https://www.gnu.org/licenses/>.
//

#include <HardwareSerial.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <AsyncUDP.h>
#include <Secret.h>
#include <ESPmDNS.h>

#include <DynamicCommandParser.h>
#include <Parsers.h>

#include <Wire.h>
#include <U8g2lib.h>
#include <Nabby.h>

// U8G2_SH1106_64X32_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
// U8G2_SSD1306_64X48_ER_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
// U8G2_SSD1306_64X32_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R1, U8X8_PIN_NONE); // 128x64 display

/* WEMOS D1 mini OLED is a 0.49-inch display with a resolution of 64x32 pixels. Utilizes an SSD1306 */
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 128

#define VERSION "4Okt2026"
String version;

// Initialize the command parsers using the start, end, delimiting characters
// A seperate parser is instantiated for UDP. This is strictly not neccesry, but had advantages like:
//    - the udp parser can have different commands (or delimiting chars) then the serial parser, or a subset/superset of commands
//    - with one parser there is a very small chance serial commands mix up with udp commands. Seperation resolves this
// Downside is memory space.
//
DynamicCommandParser dcp_ser('/', 0x0D, ','); // parser for serial
DynamicCommandParser dcp_udp('/', 0x0D, ','); // parser for udp

AsyncUDP udp;
WiFiMulti wifiMulti;

bool connectWifi()
{
  WiFi.mode(WIFI_STA); // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.disconnect();
  delay(100);

  wifiMulti.addAP(SSID1, PSW1);
  // wifiMulti.addAP(SSID2, PSW2); // more routers can be registered, strongest RSSI will be selected. SSID needs definition in secret.h
  // wifiMulti.addAP(SSID3, PSW3); // more routers can be registered, strongest RSSI will be selected. SSID needs definition in secret.h

  Serial.print("Connecting Wifi -");
  const unsigned long wifiTimeout = millis() + 20000;
  while (wifiMulti.run() != WL_CONNECTED)
  {
    if (millis() >= wifiTimeout)
    {
      Serial.println(" timeout");
      return false;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to network");

  Serial.print("WiFi connected: ");
  Serial.print(WiFi.SSID());
  Serial.print(";   IP addr: ");
  Serial.println(WiFi.localIP());
  return true;
}

void handleUdp()
{
  if (udp.listen(1235))
  {
    udp.onPacket([](AsyncUDPPacket packet)
                 {
                   //  Serial.printf("udp rcv: %s\n",(char *)packet.data());
                   dcp_udp.append(&packet);
                   //                  dcp_udp.append(&packet);
                   //      String reply = dcp_udp.append(myString);
                 });
  }
}

#define EYE_WIDTH 3
#define EYE_HEIGHT 6

#define LEFT_EYE_X 32
#define LEFT_EYE_Y 68
#define RIGHT_EYE_X (LEFT_EYE_X + 16)
#define RIGHT_EYE_Y 68

#define NOSE_X (LEFT_EYE_X + 8)
#define NOSE_Y 80

void eyes(int mood)
{
  char *TextString;

  u8g2.setDrawColor(1);
  u8g2.drawBox(LEFT_EYE_X - 4, LEFT_EYE_Y - 6, 30, 15); // draw a filled box to clear the eyes
  u8g2.setDrawColor(0);                                 // set draw color to black for the eyes

  switch (mood)
  {
  case 0: // normal
    u8g2.drawFilledEllipse(LEFT_EYE_X, LEFT_EYE_Y, EYE_WIDTH, EYE_HEIGHT);
    u8g2.drawFilledEllipse(RIGHT_EYE_X, RIGHT_EYE_Y, EYE_WIDTH, EYE_HEIGHT);
    TextString = "NORMAL";
    break;

  case 1: // BLINK
    u8g2.drawFilledEllipse(LEFT_EYE_X, LEFT_EYE_Y, EYE_WIDTH, 2);
    u8g2.drawFilledEllipse(RIGHT_EYE_X, RIGHT_EYE_Y, EYE_WIDTH, EYE_HEIGHT);

    TextString = "BLINK";
    break;

  case 2: // sleepy
    u8g2.drawFilledEllipse(LEFT_EYE_X, LEFT_EYE_Y, EYE_WIDTH, 2);
    u8g2.drawFilledEllipse(RIGHT_EYE_X, RIGHT_EYE_Y, EYE_WIDTH, 2);

    TextString = "SLEEPY";
    break;

  case 3: // down
    u8g2.drawFilledEllipse(LEFT_EYE_X, LEFT_EYE_Y, EYE_WIDTH, EYE_HEIGHT, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
    u8g2.drawFilledEllipse(RIGHT_EYE_X, RIGHT_EYE_Y, EYE_WIDTH, EYE_HEIGHT, U8G2_DRAW_LOWER_LEFT | U8G2_DRAW_LOWER_RIGHT);
    TextString = "DOWN";
    break;

  case 4: // angry
    u8g2.drawFilledEllipse(LEFT_EYE_X, LEFT_EYE_Y, EYE_WIDTH, EYE_HEIGHT);
    u8g2.drawFilledEllipse(RIGHT_EYE_X, RIGHT_EYE_Y, EYE_WIDTH, EYE_HEIGHT);
    u8g2.setDrawColor(1);
    u8g2.drawTriangle(
        LEFT_EYE_X - EYE_WIDTH - 1, LEFT_EYE_Y - EYE_HEIGHT,
        LEFT_EYE_X + EYE_WIDTH + 1, LEFT_EYE_Y - EYE_HEIGHT,
        LEFT_EYE_X + EYE_WIDTH + 1, LEFT_EYE_Y + 2);
    u8g2.drawTriangle(
        RIGHT_EYE_X - EYE_WIDTH - 1, RIGHT_EYE_Y - EYE_HEIGHT,
        RIGHT_EYE_X + EYE_WIDTH + 1, RIGHT_EYE_Y - EYE_HEIGHT,
        RIGHT_EYE_X - EYE_WIDTH - 1, RIGHT_EYE_Y + 2);
    TextString = "ANGRY";
    break;

  case 5: // sad
    u8g2.drawFilledEllipse(LEFT_EYE_X, LEFT_EYE_Y, EYE_WIDTH, EYE_HEIGHT);
    u8g2.drawFilledEllipse(RIGHT_EYE_X, RIGHT_EYE_Y, EYE_WIDTH, EYE_HEIGHT);
    u8g2.setDrawColor(1);
    u8g2.drawTriangle(
        LEFT_EYE_X - EYE_WIDTH - 1, LEFT_EYE_Y + 2,
        LEFT_EYE_X - EYE_WIDTH - 1, LEFT_EYE_Y - EYE_HEIGHT,
        LEFT_EYE_X + EYE_WIDTH + 1, LEFT_EYE_Y - EYE_HEIGHT);
    u8g2.drawTriangle(
        RIGHT_EYE_X + EYE_WIDTH + 1, RIGHT_EYE_Y + 2,
        RIGHT_EYE_X + EYE_WIDTH + 1, RIGHT_EYE_Y - EYE_HEIGHT,
        RIGHT_EYE_X - EYE_WIDTH - 1, RIGHT_EYE_Y - EYE_HEIGHT);
    TextString = "SAD";
    break;

  case 6: // develish
    u8g2.drawFilledEllipse(LEFT_EYE_X, LEFT_EYE_Y, EYE_WIDTH, EYE_HEIGHT);
    u8g2.drawFilledEllipse(RIGHT_EYE_X, RIGHT_EYE_Y, EYE_WIDTH, EYE_HEIGHT);
    u8g2.setDrawColor(1);
    u8g2.drawTriangle(
        LEFT_EYE_X - EYE_WIDTH - 1, LEFT_EYE_Y - EYE_HEIGHT - 1,
        LEFT_EYE_X + EYE_WIDTH + 1, LEFT_EYE_Y - EYE_HEIGHT - 1,
        LEFT_EYE_X + EYE_WIDTH + 1, LEFT_EYE_Y + EYE_HEIGHT + 1);
    u8g2.drawTriangle(
        RIGHT_EYE_X - EYE_WIDTH - 1, RIGHT_EYE_Y - EYE_HEIGHT - 1,
        RIGHT_EYE_X + EYE_WIDTH + 1, RIGHT_EYE_Y - EYE_HEIGHT - 1,
        RIGHT_EYE_X - EYE_WIDTH - 1, RIGHT_EYE_Y + EYE_HEIGHT + 1);
    TextString = "DEVELISH";
    break;

  default:
    break;
  }
  // nose
  u8g2.setDrawColor(0);
  u8g2.drawFilledEllipse(NOSE_X, NOSE_Y, 2, 4);

// text
#define txt_X 21
#define txt_Y 12
  u8g2.setDrawColor(0);                 // set draw color to black for the eyes and text
  u8g2.drawBox(0, 0, SCREEN_WIDTH, txt_Y + 5); // clear the entire text area at the top of the display
  u8g2.setDrawColor(1);
  // u8g2.setFont(u8g2_font_7x13_tf);
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(txt_X, txt_Y, TextString);

  u8g2.sendBuffer();
}

void setup()
{
  version = VERSION;
  Serial.begin(115200); // debug interface
  Serial.print("\nNabby-tiny-blink is starting\n");

  connectWifi(); // connect to WiFi access point
                 //  delay(2000);

  // Add the parser commands to the DynamicCommandParser
  dcp_ser.addParser("inf", getInfo);
  dcp_ser.addParser("hlp", printHelp);
  dcp_ser.addParser("mvp", multipleVariableParser);
  printParserCommands();

  dcp_udp.addParser("inf", getInfo);

  gpio_reset_pin(GPIO_NUM_22); // disable old pin function on GPIO22
  gpio_reset_pin(GPIO_NUM_23); // disable old pin function on GPIO23
  ;
  gpio_set_drive_capability(GPIO_NUM_22, GPIO_DRIVE_CAP_2); // Set GPIO22 drive capability to default (medium)
  ;
  gpio_set_drive_capability(GPIO_NUM_23, GPIO_DRIVE_CAP_2); // Set GPIO23 drive capability to default (medium)

  pinMode(LED_BUILTIN, OUTPUT); // GPIO2 is connected to the on-board LED on the XIAO ESP32C6 board
  // pinMode(21, OUTPUT);          // GPIO21 is connected to the external LED connected to the XIAO ESP32C6 board

  pinMode(A0, INPUT); // Configure A0 as ADC input for Battery measurement

  pinMode(22, OUTPUT); // SDA on the XIAO ESP32C6 board
  pinMode(23, OUTPUT); // SCL on the XIAO ESP32C6 board

  Wire.begin(22, 23, 100000); // XIAO ESP32C6: SDA=6, SCL=7
  u8g2.setBusClock(100000);
  u8g2.begin();
  u8g2.clearBuffer();

  u8g2.setFlipMode(1); // flip the display vertically

  u8g2.setDrawColor(0);
  u8g2.drawXBM(10, 20, nabbyBW100x61_width, nabbyBW100x61_height, nabbyBW100x61_bits);

  u8g2.sendBuffer();

  Serial.println("end of setup()");
}

int i = 0;
char c;

void loop()
{
  eyes(i);
  i++;
  if (i > 5)
  {
    i = 0;
  }
  digitalWrite(LED_BUILTIN, HIGH);
  // digitalWrite(21, HIGH);

  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  // digitalWrite(21, LOW);

  uint32_t Vbatt = 0;
  for (int ii = 0; ii < 16; ii++)
  {
    Vbatt += analogReadMilliVolts(A0); // Read and accumulate ADC voltage
  }
  float Vbattf = 2 * Vbatt / 16 / 1000.0; // Adjust for 1:2 divider and convert to volts

  #define BATT_MIN 3.6
  #define BATT_MAX 4.2
  int barWidth = ((Vbattf - BATT_MIN) / (BATT_MAX - BATT_MIN)) * (SCREEN_WIDTH - 10); // Calculate the width of the battery bar based on voltage

  Serial.printf("Battery Voltage: %.3f V   %d\n", Vbattf, barWidth);              // Output voltage to 3 decimal places

  u8g2.setDrawColor(0);
  u8g2.drawBox(10, SCREEN_HEIGHT - 10, SCREEN_WIDTH - 5, 10); // erase bar
  u8g2.setDrawColor(1);
  u8g2.drawBox(10, SCREEN_HEIGHT - 5, barWidth, 3); // draw a bar to indicate battery voltage

  delay(500);

  while (Serial.available() > 0)
  {
    c = Serial.read();
    Serial.print(c);
    dcp_ser.appendChar(c);
  }
  handleUdp(); // handle and parse commands received via UDP
}
