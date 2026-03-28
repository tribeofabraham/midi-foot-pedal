#pragma once
#include <Arduino.h>

#define NUM_BUTTONS 4

enum ActionType : uint8_t {
    ACTION_MIDI_CC = 0,
    ACTION_MIDI_NOTE,
    ACTION_MIDI_PC,
    ACTION_KEYBOARD,
    ACTION_OSC,
    ACTION_NONE
};

enum ButtonBehavior : uint8_t {
    BEHAVIOR_MOMENTARY = 0,
    BEHAVIOR_TOGGLE
};

struct ButtonConfig {
    ActionType   type       = ACTION_MIDI_CC;
    ButtonBehavior behavior = BEHAVIOR_MOMENTARY;

    // LED colors (single NeoPixel shows active button state)
    uint8_t color_r = 0;
    uint8_t color_g = 40;
    uint8_t color_b = 80;
    uint8_t color_on_r = 255;
    uint8_t color_on_g = 255;
    uint8_t color_on_b = 255;

    // MIDI
    uint8_t midi_channel   = 1;   // 1-16
    uint8_t midi_cc        = 1;   // CC number
    uint8_t midi_cc_on     = 127; // CC value on press/toggle on
    uint8_t midi_cc_off    = 0;   // CC value on release/toggle off
    uint8_t midi_note      = 60;  // note number
    uint8_t midi_velocity  = 127;
    uint8_t midi_program   = 0;   // program change number

    // Keyboard
    uint8_t key_modifiers  = 0;   // bitmask: bit0=LCtrl, 1=LShift, 2=LAlt, 3=LGUI
    uint8_t key_code       = 0;   // ASCII or KEY_* constant

    // OSC (stored as short strings)
    char    osc_addr[48]   = "/button/1";
    float   osc_on         = 1.0f;
    float   osc_off        = 0.0f;
};

struct PedalConfig {
    ButtonConfig buttons[NUM_BUTTONS];
    uint8_t led_brightness = 30;
};

void config_init();
void config_loop();
PedalConfig& config_get();
void config_save();
