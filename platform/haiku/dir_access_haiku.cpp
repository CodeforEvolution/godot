/*************************************************************************/
/*  dir_access_haiku.cpp                                                  */
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

#include "dir_access_haiku.h"

#if defined(UNIX_ENABLED) || defined(LIBC_FILEIO_ENABLED)

#include <storage/Directory.h>
#include <storage/Entry.h>
#include <storage/Path.h>
#include <storage/Volume.h>
#include <storage/VolumeRoster.h>

#include "core/list.h"
#include "core/os/memory.h"
#include "core/print_string.h"

static void _get_drives(List<String> *list) {
	// Add all mounted partitions
	BVolumeRoster volumeRoster;
	BVolume volume;

	volumeRoster.Rewind();
	while (volumeRoster.GetNextVolume(&volume) == B_OK) {
		BDirectory rootDir;
		BEntry entry;
		BPath path;

		if (volume.GetRootDirectory(&rootDir) == B_OK &&
			rootDir.GetEntry(&entry) == B_OK &&
			entry.GetPath(&path) == B_OK &&
			!list->find(path.Path())) {
				list->push_back(path.Path());
		}
	}

	// Add $HOME
	const char *home = getenv("HOME");
	if (home && !list->find(home)) {
		list->push_back(home);
	}

	list->sort();
}

int DirAccessHaiku::get_drive_count() {
	List<String> list;
	_get_drives(&list);

	return list.size();
}

String DirAccessHaiku::get_drive(int p_drive) {
	List<String> list;
	_get_drives(&list);

	ERR_FAIL_INDEX_V(p_drive, list.size(), "");

	return list[p_drive];
}

String DirAccessHaiku::get_filesystem_type() const {
	String path = fix_path(const_cast<DirAccessHaiku *>(this)->get_current_dir());

	dev_t currentVolume = dev_for_path(path.utf8());
	if (currentVolume < B_NO_ERROR) {
		ERR_FAIL_V("");
	}

	fs_info info;
	if (fs_stat_dev(currentVolume, &info) != B_OK) {
		ERR_FAIL_V("");
	}

	return String(info.fsh_name);
}

#endif //posix_enabled
