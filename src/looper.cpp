#include "looper.h"

static LooperState state = LOOP_CLEAR;
static LooperState prev_state = LOOP_CLEAR;  // for undo tracking
static bool flashing = false;
static uint32_t flash_start = 0;
static const uint32_t FLASH_MS = 120;
static int layer_count = 0;
static bool drum_route_on = false;
static int drum_event = 0;
static uint8_t fx_chain = 0;  // 0-3

// State colors
static const LooperColor COLORS[] = {
    {   0,   0, 180 },  // CLEAR — blue
    { 255,   0,   0 },  // RECORDING — red
    {   0, 255,   0 },  // PLAYING — green
    { 255,   0,   0 },  // OVERDUBBING — red
    { 128,   0, 180 },  // PAUSED — purple
};
static const LooperColor WHITE = { 255, 255, 255 };

// FX chain colors: clean=white, chorus=blue, overdrive=green, distortion=orange
static const LooperColor FX_COLORS[] = {
    { 255, 255, 255 },  // 0: clean — white
    {   0,  80, 255 },  // 1: chorus — blue
    {   0, 255,   0 },  // 2: overdrive — green
    { 255, 140,   0 },  // 3: distortion — orange
};

static bool fx_flashing = false;
static uint32_t fx_flash_start = 0;
static const uint32_t FX_FLASH_MS = 400;  // longer so you can see the color

static void flash() {
    flashing = true;
    flash_start = millis();
}

void looper_init() {
    state = LOOP_CLEAR;
    prev_state = LOOP_CLEAR;
    flashing = false;
    layer_count = 0;
    drum_route_on = false;
    drum_event = 0;
    fx_chain = 0;
}

bool looper_press(uint8_t button) {
    flash();
    drum_event = 0;

    switch (button) {
        case 0:  // Record / Overdub
            switch (state) {
                case LOOP_CLEAR:
                    state = LOOP_RECORDING;
                    break;
                case LOOP_RECORDING:
                    state = LOOP_PLAYING;
                    layer_count = 1;
                    // Auto-mute drums when first loop closes
                    if (drum_route_on) {
                        drum_route_on = false;
                        drum_event = -1;
                    }
                    break;
                case LOOP_PLAYING:
                    state = LOOP_OVERDUBBING;
                    break;
                case LOOP_OVERDUBBING:
                    state = LOOP_PLAYING;
                    layer_count++;
                    break;
                case LOOP_PAUSED:
                    state = LOOP_OVERDUBBING;
                    break;
            }
            return true;

        case 1:  // Drum mute toggle + FX chain cycle
            // Toggle drum mute
            drum_route_on = !drum_route_on;
            drum_event = drum_route_on ? 1 : -1;
            // Cycle FX chain 0→1→2→3→0
            fx_chain = (fx_chain + 1) & 0x03;
            // Show FX color instead of white flash
            flashing = false;
            fx_flashing = true;
            fx_flash_start = millis();
            return true;

        case 2:  // Undo
            if (layer_count > 0) {
                layer_count--;
                if (layer_count == 0) {
                    state = LOOP_CLEAR;
                } else if (state == LOOP_OVERDUBBING) {
                    state = LOOP_PLAYING;
                }
                // If playing or paused, stay in that state
            }
            return true;

        case 3:  // Reset All — hard reset everything
            state = LOOP_CLEAR;
            layer_count = 0;
            // Mute drums if they were on
            if (drum_route_on) {
                drum_route_on = false;
                drum_event = -1;
            }
            fx_chain = 0;
            return true;
    }
    return false;
}

LooperState looper_state() {
    return state;
}

LooperColor looper_led_color() {
    if (fx_flashing) return FX_COLORS[fx_chain];
    if (flashing) return WHITE;
    return COLORS[state];
}

void looper_update() {
    if (flashing && (millis() - flash_start > FLASH_MS)) {
        flashing = false;
    }
    if (fx_flashing && (millis() - fx_flash_start > FX_FLASH_MS)) {
        fx_flashing = false;
    }
}

int looper_drum_route_event() {
    int e = drum_event;
    drum_event = 0;
    return e;
}

uint8_t looper_fx_chain() {
    return fx_chain;
}
