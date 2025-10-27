// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// Released under the GPLv3 license to match the rest of the
// Adafruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        6 // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 16 // Popular NeoPixel ring size

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels

void setup() {
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
}

  void loop() {
  rainbowCycle(10);
  theaterChase(pixels.Color(0,255,0), 100);
  breathing(10);
  larsonScanner(pixels.Color(255,0,0), 50);
  for (int i = 0; i < 50; i++) sparkle(100);
}

void rainbowCycle(uint8_t wait) {
  for (long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for (int i = 0; i < NUMPIXELS; i++) {
      int pixelHue = firstPixelHue + (i * 65536L / NUMPIXELS);
      pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(pixelHue)));
    }
    pixels.show();
    delay(wait);
  }
}

void theaterChase(uint32_t c, int wait) {
  for (int a = 0; a < 10; a++) { 
    for (int b = 0; b < 3; b++) { 
      pixels.clear();
      for (int c1 = b; c1 < NUMPIXELS; c1 += 3) {
        pixels.setPixelColor(c1, c);
      }
      pixels.show();
      delay(wait);
    }
  }
}

void breathing(uint8_t wait) {
  for (int b = 0; b < 256; b++) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(b, 0, 255-b)); 
    }
    pixels.show();
    delay(wait);
  }
  for (int b = 255; b >= 0; b--) {
    for (int i = 0; i < NUMPIXELS; i++) {
      pixels.setPixelColor(i, pixels.Color(b, 0, 255-b));
    }
    pixels.show();
    delay(wait);
  }
}


void larsonScanner(uint32_t c, int wait) {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.clear();
    pixels.setPixelColor(i, c);
    if (i > 0) pixels.setPixelColor(i-1, pixels.Color(50, 0, 0)); // tail
    pixels.show();
    delay(wait);
  }
  for (int i = NUMPIXELS-2; i >= 0; i--) {
    pixels.clear();
    pixels.setPixelColor(i, c);
    if (i < NUMPIXELS-1) pixels.setPixelColor(i+1, pixels.Color(50, 0, 0));
    pixels.show();
    delay(wait);
  }
}


void sparkle(int wait) {
  pixels.clear();
  int idx = random(NUMPIXELS);
  pixels.setPixelColor(idx, pixels.Color(random(255), random(255), random(255)));
  pixels.show();
  delay(wait);
}

