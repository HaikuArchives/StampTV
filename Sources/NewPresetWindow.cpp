/*

	NewPresetWindow.cpp

	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#include "NewPresetWindow.h"
#include "PresetMenuItem.h"
#include "Preferences.h"
#include "stampView.h"

#include <MessageFilter.h>
#include <View.h>
#include <Message.h>
#include <TextControl.h>
#include <Button.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <stdio.h>

filter_result esc_interceptor(BMessage *message, BHandler **target, BMessageFilter *filter)
{
	int8	byte;
	if (message->FindInt8("byte", &byte) == B_OK) {
		if (byte == B_ESCAPE) {
			filter->Looper()->PostMessage(B_QUIT_REQUESTED);
			return B_SKIP_MESSAGE;
		}
	}
	return B_DISPATCH_MESSAGE;
}

NewPresetWindow::NewPresetWindow(BMessenger* messenger, BRect parent, int preset)
	: BWindow(BRect(0, 0, 360, 90), "Preset Name", B_MODAL_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
		B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
		messenger(messenger), preset(preset)
{
	BRect bounds = Bounds();
	BView * back = new BView(bounds, B_EMPTY_STRING, 0, B_WILL_DRAW);
	back->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BRect r(15, 10, bounds.right - 170, 25);
	textcontrol = new BTextControl(r, B_EMPTY_STRING, "Preset Name:", gPrefs.presets[preset].name.String(), NULL);
	textcontrol->SetDivider(70);
	back->AddChild(textcontrol);

	BPopUpMenu * presets = new BPopUpMenu("<Select>");
	presets->SetFont(be_plain_font);
	for (int p = 0; p < kMaxPresets; p++) {
		BMessage * m = new BMessage('Shor');
		m->AddInt32("preset", p);
		m->AddString("name", gPrefs.presets[p].name.String());
		char	shortcut[4];
		PresetMenuItem::PresetIndexToShorcutName(p, shortcut);
		char	full[256];
		if (gPrefs.presets[p].IsValid())
			sprintf(full, "%s   %s", shortcut, gPrefs.presets[p].name.String());
		else
			sprintf(full, "%s", shortcut);
		BMenuItem * item = new BMenuItem(full, m);
		if (p == preset)
			item->SetMarked(true);
		presets->AddItem(item);
	}
	r.Set(r.right + 20, r.top, bounds.right - 15, r.bottom);
	BMenuField * menufield = new BMenuField(r, B_EMPTY_STRING, "Shortcut:", presets);
	menufield->SetDivider(50);
	back->AddChild(menufield);

	r.Set(20, bounds.bottom - 40, 20 + 70, 0);
	BButton * button = new BButton(r, B_EMPTY_STRING, "Cancel", new BMessage(B_QUIT_REQUESTED));
	back->AddChild(button);
	r.Set(bounds.right - 20 - 70, r.top, bounds.right - 20, r.top);
	button = new BButton(r, B_EMPTY_STRING, "OK", new BMessage('Done'));
	button->MakeDefault(true);
	back->AddChild(button);
	AddChild(back);
	MoveTo((parent.right + parent.left - bounds.right - bounds.left) / 2, (parent.bottom + parent.top) / 2 - bounds.bottom);
	textcontrol->MakeFocus(true);
	AddCommonFilter(new BMessageFilter(B_ANY_DELIVERY, B_LOCAL_SOURCE, B_KEY_DOWN, &esc_interceptor));
	Show();
}

NewPresetWindow::~NewPresetWindow()
{
	delete messenger;
}

void NewPresetWindow::MessageReceived(BMessage *message)
{
	if (message->what == 'Done') {
		BMessage	m(stampView::CREATE_PRESET);
		m.AddString("name", textcontrol->Text());
		m.AddInt32("preset", preset);
		messenger->SendMessage(&m);
		Quit();
	} else if (message->what == 'Shor') {
		int32	p;
		if (message->FindInt32("preset", &p) == B_OK)
			preset = p;
		const char * name;
		if ((modifiers() & (B_SHIFT_KEY | B_COMMAND_KEY | B_CONTROL_KEY | B_OPTION_KEY)) && message->FindString("name", &name) == B_OK) {
			textcontrol->SetText(name);
			textcontrol->TextView()->SelectAll();
		}
	} else
		BWindow::MessageReceived(message);
}
