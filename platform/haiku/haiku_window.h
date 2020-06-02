/*************************************************************************/
/*  haiku_direct_window.h                                                */
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

#ifndef HAIKU_DIRECT_WINDOW_H
#define HAIKU_DIRECT_WINDOW_H

#include <interface/Window.h>
#include <kernel/image.h> // needed for image_id

#include "core/os/os.h"
#include "main/input_default.h"

#include "haiku_gl_view.h"

class HaikuWindow : public BWindow {
private:
	Point2i last_mouse_position;
	bool last_mouse_pos_valid;
	uint32 last_buttons_state;
	uint32 last_key_modifier_state;
	int last_button_mask;
	OS::VideoMode *current_video_mode;
	bool cursor_grab_mode;

	MainLoop *main_loop;
	InputDefault *input;
	HaikuGLView *view;

	void HandleMouseButton(BMessage *message);
	void HandleMouseMoved(BMessage *message);
	void HandleMouseWheelChanged(BMessage *message);
	void HandleWindowActivated(BMessage *message);
	void HandleWindowResized(BMessage *message);
	void HandleKeyboardEvent(BMessage *message);
	void HandleKeyboardModifierEvent(BMessage *message);
	void HandleDragData(BMessage *message);
	inline void GetKeyModifierState(Ref<InputEventWithModifiers> event, uint32 p_state);
	inline int GetMouseButtonState(uint32 p_state);

public:
	HaikuWindow(BRect p_frame);
	~HaikuWindow();

	void SetHaikuGLView(HaikuGLView *p_view);
	void StartMessageRunner();
	void StopMessageRunner();
	void SetInput(InputDefault *p_input);
	inline void SetGrabCursorMode(bool p_enabled) { cursor_grab_mode = p_enabled; };
	void SetMainLoop(MainLoop *p_main_loop);
	inline void SetVideoMode(OS::VideoMode *video_mode) { current_video_mode = video_mode; };
	virtual bool QuitRequested();
	virtual void DispatchMessage(BMessage *message, BHandler *handler);

	inline void SetLastMousePosition(Point2i p_last_position) { last_mouse_position = p_last_position; };
	inline Point2i GetLastMousePosition() { return last_mouse_position; };
	inline int GetLastButtonMask() { return last_button_mask; };
};

#endif
