#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WifiManager.h>
#include <Ticker.h>

#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

#include <GithubStatus.h>

GithubStatus status;

Ticker ticker;
int needsUpdate = true;
RgbColor statusColor;

const uint16_t PixelCount = 12;
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount);
const uint8_t AnimationChannels = 1; // we only need one as all the pixels are animated at once
NeoPixelAnimator animations(AnimationChannels); // NeoPixel animation management object
uint16_t effectState = 0;  // general purpose variable used to store effect state

#define colorSaturation 128
RgbColor green(0, colorSaturation, 0);
RgbColor darkgreen(HtmlColor(0x285900));
RgbColor yellow(HtmlColor(0xffd800));
RgbColor red(colorSaturation, 0, 0);
RgbColor black(0);


struct MyAnimationState {
  RgbColor StartingColor;
  RgbColor EndingColor;
};
MyAnimationState animationState[AnimationChannels];

void SetRandomSeed() {
  uint32_t seed;

  // random works best with a seed that can use 31 bits
  // analogRead on a unconnected pin tends toward less than four bits
  seed = analogRead(0);
  delay(1);

  for (int shifts = 3; shifts < 31; shifts += 3) {
    seed ^= analogRead(0) << shifts;
    delay(1);
  }

  randomSeed(seed);
}

void UpdateAnim(AnimationParam param) {
  // apply a exponential curve to both front and back
  float progress = NeoEase::ExponentialInOut(param.progress);
  // lerp between Red and Green
  RgbColor color = RgbColor::LinearBlend(RgbColor(255,0,0), RgbColor(0,255,0), progress);
}

// simple blend function
void BlendAnimUpdate(const AnimationParam& param) {
  // this gets called for each animation on every time step
  // progress will start at 0.0 and end at 1.0
  // we use the blend function on the RgbColor to mix
  // color based on the progress given to us in the animation
  RgbColor updatedColor = RgbColor::LinearBlend(
      animationState[param.index].StartingColor,
      animationState[param.index].EndingColor,
      param.progress);

  // apply the color to the strip
  for (uint16_t pixel = 0; pixel < PixelCount; pixel++) {
    strip.SetPixelColor(pixel, updatedColor);
  }
}

void FadeInFadeOutRinseRepeat(float luminance, uint16_t time) {
  if (effectState == 0) {
    // Fade upto a random color
    // we use HslColor object as it allows us to easily pick a hue
    // with the same saturation and luminance so the colors picked
    // will have similiar overall brightness
    RgbColor target = statusColor; // HslColor(random(360) / 360.0f, 1.0f, luminance);

    animationState[0].StartingColor = strip.GetPixelColor(0);
    animationState[0].EndingColor = target;

    animations.StartAnimation(0, time, BlendAnimUpdate);
  } else if (effectState == 1) {
    // fade to black
    uint16_t time = random(600, 700);

    animationState[0].StartingColor = strip.GetPixelColor(0);
    animationState[0].EndingColor = RgbColor(0);

    animations.StartAnimation(0, time, BlendAnimUpdate);
  }

  // toggle to the next effect state
  effectState = (effectState + 1) % 2;
}

void FadeIn(float luminance)
{
  effectState = 0;
  // Fade upto a random color
  // we use HslColor object as it allows us to easily pick a hue
  // with the same saturation and luminance so the colors picked
  // will have similiar overall brightness
  RgbColor target = statusColor; // HslColor(random(360) / 360.0f, 1.0f, luminance);
  uint16_t time = random(800, 2000);

  animationState[0].StartingColor = strip.GetPixelColor(0);
  animationState[0].EndingColor = target;

  animations.StartAnimation(0, time, BlendAnimUpdate);
}

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
  SetRandomSeed();

  Serial.begin(115200);
  // wait for serial monitor to open
  while(! Serial);

  strip.Begin();
  statusColor = black;
  setLedColor(statusColor);
  strip.Show();

  pinMode(BUILTIN_LED, OUTPUT);
  setupWifi();
  digitalWrite(BUILTIN_LED, LOW);

  ticker.attach(30, setNeedsUpdate);
}

void loop() {
  if (animations.IsAnimating()) {
    // the normal loop just needs these two to run the active animations
    animations.UpdateAnimations();
    strip.Show();
  } else {
    if (status.getStatus() == "good") {
      statusColor = green;
      FadeIn(0.2f);
    } else if (status.getStatus() == "minor") {
      statusColor = yellow;
      FadeInFadeOutRinseRepeat(0.2f, 3000);
    } else if (status.getStatus() == "major") {
      statusColor = red;
      FadeInFadeOutRinseRepeat(0.2f, 1500);
    } else if (status.getStatus() == "unknown") {
      statusColor = black;
      FadeIn(0.2f);
    }
  }

  if (needsUpdate) {
    status.update();
    needsUpdate = false;
    Serial.print("status: ");
    Serial.println(status.getStatus());
  }
}
