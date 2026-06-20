//
// Nabby-tiny ESP32 based by RKB  
// Board used: ESP32Dev module. Baudrate: 115200
// (COM4 on ACER and HP, right-usb-poort; not all usb-cables are OK)
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
#include <dfplayer.h>
#include <DynamicCommandParser.h>
#include <Parsers.h>
#include <Watchdog.h>
#include <Nabby.h>

// #include <freertos/timers.h>

#define VERSION "20Jun2026a"
String version;

#define MP3_SERIAL_SPEED 9600  // DFPlayer Mini suport only 9600-baud
#define MP3_SERIAL_TIMEOUT 100 // average DFPlayer response timeout 100msec..200msec
DFPlayer mp3;                  // connect DFPlayer RX-pin to GPIO15(TX) & DFPlayer TX-pin to GPIO13(RX)
uint8_t response = 0;
#define RXD2 16
#define TXD2 17

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

void connectWifi()
{
  WiFi.mode(WIFI_STA); // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.disconnect();
  delay(100);

  wifiMulti.addAP(SSID1, PSW1);
  // wifiMulti.addAP(SSID2, PSW2); // more routers can be registered, strongest RSSI will be selected. SSID needs definition in secret.h
  // wifiMulti.addAP(SSID3, PSW3); // more routers can be registered, strongest RSSI will be selected. SSID needs definition in secret.h

  Serial.print("Connecting Wifi -");
  while (wifiMulti.run() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to network");

  Serial.print("WiFi connected: ");
  Serial.print(WiFi.SSID());
  Serial.print(";   IP addr: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("Nabby-mini-speaker"))
  {
    MDNS.addService("mydoorbell", "udp", 1235); // Announce service on port x
    Serial.println("MDNS responder started");
  }
}

int finddoorbell(void) // find doorbell and send scan MDNS command
{
  WiFiUDP wifiudp;
  int n = MDNS.queryService("doorbell", "udp"); // Send query for mydoorbell services
 // Serial.println("mDNS query done");
  if (n > 0)
  {
    // send MDNS scan command
    wifiudp.beginPacket(MDNS.IP(0), MDNS.port(0)); // send udp packet to doorbell
    wifiudp.print("/mdns\r");
    wifiudp.endPacket();
    delay(100);
  }
  return (n);
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

void setup()
{
  version = VERSION;
  Serial.begin(115200); // debug interface
  Serial.print("\nNabby-tiny is starting\n");

  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); // MP# interface connection

  connectWifi(); // connect to WiFi access point
                 //  delay(2000);
  int nr = finddoorbell();
  if (nr == 0)
    Serial.println("no MDNS 'doorbell' services found");
  else
  {
    Serial.printf("Found %d 'doorbell' services", nr);
    Serial.println("=> Sending MDNS scan cmd to doorbell\n"); // Request all found doorbell units to perform an mdns scan. This will register the Nabby in the doorbell unit
  }

  mp3.begin(Serial2, MP3_SERIAL_TIMEOUT, DFPLAYER_MINI, false); // DFPLAYER_MINI see NOTE, false=no response from module after the command
  mp3.stop();                                                   // if player was runing during ESP8266 reboot
  delay(100);
  mp3.reset(); // reset all setting to default
  delay(100);
  mp3.setSource(2);  // 1=USB-Disk, 2=TF-Card, 3=Aux, 4=Sleep, 5=NOR Flash
                     // delay(500);
  mp3.setVolume(25); // 0..30, module persists volume on power failure
  delay(500);
  mp3.setEQ(2);
  delay(500);
  mp3.wakeup(2); // exit standby mode & initialize sourse 1=USB-Disk, 2=TF-Card, 3=Aux, 5=NOR Flash
  delay(500);

  // Add the parser commands to the DynamicCommandParser
  dcp_ser.addParser("inf", getInfo);
  dcp_ser.addParser("hlp", printHelp);
  dcp_ser.addParser("mvp", multipleVariableParser);
  dcp_ser.addParser("tra", selectTrack);
  dcp_ser.addParser("all", playAllTracks);
  dcp_ser.addParser("vol", setVolume);
  dcp_ser.addParser("rng", RingBell);
  dcp_ser.addParser("png", Ping);
  dcp_ser.addParser("lft", LeftEar);
  dcp_ser.addParser("rgt", RightEar);
  dcp_ser.addParser("stp", StopEars);
  printParserCommands();

  dcp_udp.addParser("inf", getInfo);
  dcp_udp.addParser("tra", selectTrack);
  dcp_udp.addParser("all", playAllTracks);
  dcp_udp.addParser("vol", setVolume);
  dcp_udp.addParser("rng", RingBell);
  dcp_udp.addParser("png", Ping);
  dcp_udp.addParser("lft", LeftEar);
  dcp_udp.addParser("rgt", RightEar);
  dcp_udp.addParser("stp", StopEars);

  initNabby();

  wdInit(60000); // initilize watchdog timeout on 60 seconds

  Serial.println("end of setup()");
}

int i = 0;
char c;
unsigned long lastWDtrigger = 0;

void loop()
{
  blinkLeds();
  while (Serial.available() > 0)
  {
    c = Serial.read();
    Serial.print(c);
    dcp_ser.appendChar(c);
    //   dcp_ser.appendChar(c);
  }
  handleUdp(); // handle and parse commands received via UDP

  if (millis() - lastWDtrigger >= 10000) // check watchdog every 10 second. This is same timeinterval for mdnsScan
  {
    lastWDtrigger = millis();

    if (wdStatus())
    {
      //  Serial.printf("... Watchdog Expired\n");
      int nr = finddoorbell();
      //  if (nr == 0)
      //   Serial.println("no MDNS 'doorbell' services found");

      // else
      //      Serial.printf("Found %d 'doorbell' services", nr);
      //   Serial.println("=> Sending MDNS scan cmd to doorbell\n"); // Request all found doorbell units to perform an mdns scan. This will register the Nabby in the doorbell unit
    }
  }
}
