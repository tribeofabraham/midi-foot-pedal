// MIDI Foot Pedal — USB Composite (MIDI + HID Keyboard) + NeoPixel + WiFi

#include <Arduino.h>
#include "soc/rtc_cntl_reg.h"
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
static const uint32_t DEBOUNCE_MS = 10;
static uint32_t btn_last_change[NUM_BUTTONS] = {};
static bool     btn_state[NUM_BUTTONS] = {};      // true = pressed
static bool     btn_prev_raw[NUM_BUTTONS] = {};

// Toggle state for toggle-mode buttons
static bool     btn_toggle_on[NUM_BUTTONS] = {};

// Non-looper LED flash state
static uint8_t  last_pressed_btn = 0;
static bool     generic_flashing = false;
static uint32_t generic_flash_start = 0;

// Called from web UI test buttons or physical buttons
void handle_button_press(uint8_t btn) {
    if (btn >= NUM_BUTTONS) return;
    PedalConfig& cfg = config_get();
    ButtonConfig& b = cfg.buttons[btn];

    // LED feedback + looper state
    if (cfg.mode == MODE_LOOPER || cfg.mode == MODE_ZLOOPER) {
        looper_press(btn);

        // Drum route CC 41: unmute/mute
        int drum = looper_drum_route_event();
        if (drum == 1)  MIDI.controlChange(41, 0, 11);    // unmute — drums flow
        if (drum == -1) MIDI.controlChange(41, 127, 11);  // mute — drums cut

        // FX chain select on button 2 — CCs 42-45 on ch 11
        // Active chain gets 127, all others get 0
        if (btn == 1) {
            uint8_t active = looper_fx_chain();
            for (uint8_t i = 0; i < 4; i++) {
                MIDI.controlChange(42 + i, (i == active) ? 127 : 0, 11);
            }
        }

        // Reset clears all toggle states
        if (btn == 3) {
            for (int i = 0; i < NUM_BUTTONS; i++) btn_toggle_on[i] = false;

            // Z-Looper: simulate double-press-and-hold on single_pedal CC
            // to trigger reset/clear in SooperLooper
            if (cfg.mode == MODE_ZLOOPER) {
                uint8_t cc = cfg.buttons[0].midi_cc;  // same CC as switch 1
                uint8_t ch = cfg.buttons[0].midi_channel;
                // First press/release
                MIDI.controlChange(cc, 127, ch);
                delay(50);
                MIDI.controlChange(cc, 0, ch);
                delay(50);
                // Second press and hold >1.5s
                MIDI.controlChange(cc, 127, ch);
                delay(1600);
                MIDI.controlChange(cc, 0, ch);
            }
        }
    } else {
        last_pressed_btn = btn;
        generic_flashing = true;
        generic_flash_start = millis();
    }

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
                // Momentary: send on value, off sent on button release
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
    // Check for bootloader combo: hold buttons 1+4 (GPIO 9+12) on power-up
    pinMode(9, INPUT_PULLUP);
    pinMode(12, INPUT_PULLUP);
    delay(50);  // let pull-ups settle
    if (!digitalRead(9) && !digitalRead(12)) {
        // Both outer switches held — reboot into USB download mode
        // Flash NeoPixel yellow to confirm
        strip.begin();
        strip.setBrightness(60);
        strip.setPixelColor(0, strip.Color(255, 180, 0));
        strip.show();
        delay(500);
        // Force reboot into ROM download mode
        REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
        esp_restart();
    }

    // Load the pedal name from NVS early so USB descriptor reflects it
    config_load_name_early();
    USB.productName(config_get_pedal_name());
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

    // Update NeoPixel
    if (config_get().mode == MODE_LOOPER || config_get().mode == MODE_ZLOOPER) {
        looper_update();
        LooperColor c = looper_led_color();
        strip.setBrightness(c.r == 255 && c.g == 255 && c.b == 255 ? 80 : 30);
        strip.setPixelColor(0, strip.Color(c.r, c.g, c.b));
    } else {
        PedalConfig& cfg = config_get();
        ButtonConfig& lb = cfg.buttons[last_pressed_btn];

        if (generic_flashing) {
            // White flash on press
            strip.setBrightness(80);
            strip.setPixelColor(0, strip.Color(255, 255, 255));
            if (millis() - generic_flash_start > 120) {
                generic_flashing = false;
            }
        } else {
            // Show active color of the selected button
            strip.setBrightness(40);
            strip.setPixelColor(0, strip.Color(lb.color_on_r, lb.color_on_g, lb.color_on_b));
        }
    }
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

}
