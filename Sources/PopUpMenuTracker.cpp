/*

	PopUpMenuTracker.cpp

	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#include "PopUpMenuTracker.h"
#include <Looper.h>
#include <ParameterWeb.h>

// Permits notification when the popup menu disappears

PopUpMenuTracker::PopUpMenuTracker(BLooper * where, BHandler * who, uint32 what, const char *title, bool radioMode,
	bool autoRename, menu_layout layout) : BPopUpMenu(title, radioMode, autoRename),
	where(where), who(who), what(what)
{
}

PopUpMenuTracker::~PopUpMenuTracker()
{
	where->PostMessage(what, who);
	for (int w = webs.CountItems() - 1; w >= 0; w--)
		delete (BParameterWeb *) webs.ItemAtFast(w);
}

void PopUpMenuTracker::AddWebToDelete(BParameterWeb * web)
{
	webs.AddItem(web);
}