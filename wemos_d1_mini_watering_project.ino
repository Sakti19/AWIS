#include <TM1637Display.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <FS.h>
#include <LittleFS.h>

#ifndef SSID
#define SSID "singtel"
#define PSK "WhiteHorse19"
#endif

#define CLK D6
#define DIO D5
#define RLY D1


const char *ssid = SSID;
const char *password = PSK;


const int slotTime1 = 800;
const int slotSec1 = 10;
const int slotTime2 = 1100;
const int slotSec2 = 7;
const int slotTime3 = 1400;
const int slotSec3 = 5;
const int resetTime = 1600;

bool slotDone1, slotDone2, slotDone3;

ESP8266WiFiMulti WiFiMulti;
TM1637Display display = TM1637Display(CLK, DIO);
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP);
WiFiServer server(80);

void setup() {
  pinMode(RLY, OUTPUT);
  digitalWrite(RLY, HIGH);

  display.clear();
  display.setBrightness(7);  // set the brightness to 7 (0:dimmest, 7:brightest)

  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);
  Serial.print("Wait for WiFi... ");

  while (WiFiMulti.run(60000) != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  ntpClient.begin();

  delay(1000);
  ntpClient.update();
  ntpClient.setTimeOffset(19800);
  delay(1000);

  server.begin();
  Serial.println(F("Server started"));

  // Print the IP address
  Serial.println(WiFi.localIP());
}

void loop() {

  webserverRead();

  ntpClient.update();
  int localTime = ntpClient.getHours() * 100 + ntpClient.getMinutes();

  display.clear();
  display.showNumberDecEx(localTime, 0b11100000, false, 4, 0);

  if (!slotDone1 && (localTime >= slotTime1 && localTime < slotTime2)) {
    startRelay(slotSec1);
    slotDone1 = true;
    Serial.println("<><><><><>Slot 1 Done<><><><><> " + ntpClient.getFormattedTime());
  } else if (!slotDone2 && (localTime >= slotTime2 && localTime < slotTime3)) {
    startRelay(slotSec2);
    slotDone2 = true;
    Serial.println("<><><><><>Slot 2 Done<><><><><> " + ntpClient.getFormattedTime());
  } else if (!slotDone3 && (localTime >= slotTime3 && localTime < resetTime)) {
    startRelay(slotSec3);
    slotDone3 = true;
    Serial.println("<><><><><>Slot 3 Done<><><><><> " + ntpClient.getFormattedTime());
  } else if ((slotDone1 || slotDone2 || slotDone3) && localTime >= resetTime) {
    resetSlots();
  } else {
    Serial.println("< slot1 = " + String(slotDone1) + " > " + "< slot2 = " + String(slotDone2) + " > " + "< slot3 = " + String(slotDone3) + " > " + ntpClient.getFormattedTime());
    delay(1000);
  }
}

void webserverRead() {
  WiFiClient client = server.accept();
  if (!client) { return; }
  Serial.println(F("new client"));

  client.setTimeout(5000);  // default is 1000

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(F("request: "));
  Serial.println(req);


  while (client.available()) {
    // byte by byte is not very efficient
    client.read();
  }

  
  //client.print((val) ? F("high") : F("low"));
  client.print(F("<html>"));
  client.print(F("<br><br>Click <a href='http://"));
  client.print(WiFi.localIP());
  client.print(F("/gpio/1'>here</a> to switch LED GPIO on, or <a href='http://"));
  client.print(WiFi.localIP());
  client.print(F("/gpio/0'>here</a> to switch LED GPIO off."));

  client.print(F("</html>"));
}

void resetSlots() {
  Serial.println(">>>>Reset<<<<< " + ntpClient.getFormattedTime());
  slotDone1 = false;
  slotDone2 = false;
  slotDone3 = false;
}


void startRelay(int periodSec) {
  Serial.println("+++++Relay On+++++ " + ntpClient.getFormattedTime());
  digitalWrite(RLY, LOW);
  delay(periodSec * 1000);
  digitalWrite(RLY, HIGH);
  Serial.println("-----Relay Off---- " + ntpClient.getFormattedTime());
}
