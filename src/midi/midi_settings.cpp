#include "midi.h"
#include "midi_settings.h"
#include "ble_midi.h"
#include "usb_midi.h"
#include "../util.h"

MidiSettings::MidiSettings(Display* display, MidiSettingsState* state, SignalProcessor* processor, ScreenSwitcher* screen_switcher)
    : ScreenInterface(display), state(state), processor(processor), screen_switcher(screen_switcher),
      current_page(PAGE_OUTPUTS), current_item(MENU_CHANNEL), is_editing(false), row_number(0), paginator_selected(true) {}

void MidiSettings::set_screen_switcher(ScreenSwitcher* screen_switcher) {
    this->screen_switcher = screen_switcher;
}

void MidiSettings::enter() {
    current_page = PAGE_OUTPUTS;
    current_item = MENU_CHANNEL;
    is_editing = false;
    row_number = 0;
    paginator_selected = true;
}

void MidiSettings::exit() {

}

void MidiSettings::render() {
    display->clearDisplay();

    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0,0);

    render_paginator();

    switch (current_page) {
        case PAGE_OUTPUTS:
            render_menu();
            break;
        case PAGE_BLUETOOTH:
            render_bluetooth_page();
            break;
        case PAGE_USB:
            render_usb_page();
            break;
        default:
            break;
    }

    display->display();
}

void MidiSettings::render_paginator() {
    const char* page_names[] = {"Out", "BLE", "USB"};
    int page_width = SCREEN_WIDTH / PAGE_COUNT;
    
    for (int i = 0; i < PAGE_COUNT; i++) {
        int x = i * page_width;
        bool is_current = (i == current_page);
        
        if (is_current && paginator_selected) {
            // Selected and paginator is active - filled rectangle
            display->fillRect(x, 0, page_width, PAGINATOR_HEIGHT, SSD1306_WHITE);
            display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
        } else if (is_current) {
            // Current page but not selected - outline
            display->drawRect(x, 0, page_width, PAGINATOR_HEIGHT, SSD1306_WHITE);
            display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
        } else {
            display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
        }
        
        // Center text in tab
        int text_width = strlen(page_names[i]) * 6; // 6 pixels per char at size 1
        int text_x = x + (page_width - text_width) / 2;
        display->setCursor(text_x, 2);
        display->print(page_names[i]);
    }
    
    // Draw separator line
    display->drawLine(0, PAGINATOR_HEIGHT, SCREEN_WIDTH, PAGINATOR_HEIGHT, SSD1306_WHITE);
}

void MidiSettings::render_menu() {
    // Render all rows with offset for paginator
    for (int i = 0; i < MENU_COUNT; i++) {
        int y = PAGINATOR_HEIGHT + 2 + i * LINE_HEIGHT;
        bool is_selected = (i == current_item) && !paginator_selected;
        bool is_editing_selected = is_selected && is_editing;
        ColumnType col_type = items[i].type;

        // Determine which column is selected for outputs
        bool type_selected = is_selected && (col_type == SingleItem || row_number == 0);
        bool channel_selected = is_selected && col_type == ChannelItem && row_number == 1;

        // Draw rectangle cursor
        if (is_selected) {
            if (col_type == ChannelItem) {
                // For outputs: draw cursor on selected column
                if (row_number == 1) {
                    // Channel column selected
                    if (is_editing_selected) {
                        display->fillRect(COL3_X, y, COL3_WIDTH, LINE_HEIGHT, SSD1306_WHITE);
                    } else {
                        display->drawRect(COL3_X, y, COL3_WIDTH, LINE_HEIGHT, SSD1306_WHITE);
                    }
                } else {
                    // Type column selected
                    if (is_editing_selected) {
                        display->fillRect(COL2_X, y, COL2_WIDTH, LINE_HEIGHT, SSD1306_WHITE);
                    } else {
                        display->drawRect(COL2_X, y, COL2_WIDTH, LINE_HEIGHT, SSD1306_WHITE);
                    }
                }
            } else {
                // For single-column items: full width
                if (is_editing_selected) {
                    display->fillRect(COL1_X, y, SCREEN_WIDTH, LINE_HEIGHT, SSD1306_WHITE);
                } else {
                    display->drawRect(COL1_X, y, SCREEN_WIDTH, LINE_HEIGHT, SSD1306_WHITE);
                }
            }
        }

        if (col_type == SingleItem) {
            // Single column: output name and value sequentially with space
            bool is_editing_selected = is_selected && is_editing;
            if (is_editing_selected) {
                display->setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Inverted for editing
            } else {
                display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
            }
            display->setCursor(COL1_X + 2, y + 1);
            display->print(items[i].text);
            display->print(" ");
            if (i == MENU_CHANNEL) {
                display->print(state->get_midi_channel_str());
            } else if (i == MENU_CLOCK) {
                display->print(state->get_midi_clk_type_str());
            }
        } else {
            // Multiple columns: Column 1 - Item name
            display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
            display->setCursor(COL1_X + 2, y + 1);
            display->print(items[i].text);

            // Column 2: MIDI out type
            bool col2_editing = type_selected && is_editing;
            if (col2_editing) {
                display->setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Inverted for editing
            } else {
                display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
            }
            display->setCursor(COL2_X, y + 1);
            display->print(state->get_midi_out_type_str(items[i].data.output_idx));

            // Column 3: MIDI out channel
            if (channel_selected && is_editing) {
                display->setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Inverted for editing
            } else {
                display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
            }
            display->setCursor(COL3_X, y + 1);
            display->print(state->get_midi_out_channel_str(items[i].data.output_idx));
        }
    }
}

void MidiSettings::render_bluetooth_page() {
    int y = PAGINATOR_HEIGHT + 4;
    
    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display->setCursor(0, y);
    display->println("Bluetooth MIDI");
    
    y += LINE_HEIGHT + 4;
    
    bool is_selected = !paginator_selected;
    bool enabled = state->get_bluetooth_enabled();
    
    if (is_selected && is_editing) {
        display->fillRect(0, y, SCREEN_WIDTH, LINE_HEIGHT, SSD1306_WHITE);
        display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else if (is_selected) {
        display->drawRect(0, y, SCREEN_WIDTH, LINE_HEIGHT, SSD1306_WHITE);
        display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    } else {
        display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    
    display->setCursor(2, y + 1);
    display->print("Enable: ");
    display->print(state->get_bluetooth_enabled_str());
    
    y += LINE_HEIGHT + 4;
    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display->setCursor(0, y);
    display->print("Status: ");
    if (enabled) {
        display->print(ble_midi.is_connected() ? "Connected" : "Waiting...");
    } else {
        display->print("Disabled");
    }
}

void MidiSettings::render_usb_page() {
    int y = PAGINATOR_HEIGHT + 4;
    
    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display->setCursor(0, y);
    display->println("USB MIDI");
    
    y += LINE_HEIGHT + 4;
    
    bool is_selected = !paginator_selected;
    bool enabled = state->get_usb_enabled();
    
    if (is_selected && is_editing) {
        display->fillRect(0, y, SCREEN_WIDTH, LINE_HEIGHT, SSD1306_WHITE);
        display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    } else if (is_selected) {
        display->drawRect(0, y, SCREEN_WIDTH, LINE_HEIGHT, SSD1306_WHITE);
        display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    } else {
        display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    }
    
    display->setCursor(2, y + 1);
    display->print("Enable: ");
    display->print(state->get_usb_enabled_str());
    
    y += LINE_HEIGHT + 4;
    display->setTextColor(SSD1306_WHITE, SSD1306_BLACK);
    display->setCursor(0, y);
    display->print("Status: ");
    if (enabled) {
        display->print("Active");
    } else {
        display->print("Disabled");
    }
}

void MidiSettings::handle_input(Event* event) {
    if (event == nullptr) return;

    if(is_editing) {
        if (event->button_sw == ButtonPress) {
            is_editing = false;
        }

        if(event->button_a == ButtonPress) {
            is_editing = false;
        }
    } else {
        if (event->button_sw == ButtonPress) {
            if (paginator_selected) {
                // Move from paginator to content
                paginator_selected = false;
            } else {
                is_editing = true;

                // cleanup last cc array
                for(int i = 0; i < MIDI_CHANNEL_COUNT; i++) {
                    processor->last_cc[i] = 128;
                    processor->pitchbend[i] = 0;
                }
            }
        }

        if(event->button_a == ButtonPress) {
            if (!paginator_selected) {
                // Move back to paginator
                paginator_selected = true;
            } else {
                screen_switcher->set_screen(MidiScreen::MidiScreenInfo);
            }
        }
    }

    // Handle page-specific input
    switch (current_page) {
        case PAGE_OUTPUTS:
            handle_menu_input(event);
            break;
        case PAGE_BLUETOOTH:
            handle_bluetooth_input(event);
            break;
        case PAGE_USB:
            handle_usb_input(event);
            break;
        default:
            break;
    }
}

void MidiSettings::handle_bluetooth_input(Event* event) {
    if (event->encoder != 0) {
        if (paginator_selected) {
            // Navigate between pages
            int new_page = (int)current_page + event->encoder;
            if (new_page >= 0 && new_page < PAGE_COUNT) {
                current_page = (SettingsPage)new_page;
            }
        } else if (is_editing) {
            // Toggle bluetooth enabled
            bool enabled = state->get_bluetooth_enabled();
            state->set_bluetooth_enabled(!enabled);
            
            // Enable/disable BLE MIDI
            if (state->get_bluetooth_enabled()) {
                ble_midi.enable();
            } else {
                ble_midi.disable();
            }
            
            state->store();
        }
    }
}

void MidiSettings::handle_usb_input(Event* event) {
    if (event->encoder != 0) {
        if (paginator_selected) {
            // Navigate between pages
            int new_page = (int)current_page + event->encoder;
            if (new_page >= 0 && new_page < PAGE_COUNT) {
                current_page = (SettingsPage)new_page;
            }
        } else if (is_editing) {
            // Toggle USB enabled
            bool enabled = state->get_usb_enabled();
            state->set_usb_enabled(!enabled);
            
            // Enable/disable USB MIDI
            if (state->get_usb_enabled()) {
                usb_midi.enable();
            } else {
                usb_midi.disable();
            }
            
            state->store();
        }
    }
}

void MidiSettings::handle_menu_input(Event* event) {
    if (event->encoder != 0) {
        if (paginator_selected) {
            // Navigate between pages
            int new_page = (int)current_page + event->encoder;
            if (new_page >= 0 && new_page < PAGE_COUNT) {
                current_page = (SettingsPage)new_page;
            }
        } else if (is_editing) {
            // Adjust value
            const MenuItemInfo& item = items[current_item];
            
            if (item.type == SingleItem) {
                // Single column items
                switch (current_item) {
                    case MENU_CHANNEL:
                        state->set_midi_channel((MidiChannel)clampi(state->get_midi_channel() + event->encoder,
                                                                 state->get_min_midi_channel(),
                                                                 state->get_max_midi_channel()));
                        break;
                    case MENU_CLOCK:
                        state->set_midi_clk_type((MidiClkType)clampi(state->get_midi_clk_type() + event->encoder,
                                                                   state->get_min_midi_clk_type(),
                                                                   state->get_max_midi_clk_type()));
                        break;
                    default:
                        break;
                }
            } else {
                // ChannelItem: outputs with two columns
                int idx = item.data.output_idx;
                if (row_number == 1) {
                    // Editing channel column
                    state->set_midi_out_channel(idx, (MidiChannel)clampi(state->get_midi_out_channel(idx) + event->encoder,
                                                                       state->get_min_midi_out_channel(),
                                                                       state->get_max_midi_out_channel()));
                } else {
                    // Editing type column
                    state->set_midi_out_type(idx, (MidiOutType)clampi(state->get_midi_out_type(idx) + event->encoder,
                                                                   state->get_min_midi_out_type(idx),
                                                                   state->get_max_midi_out_type(idx)));
                }
            }
            state->store();
        } else {
            // Move selection sequentially
            Direction direction = (event->encoder > 0) ? DOWN : UP;
            int steps = (event->encoder > 0) ? event->encoder : -event->encoder;
            
            for (int step = 0; step < steps; step++) {
                ColumnType current_type = items[current_item].type;
                
                if (current_type == SingleItem) {
                    // Single column: move to next/previous row
                    int new_item = (int)current_item + direction;
                    if (new_item >= 0 && new_item < MENU_COUNT) {
                        current_item = (MenuItems)new_item;
                        row_number = 0; // Reset to first column
                    }
                } else {
                    // Multiple columns: move within row first
                    int new_row_number = row_number + direction;
                    if (new_row_number < 0) {
                        // Hit left edge, move to previous row
                        int new_item = (int)current_item + direction;
                        if (new_item >= 0) {
                            current_item = (MenuItems)new_item;
                            // Set to last column of previous row
                            row_number = items[current_item].type - 1;
                        } else {
                            // Can't go further, stay at current position
                            row_number = 0;
                        }
                    } else if (new_row_number >= current_type) {
                        // Hit right edge, move to next row
                        int new_item = (int)current_item + direction;
                        if (new_item < MENU_COUNT) {
                            current_item = (MenuItems)new_item;
                            row_number = 0; // Start at first column of next row
                        } else {
                            // Can't go further, stay at current position
                            row_number = current_type - 1;
                        }
                    } else {
                        // Move within current row
                        row_number = new_row_number;
                    }
                }
            }
        }
    }

    // MIDI learn
    if(is_editing && !paginator_selected && (processor->last_cc[current_item] != 0 || processor->pitchbend[current_item] != 0)) {
        const MenuItemInfo& item = items[current_item];
        int idx = item.data.output_idx;
        MidiChannel channel = state->get_midi_out_channel(idx);
        if(channel == MidiChannelUnchanged) {
            channel = state->get_midi_channel();
        }

        int last_cc = 128;
        int last_pitchbend = 0;

        if(channel != MidiChannelAll) {
            last_cc = processor->last_cc[channel];
            last_pitchbend = processor->pitchbend[channel];
        } else {
            for(int i = 0; i < MIDI_CHANNEL_COUNT; i++) {
                if(processor->last_cc[i] != 128) {
                    last_cc = processor->last_cc[i];
                }
                if(processor->pitchbend[i] != 0) {
                    last_pitchbend = processor->pitchbend[i];
                }
            }
        }

        if(last_cc != 128) {
            state->set_midi_out_type(idx, (MidiOutType)((int)MidiOutType::MidiOutCc0 + last_cc));
        }
        if(last_pitchbend != 0) {
            state->set_midi_out_type(idx, MidiOutType::MidiOutPitchBend);
        }

        processor->last_cc[current_item] = 0;
        processor->pitchbend[current_item] = 0;
    }
}



void MidiSettings::update(Event* event) {
    handle_input(event);
    render();
}
