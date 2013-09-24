/*

	Preferences.h

	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#ifndef _PREFERENCES_H_
#define _PREFERENCES_H_

#include <String.h>
#include <File.h>
#include <Message.h>

const int kMaxPresets = 38; // F1 - F12 + 'A' - 'Z'

class Preset
{
public:
	BString	name;
	int32	channel;
};

class Preferences : BFile
{
public:
			Preferences();
			~Preferences();
	
	int 	X;
	int 	Y;
	int		VideoSizeX;
	int		VideoSizeY;
	int		FullScreenX;
	int		FullScreenY;
	
	bool	TabLess;
	bool	StayOnTop;
	bool	StayOnScreen;
	bool	Subdivide;
	bool	FullScreen;
	bool	DisableScreenSaver;
	Preset	presets[kMaxPresets];
	uint32	PreferredMode;
	BString	AudioName;
	BMessage	Plugins;
};

// A good old nice global will do better than anything else:
// It garanties automatic construction and destruction are done at the right time
// with or without use of the BApplication object (think about replicant use).
// Concurrent uses are *not* handled specificaly.
// A proper locking could be added here. It isn't necessary at all yet.

extern Preferences gPrefs;

#endif // _PREFERENCES_H_
