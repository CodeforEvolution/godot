/*************************************************************************/
/*  os_haiku.cpp                                                         */
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

#include "drivers/gles3/rasterizer_gles3.h"

#include "os_haiku.h"

#include "main/main.h"
#include "servers/physics/physics_server_sw.h"
#include "servers/visual/visual_server_raster.h"
#include "servers/visual/visual_server_wrap_mt.h"

#include <Alert.h>
#include <Application.h>
#include <Clipboard.h>
#include <Cursor.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <kernel/fs_info.h>
#include <LocaleRoster.h>
#include <Path.h>
#include <Screen.h>
#include <Volume.h>

OS_Haiku::OS_Haiku() {
#ifdef MEDIA_KIT_ENABLED
	AudioDriverManager::add_driver(&driver_media_kit);
#endif
};

void OS_Haiku::run() {
	if (!main_loop) {
		return;
	}

	main_loop->init();
	context_gl->release_current();

	// TODO: clean up
	BMessenger *bms = new BMessenger(window);
	bms->SendMessage(LOCKGL_MSG);

	window->StartMessageRunner();
	app->Run();
	window->StopMessageRunner();

	delete app;

	delete bms;
	main_loop->finish();
}

String OS_Haiku::get_name() const {
	return "Haiku";
}

int OS_Haiku::get_video_driver_count() const {
	return 1;
}

const char *OS_Haiku::get_video_driver_name(int p_driver) const {
	return "GLES3";
}

int OS_Haiku::get_current_video_driver() const {
	return video_driver_index;
}

void OS_Haiku::initialize_core() {
	OS_Unix::initialize_core();
}

Error OS_Haiku::initialize(const VideoMode &p_desired, int p_video_driver, int p_audio_driver) {
	main_loop = NULL;
	current_video_mode = p_desired;

	app = new HaikuApplication();

	BRect frame;
	frame.Set(0, 0, current_video_mode.width - 1, current_video_mode.height - 1);

	window = new HaikuDirectWindow(frame);
	window->SetVideoMode(&current_video_mode);

	float tempMinWidth;
	float tempMaxWidth;
	float tempMinHeight;
	float tempMaxHeight;
	
	window->GetSizeLimits(&tempMinWidth, &tempMaxWidth, &tempMinHeight, &tempMaxHeight);
	
	min_size = Size2(tempMinWidth, tempMinHeight);
	max_size = Size2(tempMaxWidth, tempMaxHeight);
	
	if (current_video_mode.fullscreen) {
		current_video_mode.fullscreen = false;
		set_window_fullscreen(true);
	}
	
	if (current_video_mode.resizable) {
		current_video_mode.resizable = false;
		set_window_resizable(true);
	}
	
	if (current_video_mode.always_on_top) {
		current_video_mode.always_on_top = false;
		set_window_always_on_top(true);
	}
	
	if (current_video_mode.borderless_window) {
		current_video_mode.borderless_window = false;
		set_borderless_window(true);
	}

#if defined(OPENGL_ENABLED)
	context_gl = memnew(ContextGL_Haiku(window));
	context_gl->initialize();
	context_gl->make_current();
	context_gl->set_use_vsync(current_video_mode.use_vsync);
	RasterizerGLES3::register_config();
	RasterizerGLES3::make_current();

#endif

	visual_server = memnew(VisualServerRaster);
	// FIXME: Reimplement threaded rendering
	if (get_render_thread_mode() != RENDER_THREAD_UNSAFE) {
		visual_server = memnew(VisualServerWrapMT(visual_server, false));
	}

	ERR_FAIL_COND_V(!visual_server, ERR_UNAVAILABLE);

	video_driver_index = p_video_driver;

	input = memnew(InputDefault);
	window->SetInput(input);

	window->Show();
	visual_server->init();

	AudioDriverManager::initialize(p_audio_driver);

	return OK;
}

void OS_Haiku::finalize() {
	if (main_loop) {
		memdelete(main_loop);
	}

	main_loop = NULL;

	visual_server->finish();
	memdelete(visual_server);

	memdelete(input);

#if defined(OPENGL_ENABLED)
	memdelete(context_gl);
#endif
}

void OS_Haiku::set_main_loop(MainLoop *p_main_loop) {
	main_loop = p_main_loop;
	input->set_main_loop(p_main_loop);
	window->SetMainLoop(p_main_loop);
}

MainLoop *OS_Haiku::get_main_loop() const {
	return main_loop;
}

void OS_Haiku::delete_main_loop() {
	if (main_loop) {
		memdelete(main_loop);
	}

	main_loop = NULL;
	window->SetMainLoop(NULL);
}

void OS_Haiku::release_rendering_thread() {
	context_gl->release_current();
}

void OS_Haiku::make_rendering_thread() {
	context_gl->make_current();
}

bool OS_Haiku::can_draw() const {
	return !window->IsMinimized();
}

void OS_Haiku::swap_buffers() {
	context_gl->swap_buffers();
}

void OS_Haiku::set_clipboard(const String &p_text) {
	if (!be_clipboard->Lock())
		return;

	be_clipboard->Clear();
	BMessage *clipData = be_clipboard->Data();
	if (clipData == NULL)
		return;

	BString string(p_text.utf8().get_data());
	clipData->AddData("text/plain", B_MIME_TYPE,
		string.String(), string.Length());
	be_clipboard->Commit();

	be_clipboard->Unlock();
}

String OS_Haiku::get_clipboard() const {
	if (!be_clipboard->Lock())
		return "";
	
	BMessage *clipData = be_clipboard->Data();
	if (clipData == NULL)
		return "";

	const char* buffer;
	ssize_t bufferLength;
	clipData->FindData("text/plain", B_MIME_TYPE,
		reinterpret_cast<const void**>(&buffer), &bufferLength);
	
	BString clipStr;
	clipStr.SetTo(buffer, bufferLength);
	
	String outStr;
	outStr.parse_utf8(clipStr.String());
	
	return outStr;
}

Point2 OS_Haiku::get_mouse_position() const {
	return window->GetLastMousePosition();
}

int OS_Haiku::get_mouse_button_state() const {
	return window->GetLastButtonMask();
}

void OS_Haiku::alert(const String &p_alert, const String &p_title) {
	BAlert *alert = new BAlert(p_title.utf8().get_data(),
				   p_alert.utf8().get_data(),
				   "OK", NULL, NULL, B_WIDTH_AS_USUAL,
				   B_WARNING_ALERT);
	alert->Go(NULL);
	delete alert;
}

void OS_Haiku::set_cursor_shape(CursorShape p_shape) {
	ERR_FAIL_INDEX(p_shape, CURSOR_MAX);
	
	if (cursor_shape == p_shape)
		return;

//	if (mouse_mode != MOUSE_MODE_VISIBLE && mouse_mode != MOUSE_MODE_CONFINED) {
//		cursor_shape = p_shape;
//		return;
//	}
	
	static const BCursorID native_cursors[CURSOR_MAX] = {
		B_CURSOR_ID_SYSTEM_DEFAULT,
		B_CURSOR_ID_I_BEAM,
		B_CURSOR_ID_FOLLOW_LINK,
		B_CURSOR_ID_CROSS_HAIR,
		B_CURSOR_ID_PROGRESS,
		B_CURSOR_ID_PROGRESS,
		B_CURSOR_ID_GRABBING,
		B_CURSOR_ID_GRAB,
		B_CURSOR_ID_NOT_ALLOWED,
		B_CURSOR_ID_RESIZE_NORTH_SOUTH,
		B_CURSOR_ID_RESIZE_EAST_WEST,
		B_CURSOR_ID_RESIZE_NORTH_EAST_SOUTH_WEST,
		B_CURSOR_ID_RESIZE_NORTH_WEST_SOUTH_EAST,
		B_CURSOR_ID_MOVE,
		B_CURSOR_ID_RESIZE_NORTH_SOUTH,
		B_CURSOR_ID_RESIZE_EAST_WEST,
		B_CURSOR_ID_HELP
	};

	be_app->SetCursor(new BCursor(native_cursors[p_shape]));

	cursor_shape = p_shape;
}

OS::CursorShape OS_Haiku::get_cursor_shape() const {
	return cursor_shape;
}

void OS_Haiku::set_custom_mouse_cursor(const RES &p_cursor, CursorShape p_shape, const Vector2 &p_hotspot) {
	// TODO
}

int OS_Haiku::get_screen_count() const {
	// TODO: implement get_screen_count()
	return 1;
}

int OS_Haiku::get_current_screen() const {
	// TODO: implement get_current_screen()
	return 0;
}

void OS_Haiku::set_current_screen(int p_screen) {
	// TODO: implement set_current_screen()
}

Point2 OS_Haiku::get_screen_position(int p_screen) const {
	// TODO: make this work with the p_screen parameter
	BScreen *screen = new BScreen(window);
	BRect frame = screen->Frame();
	delete screen;
	return Point2i(frame.left, frame.top);
}

Size2 OS_Haiku::get_screen_size(int p_screen) const {
	// TODO: make this work with the p_screen parameter
	BScreen *screen = new BScreen(window);
	BRect frame = screen->Frame();
	delete screen;
	return Size2i(frame.IntegerWidth() + 1, frame.IntegerHeight() + 1);
}

void OS_Haiku::set_window_title(const String &p_title) {
	window->SetTitle(p_title.utf8().get_data());
}

Size2 OS_Haiku::get_window_size() const {
	BSize size = window->Size();
	return Size2i(size.IntegerWidth() + 1, size.IntegerHeight() + 1);
}

void OS_Haiku::set_window_size(const Size2 p_size) {
	// TODO: why does it stop redrawing after this is called?
	window->ResizeTo(p_size.x, p_size.y);
}

Size2 OS_Haiku::get_max_window_size() const {
	return max_size;
};

void OS_Haiku::set_max_window_size(const Size2 p_size) {
	if ((p_size != Size2()) && ((p_size.x < min_size.x) || (p_size.y < min_size.y))) {
		ERR_PRINT("Maximum window size can't be smaller than minimum window size!");
		return;
	}
	
	window->SetSizeLimits(min_size.width, p_size.width, min_size.height, p_size.height);
	max_size = p_size;
}

Size2 OS_Haiku::get_min_window_size() const {
	return min_size;
};

void OS_Haiku::set_min_window_size(const Size2 p_size) {
	if ((p_size != Size2()) && (max_size != Size2()) && ((p_size.x > max_size.x) || (p_size.y > max_size.y))) {
		ERR_PRINT("Minimum window size can't be larger than maximum window size!");
		return;
	}
	
	window->SetSizeLimits(p_size.width, max_size.width, p_size.height, max_size.height);
	min_size = p_size;
}

Point2 OS_Haiku::get_window_position() const {
	BPoint point(0, 0);
	window->ConvertToScreen(&point);
	return Point2i(point.x, point.y);
}

void OS_Haiku::set_window_position(const Point2 &p_position) {
	window->MoveTo(p_position.x, p_position.y);
}

void OS_Haiku::set_window_fullscreen(bool p_enabled) {
	window->SetFullScreen(p_enabled);
	current_video_mode.fullscreen = p_enabled;
	visual_server->init();
}

bool OS_Haiku::is_window_fullscreen() const {
	return current_video_mode.fullscreen;
}

void OS_Haiku::set_window_resizable(bool p_enabled) {
	uint32 flags = window->Flags();

	if (p_enabled) {
		flags &= ~(B_NOT_RESIZABLE);
	} else {
		flags |= B_NOT_RESIZABLE;
	}

	window->SetFlags(flags);
	current_video_mode.resizable = p_enabled;
}

bool OS_Haiku::is_window_resizable() const {
	return current_video_mode.resizable;
}

void OS_Haiku::set_window_minimized(bool p_enabled) {
	window->Minimize(p_enabled);
}

bool OS_Haiku::is_window_minimized() const {
	return window->IsMinimized();
}

void OS_Haiku::set_window_maximized(bool p_enabled) {
	window->Minimize(!p_enabled);
}

bool OS_Haiku::is_window_maximized() const {
	return !window->IsMinimized();
}

void OS_Haiku::set_window_always_on_top(bool p_enabled) {
	if (current_video_mode.always_on_top == p_enabled)
		return;

	status_t result = window->SetFeel(p_enabled ? B_FLOATING_ALL_WINDOW_FEEL :
						      B_NORMAL_WINDOW_FEEL);
	
	if (result == B_OK)
		current_video_mode.always_on_top = p_enabled;
}

bool OS_Haiku::is_window_always_on_top() const {
	return current_video_mode.always_on_top;
}

bool OS_Haiku::is_window_focused() const {
	return window->IsActive();	
}

void OS_Haiku::set_borderless_window(bool p_borderless) {
	if (current_video_mode.borderless_window == p_borderless)
		return;
	
	status_t result = window->SetLook(p_borderless ? B_NO_BORDER_WINDOW_LOOK :
							 B_DOCUMENT_WINDOW_LOOK);
	
	if (result == B_OK)
		current_video_mode.borderless_window = p_borderless;
}

bool OS_Haiku::get_borderless_window() {
	return current_video_mode.borderless_window;
}

void OS_Haiku::move_window_to_foreground() {
	window->Activate();	
}

String OS_Haiku::get_locale() const {
	BMessage preferredLanguages;
	BLocaleRoster::Default()->GetPreferredLanguages(&preferredLanguages);
	const char* firstPreferredLanguage;
	if (preferredLanguages.FindString("language", &firstPreferredLanguage)
			!= B_OK) {
		// Default to English
		firstPreferredLanguage = "en";
	}

	return firstPreferredLanguage;
}

void OS_Haiku::set_video_mode(const VideoMode &p_video_mode, int p_screen) {
	ERR_PRINT("set_video_mode() DEPRECATED");
}

OS::VideoMode OS_Haiku::get_video_mode(int p_screen) const {
	return current_video_mode;
}

void OS_Haiku::get_fullscreen_mode_list(List<VideoMode> *p_list, int p_screen) const {
	ERR_PRINT("get_fullscreen_mode_list() DEPRECATED");
}

String OS_Haiku::get_executable_path() const {
	char pathBuffer[B_PATH_NAME_LENGTH];		
	image_info info;
	int32 cookie = 0;

	while (get_next_image_info(B_CURRENT_TEAM, &cookie, &info) == B_OK) {
		if (info.type == B_APP_IMAGE) {
			strlcpy(pathBuffer, info.name, B_PATH_NAME_LENGTH - 1);

			String path;
			path.parse_utf8(pathBuffer);
			return path;
		}
	}
	
	return OS::get_executable_path();
}

bool OS_Haiku::_check_internal_feature_support(const String &p_feature) {
	return p_feature == "pc";
}

String OS_Haiku::get_config_path() const {
	if (has_environment("XDG_CONFIG_HOME")) {
		return get_environment("XDG_CONFIG_HOME");
	} else if (has_environment("HOME")) {
		return get_environment("HOME").plus_file("config/settings");
	} else {
		return ".";
	}
}

String OS_Haiku::get_data_path() const {
	if (has_environment("XDG_DATA_HOME")) {
		return get_environment("XDG_DATA_HOME");
	} else if (has_environment("HOME")) {
		return get_environment("HOME").plus_file("config/data");
	} else {
		return get_config_path();
	}
}

String OS_Haiku::get_cache_path() const {
	if (has_environment("XDG_CACHE_HOME")) {
		return get_environment("XDG_CACHE_HOME");
	} else if (has_environment("HOME")) {
		return get_environment("HOME").plus_file("config/cache");
	} else {
		return get_config_path();
	}
}

Error OS_Haiku::move_to_trash(const String &p_path) {
	// Find device the path is on
	dev_t trashDev = dev_for_path(p_path.utf8().get_data());
	if (trashDev < B_NO_ERROR)
		return FAILED;

	// Create BVolume representing the volume the path is located on
	BVolume *trashVol = new BVolume(trashDev);
	if (trashVol == NULL || trashVol->InitCheck() != B_OK)
		return FAILED;

	// Find trash directory on volume
	BPath trashPath;
	if (find_directory(B_TRASH_DIRECTORY, &trashPath, true, trashVol) != B_OK)
		return FAILED;

	// Create BDirectory representing the trash directory
	BDirectory *trashDir = new BDirectory(trashPath.Path());
	if (trashDir == NULL || trashDir->InitCheck() != B_OK)
		return FAILED;

	// Create BEntry representing file to move to trash
	BEntry *fileEntry = new BEntry(p_path.utf8().get_data());
	if (fileEntry == NULL || fileEntry->InitCheck() != B_OK)
		return FAILED;

	// Do it now!
	if (fileEntry->MoveTo(trashDir) != B_OK) {
		// That was anti-climatic...
		return FAILED;
	}

	return OK;
}

OS::PowerState OS_Haiku::get_power_state() {
	WARN_PRINT("Power management is not implemented on this platform, defaulting to POWERSTATE_UNKNOWN");
	return OS::POWERSTATE_UNKNOWN;
}

int OS_Haiku::get_power_seconds_left() {
	WARN_PRINT("Power management is not implemented on this platform, defaulting to -1");
	return -1;
}

int OS_Haiku::get_power_percent_left() {
	WARN_PRINT("Power management is not implemented on this platform, defaulting to -1");
	return -1;
}
