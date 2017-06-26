/*

	Preferences.h

	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#ifndef _PREFERENCES_H_
#define _PREFERENCES_H_

#include <File.h>
#include <Message.h>

#include "Preset.h"

class Preferences : BFile
{
public:
			Preferences();
			~Preferences();

	// Accessors to know how the setting should be considered "right now"
	// Allows temporary changes, like when a modifier is pressed.
	bool	ShouldStayOnScreen();
	void	CheckAndConvert(Preset & model);
	
	int32 	X;
	int32 	Y;
	int32	WindowWidth;
	int32	WindowHeight;
	int32	VideoSizeX;
	int32	VideoSizeY;
	int32	FullScreenX;
	int32	FullScreenY;
	
	bool	TabLess;
	bool	StayOnTop;
	bool	StayOnScreen;
	bool	Subdivide;
	bool	FullScreen;
	bool	AllWorkspaces;
	bool	DisableScreenSaver;
	bool	VideoSizeIsWindowSize;
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
