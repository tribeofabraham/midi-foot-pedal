#pragma once
#include <Adafruit_NeoPixel.h>

// WS2812B driven via RMT peripheral (automatic on ESP32 with Adafruit_NeoPixel >= 1.12)
// This avoids disabling interrupts, so USB MIDI stays stable.

#define LED_PIN       48   // ESP32-S3 Super Mini onboard NeoPixel (GPIO48)
#define LED_COUNT     1

void leds_init();
void leds_set(uint8_t r, uint8_t g, uint8_t b);
void leds_off();
void leds_update();

// Preset colors for MIDI feedback
void leds_flash_send();    // brief flash on MIDI send
void leds_idle();          // dim steady color when connected
void leds_disconnected();  // pulsing red when no USB host
