#include "midi_usb.h"
#include "USB.h"
#include "USBMIDI.h"

static USBMIDI midi;

void midi_init() {
    USB.productName("Tribe Pedal MIDI");
    USB.manufacturerName("Tribe of Abraham");
    USB.PID(0x8010);
    midi.begin();
    USB.begin();
}

void midi_task() {
    midiEventPacket_t packet;
    while (midi.readPacket(&packet)) {
        // Drain incoming
    }
}

bool midi_connected() {
    return USB;
}

void midi_send_cc(uint8_t channel, uint8_t cc, uint8_t value) {
    midi.controlChange(cc, value, channel + 1);
}

void midi_send_note_on(uint8_t channel, uint8_t note, uint8_t velocity) {
    midi.noteOn(note, velocity, channel + 1);
}

void midi_send_note_off(uint8_t channel, uint8_t note) {
    midi.noteOff(note, 0, channel + 1);
}

void midi_send_program_change(uint8_t channel, uint8_t program) {
    midi.programChange(program, channel + 1);
}
