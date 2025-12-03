#include "usb_midi.h"
#include "../signal_processor/signal_processor.h"

#if CONFIG_TINYUSB_ENABLED
#include <USB.h>
#include <USBMIDI.h>

USBMIDI usbMIDI;
#endif

// Global instance
UsbMidi usb_midi;

UsbMidi::UsbMidi() 
    : processor(nullptr), enabled(false), initialized(false) {
}

UsbMidi::~UsbMidi() {
    disable();
}

void UsbMidi::begin(SignalProcessor* proc) {
    processor = proc;
}

void UsbMidi::enable() {
    if (enabled) return;
    
#if CONFIG_TINYUSB_ENABLED
    if (!initialized) {
        USB.begin();
        usbMIDI.begin();
        initialized = true;
    }
    enabled = true;
    Serial.println("USB MIDI enabled");
#else
    Serial.println("USB MIDI not available (TinyUSB not enabled)");
#endif
}

void UsbMidi::disable() {
    if (!enabled) return;
    enabled = false;
    Serial.println("USB MIDI disabled");
}

bool UsbMidi::is_enabled() const {
    return enabled;
}

void UsbMidi::update() {
#if CONFIG_TINYUSB_ENABLED
    if (!enabled || !processor) return;
    
    midiEventPacket_t packet;
    while (usbMIDI.readPacket(&packet)) {
        // Parse MIDI packet
        uint8_t type = packet.header & 0x0F;
        uint8_t channel = packet.byte1 & 0x0F;
        
        switch (type) {
            case 0x09: // Note On
                if (packet.byte3 > 0) {
                    processor->handle_note_on(channel + 1, packet.byte2, packet.byte3);
                } else {
                    processor->handle_note_off(channel + 1, packet.byte2, 0);
                }
                break;
                
            case 0x08: // Note Off
                processor->handle_note_off(channel + 1, packet.byte2, packet.byte3);
                break;
                
            case 0x0B: // Control Change
                processor->handle_cc(channel + 1, packet.byte2, packet.byte3);
                break;
                
            case 0x0E: // Pitch Bend
                {
                    int value = ((int)packet.byte3 << 7) | packet.byte2;
                    value -= 8192; // Center at 0
                    processor->handle_pitchbend(channel + 1, value);
                }
                break;
                
            case 0x0F: // System messages
                switch (packet.byte1) {
                    case 0xF8: // Clock
                        processor->handle_clock();
                        break;
                    case 0xFA: // Start
                        processor->handle_start();
                        break;
                    case 0xFC: // Stop
                        processor->handle_stop();
                        break;
                }
                break;
        }
    }
#endif
}
