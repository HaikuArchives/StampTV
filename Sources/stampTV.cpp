/*

	stampTV.cpp

	© 1991-1999, Be Incorporated, All rights reserved, was Be's CodyCam Sample Code
	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#include "stampTV.h"
#include "VideoWindow.h"
#include "Preferences.h"

#include <Alert.h>
#include <stdio.h>
#include <Application.h>

void
ErrorAlert(const char * message, status_t err)
{
	if (err != B_OK) {
		static	bool first = true;
		if (first) {
			char msg[1024];
			sprintf(msg, "%s...\n(%s)\n\nYou should quit & restart stampTV...\n"
				"(If that is not enough, you may want to try to restart the media kit)",
				message, (const char*) (err == B_ERROR ? "-" : strerror(err)));
			(new BAlert("", msg, "Ouch!", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		}
		first = false;
	}
}

class stampTV : public BApplication {
	public:
							stampTV();
		void				ReadyToRun();
};

stampTV::stampTV() :
	BApplication("application/x-vnd.Me.stampTV")
{
}

void 
stampTV::ReadyToRun()
{
	new VideoWindow(BRect(gPrefs.X, gPrefs.Y, gPrefs.X + gPrefs.WindowWidth - 1,
			gPrefs.Y + gPrefs.WindowHeight - 1), "stampTV",
			gPrefs.TabLess ? B_MODAL_WINDOW_LOOK : B_TITLED_WINDOW_LOOK,
			gPrefs.StayOnTop ? B_FLOATING_ALL_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL,
			B_WILL_ACCEPT_FIRST_CLICK | B_ASYNCHRONOUS_CONTROLS,
			gPrefs.AllWorkspaces ? B_ALL_WORKSPACES : B_CURRENT_WORKSPACE);
}

int main()
{
	stampTV app;
	app.Run();
	return B_OK;
}
