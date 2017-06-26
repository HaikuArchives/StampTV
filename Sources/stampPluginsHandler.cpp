/*

	stampPluginsHandler.cpp

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video Plugin License for usage restrictions.

	Here happens all what concerns interraction between the host application
	and the plugins handler engine.
	Implementation which requires detailled knowledge on plugins is here
	to be excluded and done at the PluginsHandler level.
	You should be handling at this level:
	- settings of plugins for the host (which plugin is active, what order),
	- settings for the plugins themselves,
	- provide app requirements to build the UI around plugins.
	
	This part is highly host-dependent, and should be only taken as exemple
	for other host applications.

*/

#include "stampPluginsHandler.h"
#include "VideoConsumer.h"
#include "stampView.h"
#include "Preferences.h"

#include <Menu.h>
#include <MenuItem.h>
#include <ParameterWeb.h>
#include <stdio.h>
#include <Window.h>

class ConfigWindow : public BWindow {
public:
						ConfigWindow(VideoPluginEngine * engine, BRect frame, const char *title, 
								window_look look, window_feel feel, uint32 flags):
							BWindow(frame, title, look, feel, flags), fEngine(engine) {}
						~ConfigWindow();
private:
		VideoPluginEngine * 	fEngine;
};

ConfigWindow::~ConfigWindow()
{
	fEngine->SetConfigWindowFrame(Frame());
}

stampPluginsHandler::stampPluginsHandler(stampView * stamp, VideoConsumer * videoConsumer):
	fStamp(stamp), fVideoConsumer(videoConsumer)
{
	fParameterWebCache.Init(stamp->fVideoInputNode, stamp->fRoster);
	ScanPluginsFolder();
	RestoreConfig(gPrefs.Plugins);
	BuildPaths();
	
	printf("Found %d path(s):\n", fPathCount);
	int last = fEngineList.CountItems();
	if (last < 1)
		printf("Default path...\n");
	else {
		for (int p = 1; p <= fPathCount; p++) {
			Tpath & path = Path(p);
			printf("Path %d: %d --> %d Q=%.2f F=%d\n", p, int(path.in_cspace), int(path.out_cspace), path.quality, int(path.flags));
			for (int e = 0; e < last; e++) {
				printf("  %d -> %d (Q=%.2f F=%d)", int(path.vertice[e]->in_cspace), int(path.vertice[e]->out_cspace), path.vertice[e]->quality, int(path.vertice[e]->flags));
			}
			printf(".\n");
		}
	}
}

stampPluginsHandler::~stampPluginsHandler()
{
	StoreConfig(gPrefs.Plugins);
}

const char* stampPluginsHandler::Name()
{
	return "stampTV";
}

uint32 stampPluginsHandler::Version()
{
	return 100;
}

bool stampPluginsHandler::OverlayCompatible(color_space cspace)
{
	return fVideoConsumer->OverlayCompatible(cspace);
}

status_t stampPluginsHandler::GetChannel(int32 & channel)
{
	channel = GetDiscreteParameterValue(kChannelParameter);
	return channel >= 0 ? B_OK : B_ERROR;
}

status_t stampPluginsHandler::SwitchToChannel(int32 channel)
{
	SetDiscreteParameterValue(kChannelParameter, channel);
	return B_OK;
}

status_t stampPluginsHandler::GetPresetCount(int32 & count)
{
	return kMaxPresets;
}

status_t stampPluginsHandler::GetPreset(int32 & slot, BString & name, int32 & channel)
{
	status_t	r = B_ERROR;
	slot = fStamp->GetCurrentPreset(fParameterWebCache);
	if (slot >= 0) {
		name = gPrefs.presets[slot].name;
		channel = gPrefs.presets[slot].channel;
		r = B_OK;
	} else {
		channel = GetDiscreteParameterValue(kChannelParameter);
		BDiscreteParameter	* tuner = fParameterWebCache.GetDiscreteParameter(kChannelParameter);
		if (tuner)
			name = tuner->ItemNameAt(channel);
		else
			name = "???";
	}
	return r;
}

status_t stampPluginsHandler::GetNthPreset(int32 slot, BString & name, int32 & channel)
{
	status_t	r = B_ERROR;
	if (slot >= 0 && slot < kMaxPresets && gPrefs.presets[slot].IsValid()) {
		name = gPrefs.presets[slot].name;
		channel = gPrefs.presets[slot].channel;
		r = B_OK;
	}
	return r;
}

status_t stampPluginsHandler::SwitchToPreset(BString & name)
{
	status_t	r = B_ERROR;
	int slot;
	for (slot = 0; slot < kMaxPresets && name != gPrefs.presets[slot].name; slot++)
		;
	if (slot < kMaxPresets && gPrefs.presets[slot].SwitchTo(fParameterWebCache))
		r = B_OK;
	return r;
}

status_t stampPluginsHandler::SwitchToPreset(int32 slot)
{
	status_t	r = B_ERROR;
	if (slot >= 0 && slot < kMaxPresets && gPrefs.presets[slot].IsValid()) {
		if (gPrefs.presets[slot].SwitchTo(fParameterWebCache))
			r = B_OK;
	}
	return r;
}

status_t stampPluginsHandler::SwitchToNext(int how_many, bool preset, int32 & index)
{
	status_t	r = B_ERROR;
	if (preset) {
		int32	p = fStamp->GetNextPreset(how_many, fParameterWebCache);
		if (gPrefs.presets[p].SwitchTo(fParameterWebCache))
			r = B_OK;
	} else {
		index = GetDiscreteParameterValue(kChannelParameter) + how_many;
		SetDiscreteParameterValue(kChannelParameter, index);
		int32 res = GetDiscreteParameterValue(kChannelParameter);	// in case we reached a limit
		if (index == res)
			r = B_OK;
	}
	return r;
}

BMenu * stampPluginsHandler::PluginsMenu()
{
	BMenu *	menu = new BMenu("Plugins");
	menu->SetFont(be_plain_font);
	BMessage *	msg = NULL;
	BMenuItem * item;
	BMenu * sub;

	if (fEngineList.CountItems() < 1) {
		item = new BMenuItem("(no active plugin)", NULL);
		item->SetEnabled(false);
		menu->AddItem(item);
	} else
		for (int e = 0; e < fEngineList.CountItems(); e++) {
			sub = new BMenu(fEngineList.ItemAt(e)->Plugin()->Name());
			sub->SetFont(be_plain_font);

			bool	configurable = fEngineList.ItemAt(e)->Plugin()->Flags() & VAPI_CONFIGURABLE;
			if (configurable) {
				msg = new BMessage('CfEg');
				msg->AddInt32("index", e);
				item = new BMenuItem("Configure...", msg);
				item->SetTarget(this);
				sub->AddItem(item);
			}
	
			msg = new BMessage('RmEg');
			msg->AddInt32("index", e);
			item = new BMenuItem("Remove", msg);
			item->SetTarget(this);
			sub->AddItem(item);
			
			msg = new BMessage('AbEn');
			msg->AddInt32("index", e);
			item = new BMenuItem("About...", msg);
			item->SetTarget(this);
			sub->AddItem(item);
			
			if (configurable) {
				msg = new BMessage('CfEg');
				msg->AddInt32("index", e);
			} else
				msg = NULL;
			item = new BMenuItem(sub, msg);
			item->SetTarget(this);
			menu->AddItem(item);
		}

	menu->AddSeparatorItem();

	sub = new BMenu("Add");
	sub->SetFont(be_plain_font);
	for (int e = 0; e < fTypeList.CountItems(); e++) {
		msg = new BMessage('AdEg');
		msg->AddInt32("index", e);
		item = new BMenuItem(fTypeList.ItemAt(e)->Name(), msg);
		item->SetTarget(this);
		sub->AddItem(item);
	}
	menu->AddItem(sub);

	return menu;
}

status_t stampPluginsHandler::SetWindowTitle(const char* name = NULL)
{
	BMessage	title(stampView::UPDATE_WINDOW_TITLE);
	if (name)
		title.AddString("name", name);
	fStamp->Looper()->PostMessage(&title, fStamp);
	return B_OK;
}

void stampPluginsHandler::InitParameterWeb(media_node & node, BMediaRoster * roster)
{
	fParameterWebCache.Init(node, roster);
}

inline float Max(float a, float b)
{
	return a > b ? a : b;
}

status_t stampPluginsHandler::OpenConfigWindow(VideoPluginEngine * engine)
{
	if (engine->ConfigWindow()->Lock()) {
		engine->ConfigWindow()->Activate();
		engine->ConfigWindow()->Unlock();
	} else {
		BView * view = engine->ConfigureView();
		if (view) {
			BRect		frame = view->Bounds();
			if (frame.IsValid()) {
				BRect	stampRect = fStamp->Window()->Frame();
				BRect	last = engine->ConfigWindowFrame();
				BRect	screen = fStamp->ScreenSize();
				float	width = last.Width();
				float	height = last.Height();
				BPoint	where = last.LeftTop();
				BString	title("Configure: ");
				title << engine->Plugin()->Name();
				uint32	view_flags = view->ResizingMode();
				// cf View.h (and with a little bit of guessing!)
				uint32	right_flags = view_flags & _rule_(0, 0, 0, 15UL);
				uint32	bottom_flags = view_flags & _rule_(0, 0, 15UL, 0);
				uint32 window_flags = B_ASYNCHRONOUS_CONTROLS | B_WILL_ACCEPT_FIRST_CLICK;
				if (right_flags != _rule_(0, 0, 0, _VIEW_RIGHT_) && right_flags != _rule_(0, 0, 0, _VIEW_CENTER_)) {
					window_flags |= B_NOT_H_RESIZABLE;
					width = frame.Width();
				}
				if (bottom_flags != _rule_(0, 0, _VIEW_BOTTOM_, 0) && bottom_flags != _rule_(0, 0, _VIEW_CENTER_, 0)) {
					window_flags |= B_NOT_V_RESIZABLE;
					height = frame.Height();
				}
				if (width < 10)
					width = frame.Width();
				if (height < 10)
					height = frame.Height();
				screen.InsetBy(2, 2);
				if (!last.IsValid()) {	// let's choose the best position for the window!
					const float kDistance = 28;
					BRect margins(stampRect.left - screen.left, stampRect.top - screen.top, screen.right - stampRect.right, screen.bottom - stampRect.bottom);
					if (Max(margins.left, margins.right) - width > Max(margins.top, margins.bottom) - height) {
						// Left or right better
						if (margins.left > margins.right)
							where.x = stampRect.left - width - kDistance;
						else
							where.x = stampRect.right + kDistance;
						where.y = stampRect.top;
					} else {
						// Above or bottom better
						where.x = stampRect.left;
						if (margins.top > margins.bottom)
							where.y = stampRect.top - height - kDistance;
						else
							where.y = stampRect.bottom + kDistance;
					}
					if (where.x + width > screen.right)
						where.x = screen.right - width;
					if (where.x < screen.left)
						where.x = screen.left;
					if (where.y + height > screen.bottom)
						where.y = screen.bottom - height;
					if (where.y < screen.top)
						where.y = screen.top;
				}
				// make sure that the window is visible!
				BRect result(where.x, where.y, where.x + width, where.y + height);
				if (!screen.Intersects(result))
					where.ConstrainTo(screen);
				frame.OffsetTo(where);
				if ((window_flags & (B_NOT_H_RESIZABLE | B_NOT_V_RESIZABLE)) == (B_NOT_H_RESIZABLE | B_NOT_V_RESIZABLE))
					window_flags |= B_NOT_ZOOMABLE;
				ConfigWindow *	window = new ConfigWindow(engine, frame, title.String(),
					B_FLOATING_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL, window_flags);
				window->AddChild(view);
				window->ResizeTo(width, height);	// must resize!
				window->Show();
				engine->SetConfigWindow(window);
			}
		}
	}
	return B_OK;
}

status_t stampPluginsHandler::SetMuteAudio(bool mute)
{
	BMessage	command(mute ? stampView::MUTE_AUDIO_ON : stampView::MUTE_AUDIO_OFF);
	fStamp->Looper()->PostMessage(&command, fStamp);
	return B_OK;
}

void stampPluginsHandler::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case 'CfEg':
		{
			int32	e;
			if (message->FindInt32("index", &e) == B_OK && e < fEngineList.CountItems())
				OpenConfigWindow(fEngineList.ItemAt(e));
			break;
		}
		case 'AdEg':
		case 'RmEg':
		{
			int32	e;
			if (message->FindInt32("index", &e) == B_OK && e < fTypeList.CountItems()) {
				bool	open_config = false;
				fStamp->LockConnection();
				fVideoConsumer->Disconnect();
				if (message->what == 'AdEg') {
					// Add
					fEngineList.AddItem(fTypeList.ItemAt(e)->InstanciateVideoPluginEngine());
					open_config = true;
				} else if (message->what == 'RmEg') {
					// Remove
					VideoPluginEngine * engine = fEngineList.RemoveItem(e);
					engine->Close();
					delete engine;
				}
				BuildPaths();
				while (fPathCount < 1 || fStamp->Connect() != B_OK) {
					if (fEngineList.CountItems() < 1)
						break; // found no way out... Should never happen!
					open_config = false;
					VideoPluginEngine * engine = fEngineList.RemoveItem(fEngineList.CountItems() - 1);
					engine->Close();
					delete engine;
					BuildPaths();
				}
				fStamp->UnlockConnection();
				if (open_config)
					OpenConfigWindow(fEngineList.ItemAt(fEngineList.CountItems() - 1));
			}
			break;
		}
		case 'AbEn':
		{
			int32	e;
			if (message->FindInt32("index", &e) == B_OK && e < fEngineList.CountItems()) {
				VideoPluginEngine * engine = fEngineList.ItemAt(e);
				engine->Plugin()->About();
			}
			break;
		}
	}
}
