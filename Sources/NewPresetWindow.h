/*

	NewPresetWindow.h

	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#ifndef _NEW_PRESET_WINDOW_H_
#define _NEW_PRESET_WINDOW_H_

#include <Window.h>

class NewPresetWindow : public BWindow {
public:
					NewPresetWindow(BMessenger* messenger, BRect parent, int preset);
					~NewPresetWindow();
	virtual void	MessageReceived(BMessage *message);
private:
	BMessenger *	messenger;
	BTextControl *	textcontrol;
	int				preset;
};

#endif // _NEW_PRESET_WINDOW_H_
