/*************************************************************************/
/*  haiku_direct_window.cpp                                              */
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

#include <locale/UnicodeChar.h>
#include <storage/Entry.h>
#include <storage/Path.h>

#include "core/os/keyboard.h"
#include "haiku_window.h"
#include "key_mapping_haiku.h"
#include "main/main.h"

HaikuWindow::HaikuWindow(BRect p_frame)
		: BWindow(p_frame, "Godot", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE) {
	last_mouse_pos_valid = false;
	last_buttons_state = 0;
	last_button_mask = 0;
	last_key_modifier_state = 0;
	cursor_grab_mode = false;

	view = nullptr;
	input = nullptr;
	main_loop = nullptr;
}

HaikuWindow::~HaikuWindow() {
}

void HaikuWindow::SetHaikuGLView(HaikuGLView *p_view) {
	view = p_view;
}

void HaikuWindow::SetInput(InputDefault *p_input) {
	input = p_input;
}

void HaikuWindow::SetMainLoop(MainLoop *p_main_loop) {
	main_loop = p_main_loop;
}

bool HaikuWindow::QuitRequested() {
	main_loop->notification(MainLoop::NOTIFICATION_WM_QUIT_REQUEST);
	return true;
}

void HaikuWindow::DispatchMessage(BMessage *message, BHandler *handler) {
	switch (message->what) {
		case B_MOUSE_DOWN:
		case B_MOUSE_UP:
			HandleMouseButton(message);
			break;

		case B_MOUSE_MOVED:
			HandleMouseMoved(message);
			break;

		case B_MOUSE_WHEEL_CHANGED:
			HandleMouseWheelChanged(message);
			break;

		case B_KEY_DOWN:
		case B_KEY_UP:
			HandleKeyboardEvent(message);
			break;

		case B_MODIFIERS_CHANGED:
			HandleKeyboardModifierEvent(message);
			break;

		case B_WINDOW_RESIZED:
			HandleWindowResized(message);
			break;

		case B_WINDOW_ACTIVATED:
			HandleWindowActivated(message);
			break;

		case B_SIMPLE_DATA:
			HandleDragData(message);
			break;

		default:
			BWindow::DispatchMessage(message, handler);
	}
}

void HaikuWindow::HandleMouseButton(BMessage *message) {
	BPoint where;
	if (message->FindPoint("where", &where) != B_OK) {
		return;
	}

	uint32 modifiers = message->FindInt32("modifiers");
	uint32 buttons = message->FindInt32("buttons");
	uint32 button = buttons ^ last_buttons_state;
	last_buttons_state = buttons;

	Ref<InputEventMouseButton> mouse_event;
	mouse_event.instance();

	mouse_event->set_button_mask(GetMouseButtonState(buttons));
	mouse_event->set_position(Vector2(where.x, where.y));
	mouse_event->set_global_position(Vector2(where.x, where.y));
	GetKeyModifierState(mouse_event, modifiers);

	switch (button) {
		default:
		case B_PRIMARY_MOUSE_BUTTON:
			mouse_event->set_button_index(BUTTON_LEFT);
			break;

		case B_SECONDARY_MOUSE_BUTTON:
			mouse_event->set_button_index(BUTTON_RIGHT);
			break;

		case B_TERTIARY_MOUSE_BUTTON:
			mouse_event->set_button_index(BUTTON_MIDDLE);
			break;
	}

	mouse_event->set_pressed(message->what == B_MOUSE_DOWN);

	if (message->what == B_MOUSE_DOWN && mouse_event->get_button_index() == BUTTON_LEFT) {
		int32 clicks = message->FindInt32("clicks");
		if (clicks > 1)
			mouse_event->set_doubleclick(true);
	}

	input->parse_input_event(mouse_event);
}

void HaikuWindow::HandleMouseMoved(BMessage *message) {
	if (main_loop) {
		uint32 transit = 0;
		if (message->FindInt32("be:transit", transit) == B_OK) {
			if (transit == B_ENTERED_VIEW) {
				main_loop->notification(MainLoop::NOTIFICATION_WM_MOUSE_ENTER);
			} else if (transit == B_EXITED_VIEW) {
				main_loop->notification(MainLoop::NOTIFICATION_WM_MOUSE_EXIT);
			}
		}
	}

	BPoint where;
	// Is it tablet time?
	if ((message->FindFloat("be:tablet_x", &where.x) != B_OK)
	     || (message->FindFloat("be:tablet_y", &where.y) != B_OK)) {
		// No...Is it mouse time?
		if (message->FindPoint("where", &where) != B_OK) {
			// Oh no...it seems there is no time...
			return;
		}
	}

	Point2i pos(where.x, where.y);
	uint32 modifiers = message->FindInt32("modifiers");
	uint32 buttons = message->FindInt32("buttons");

	if (!last_mouse_pos_valid) {
		last_mouse_position = pos;
		last_mouse_pos_valid = true;
	}

	Point2i rel = pos - last_mouse_position;

	Ref<InputEventMouseMotion> motion_event;
	motion_event.instance();

	GetKeyModifierState(motion_event, modifiers);

	motion_event->set_button_mask(GetMouseButtonState(buttons));
	motion_event->set_position(Vector2(pos.x, pos.y));
	input->set_mouse_position(pos);

	motion_event->set_global_position(Vector2(pos.x, pos.y));
	motion_event->set_speed(Vector2(input->get_last_mouse_speed().x,
			input->get_last_mouse_speed().y));
	motion_event->set_relative(Vector2(rel.x, rel.y));

	last_mouse_position = pos;

	 // Add some scrumptious tablet info if available
	float under_pressure;
	if (message->FindFloat("be:tablet_pressure", &under_pressure) == B_OK)
		motion_event->set_pressure(under_pressure);

	float tiltX, tiltY;
	if (message->FindFloat("be:tablet_tilt_x", &tiltX) == B_OK
	    && message->FindFloat("be:tablet_tilt_y", &tiltY) == B_OK)
		motion_event->set_tilt(Vector2(tiltX, tiltY));

	input->parse_input_event(motion_event);
}

void HaikuWindow::HandleMouseWheelChanged(BMessage *message) {
	Ref<InputEventMouseButton> scroll_event;
	scroll_event.instance();

	key_info info;
	if (get_key_info(&info) == B_OK)
		GetKeyModifierState(scroll_event, info.modifiers);

	scroll_event->set_button_mask(last_button_mask);
	scroll_event->set_position(Vector2(last_mouse_position.x,
			last_mouse_position.y));
	scroll_event->set_global_position(Vector2(last_mouse_position.x,
			last_mouse_position.y));

	// Vertical scrolling...
	float wheel_delta_y = 0;
	if (message->FindFloat("be:wheel_delta_y", &wheel_delta_y) == B_OK) {
		scroll_event->set_button_index(wheel_delta_y > 0 ? BUTTON_WHEEL_UP : BUTTON_WHEEL_DOWN);
		scroll_event->set_factor(wheel_delta_y);

		scroll_event->set_pressed(true);
		input->parse_input_event(scroll_event);

		scroll_event->set_pressed(false);
		input->parse_input_event(scroll_event);
	}

	// and horizontal scrolling! Is this the 21st century or what???
	float wheel_delta_x = 0;
	if (message->FindFloat("be:wheel_delta_x", &wheel_delta_x) == B_OK) {
		scroll_event->set_button_index(wheel_delta_x > 0 ? BUTTON_WHEEL_RIGHT : BUTTON_WHEEL_LEFT);
		scroll_event->set_factor(wheel_delta_x);

		scroll_event->set_pressed(true);
		input->parse_input_event(scroll_event);

		scroll_event->set_pressed(false);
		input->parse_input_event(scroll_event);
	}
}

void HaikuWindow::HandleKeyboardEvent(BMessage *message) {
	int32 raw_char = 0;
	int32 key = 0;
	int32 modifiers = 0;

	if (message->FindInt32("raw_char", &raw_char) != B_OK)
		return;

	if (message->FindInt32("key", &key) != B_OK)
		return;

	if (message->FindInt32("modifiers", &modifiers) != B_OK)
		return;

	Ref<InputEventKey> event;
	event.instance();
	GetKeyModifierState(event, modifiers);
	event->set_pressed(message->what == B_KEY_DOWN);
	event->set_scancode(KeyMappingHaiku::get_keysym(raw_char, key));
	event->set_echo(message->HasInt32("be:key_repeat"));
	event->set_unicode(0);

	const char *bytes = NULL;
	if (message->FindString("bytes", &bytes) == B_OK) {
		event->set_unicode(BUnicodeChar::FromUTF8(&bytes));
	}

	//make it consistent across platforms.
	if (event->get_scancode() == KEY_BACKTAB) {
		event->set_scancode(KEY_TAB);
		event->set_shift(true);
	}

	input->parse_input_event(event);
}

void HaikuWindow::HandleKeyboardModifierEvent(BMessage *message) {
	int32 old_modifiers = 0;
	int32 modifiers = 0;

	if (message->FindInt32("be:old_modifiers", &old_modifiers) != B_OK) {
		return;
	}

	if (message->FindInt32("modifiers", &modifiers) != B_OK) {
		return;
	}

	int32 key = old_modifiers ^ modifiers;

	Ref<InputEventWithModifiers> event;
	event.instance();
	GetKeyModifierState(event, modifiers);

	event->set_shift(key & B_SHIFT_KEY);
	event->set_alt(key & B_OPTION_KEY);
	event->set_control(key & B_CONTROL_KEY);
	event->set_command(key & B_COMMAND_KEY);

	input->parse_input_event(event);
}

void HaikuWindow::HandleWindowResized(BMessage *message) {
	int32 width = 0;
	int32 height = 0;

	if ((message->FindInt32("width", &width) != B_OK) ||
		(message->FindInt32("height", &height) != B_OK)) {
		return;
	}

	current_video_mode->width = width;
	current_video_mode->height = height;
}

void HaikuWindow::HandleWindowActivated(BMessage *message) {
	bool active = false;

	if (message->FindBool("active", &active) != B_OK)
		return;

	if (main_loop) {
		main_loop->notification(active ? MainLoop::NOTIFICATION_WM_FOCUS_IN :
			MainLoop::NOTIFICATION_WM_FOCUS_OUT);
	}
}

void HaikuWindow::HandleDragData(BMessage *message) {
	BPath path;
	BEntry entry;
	entry_ref ref;
	int32 index = 0;

	Vector<String> files;
	while (message->FindRef("refs", index++, &ref) == B_OK) {
		if (entry.SetTo(&ref) != B_OK || entry.GetPath(&path) != B_OK) {
			continue;
		}

		String file = path.Path();
		files.push_back(file);
	}

	if (OS::get_singleton()->get_main_loop() != NULL && files.size()) {
		OS::get_singleton()->get_main_loop()->drop_files(files, 0);
	}
}

inline void HaikuWindow::GetKeyModifierState(Ref<InputEventWithModifiers> event, uint32 p_state) {
	last_key_modifier_state = p_state;

	event->set_shift(p_state & B_SHIFT_KEY);
	event->set_control(p_state & B_CONTROL_KEY);
	event->set_alt(p_state & B_OPTION_KEY);
	event->set_metakey(p_state & B_COMMAND_KEY);
}

inline int HaikuWindow::GetMouseButtonState(uint32 p_state) {
	int state = 0;

	if (p_state & B_PRIMARY_MOUSE_BUTTON) {
		state |= BUTTON_MASK_LEFT;
	}

	if (p_state & B_SECONDARY_MOUSE_BUTTON) {
		state |= BUTTON_MASK_RIGHT;
	}

	if (p_state & B_TERTIARY_MOUSE_BUTTON) {
		state |= BUTTON_MASK_MIDDLE;
	}

	last_button_mask = state;

	return state;
}
