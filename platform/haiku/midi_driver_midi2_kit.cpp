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

#include <MidiConsumer.h>
#include <MidiProducer.h>
#include <MidiRoster.h>

Error MIDIDriverMIDI2Kit::open() {

	CFStringRef name = CFStringCreateWithCString(NULL, "Godot", kCFStringEncodingASCII);
	OSStatus result = MIDIClientCreate(name, NULL, NULL, &client);
	CFRelease(name);
	if (result != noErr) {
		ERR_PRINTS("MIDIClientCreate failed, code: " + itos(result));
		return ERR_CANT_OPEN;
	}

	result = MIDIInputPortCreate(client, CFSTR("Godot Input"), MIDIDriverCoreMidi::read, (void *)this, &port_in);
	if (result != noErr) {
		ERR_PRINTS("MIDIInputPortCreate failed, code: " + itos(result));
		return ERR_CANT_OPEN;
	}

	int sources = MIDIGetNumberOfSources();
	for (int i = 0; i < sources; i++) {

		MIDIEndpointRef source = MIDIGetSource(i);
		if (source) {
			MIDIPortConnectSource(port_in, source, (void *)this);
			connected_sources.insert(i, source);
		}
	}

	return OK;
	
	input_consumer = new MidiInternalLocalConsumer("Godot");
	if (input_consumer == NULL) {
		ERR_PRINTS("MidiInternalLocalConsumer construction failed");
		return ERR_CANT_OPEN;
	}
	
	
}

void MIDIDriverMIDI2Kit::close() {

	for (int index = 0; index < connected_sources.size(); index++) {
		int32 producer_id = connected_sources[index];
		MIDIPortDisconnectSource(port_in, source);
	}
	connected_sources.clear();

	if (port_in != 0) {
		MIDIPortDispose(port_in);
		port_in = 0;
	}

	if (client != 0) {
		MIDIClientDispose(client);
		client = 0;
	}
}

PoolStringArray MIDIDriverMIDI2Kit::get_connected_inputs() {

	PoolStringArray list;

	for (int i = 0; i < connected_sources.size(); i++) {
		MIDIEndpointRef source = connected_sources[i];
		CFStringRef ref = NULL;
		char name[256];

		MIDIObjectGetStringProperty(source, kMIDIPropertyDisplayName, &ref);
		CFStringGetCString(ref, name, sizeof(name), kCFStringEncodingUTF8);
		CFRelease(ref);

		list.push_back(name);
	}

	return list;

	bool more_producers = true;
	int32 id = 0;
	while (more_producers) {
		BMidiProducer *producer = BMidiRoster::NextProducer(&id);
		if (producer != NULL) {
			list.push_back(producer->Name());
			producer->Release();
		} else {
			more_producers = false;
		}
	}

	return list;
	
	PoolStringArray list;
	
	for (int index = 0; index < connected_sources.size(); index++) {
		int32 producer_id = connected_sources[index];
		BMidiProducer *producer 
	}
}

MIDIDriverMIDI2Kit::MIDIDriverMIDI2Kit() :
		input_consumer(NULL) {
}

MIDIDriverMIDI2Kit::~MIDIDriverMIDI2Kit() {
	close();
}

#endif // COREMIDI_ENABLED
