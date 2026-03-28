// MIDI Foot Pedal — USB Composite (MIDI + HID Keyboard) + NeoPixel + WiFi

#include <Arduino.h>
#include "USB.h"
#include "USBMIDI.h"
#include "USBHIDKeyboard.h"
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "looper.h"

// USB composite device
USBMIDI MIDI;
USBHIDKeyboard Keyboard;

// NeoPixel on GPIO13
#define LED_PIN   13
#define LED_COUNT 1
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Footswitch pins — active low with internal pull-ups
// Left to right: Record(9), Pause/Play(10), Undo(11), Reset(12)
static const uint8_t BTN_PINS[NUM_BUTTONS] = { 9, 10, 11, 12 };

// Debounce state
static const uint32_t DEBOUNCE_MS = 30;
static uint32_t btn_last_change[NUM_BUTTONS] = {};
static bool     btn_state[NUM_BUTTONS] = {};      // true = pressed
static bool     btn_prev_raw[NUM_BUTTONS] = {};

// Toggle state for toggle-mode buttons
static bool     btn_toggle_on[NUM_BUTTONS] = {};

// Called from web UI test buttons or physical buttons
void handle_button_press(uint8_t btn) {
    if (btn >= NUM_BUTTONS) return;
    PedalConfig& cfg = config_get();
    ButtonConfig& b = cfg.buttons[btn];

    // Looper state machine handles LED
    looper_press(btn);

    // For toggle buttons, flip state
    if (b.behavior == BEHAVIOR_TOGGLE) {
        btn_toggle_on[btn] = !btn_toggle_on[btn];
    }

    // Send the configured action
    switch (b.type) {
        case ACTION_MIDI_CC:
            if (b.behavior == BEHAVIOR_TOGGLE) {
                MIDI.controlChange(b.midi_cc,
                    btn_toggle_on[btn] ? b.midi_cc_on : b.midi_cc_off,
                    b.midi_channel);
            } else {
                MIDI.controlChange(b.midi_cc, b.midi_cc_on, b.midi_channel);
            }
            break;
        case ACTION_MIDI_NOTE:
            if (b.behavior == BEHAVIOR_TOGGLE) {
                if (btn_toggle_on[btn])
                    MIDI.noteOn(b.midi_note, b.midi_velocity, b.midi_channel);
                else
                    MIDI.noteOff(b.midi_note, 0, b.midi_channel);
            } else {
                MIDI.noteOn(b.midi_note, b.midi_velocity, b.midi_channel);
            }
            break;
        case ACTION_MIDI_PC:
            MIDI.programChange(b.midi_program, b.midi_channel);
            break;
        case ACTION_KEYBOARD:
            if (b.key_modifiers & 1) Keyboard.press(KEY_LEFT_CTRL);
            if (b.key_modifiers & 2) Keyboard.press(KEY_LEFT_SHIFT);
            if (b.key_modifiers & 4) Keyboard.press(KEY_LEFT_ALT);
            if (b.key_modifiers & 8) Keyboard.press(KEY_LEFT_GUI);
            if (b.key_code) Keyboard.press(b.key_code);
            delay(10);
            Keyboard.releaseAll();
            break;
        case ACTION_NONE:
        default:
            break;
    }
}

// Called on physical button release (momentary modes)
void handle_button_release(uint8_t btn) {
    if (btn >= NUM_BUTTONS) return;
    PedalConfig& cfg = config_get();
    ButtonConfig& b = cfg.buttons[btn];

    if (b.behavior != BEHAVIOR_MOMENTARY) return;

    if (b.type == ACTION_MIDI_NOTE) {
        MIDI.noteOff(b.midi_note, 0, b.midi_channel);
    }
    if (b.type == ACTION_MIDI_CC) {
        MIDI.controlChange(b.midi_cc, b.midi_cc_off, b.midi_channel);
    }
}

// Read and debounce physical buttons
void read_buttons() {
    uint32_t now = millis();
    for (int i = 0; i < NUM_BUTTONS; i++) {
        bool raw = !digitalRead(BTN_PINS[i]);  // active low

        if (raw != btn_prev_raw[i]) {
            btn_last_change[i] = now;
            btn_prev_raw[i] = raw;
        }

        if ((now - btn_last_change[i]) >= DEBOUNCE_MS) {
            if (raw != btn_state[i]) {
                btn_state[i] = raw;
                if (raw) {
                    handle_button_press(i);
                } else {
                    handle_button_release(i);
                }
            }
        }
    }
}

void setup() {
    USB.productName("Tribe Pedal");
    USB.manufacturerName("Tribe of Abraham");
    USB.PID(0x8030);
    Keyboard.begin();
    MIDI.begin();
    USB.begin();

    looper_init();

    // Init footswitch pins with internal pull-ups
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(BTN_PINS[i], INPUT_PULLUP);
    }

    strip.begin();
    strip.setBrightness(30);
    strip.setPixelColor(0, strip.Color(0, 0, 180));  // blue = clear
    strip.show();
}

static bool wifi_started = false;

void loop() {
    if (!wifi_started && (bool)USB) {
        config_init();
        wifi_started = true;
    }
    if (wifi_started) config_loop();

    // Drain incoming MIDI
    midiEventPacket_t packet;
    while (MIDI.readPacket(&packet)) {}

    // Read physical buttons
    read_buttons();

    // Update looper flash timing
    looper_update();

    // Update NeoPixel from looper state
    LooperColor c = looper_led_color();
    strip.setBrightness(c.r == 255 && c.g == 255 && c.b == 255 ? 80 : 30);
    strip.setPixelColor(0, strip.Color(c.r, c.g, c.b));
    strip.show();

    if (!(bool)USB) {
        static uint8_t brightness = 0;
        static int8_t dir = 1;
        brightness += dir * 2;
        if (brightness >= 60) dir = -1;
        if (brightness <= 2)  dir = 1;
        strip.setBrightness(brightness);
        strip.setPixelColor(0, strip.Color(255, 0, 0));
        strip.show();
        delay(20);
        return;
    }

    delay(1);
}
