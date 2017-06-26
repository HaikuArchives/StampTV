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
#include <InterfaceDefs.h>

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
	if (ReadAttr("WW", B_INT32_TYPE, 0, &WindowWidth, sizeof(int)) != sizeof(int))
		WindowWidth = 640;
	if (ReadAttr("WH", B_INT32_TYPE, 0, &WindowHeight, sizeof(int)) != sizeof(int))
		WindowHeight = 480;

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
	if (ReadAttr("AllWorkspaces", B_BOOL_TYPE, 0, &AllWorkspaces, sizeof(bool)) != sizeof(bool))
		AllWorkspaces = true;
	if (ReadAttr("DisableScreenSaver", B_BOOL_TYPE, 0, &DisableScreenSaver, sizeof(bool)) != sizeof(bool))
		DisableScreenSaver = false;
	if (ReadAttr("VSIWS", B_BOOL_TYPE, 0, &VideoSizeIsWindowSize, sizeof(bool)) != sizeof(bool))
		VideoSizeIsWindowSize = true;
		
	attr_info	infos;
	bool		presets_ok = false;
	BMessage	settings;
	// Try new preset storage
	if (GetAttrInfo("FullPresets", &infos) == B_OK) {
		char * buffer = new char[infos.size];
		if (ReadAttr("FullPresets", infos.type, 0, buffer, infos.size) == infos.size
			&& settings.Unflatten(buffer) == B_OK && settings.what == 'Pref')
		{
			for (int p = 0; p < kMaxPresets; p++) {
				settings.FindString("n", p, &presets[p].name);
				if (settings.FindInt32("c", p, &presets[p].channel) != B_OK)
					presets[p].channel = LONG_MIN;
				if (settings.FindInt32("vf", p, &presets[p].videoFormat) != B_OK)
					presets[p].videoFormat = LONG_MIN;
				if (settings.FindInt32("vi", p, &presets[p].videoInput) != B_OK)
					presets[p].videoInput = LONG_MIN;
				if (settings.FindInt32("ai", p, &presets[p].audioInput) != B_OK)
					presets[p].audioInput = LONG_MIN;
				if (settings.FindInt32("tl", p, &presets[p].tunerLocale) != B_OK)
					presets[p].tunerLocale = LONG_MIN;
			}
			presets_ok = true;
		}
		delete[] buffer;
	}
	// Try to load older preset format if present
	if (!presets_ok && GetAttrInfo("Presets", &infos) == B_OK) {
		char * buffer = new char[infos.size];
		if (ReadAttr("Presets", infos.type, 0, buffer, infos.size) == infos.size
			&& settings.Unflatten(buffer) == B_OK && settings.what == 'Pref')
		{
			for (int p = 0; p < kMaxPresets; p++) {
				settings.FindString("PresetName", p, &presets[p].name);
				if (settings.FindInt32("PresetChannel", p, &presets[p].channel) != B_OK)
					presets[p].channel = -1;
			}
			presets_ok = true;
		}
		delete[] buffer;
	}
	if (settings.FindString("AudioName", &AudioName) != B_OK)
		AudioName = "Line";
	
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
}

Preferences::~Preferences()
{
	WriteAttr("X", B_INT32_TYPE, 0, &X, sizeof(int));
	WriteAttr("Y", B_INT32_TYPE, 0, &Y, sizeof(int));

	WriteAttr("VideoSizeX", B_INT32_TYPE, 0, &VideoSizeX, sizeof(int));
	WriteAttr("VideoSizeY", B_INT32_TYPE, 0, &VideoSizeY, sizeof(int));

	WriteAttr("WW", B_INT32_TYPE, 0, &WindowWidth, sizeof(int));
	WriteAttr("WH", B_INT32_TYPE, 0, &WindowHeight, sizeof(int));

	WriteAttr("FullScreenX", B_INT32_TYPE, 0, &FullScreenX, sizeof(int));
	WriteAttr("FullScreenY", B_INT32_TYPE, 0, &FullScreenY, sizeof(int));

	WriteAttr("TabLess", B_BOOL_TYPE, 0, &TabLess, sizeof(bool));
	WriteAttr("StayOnTop", B_BOOL_TYPE, 0, &StayOnTop, sizeof(bool));
	WriteAttr("StayOnScreen", B_BOOL_TYPE, 0, &StayOnScreen, sizeof(bool));
	WriteAttr("Subdivide", B_BOOL_TYPE, 0, &Subdivide, sizeof(bool));
	WriteAttr("FullScreen", B_BOOL_TYPE, 0, &FullScreen, sizeof(bool));
	WriteAttr("AllWorkspaces", B_BOOL_TYPE, 0, &AllWorkspaces, sizeof(bool));
	WriteAttr("DisableScreenSaver", B_BOOL_TYPE, 0, &DisableScreenSaver, sizeof(bool));
	WriteAttr("VSIWS", B_BOOL_TYPE, 0, &VideoSizeIsWindowSize, sizeof(bool));

	BMessage	settings('Pref');
	for (int p = 0; p < kMaxPresets; p++) {
		settings.AddString("n", presets[p].name);
		settings.AddInt32("c", presets[p].channel);
		settings.AddInt32("vf", presets[p].videoFormat);
		settings.AddInt32("vi", presets[p].videoInput);
		settings.AddInt32("ai", presets[p].audioInput);
		settings.AddInt32("tl", presets[p].tunerLocale);
	}
	settings.AddString("AudioName", AudioName);
	ssize_t size = settings.FlattenedSize();
	char* buffer = new char[size];
	settings.Flatten(buffer, size);
	WriteAttr("FullPresets", B_MESSAGE_TYPE, 0, buffer, size);
	delete[] buffer;
	
	Plugins.what = 'Plug';
	size = Plugins.FlattenedSize();
	buffer = new char[size];
	Plugins.Flatten(buffer, size);
	WriteAttr("Plugins", B_MESSAGE_TYPE, 0, buffer, size);
	delete[] buffer;

	WriteAttr("PreferredMode", B_INT32_TYPE, 0, &PreferredMode, sizeof(int32));
}

bool Preferences::ShouldStayOnScreen()
{
	return StayOnScreen && (modifiers() & B_OPTION_KEY) == 0;
}

void Preferences::CheckAndConvert(Preset & model)
{
	for (int k = 0; k < kMaxPresets; k++) {
		Preset & p = gPrefs.presets[k];
		if (p.IsValid()) {
			// Set default values in case of convertion (& check)
			if (p.videoFormat == LONG_MIN)
				p.videoFormat = model.videoFormat;
			if (p.videoInput == LONG_MIN)
				p.videoInput = model.videoInput;
			if (p.audioInput == LONG_MIN)
				p.audioInput = model.audioInput;
			if (p.tunerLocale == LONG_MIN)
				p.tunerLocale = model.tunerLocale;
		}
		// Check that a name exists
		if (p.name.Length() < 1) {
			p.name = "Preset ";
			p.name << k + 1;
		}
		// Remove double presets
		for (int n = 0; n < k; n++) {
			if (p == gPrefs.presets[n])
				p.Unset(false);
		}
	}
}
