/*

	PopUpMenuTracker.h

	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#ifndef _POP_UP_MENU_TRACKER_H_
#define _POP_UP_MENU_TRACKER_H_

#include <PopUpMenu.h>
#include <List.h>

class PopUpMenuTracker : public BPopUpMenu
{
public:
					PopUpMenuTracker(	BLooper * where, BHandler * who, uint32 what,
										const char *title, bool radioMode = true,
										bool autoRename = true,
										menu_layout layout = B_ITEMS_IN_COLUMN
										);
	virtual			~PopUpMenuTracker();
			void	AddWebToDelete(BParameterWeb * web);
private:
	BLooper			*where;
	BHandler		*who;
	uint32			what;
	BList			webs;
};

#endif // _POP_UP_MENU_TRACKER_H_
