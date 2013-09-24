/*

	PresetMenuItem.h

	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#ifndef _PRESET_MENU_ITEM_H_
#define _PRESET_MENU_ITEM_H_

#include <MenuItem.h>

enum {
	B_NO_COMMAND_KEY = 0x80000000
};

class PresetMenuItem : public BMenuItem {

public:
						PresetMenuItem(const char* label, int index, BMessage* message);
						PresetMenuItem(const char* label, char* rightText, BMessage* message);
virtual	void			DrawContent();
virtual	void			GetContentSize(float* width, float* height);

static	void			PresetIndexToShorcutName (int index, char * shortcut);

private:
		char			shortcut[32];
		float			vpos;
};

#endif // _PRESET_MENU_ITEM_H_
