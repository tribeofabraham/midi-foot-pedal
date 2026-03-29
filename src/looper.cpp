#include "looper.h"

static LooperState state = LOOP_CLEAR;
static bool flashing = false;
static uint32_t flash_start = 0;
static const uint32_t FLASH_MS = 120;
static int layer_count = 0;  // tracks base loop + overdubs
static bool drum_route_on = false;
static int drum_event = 0;   // 1=turn on, -1=turn off, 0=no change

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
    layer_count = 0;
    drum_route_on = false;
    drum_event = 0;
}

bool looper_press(uint8_t button) {
    flash();  // white confirmation flash on every press
    drum_event = 0;  // reset event each press

    switch (button) {
        case 0:  // Record / Overdub
            switch (state) {
                case LOOP_CLEAR:       state = LOOP_RECORDING;  break;
                case LOOP_RECORDING:
                    state = LOOP_PLAYING;
                    layer_count = 1;
                    // Auto-mute drums when first loop closes
                    if (drum_route_on) {
                        drum_route_on = false;
                        drum_event = -1;
                    }
                    break;
                case LOOP_PLAYING:     state = LOOP_OVERDUBBING; break;
                case LOOP_OVERDUBBING: state = LOOP_PLAYING; layer_count++; break;
                case LOOP_PAUSED:      state = LOOP_OVERDUBBING; break;
            }
            return true;

        case 1:  // Drum mute toggle
            drum_route_on = !drum_route_on;
            drum_event = drum_route_on ? 1 : -1;
            return true;

        case 2:  // Undo
            if (state == LOOP_OVERDUBBING) {
                state = LOOP_PLAYING;
            }
            if (layer_count > 0) {
                layer_count--;
                if (layer_count == 0) {
                    state = LOOP_CLEAR;
                }
            }
            return true;

        case 3:  // Reset All
            state = LOOP_CLEAR;
            layer_count = 0;
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

int looper_drum_route_event() {
    int e = drum_event;
    drum_event = 0;  // consume the event
    return e;
}
