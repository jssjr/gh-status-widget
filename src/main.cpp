#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WifiManager.h>
#include <Ticker.h>

#include <NeoPixelBus.h>

#include <GithubStatus.h>

GithubStatus status;

Ticker ticker;
int needsUpdate = true;
RgbColor statusColor;

const uint16_t PixelCount = 12;
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount);

#define colorSaturation 128
RgbColor green(0, colorSaturation, 0);
RgbColor darkgreen(HtmlColor(0x285900));
RgbColor yellow(HtmlColor(0xffd800));
RgbColor red(colorSaturation, 0, 0);
RgbColor black(0);

void tick() {
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

void setupWifi() {
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);
  delay(1000);
  // We start by connecting to a WiFi network
  WiFiManager wifiManager;

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
  Serial.println("connected...");
  ticker.detach();
}

void setNeedsUpdate() {
  needsUpdate = true;
}

void setLedColor(RgbColor color) {
  for (int i = 0 ; i < PixelCount ; i++) {
    strip.SetPixelColor(i, color);
  }

}

void setup() {
  Serial.begin(115200);
  // wait for serial monitor to open
  while(! Serial);

  strip.Begin();
  statusColor = darkgreen;
  setLedColor(statusColor);
  strip.Show();

  pinMode(BUILTIN_LED, OUTPUT);
  setupWifi();
  digitalWrite(BUILTIN_LED, LOW);

  ticker.attach(30, setNeedsUpdate);
}

void loop() {

  if (needsUpdate) {
    status.update();
    needsUpdate = false;
    Serial.print("status: ");
    Serial.println(status.getStatus());
  }

  if (status.getStatus() == "good") {
    statusColor = green;
  } else if (status.getStatus() == "minor") {
    statusColor = yellow;
  } else if (status.getStatus() == "major") {
    statusColor = red;
  }

  setLedColor(statusColor);

  strip.Show();
  delay(500);
}
