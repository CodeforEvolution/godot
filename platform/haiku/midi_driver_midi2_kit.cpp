/*************************************************************************/
/*  midi_driver_midi2_kit.cpp                                            */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifdef MIDI2_KIT_ENABLED

#include "midi_driver_midi2_kit.h"

#include "core/print_string.h"

#include <MidiProducer.h>
#include <MidiRoster.h>

class GodotMidiLocalConsumer : public BMidiLocalConsumer {
	
	MIDIDriverMIDI2Kit *midi_driver
	
public:
	GodotMidiLocalConsumer(const char *name, MIDIDriverMIDI2Kit *p_midi_driver) :
			BMidiLocalConsumer(name),
			midi_driver(p_midi_driver) {
	};
	
	void Data(uchar *data, size_t length, bool atomic, bigtime_t time) {

		if (atomic) {
			midi_driver->receive_input_packet(time, data, length);
		}
	};
};

Error MIDIDriverMIDI2Kit::open() {
	
	input_consumer = new GodotMidiLocalConsumer("Godot Input", this);
	if (input_consumer == NULL) {
		ERR_PRINTS("GodotMidiLocalConsumer construction failed");
		return ERR_CANT_OPEN;
	}
	
	if (!input_consumer->IsValid()) {
		ERR_PRINTS("GodotMidiLocalConsumer failed to start up");
		input_consumer->Release();
		return ERR_CANT_OPEN;
	}
	
	if (input_consumer->Register() != B_OK) {
		ERR_PRINTS("GodotMidiLocalConsumer failed to register itself with BMidiRoster");
		input_consumer->Release();
		return ERR_CANT_OPEN;
	}

	bool more_producers = true;
	int32 producer_id = 0;
	while (more_producers) {
		BMidiProducer *producer = BMidiRoster::NextProducer(&producer_id);
		if (producer != NULL) {
			if (producer->Connect(input_consumer) == B_OK) {
				connected_sources.push_back(producer_id);
			}
		} else {
			more_producers = false;
		}
	}
	
	return OK;
}

void MIDIDriverMIDI2Kit::close() {

	for (int index = 0; index < connected_sources.size(); index++) {
		int32 producer_id = connected_sources[index];
		BMidiProducer *producer = BMidiRoster::FindProducer(producer_id);
		if (producer != NULL) {
			producer->Disconnect(input_consumer);
			producer->Release();
		}	
	}
	connected_sources.clear();

	input_consumer->Unregister();
	input_consumer->Release();
}

PoolStringArray MIDIDriverMIDI2Kit::get_connected_inputs() {

	PoolStringArray list;
	for (int index = 0; index < connected_sources.size(); index++) {
		int32 producer_id = connected_sources[index];
		BMidiProducer *producer = BMidiRoster::FindProducer(producer_id);
		if (producer != NULL) {
			list.push_back(producer->Name());
		}
	}
	
	return list;
}

MIDIDriverMIDI2Kit::MIDIDriverMIDI2Kit() :
		input_consumer(NULL) {
}

MIDIDriverMIDI2Kit::~MIDIDriverMIDI2Kit() {
	close();
}

#endif // COREMIDI_ENABLED
