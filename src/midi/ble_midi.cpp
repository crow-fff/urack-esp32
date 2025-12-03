#include "ble_midi.h"
#include "../signal_processor/signal_processor.h"
#include <BLEMidi.h>

// Global instance
BleMidi ble_midi;

// Static pointer for callbacks
static SignalProcessor* s_processor = nullptr;
static BleMidi* s_ble_midi = nullptr;

BleMidi::BleMidi() 
    : processor(nullptr), enabled(false), connected(false), initialized(false) {
}

BleMidi::~BleMidi() {
    disable();
}

void BleMidi::begin(SignalProcessor* proc) {
    processor = proc;
    s_processor = proc;
    s_ble_midi = this;
}

void BleMidi::enable() {
    if (enabled) return;
    
    if (!initialized) {
        BLEMidiServer.begin("URack MIDI");
        BLEMidiServer.setOnConnectCallback(onConnect);
        BLEMidiServer.setOnDisconnectCallback(onDisconnect);
        BLEMidiServer.setNoteOnCallback(onNoteOn);
        BLEMidiServer.setNoteOffCallback(onNoteOff);
        BLEMidiServer.setControlChangeCallback(onControlChange);
        // Note: ESP32-BLE-MIDI library doesn't support pitch bend callback directly
        initialized = true;
    }
    
    enabled = true;
    Serial.println("BLE MIDI enabled");
}

void BleMidi::disable() {
    if (!enabled) return;
    
    // Note: BLEMidi library doesn't have a clean way to fully disable,
    // but we can stop advertising and ignore callbacks
    enabled = false;
    connected = false;
    Serial.println("BLE MIDI disabled");
}

bool BleMidi::is_enabled() const {
    return enabled;
}

bool BleMidi::is_connected() const {
    return connected && enabled;
}

void BleMidi::onConnect() {
    if (s_ble_midi) {
        s_ble_midi->connected = true;
    }
    Serial.println("BLE MIDI connected");
}

void BleMidi::onDisconnect() {
    if (s_ble_midi) {
        s_ble_midi->connected = false;
    }
    Serial.println("BLE MIDI disconnected");
}

void BleMidi::onNoteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp) {
    (void)timestamp;
    if (!s_ble_midi || !s_ble_midi->enabled || !s_processor) return;
    s_processor->handle_note_on(channel, note, velocity);
}

void BleMidi::onNoteOff(uint8_t channel, uint8_t note, uint8_t velocity, uint16_t timestamp) {
    (void)timestamp;
    if (!s_ble_midi || !s_ble_midi->enabled || !s_processor) return;
    s_processor->handle_note_off(channel, note, velocity);
}

void BleMidi::onControlChange(uint8_t channel, uint8_t controller, uint8_t value, uint16_t timestamp) {
    (void)timestamp;
    if (!s_ble_midi || !s_ble_midi->enabled || !s_processor) return;
    s_processor->handle_cc(channel, controller, value);
}

void BleMidi::onClock() {
    if (!s_ble_midi || !s_ble_midi->enabled || !s_processor) return;
    s_processor->handle_clock();
}

void BleMidi::onStart() {
    if (!s_ble_midi || !s_ble_midi->enabled || !s_processor) return;
    s_processor->handle_start();
}

void BleMidi::onStop() {
    if (!s_ble_midi || !s_ble_midi->enabled || !s_processor) return;
    s_processor->handle_stop();
}
