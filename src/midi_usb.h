#pragma once
#include <Arduino.h>

void midi_init();
void midi_task();
bool midi_connected();

void midi_send_cc(uint8_t channel, uint8_t cc, uint8_t value);
void midi_send_note_on(uint8_t channel, uint8_t note, uint8_t velocity);
void midi_send_note_off(uint8_t channel, uint8_t note);
void midi_send_program_change(uint8_t channel, uint8_t program);
