#include "Adafruit_NeoPixel.h"
#include "Bounce2.h"
#include "TaskScheduler.h"

#define NUMPIXELS   1
#define PIN_WS2812  2
#define PIN_ILS     7

Adafruit_NeoPixel neopixel = Adafruit_NeoPixel(NUMPIXELS, PIN_WS2812);
//Button ils = Button();
Bounce2::Button ils = Bounce2::Button();
unsigned long t, previousT;
void check();
Task mainTask(4000, TASK_FOREVER, &check);
Scheduler runner;

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return neopixel.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return neopixel.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return neopixel.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void check() {
  //Serial.println(millis() - previousT);
  if (previousT == NULL) {
    return;
  }
  if (millis() - previousT > 4000) {
    // means we're going at a speed tower than 2km/h; or we've stopped abruptly
    neopixel.setPixelColor(0, Wheel(0));
    neopixel.show();
  }
}

void setup() {
  neopixel.begin();
  neopixel.setBrightness(255);
  neopixel.setPixelColor(0, Wheel(0));
  neopixel.show();

  ils.attach(PIN_ILS, INPUT_PULLUP);
  ils.interval(10);
  // HIGH state corresponds to physically pressing the button:
  ils.setPressedState(HIGH);
  t = millis();

  runner.init();
  runner.addTask(mainTask);
  mainTask.enable();
}

void loop() {
  runner.execute();
  ils.update();

  if (ils.pressed()) {
    previousT = t;
    t = millis();
    //period_heure = (t - previousT)/3600000;
    //diam_km = pi * (28 * 2.54) / 100000; // roue 28 pouces diamètre
    int speed = (3.1416 * 28 * 2.54 * 36) / (t - previousT);
    if (speed > 40) speed = 40; // cap speed

    // 0 to 40 km/h mapped 0 to 255
    neopixel.setPixelColor(0, Wheel(byte(speed*255/40)));
    neopixel.show();
  }
}
