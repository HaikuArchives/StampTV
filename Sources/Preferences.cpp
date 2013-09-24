/*

	Preferences.cpp

	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#include "Preferences.h"

#include <FindDirectory.h>
#include <Path.h>
#include <Mime.h>
#include <fs_attr.h>
#include <Debug.h>

Preferences gPrefs;

Preferences::Preferences()
{
	BPath	path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	path.Append("stampTV.preferences");
	SetTo(path.Path(), B_READ_WRITE | B_CREATE_FILE);

	if (ReadAttr("X", B_INT32_TYPE, 0, &X, sizeof(int)) != sizeof(int))
		X = 100;
	if (ReadAttr("Y", B_INT32_TYPE, 0, &Y, sizeof(int)) != sizeof(int))
		Y = 100;

	if (ReadAttr("VideoSizeX", B_INT32_TYPE, 0, &VideoSizeX, sizeof(int)) != sizeof(int))
		VideoSizeX = 640;
	if (ReadAttr("VideoSizeY", B_INT32_TYPE, 0, &VideoSizeY, sizeof(int)) != sizeof(int))
		VideoSizeY = VideoSizeX * 3 / 4;

	if (ReadAttr("FullScreenX", B_INT32_TYPE, 0, &FullScreenX, sizeof(int)) != sizeof(int))
		FullScreenX = 640;
	if (ReadAttr("FullScreenY", B_INT32_TYPE, 0, &FullScreenY, sizeof(int)) != sizeof(int))
		FullScreenY = FullScreenX * 3 / 4;

	if (ReadAttr("TabLess", B_BOOL_TYPE, 0, &TabLess, sizeof(bool)) != sizeof(bool))
		TabLess = false;
	if (ReadAttr("StayOnTop", B_BOOL_TYPE, 0, &StayOnTop, sizeof(bool)) != sizeof(bool))
		StayOnTop = false;
	if (ReadAttr("StayOnScreen", B_BOOL_TYPE, 0, &StayOnScreen, sizeof(bool)) != sizeof(bool))
		StayOnScreen = false;
	if (ReadAttr("Subdivide", B_BOOL_TYPE, 0, &Subdivide, sizeof(bool)) != sizeof(bool))
		Subdivide = true;
	if (ReadAttr("FullScreen", B_BOOL_TYPE, 0, &FullScreen, sizeof(bool)) != sizeof(bool))
		FullScreen = false;
	if (ReadAttr("DisableScreenSaver", B_BOOL_TYPE, 0, &DisableScreenSaver, sizeof(bool)) != sizeof(bool))
		DisableScreenSaver = false;
		
	attr_info	infos;
	bool		presets_ok = false;
	if (GetAttrInfo("Presets", &infos) == B_OK) {
		char* buffer = new char[infos.size];
		BMessage	settings;
		if (ReadAttr("Presets", infos.type, 0, buffer, infos.size) == infos.size
			&& settings.Unflatten(buffer) == B_OK && settings.what == 'Pref') {
			// presets loaded normaly
			presets_ok = true;
			for (int p = 0; p < kMaxPresets; p++) {
				if (settings.FindString("PresetName", p, &presets[p].name) != B_OK) {
					presets[p].name = "Preset ";
					presets[p].name << p + 1;
				}
				if (settings.FindInt32("PresetChannel", p, &presets[p].channel) != B_OK)
					presets[p].channel = -1;
			}
			if (settings.FindString("AudioName", &AudioName) != B_OK)
				AudioName = "Line";
		}
		delete[] buffer;
	}
	if (!presets_ok) {
		for (int p = 0; p < kMaxPresets; p++) {
			presets[p].name = "Preset ";
			presets[p].name << p + 1;
			presets[p].channel = -1;
		}
		AudioName = "Line";
	}
	
	// Plugins settings
	if (GetAttrInfo("Plugins", &infos) == B_OK) {
		char* buffer = new char[infos.size];
		BMessage	settings;
		if (ReadAttr("Plugins", infos.type, 0, buffer, infos.size) == infos.size
			&& Plugins.Unflatten(buffer) == B_OK && Plugins.what == 'Plug') {
			// OK
		} else
			Plugins.MakeEmpty();
		delete[] buffer;
	}

	if (ReadAttr("PreferredMode", B_INT32_TYPE, 0, &PreferredMode, sizeof(int32)) < 1)
		PreferredMode = 0;

	PRINT(("Settings loaded:\n\n"));
	PRINT(("X: %d\nY: %d\n", X, Y));
	PRINT(("VideoSizeX: %d\nVideoSizeY: %d\n", VideoSizeX, VideoSizeY));
	PRINT(("Tabless: %d\n", TabLess));
	PRINT(("StayOnTop: %d\n", StayOnTop));
	PRINT(("StayOnScreen: %d\n", StayOnScreen));
	PRINT(("Subdivide: %d\n", Subdivide));
}

Preferences::~Preferences()
{
	WriteAttr("X", B_INT32_TYPE, 0, &X, sizeof(int));
	WriteAttr("Y", B_INT32_TYPE, 0, &Y, sizeof(int));

	WriteAttr("VideoSizeX", B_INT32_TYPE, 0, &VideoSizeX, sizeof(int));
	WriteAttr("VideoSizeY", B_INT32_TYPE, 0, &VideoSizeY, sizeof(int));

	WriteAttr("FullScreenX", B_INT32_TYPE, 0, &FullScreenX, sizeof(int));
	WriteAttr("FullScreenY", B_INT32_TYPE, 0, &FullScreenY, sizeof(int));

	WriteAttr("TabLess", B_BOOL_TYPE, 0, &TabLess, sizeof(bool));
	WriteAttr("StayOnTop", B_BOOL_TYPE, 0, &StayOnTop, sizeof(bool));
	WriteAttr("StayOnScreen", B_BOOL_TYPE, 0, &StayOnScreen, sizeof(bool));
	WriteAttr("Subdivide", B_BOOL_TYPE, 0, &Subdivide, sizeof(bool));
	WriteAttr("FullScreen", B_BOOL_TYPE, 0, &FullScreen, sizeof(bool));
	WriteAttr("DisableScreenSaver", B_BOOL_TYPE, 0, &DisableScreenSaver, sizeof(bool));

	BMessage	settings('Pref');
	for (int p = 0; p < kMaxPresets; p++) {
		settings.AddString("PresetName", presets[p].name);
		settings.AddInt32("PresetChannel", presets[p].channel);
	}
	settings.AddString("AudioName", AudioName);
	ssize_t size = settings.FlattenedSize();
	char* buffer = new char[size];
	settings.Flatten(buffer, size);
	WriteAttr("Presets", B_MESSAGE_TYPE, 0, buffer, size);
	delete[] buffer;
	
	Plugins.what = 'Plug';
	size = Plugins.FlattenedSize();
	buffer = new char[size];
	Plugins.Flatten(buffer, size);
	WriteAttr("Plugins", B_MESSAGE_TYPE, 0, buffer, size);
	delete[] buffer;

	WriteAttr("PreferredMode", B_INT32_TYPE, 0, &PreferredMode, sizeof(int32));

	PRINT(("Settings saved:\n\n"));
	PRINT(("X: %d\nY: %d\n", X, Y));
	PRINT(("VideoSizeX: %d\nVideoSizeY: %d\n", VideoSizeX, VideoSizeY));
	PRINT(("Tabless: %d\n", TabLess));
	PRINT(("StayOnTop: %d\n", StayOnTop));
	PRINT(("StayOnScreen: %d\n", StayOnScreen));
	PRINT(("Subdivide: %d\n", Subdivide));
}
