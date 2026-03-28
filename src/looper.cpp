#include "looper.h"

static LooperState state = LOOP_CLEAR;
static bool flashing = false;
static uint32_t flash_start = 0;
static const uint32_t FLASH_MS = 120;

// State colors
static const LooperColor COLORS[] = {
    {   0,   0, 180 },  // CLEAR — blue
    { 255,   0,   0 },  // RECORDING — red
    {   0, 255,   0 },  // PLAYING — green
    { 255,   0,   0 },  // OVERDUBBING — red (same as recording)
    { 128,   0, 180 },  // PAUSED — purple
};
static const LooperColor WHITE = { 255, 255, 255 };

static void flash() {
    flashing = true;
    flash_start = millis();
}

void looper_init() {
    state = LOOP_CLEAR;
    flashing = false;
}

bool looper_press(uint8_t button) {
    flash();  // white confirmation flash on every press

    switch (button) {
        case 0:  // Record / Overdub
            switch (state) {
                case LOOP_CLEAR:       state = LOOP_RECORDING;  break;
                case LOOP_RECORDING:   state = LOOP_PLAYING;    break;
                case LOOP_PLAYING:     state = LOOP_OVERDUBBING; break;
                case LOOP_OVERDUBBING: state = LOOP_PLAYING;    break;
                case LOOP_PAUSED:      state = LOOP_OVERDUBBING; break;
            }
            return true;

        case 1:  // Pause / Play
            switch (state) {
                case LOOP_PLAYING:     state = LOOP_PAUSED;  break;
                case LOOP_OVERDUBBING: state = LOOP_PAUSED;  break;
                case LOOP_PAUSED:      state = LOOP_PLAYING; break;
                case LOOP_RECORDING:   state = LOOP_PLAYING; break;
                default: break;  // no-op if clear
            }
            return true;

        case 2:  // Undo
            // Undo last overdub, stay in current playback state
            if (state == LOOP_OVERDUBBING) state = LOOP_PLAYING;
            return true;

        case 3:  // Reset All
            state = LOOP_CLEAR;
            return true;
    }
    return false;
}

LooperState looper_state() {
    return state;
}

LooperColor looper_led_color() {
    if (flashing) return WHITE;
    return COLORS[state];
}

void looper_update() {
    if (flashing && (millis() - flash_start > FLASH_MS)) {
        flashing = false;
    }
}
