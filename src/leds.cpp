#include "leds.h"

static Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// State for non-blocking flash
static bool     flashing    = false;
static uint32_t flash_start = 0;
static const uint32_t FLASH_MS = 60;

// State for disconnected pulse
static uint8_t pulse_brightness = 0;
static int8_t  pulse_dir        = 1;

void leds_init() {
    strip.begin();
    strip.setBrightness(30);  // keep it gentle
    strip.show();
}

void leds_set(uint8_t r, uint8_t g, uint8_t b) {
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
}

void leds_off() {
    strip.setPixelColor(0, 0);
    strip.show();
}

void leds_flash_send() {
    // Bright white flash — call once per MIDI send, leds_update() handles timeout
    strip.setBrightness(80);
    leds_set(255, 255, 255);
    flashing = true;
    flash_start = millis();
}

void leds_idle() {
    flashing = false;
    strip.setBrightness(30);
    leds_set(0, 40, 80);  // calm blue
}

void leds_disconnected() {
    // Called from leds_update loop — gentle red pulse
    flashing = false;
    pulse_brightness += pulse_dir * 2;
    if (pulse_brightness >= 60)  pulse_dir = -1;
    if (pulse_brightness <= 2)   pulse_dir = 1;
    strip.setBrightness(pulse_brightness);
    leds_set(255, 0, 0);
}

void leds_update() {
    if (flashing && (millis() - flash_start > FLASH_MS)) {
        // Flash done, return to idle
        leds_idle();
    }
}
