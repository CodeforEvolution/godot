/*************************************************************************/
/*  thread_haiku.h                                                       */
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

#ifndef THREAD_HAIKU_H
#define THREAD_HAIKU_H

#ifndef NO_THREADS

#include <kernel/OS.h>

#include "core/os/thread.h"

class ThreadHaiku : public Thread {
	ThreadCreateCallback callback;
	void *user;
	ID id;
	thread_id threadId;

	static Thread *create_thread_haiku();

	static status_t thread_callback(void *userdata);

	static Thread *create_func_haiku(ThreadCreateCallback p_callback, void *p_user, const Settings &p_settings);
	static ID get_thread_id_func_haiku();
	static void wait_to_finish_func_haiku(Thread *p_thread);

	static Error set_name_func_haiku(const String &p_name);

	ThreadHaiku();

public:
	virtual ID get_id() const;

	static void make_default();

	~ThreadHaiku();
};

#endif

#endif
