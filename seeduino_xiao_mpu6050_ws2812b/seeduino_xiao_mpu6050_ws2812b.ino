#include "Wire.h"
#include "MPU6050.h"
#include "Adafruit_NeoPixel.h"
#include "arduinoFFT.h"

#define NUMPIXELS  1
#define PIN_WS2812  2

arduinoFFT FFT = arduinoFFT();
MPU6050 accelgyro;
Adafruit_NeoPixel neopixel = Adafruit_NeoPixel(NUMPIXELS, PIN_WS2812);

int16_t gx, gy, gz = 0;

/* Sampling frequency is chosen so that:
 *  - it is superior to twice the highest wheel frequency (cf Shannon Nyquist)
 *  - it is kept to a minimum (so we don't overload our small processor)
 *
 * At 16 Hz, we can sample 8 wheel cycles per second, which translates to 
 * 3.1416 * 28 * 2.54 * 8 * 0.00001 * 3600 = 64 km/h
 * Should be enough ;-)
 */
const double samplingFrequency = 16; // Hz

/*  
 * Samples should be a power of 2 (for FFT):
 * and the full sample should be long enough to hold one full cycle at the lowest speed (say 4 km/h).
 * 
 * One wheel turn is 3.1416 * 28 * 2.54 * 0.00001 km
 * At 4km/h the wheel does one full cycle in 3600 * 3.1416 * 28 * 2.54 * 0.00001 / 4 = 2 seconds
 * One sample lasts 1/16 second according to the above
 * which translates to ...
 */
const uint16_t samples = 32;

double valgx[3];
double valgy[3];
double valgz[samples];
double val[samples];
double vImagRef[samples];
double vImag[samples];


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


void setup() {
  Wire.begin(); // I2C bus
  Serial.begin(9600);
  //while (!Serial) ;

  accelgyro.initialize();

  neopixel.begin();
  neopixel.setBrightness(255);
  neopixel.setPixelColor(0, Wheel(0));
  neopixel.show();

  for (byte i = 0; i < samples; i = i + 1) {
     vImagRef[i] = 0;
  }
}


void loop() {
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // shift array to leave room for the latest value:
  memcpy(valgz, &valgz[1], sizeof(valgz) - sizeof(double));
  valgz[samples-1] =  (double)gz;

  memcpy(valgx, &valgx[1], sizeof(valgx) - sizeof(double));
  valgx[2] =  (double)gx;

  memcpy(valgy, &valgy[1], sizeof(valgy) - sizeof(double));
  valgy[2] =  (double)gy;

  if (sqrt(sq(valgx[2])+sq(valgy[2])+sq(valgz[samples-1]))
     +sqrt(sq(valgx[1])+sq(valgy[1])+sq(valgz[samples-2]))
     +sqrt(sq(valgx[0])+sq(valgy[0])+sq(valgz[samples-3])) > 2000) {
 
      memcpy(vImag, &vImagRef[0], sizeof(vImagRef));
      memcpy(val, &valgz[0], sizeof(valgz));
      FFT.Windowing(val, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
      FFT.Compute(val, vImag, samples, FFT_FORWARD);
      FFT.ComplexToMagnitude(val, vImag, samples);
      double x = FFT.MajorPeak(val, samples, samplingFrequency);

      neopixel.setPixelColor(0, Wheel(byte(x*255/(samplingFrequency/2))));
      neopixel.show();
  } else {
      neopixel.setPixelColor(0, Wheel(0));
      neopixel.show();
  }
  delay(round((1/samplingFrequency)*1000));
}
