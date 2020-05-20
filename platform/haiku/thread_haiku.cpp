/*************************************************************************/
/*  thread_haiku.cpp                                                     */
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

#include "thread_haiku.h"

#include "core/script_language.h"

#ifndef NO_THREADS

#include <kernel/OS.h>

#include "core/os/memory.h"
#include "core/safe_refcount.h"

Thread::ID ThreadHaiku::get_id() const {
	return id;
}

Thread *ThreadHaiku::create_thread_haiku() {
	return memnew(ThreadHaiku);
}

status_t ThreadHaiku::thread_callback(void *userdata) {
	ThreadHaiku *th = reinterpret_cast<ThreadHaiku *>(userdata);

	ScriptServer::thread_enter(); //scripts may need to attach a stack

	th->id = (ID)find_thread(NULL);
	th->callback(th->user);

	ScriptServer::thread_exit();

	return B_OK;
}

Thread *ThreadHaiku::create_func_haiku(ThreadCreateCallback p_callback, void *p_user, const Settings &p_settings) {
	ThreadHaiku *th = memnew(ThreadHaiku);
	th->callback = p_callback;
	th->user = p_user;

	int32 priority = B_NORMAL_PRIORITY;
	switch (p_settings.priority) {
		case PRIORITY_LOW:
			priority = B_LOW_PRIORITY;
		case PRIORITY_NORMAL:
			priority = B_NORMAL_PRIORITY;
		case PRIORITY_HIGH:
			priority = B_DISPLAY_PRIORITY;
	}

	th->threadId = spawn_thread(thread_callback, "Godot thread", priority, th);
	resume_thread(th->threadId);

	return th;
}

Thread::ID ThreadHaiku::get_thread_id_func_haiku() {
	return (ID)find_thread(NULL);
}

void ThreadHaiku::wait_to_finish_func_haiku(Thread *p_thread) {
	ThreadHaiku *th = static_cast<ThreadHaiku *>(p_thread);
	ERR_FAIL_COND(!th);
	ERR_FAIL_COND(th->threadId == -1);

	wait_for_thread(th->threadId, NULL);
	th->threadId = -1;
}

Error ThreadHaiku::set_name_func_haiku(const String &p_name) {
	status_t result = rename_thread(find_thread(NULL), p_name.utf8().get_data());
	return result == B_OK ? OK : ERR_INVALID_PARAMETER;
};

void ThreadHaiku::make_default() {
	create_func = create_func_haiku;
	get_thread_id_func = get_thread_id_func_haiku;
	wait_to_finish_func = wait_to_finish_func_haiku;
	set_name_func = set_name_func_haiku;
}

ThreadHaiku::ThreadHaiku() {
	threadId = -1;
}

ThreadHaiku::~ThreadHaiku() {
}

#endif
