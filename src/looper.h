#pragma once
#include <Arduino.h>

enum LooperState : uint8_t {
    LOOP_CLEAR = 0,     // blue — no loop recorded
    LOOP_RECORDING,     // red — recording first loop
    LOOP_PLAYING,       // green — playing back
    LOOP_OVERDUBBING,   // red — overdubbing onto loop
    LOOP_PAUSED         // purple — paused
};

struct LooperColor {
    uint8_t r, g, b;
};

void looper_init();

// Call when a button is pressed (0-3). Returns true if a MIDI message should be sent.
bool looper_press(uint8_t button);

// Current state
LooperState looper_state();

// Current LED color (accounts for white flash)
LooperColor looper_led_color();

// Call every loop iteration to handle flash timing
void looper_update();

// Check if drum route CC should be sent (returns 1=on, -1=off, 0=no change)
int looper_drum_route_event();

// Current FX chain (0-3), changed by button 2 long-cycling
uint8_t looper_fx_chain();
