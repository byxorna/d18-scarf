/*
* Project d18-scarf
* Description: LEDs for DI8ORIENT scarf
* Author: Gabe Conradi
* Date: idklol
*/

#include "Particle.h"
#include "FastLED.h"

FASTLED_USING_NAMESPACE;
SYSTEM_MODE(SEMI_AUTOMATIC);
SYSTEM_THREAD(ENABLED);

typedef void (*FP)();

#define NUM_LEDS 83
#define LEDS_PIN D6
#define LED_TYPE NEOPIXEL
#define UPDATES_PER_SECOND 240
#define MAX_BRIGHTNESS 255
#define MAX_SATURATION 255
#define BOOTUP_ANIM_DURATION_MS 2000
#define PATTERN_CHANGE_INTERVAL_MS 60000
#define PALETTE_CHANGE_INTERVAL_MS 30000
#define AUTO_CHANGE_PALETTE 1
bool AUTO_PATTERN_CHANGE = true;
#define GLOBAL_BRIGHTNESS 255
#define SETUP_BUTTON_HOLD_DURATION_MS 800

uint8_t gPattern = 0; // global pattern
uint8_t gPalette = 0; // global palette
uint8_t gAnimIndex = 0; // animation index for ColorFromPalette

unsigned long t_now;                // time now in each loop iteration
unsigned long t_boot;               // time at bootup
unsigned long t_pattern_start = 0;  // time last pattern changed
unsigned long t_palette_start = 0;  // time last palette changed

/* state for controlling user-mode button for pattern changes */
uint8_t button_state = 0;
unsigned long button_timer = 0;

CFastLED* gLED; // global CFastLED object

/* custom color palettes */
// orange 255,102,0 FF6600
// pink 255,0,255 #ff00ff
// pornj 255,51,51 #ff3333
DEFINE_GRADIENT_PALETTE( Disorient_gp ) {
      0,   0,   0,   0,    // black
     75, 255,  26, 153,    // pink
    147, 255,  51,  51,    // pornj
    208, 255, 111,  15,    // orange
    255, 255, 255, 255, }; // white

// for effects that are palette based
CRGBPalette16 currentPalette; // current color palette
CRGBPalette16 palettes[6] = {
  Disorient_gp,
  RainbowColors_p,
  CloudColors_p,
  ForestColors_p,
  OceanColors_p,
  LavaColors_p,
};

TBlendType currentBlending = LINEARBLEND;
CRGB leds[NUM_LEDS];

// setup() runs once, when the device is first turned on.
void setup() {

  Serial.begin(9600);
  Serial.println("resetting");

  currentPalette = palettes[0];

  // led controller, data pin, clock pin, RGB type (RGB is already defined in particle)
  gLED = new CFastLED();
  gLED->addLeds<LED_TYPE, LEDS_PIN>(leds, NUM_LEDS);
  gLED->setBrightness(GLOBAL_BRIGHTNESS);
  pattern_clear();
  gLED->show();

  t_boot = millis();
  Serial.println("booted up");
}

void pattern_slow_pulse_with_sparkles() {
  // pick a color, and pulse it 
  uint8_t cBrightness = beatsin8(20, 120, 255);
  uint8_t cHue = beatsin8(4, 0, 255);
  CHSV hsv_led = CHSV(cHue, 255, cBrightness);
  CRGB rgb_led;
  hsv2rgb_rainbow(hsv_led, rgb_led);
  for( int i = 0; i < NUM_LEDS; i++) {
    if (random(NUM_LEDS*3) == 0) {
      leds[i] = CRGB::White;
    } else {
      leds[i] = rgb_led;
    }
  }
}

void pattern_cylon_eye() {
  // cylon eye is 4 pixels wide, +/++ base index
  // we map a 60bpm(1s) cycle into 0..num leds-1
  uint8_t h = beatsin8(8, 0, 255);
  CHSV hsv_led = CHSV(h, 255, 255);
  CRGB rgb_led;
  hsv2rgb_rainbow(hsv_led, rgb_led);
  uint8_t mappedIndex = beatsin8(60, 0, NUM_LEDS-1);
  for(int i = 0; i < NUM_LEDS; ++i) {
    if (mappedIndex == i) {
      leds[i] = rgb_led;
    } else if (addmod8(mappedIndex, 1, 255) == i) {
      leds[i] = rgb_led;
    } else if (addmod8(mappedIndex, 2, 255) == i) {
      leds[i] = rgb_led;
    } else if (addmod8(mappedIndex, 3, 255) == i) {
      leds[i] = rgb_led;
    } else {
      leds[i] = CRGB::Black;
    }
  }
}

void pattern_bootup_with_sparkles() {
  uint8_t baseHue = beatsin8(15, 0, 255);
  uint8_t iHue = 0;
  for(int i = 0; i < NUM_LEDS; ++i) {
    if (random(NUM_LEDS) == 0) {
      leds[i] = CRGB::White;
    } else {
      iHue = addmod8(baseHue, 1, 255);
      CHSV hsv_led = CHSV(iHue, 255, 255);
      CRGB rgb_led;
      hsv2rgb_rainbow(hsv_led, rgb_led);
      leds[i] = rgb_led;
    }
  }
}

// cycle a rainbow, varying how quickly it rolls around the board
void pattern_rainbow_waves_with_sparkles() {
  for(int i = 0; i < NUM_LEDS; ++i) {
    if (random(NUM_LEDS*3) == 0) {
      leds[i] = CRGB::White;
    } else {
      uint8_t h = (t_now/12+i)%256;
      CHSV hsv_led = CHSV(h, 255, 255);
      CRGB rgb_led;
      hsv2rgb_rainbow(hsv_led, rgb_led);
      leds[i] = rgb_led;
    }
  }
}

void pattern_clear() {
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
}
void pattern_from_palette() {
  uint8_t b = beatsin8(4, 0, 255);
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, gAnimIndex + i + b, MAX_BRIGHTNESS, currentBlending);
  }
  // slow down progression by 1/3
  if (t_now%3 == 0) {
    gAnimIndex = addmod8(gAnimIndex, 1, 255);
  }
}

void pattern_brake_light() {
  for (int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = CRGB::Red;
  }
}

// NOTE: lifted and tweaked from https://learn.adafruit.com/rainbow-chakra-led-hoodie/the-code
// This function draws color waves with an ever-changing,
// widely-varying set of parameters, using a color palette.
void pattern_palette_waves() {
  uint8_t numleds = NUM_LEDS;
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  //uint8_t sat8 = beatsin88( 87, 220, 250);
  uint8_t brightdepth = beatsin88( 341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88( 203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 300, 1500);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis ;
  sLastMillis  = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88( 400, 5,9);
  uint16_t brightnesstheta16 = sPseudotime;

  for( uint16_t i = 0 ; i < numleds; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;
    uint16_t h16_128 = hue16 >> 7;
    if( h16_128 & 0x100) {
      hue8 = 255 - (h16_128 >> 1);
    } else {
      hue8 = h16_128 >> 1;
    }

    brightnesstheta16  += brightnessthetainc16;
    uint16_t b16 = sin16( brightnesstheta16  ) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    uint8_t index = hue8;
    index = scale8( index, 240);

    CRGB newcolor = ColorFromPalette(currentPalette, index, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (numleds-1) - pixelnumber;

    nblend(leds[pixelnumber], newcolor, 128);
  }
}

/** update this with patterns you want to be cycled through **/
#define NUM_PATTERNS sizeof(patternBank) / sizeof(FP)
const FP patternBank[] = {
  &pattern_from_palette,
  &pattern_slow_pulse_with_sparkles,
  &pattern_palette_waves,
  &pattern_rainbow_waves_with_sparkles,
};

void loop() {
  t_now = millis();

  // handle user interaction with reset button
  if (HAL_Core_Mode_Button_Pressed(SETUP_BUTTON_HOLD_DURATION_MS)) {
    switch (button_state) {
    case 0:
      // we werent pressed before, so start timer!
      button_state = 1;
      button_timer = t_now;
      //RGB.color(255, 0, 0);
      break;
    case 1:
      //RGB.color(0, 255, 0); // green when waiting
      if (t_now - button_timer > SETUP_BUTTON_HOLD_DURATION_MS) {
        // we have been held longer than
        button_state = 2;
      }
      break;
    // we are waiting to take action now
    case 2: break;
    // action already taken, do nothing until release!
    case 3: break;
    default:
        button_state = 0;
      break;
    }
  } else {
    button_state = 0;
  }
  if (button_state == 2) {
    // disable auto pattern changing now!
    AUTO_PATTERN_CHANGE = false;
    ++gPattern;
    button_state = 3;
  }

  // TODO: make sure patterns and pallettes change together
  // increment pattern every PATTERN_CHANGE_INTERVAL_MS
  if (AUTO_PATTERN_CHANGE) {
    if (t_now > t_pattern_start+PATTERN_CHANGE_INTERVAL_MS) {
      gPattern++;
      t_pattern_start = t_now;
      Serial.printlnf("auto pattern->%d", gPattern);
    }
  }

  // increment palette every PALETTE_CHANGE_INTERVAL_MS
  if (AUTO_CHANGE_PALETTE && (t_now > t_palette_start+PALETTE_CHANGE_INTERVAL_MS)) {
    gPalette++;
    if (gPalette >= (sizeof(palettes)/sizeof(*palettes))) {
      gPalette = 0;
    }
    currentPalette = palettes[gPalette];
    Serial.printlnf("palette->%d", gPalette);
    t_palette_start = t_now;
  }

  if (t_boot + BOOTUP_ANIM_DURATION_MS > t_now) {
    // display a bootup pattern for a bit
    pattern_bootup_with_sparkles();
  } else {
    if (gPattern < NUM_PATTERNS) {
      patternBank[gPattern]();
    } else {
      gPattern = 0;
    }
  }

  gLED->show();
  delay(1000 / UPDATES_PER_SECOND);
}
