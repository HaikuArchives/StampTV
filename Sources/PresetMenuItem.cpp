/*

	PresetMenuItem.cpp

	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#include "PresetMenuItem.h"

#include <stdio.h>

PresetMenuItem::PresetMenuItem(const char* label, int index, BMessage* message)
			:BMenuItem(label, message), vpos(-1)
{
	PresetIndexToShorcutName(index, shortcut);
}

PresetMenuItem::PresetMenuItem(const char* label, char* rightText, BMessage* message)
			:BMenuItem(label, message), vpos(-1)
{
	strcpy(shortcut, rightText);
}

void PresetMenuItem::DrawContent()
{
	BMenuItem::DrawContent();
	BRect	frame = Frame();
	BMenu * menu = Menu();
	BPoint	point = menu->PenLocation();
	if (vpos < 0)
		vpos = point.y;
	else
		point.y = vpos;
	point.x = frame.right - menu->StringWidth(shortcut) - 3;
	menu->MovePenTo(point);
	if (IsSelected())
		menu->SetLowColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_HIGHLIGHT_BACKGROUND_TINT));
	else
		menu->SetLowColor(ui_color(B_MENU_BACKGROUND_COLOR));
	menu->DrawString(shortcut);
}

void PresetMenuItem::GetContentSize(float* width, float* height)
{
	BMenuItem::GetContentSize(width, height);
	*width += 8;
}

void PresetMenuItem::PresetIndexToShorcutName(int index, char * shortcut)
{
	if (index < 12)
		sprintf(shortcut, "F%d", index + 1);
	else
	{
		shortcut[0] = 'A' + index - 12;
		shortcut[1] = 0;
	}
}
