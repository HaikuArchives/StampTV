/*

	stampView.cpp

	© 1991-1999, Be Incorporated, All rights reserved, was Be's CodyCam Sample Code
	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#include "stampTV.h"
#include "stampView.h"
#include "VideoConsumer.h"
#include "NewPresetWindow.h"
#include "VideoWindow.h"
#include "PresetMenuItem.h"
#include "Preferences.h"
#include "PopUpMenuTracker.h"
#include "SliderMenuItem.h"
#include "DisplayViews.h"

#include <Application.h>
#include <Alert.h>
#include <MessageQueue.h>
#include <Region.h>
#include <Screen.h>
#include <stdio.h>
#include <Roster.h>
#include <Path.h>

#include <MediaTheme.h>
#include <ParameterWeb.h>
#include <MediaRoster.h>
#include <WindowScreen.h>
#include <Bitmap.h>

stampView::stampView(BRect frame) :
	BView(frame, "Video View", B_FOLLOW_ALL | B_PULSE_NEEDED, B_WILL_DRAW | B_FRAME_EVENTS),
	fTVResolutions(0), fTVx(NULL), fTVy(NULL),
	fVideoConsumer(NULL),
	fAllOK(false),
	fSuspend(false),
	fSuspendTimeout(LONGLONG_MAX),
	fWindow(NULL),
	fVideoPreferences(NULL),
	fWhilePopup(false),
	fRoster(NULL),
	fVolumeDisplay(NULL),
	fChannelDisplay(NULL),
	fMuteDisplay(NULL),
	fConnectionLock(0),
	fCurrentOverlay(NULL)
{
	fClickPoint.x = -100.;
	fLastFirstClick = 0;
	fConnectionLockSem = create_sem(0, "Connection Lock");
}

stampView::~stampView()
{
	if (fVideoPreferences->Lock())
		fVideoPreferences->Quit();
	// release the video consumer node
	// the consumer node cleans up the window
	TearDownNodes();
	delete[] fTVx;
	delete[] fTVy;
	delete_sem(fConnectionLockSem);
}

void
stampView::AttachedToWindow()
{
	fWindow = dynamic_cast<VideoWindow *> (Window());

	// set up the node connections
	status_t status = SetUpNodes();
	if (status != B_OK)
	{
		ErrorAlert("Error setting up nodes", status);
		return;
	}
	UpdateWindowTitle();
	SetViewColor(B_TRANSPARENT_COLOR);
	MakeFocus(true);
	if (fWindow) {
		fWindow->AddShortcut('H', B_COMMAND_KEY, new BMessage(SHOW_NOTICE), this);
		fWindow->AddShortcut('P', B_COMMAND_KEY, new BMessage(VIDEO_PREFERENCES), this);
		fWindow->AddShortcut('M', B_COMMAND_KEY, new BMessage(MUTE_AUDIO_SWITCH), this);
	} else
		SetResizingMode(B_FOLLOW_NONE);
	AddChild(fVolumeDisplay = new volumeDisplay(fVideoFrame));
	AddChild(fChannelDisplay = new channelDisplay(fVideoFrame, &fBounceCurrentPreset));
	AddChild(fMuteDisplay = new muteDisplay(fVideoFrame));
	UpdateMute();
}

void
stampView::MessageReceived(BMessage *message)
{
	switch (message->what)
	{
		case B_COPY_TARGET:
			Looper()->DetachCurrentMessage();
			message->what = (ulong) this;
			resume_thread(spawn_thread(save_frame_thread, "Processing thread", B_NORMAL_PRIORITY, message));
			break;
		case B_MOUSE_WHEEL_CHANGED: {
			float total = 0, y;
			BMessage *msg = message;
			BMessageQueue *queue = Window()->MessageQueue();
			int32 modif = modifiers();
			do {
				if (msg->FindFloat("be:wheel_delta_y", &y) == B_OK) {
					int32 delta = (modif & B_OPTION_KEY) ? 10 : 1;
					if (y < 0)
						delta = -delta;
					total += delta;
				}
				if ((msg = queue->FindMessage(B_MOUSE_WHEEL_CHANGED, 0)) != NULL)
					queue->RemoveMessage(msg);
			} while (msg);
			BPoint	p;
			uint32	buttons;
			GetMouse(&p, &buttons);
			if (buttons & B_TERTIARY_MOUSE_BUTTON || modif & B_CONTROL_KEY)
				AdjustVolume(total);
			else if (modif & (B_SHIFT_KEY | B_CAPS_LOCK))
				SetChannel(GetChannel() + total);
			else
				SetPreset(GetNextPreset(total, fParameterWebCache));
			break;
		}
		case SET_PREFERRED_MODE: {
			int32 i;
			if (message->FindInt32("clock", &i) == B_OK)
				gPrefs.PreferredMode = i;
			if (message->FindInt32("h", &i) == B_OK)
				gPrefs.FullScreenX = i;
			if (message->FindInt32("v", &i) == B_OK)
				gPrefs.FullScreenY = i;
			if (IsFullScreen())
				fWindow->SwitchFullScreen(true);
			break;
		}
		case SET_CHANNEL: {
			int32 channel;
			if (message->FindInt32("channel", &channel) == B_OK)
				SetChannel(channel, false);
			break;
		}
		case UPDATE_WINDOW_TITLE: {
			if (fWindow) {
				const char * name;
				if (message->FindString("name", &name) != B_OK || name == NULL || name[0] == 0)
					UpdateWindowTitle("");
				else
					UpdateWindowTitle(name);
			}
			break;
		}
		case SUBDIVIDE_CHANNELS:
			gPrefs.Subdivide = !gPrefs.Subdivide;
			break;
		case VIDEO_PREFERENCES: {
			if (fVideoPreferences->Lock()) {
				fVideoPreferences->Activate();
				fVideoPreferences->Unlock();
			} else {
				// this web will be deleted when the window is closed!
				BParameterWeb	*web = ParameterWebCache::NewVideoParameterWeb(fVideoInputNode, fRoster);
				if (web) {
					// acquire the parameter view
					BView *parameterView = BMediaTheme::ViewFor(web, 0);
					if (parameterView) {
						fVideoPreferences = new BWindow(parameterView->Bounds(),
														"Video Preferences",
														B_TITLED_WINDOW,
														B_NOT_RESIZABLE | B_NOT_ZOOMABLE | 
														B_WILL_ACCEPT_FIRST_CLICK);
						fVideoPreferences->AddChild(parameterView);
						fVideoPreferences->MoveTo(100, 100);
						fVideoPreferences->Show();
					}
				}
			}
			break;
		}
		case SWITCH_TO_PRESET: {
			int32 p;
			if (message->FindInt32("preset", &p) == B_OK)
				SetPreset(p, false);
			break;
		}
		case CREATE_PRESET: {
			int32	p;
			Preset	preset;
			preset.SetToCurrent(fParameterWebCache);
			if (message->FindInt32("preset", &p) != B_OK || p < 0 || p >= kMaxPresets)
				for (p = 0; p < kMaxPresets && gPrefs.presets[p] != preset; p++)
					;
			if (p >= kMaxPresets && (message->FindInt32("preset", &p) != B_OK || p < 0 || p >= kMaxPresets))
				for (p = 0; p < kMaxPresets && gPrefs.presets[p].IsValid(); p++)
					;
			if (p < kMaxPresets) {
				if (message->FindString("name", &preset.name) != B_OK) {
					new NewPresetWindow(new BMessenger(this), Window()->Frame(), p);
				} else {
					gPrefs.presets[p] = preset;
					for (int k = 0; k < kMaxPresets; k++)
						if (k != p && gPrefs.presets[k] == preset)
							gPrefs.presets[k].Unset();
					UpdateWindowTitle(preset);
				}
			} else
				(new BAlert(B_EMPTY_STRING, "There is no more free preset!", "OK", NULL, NULL,
					B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT))->Go(NULL);
			break;
		}
		case REMOVE_PRESET: {
			Preset	preset;
			preset.SetToCurrent(fParameterWebCache);
			for (int k = 0; k < kMaxPresets; k++)
				if (gPrefs.presets[k] == preset)
					gPrefs.presets[k].Unset(false);
			UpdateWindowTitle();
			break;
		}
		case LAST_CHANNEL: {
			SetPreset(fBounceLastPreset, false);
			break;
		}
		case IDEAL_RESIZE:
			if (fWindow) {
				int32	ideal;
				if (message->FindInt32("ideal", &ideal) == B_OK) {
					bool	windowOnly = message->HasBool("window");
					if (!IsFullScreen() && (gPrefs.VideoSizeIsWindowSize || windowOnly))
						Window()->ResizeTo(fTVx[ideal] - 1, fTVy[ideal] - 1);
					if (!windowOnly)
						ResizeVideo(fTVx[ideal], fTVy[ideal]);
				}
			}
			break;
		case TAB_LESS:
			gPrefs.TabLess = !gPrefs.TabLess;
			if (fWindow)
				fWindow->SetLook(gPrefs.TabLess ? B_MODAL_WINDOW_LOOK : B_TITLED_WINDOW_LOOK);
			break;
		case STAY_ON_TOP:
			gPrefs.StayOnTop = !gPrefs.StayOnTop;
			if (fWindow)
				fWindow->SetFeel(gPrefs.StayOnTop ? B_FLOATING_ALL_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL);
			break;			
		case STAY_ON_SCREEN:
			gPrefs.StayOnScreen = !gPrefs.StayOnScreen;
			if (fWindow && gPrefs.StayOnScreen)
				fWindow->CheckWindowPosition();
			break;
		case VIDEO_SIZE_IS_WINDOW_SIZE:
			gPrefs.VideoSizeIsWindowSize = !gPrefs.VideoSizeIsWindowSize;
			if (IsFullScreen())
				ResizeVideo(gPrefs.VideoSizeX, gPrefs.VideoSizeY);
			else if (fWindow)
				fWindow->SetMaxSizes(fTVmaxX, fTVmaxY);
			FrameResized();
			break;
		case FULLSCREEN:
			if (fWindow)
				fWindow->SwitchFullScreen();
			break;
		case USE_ALL_WORKSPACES:
			gPrefs.AllWorkspaces = !gPrefs.AllWorkspaces;
			if (fWindow)
				fWindow->SetWorkspaces(gPrefs.AllWorkspaces ? B_ALL_WORKSPACES : 1 << current_workspace());
			break;
		case DISABLE_SCREEN_SAVER:
			gPrefs.DisableScreenSaver = !gPrefs.DisableScreenSaver;
			break;
		case SHOW_NOTICE:{
			app_info	ainfo;
			be_app->GetAppInfo(&ainfo);
			BPath	path(&ainfo.ref);
			path.GetParent(&path);
			path.Append("Documentation/stampTV's Notice.html");
			entry_ref	ref;
			BEntry		entry(path.Path());
			if (entry.InitCheck() == B_OK && entry.IsFile() && get_ref_for_path(path.Path(), &ref) == B_OK) {
				be_roster->Launch(&ref);
			} else {
				(new BAlert("Um...", "I couldn't find stampTV's documentation...",
					"Sorry!", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT))->Go(NULL);
			}
			break;
		}
		case GOTO_WEBSITE:
			OpenURL("http://www.bebits.com/app/907");
			break;
		case FIND_PLUGINS:
			OpenURL("http://www.bebits.com/search?search=stamptv+plugin");
			break;
		case RATE_STAMPTV:
			OpenURL("http://www.bebits.com/appvote/907");
			break;
		case BUG_REPORT:
			WriteBugReport();
			break;
		case POPUP_END:
			fWhilePopup = false;
			break;
		case SET_AUDIO_SOURCE: {
			BString	name;
			if (message->FindString("name", &name) == B_OK) {
				gPrefs.AudioName = name;
				UpdateMute();
			}
			break;
		}
		case VOLUME_UP: {
			AdjustVolume(+1);
			break;
		}
		case VOLUME_DOWN: {
			AdjustVolume(-1);
			break;
		}
		case CHANNEL_UP: {
			SetChannel(GetChannel() + 1);
			break;
		}
		case CHANNEL_DOWN: {
			SetChannel(GetChannel() - 1);
			break;
		}
		case PRESET_UP: {
			SetPreset(GetNextPreset(+1, fParameterWebCache));
			break;
		}
		case PRESET_DOWN: {
			SetPreset(GetNextPreset(-1, fParameterWebCache));
			break;
		}
		case MUTE_AUDIO_ON: {
			UpdateMute(eMuteOn);
			break;
		}
		case MUTE_AUDIO_OFF: {
			UpdateMute(eMuteOff);
			break;
		}
		case MUTE_AUDIO_SWITCH: {
			UpdateMute(eSwitchMute);
			break;
		}
		default:
			BView::MessageReceived(message);
	}
}

void
stampView::Draw(BRect updateRect)
{
	if (!fAllOK)
		return;
	BRegion	r(updateRect);
	r.Exclude(fVideoFrame);
	SetHighColor(0, 0, 0);
	if (r.Intersects(updateRect))
		FillRegion(&r);
	rgb_color	backcolor;
	if (fCurrentOverlay != NULL) {
		SetHighColor(fOverlayKeyColor);
		FillRect(fVideoFrame);
		memcpy(&backcolor, &fOverlayKeyColor, sizeof(backcolor));
	} else
		memset(&backcolor, 0, sizeof(backcolor));
	fVolumeDisplay->SetBackColor(backcolor);
	fChannelDisplay->SetBackColor(backcolor);
	fMuteDisplay->SetBackColor(backcolor);
}

void
stampView::DrawFrame(BBitmap * frame, bool overlay)
{
	// Is called while the window isn't locked!!!
	// If you can't lock the window fast enough, you'll have to skip frames
	// anyway, so a timeout makes sense on its own.
	// More important, you'll deadlock if you don't lock with timeout,
	// because when the view tries to disconnect, it needs this looper to
	// do some work: if the looper then requires to lock the view when the view
	// requires the looper to react, you are dead...
	if ((!overlay || fCurrentOverlay != frame) && LockLooperWithTimeout(50000) == B_OK) {
		if (overlay) {
			fCurrentOverlay = frame;
			SetViewOverlay(frame, frame->Bounds(), fVideoFrame, &fOverlayKeyColor,
				B_FOLLOW_ALL, B_OVERLAY_FILTER_HORIZONTAL | B_OVERLAY_FILTER_VERTICAL);
			Draw(fVideoFrame);
		} else {
			if (fCurrentOverlay != NULL) {
				fCurrentOverlay = NULL;
				ClearViewOverlay();
			}
			DrawBitmap(frame, fVideoFrame);
		}
		UnlockLooper();
	}
}

void
stampView::KeyDown(const char *bytes, int32 numBytes)
{
	char c = bytes[0];
	if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z') {
		if (c >= 'a' && c <= 'z')
			c += 'A' - 'a';
		BMessage* msg = Window()->CurrentMessage();
		if (msg) {
			int key = c - 'A' + 12;
			if (key >= 12 && key < 26 + 12 && key < kMaxPresets)
				SetPreset(key, false);
		}
	}
	else
	{
		switch (c)
		{
			case B_LEFT_ARROW:
				SetChannel(GetChannel() - 1);
				break;
			case B_RIGHT_ARROW:
				SetChannel(GetChannel() + 1);
				break;
			case B_UP_ARROW:
				AdjustVolume(+1);
				break;
			case B_DOWN_ARROW:
				AdjustVolume(-1);
				break;
			case B_PAGE_DOWN:
				SetChannel(GetChannel() - 10);
				break;
			case B_PAGE_UP:
				SetChannel(GetChannel() + 10);
				break;
			case B_HOME:
				SetChannel(0);
				break;
			case B_END:
				SetChannel(LONG_MAX);
				break;
			case '-':
			case '+': {
				SetPreset(GetNextPreset(c == '+' ? +1 : -1, fParameterWebCache));
				break;
			}
			case B_FUNCTION_KEY: {
				BMessage* msg = Window()->CurrentMessage();
				int32	key;
				if (msg && msg->FindInt32("key", &key) == B_OK) {
					key -= B_F1_KEY;
					if (key >= 0 && key < 12)
						SetPreset(key, false);
				}
				break;
			}
			case B_RETURN:
			case B_INSERT:
				Window()->PostMessage(CREATE_PRESET, this);
				break;
			case B_BACKSPACE: {
				SetPreset(fBounceLastPreset, false);
				break;
			}
			case B_DELETE:
				Window()->PostMessage(REMOVE_PRESET, this);
				break;
			case B_ESCAPE:
				if (!IsFullScreen())
					break;
			case B_TAB:
				if (fWindow)
					fWindow->SwitchFullScreen();
				break;
			case '?':
				Window()->PostMessage(SHOW_NOTICE, this);
				break;
			default:
				BView::KeyDown(bytes, numBytes);
		}
	}
}

void
stampView::MouseMoved(BPoint point, uint32, const BMessage *dragged_msg)
{
	if (IsFullScreen()) {
		if (fWindow->IsFront() && !fWhilePopup)
			while (!be_app->IsCursorHidden())
				be_app->HideCursor();
		else
			while (be_app->IsCursorHidden())
				be_app->ShowCursor();
		return;
	}
	BMessage *msg = Looper()->CurrentMessage();
	int32 buttons = msg->FindInt32("buttons");
	
	if (fClickPoint.x > -10) {
		if (fAllOK && (buttons & B_TERTIARY_MOUSE_BUTTON) && dragged_msg == 0
				&& (fabsf(point.x - fClickPoint.x) > 10. || fabsf(point.y - fClickPoint.y) > 10.))
			InitiateDrag(point);
	
		if (buttons & B_PRIMARY_MOUSE_BUTTON) {
			uint32	buttons;
			GetMouse(&point, &buttons);
			point = point - fClickPoint;
			if (gPrefs.ShouldStayOnScreen()) {
				BRect frame = Window()->Frame();
				BRect scrframe = ScreenSize();
				if (frame.Width() + 8. < scrframe.Width() && frame.Height() + 8. < scrframe.Height()) {
					fClickPoint += point;
					frame.InsetBy(-4., -4.);
					BRect limit(scrframe.left - frame.left, scrframe.top - frame.top,
								scrframe.right - frame.right, scrframe.bottom - frame.bottom);
					point.ConstrainTo(limit);
					fClickPoint -= point;
				}
			}
			if (point.x != 0 || point.y != 0)
				Window()->MoveBy(point.x, point.y);
		}
	}
}

void
stampView::MouseUp(BPoint)
{
	fClickPoint.x = -100;
}

void
stampView::MouseDown(BPoint point)
{
	int32 buttons = Window()->CurrentMessage()->FindInt32("buttons");

	if (buttons & (B_PRIMARY_MOUSE_BUTTON | B_TERTIARY_MOUSE_BUTTON)) {
		Window()->Activate(true);
		int32	clicks;
		if (Window()->CurrentMessage()->FindInt32("clicks", &clicks) == B_OK) {
			bigtime_t	click_speed;
			if (clicks == 2 && fWindow)
				if (get_click_speed(&click_speed) == B_OK && fLastFirstClick + click_speed > system_time())
					fWindow->SwitchFullScreen();
				else
					clicks = 1;
			if (clicks == 1)
				fLastFirstClick = system_time();
			if (!IsFullScreen() && clicks == 1) {
				SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
				fClickPoint = point;
			}
		}
	}
	else if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		PopUpMenuTracker *menu = new PopUpMenuTracker(Window(), this, POPUP_END, "stampTV Menu", false, false, B_ITEMS_IN_COLUMN);
		menu->SetFont(be_plain_font);
		BMenuItem *item;
		BMessage *message;
		int32 current = -1;
		BMenu *subMenu = new BMenu("Channel");
		subMenu->SetFont(be_plain_font);
		message = new BMessage(CHANNEL_UP);
		subMenu->AddItem(item = new BMenuItem("Next Channel", message, B_RIGHT_ARROW, B_NO_COMMAND_KEY));
		message = new BMessage(CHANNEL_DOWN);
		subMenu->AddItem(item = new BMenuItem("Previous Channel", message, B_LEFT_ARROW, B_NO_COMMAND_KEY));
		subMenu->AddSeparatorItem();
		BDiscreteParameter	*tuner = fParameterWebCache.GetDiscreteParameter(kChannelParameter);
		if (tuner) {
			BMenu *channelMenu = subMenu;
			BMenuItem *channelItem = 0;
			size_t size = sizeof(int32);
			bigtime_t lastChange;
			tuner->GetValue(reinterpret_cast<void *>(&current), &size, &lastChange);
			for (int32 i = 0; i < tuner->CountItems(); i++) {
				if (gPrefs.Subdivide && !(i % 10)) {
					BString subLabel;
					subLabel << tuner->ItemNameAt(i) << " - " << tuner->ItemNameAt(i + 9);
					channelMenu = new BMenu(subLabel.String());
						
					channelMenu->SetFont(be_plain_font);
					channelItem = new BMenuItem(channelMenu);
					if ((current >= i) && (current < (i + 10)))
						channelItem->SetMarked(true);
					subMenu->AddItem(channelItem);
				}
				message = new BMessage(SET_CHANNEL);
				message->AddInt32("channel", i);
				channelMenu->AddItem(item = new BMenuItem(tuner->ItemNameAt(i), message));
				item->SetTarget(this);
				if (i == current) {
					item->SetMarked(true);
				}
			}
		}
		subMenu->AddSeparatorItem();
		message = new BMessage(SUBDIVIDE_CHANNELS);
		subMenu->AddItem(item = new BMenuItem("Subdivide Menu", message));
		item->SetMarked(gPrefs.Subdivide);
		subMenu->SetTargetForItems(this);;
		menu->AddItem(subMenu);

		subMenu = new BMenu("Presets");
		subMenu->SetFont(be_plain_font);
		message = new BMessage(PRESET_UP);
		subMenu->AddItem(item = new BMenuItem("Next Preset", message, '+', B_NO_COMMAND_KEY));
		message = new BMessage(PRESET_DOWN);
		subMenu->AddItem(item = new BMenuItem("Previous Preset", message, '-', B_NO_COMMAND_KEY));
		subMenu->AddSeparatorItem();

		bool	presetExists = false;
		Preset	currentPreset;
		currentPreset.SetToCurrent(fParameterWebCache);
		for (int p = 0; p < kMaxPresets; p++) {
			if (gPrefs.presets[p].IsValid()) {
				BMessage * m = new BMessage(SWITCH_TO_PRESET);
				m->AddInt32("preset", p);
				item = new PresetMenuItem(gPrefs.presets[p].name.String(), p, m);
				item->SetTarget(this);
				subMenu->AddItem(item);
				if (currentPreset == gPrefs.presets[p]) {
					presetExists = true;
					item->SetMarked(true);
				}
			}
		}
		if (subMenu->CountItems() > 0)
			subMenu->AddSeparatorItem();
		if (presetExists) {
			item = new BMenuItem("Edit Preset" B_UTF8_ELLIPSIS, new BMessage(CREATE_PRESET), B_RETURN, B_NO_COMMAND_KEY);
			subMenu->AddItem(item);
			item = new PresetMenuItem("Delete Preset", "Del", new BMessage(REMOVE_PRESET));
		} else
			item = new BMenuItem("Create Preset" B_UTF8_ELLIPSIS, new BMessage(CREATE_PRESET), B_RETURN, B_NO_COMMAND_KEY);
		subMenu->AddItem(item);
		subMenu->SetTargetForItems(this);
		menu->AddItem(subMenu);

		if (fBounceLastPreset.IsValid())
			menu->AddItem(new PresetMenuItem("Back to the Last Channel", "Backspace", new BMessage(LAST_CHANNEL)));

		menu->AddSeparatorItem();
		
		message = new BMessage(VIDEO_PREFERENCES);
		menu->AddItem(item = new BMenuItem("Video Preferences" B_UTF8_ELLIPSIS, message, 'P'));

		if (fWindow) {
			if (fTVResolutions > 0) {
				subMenu = new BMenu("Video Size");
				subMenu->SetFont(be_plain_font);
				message = new BMessage(VIDEO_SIZE_IS_WINDOW_SIZE);
				subMenu->AddItem(item = new BMenuItem("Locked (Scale)", message));
				item->SetMarked(!gPrefs.VideoSizeIsWindowSize);
				subMenu->AddSeparatorItem();
				char	buf[64];
				for (int k = 0; k < fTVResolutions; k++) {
					message = new BMessage(IDEAL_RESIZE);
					message->AddInt32("ideal", k);
					sprintf(buf, "%dx%d", fTVx[k], fTVy[k]);
					subMenu->AddItem(item = new BMenuItem(buf, message));
					item->SetMarked(fTVx[k] == gPrefs.VideoSizeX && fTVy[k] == gPrefs.VideoSizeY);
				}
				subMenu->SetTargetForItems(this);
				menu->AddItem(subMenu);
			}
	
			if (!gPrefs.VideoSizeIsWindowSize && !IsFullScreen()) {
				subMenu = new BMenu("Window Size");
				subMenu->SetFont(be_plain_font);
				char	buf[64];
				for (int k = 0; k < fTVResolutions; k++) {
					message = new BMessage(IDEAL_RESIZE);
					message->AddBool("window", true);
					message->AddInt32("ideal", k);
					sprintf(buf, "%dx%d", fTVx[k], fTVy[k]);
					subMenu->AddItem(item = new BMenuItem(buf, message));
					item->SetMarked(fTVx[k] == gPrefs.WindowWidth && fTVy[k] == gPrefs.WindowHeight);
				}
				subMenu->SetTargetForItems(this);
				menu->AddItem(subMenu);
			}

			subMenu = BuildModesSubmenu();
			menu->AddItem(subMenu);
		}

		BParameterWeb	*web = ParameterWebCache::NewVideoParameterWeb(fVideoInputNode, fRoster);
		if (web) {
			BContinuousParameter * brightnessParameter = NULL;
			BContinuousParameter * contrastParameter = NULL;
			BContinuousParameter * saturationParameter = NULL;
			for (int32 i = 0; i < web->CountParameters(); i++) {
				BParameter *parameter = web->ParameterAt(i);
				if (strcmp(parameter->Kind(), kBrightnessParameterKind) == 0) {
					brightnessParameter = dynamic_cast<BContinuousParameter *> (parameter);
				} else if (strcmp(parameter->Kind(), kContrastParameterKind) == 0)
					contrastParameter = dynamic_cast<BContinuousParameter *> (parameter);
				else if (strcmp(parameter->Kind(), kSaturationParameterKind) == 0)
					saturationParameter = dynamic_cast<BContinuousParameter *> (parameter);
			}
			if (brightnessParameter || contrastParameter || saturationParameter) {
				subMenu = new BMenu("Image Tuning");
				subMenu->SetFont(be_plain_font);
				if (brightnessParameter)
					subMenu->AddItem(new SliderMenuItem(brightnessParameter));
				if (contrastParameter)
					subMenu->AddItem(new SliderMenuItem(contrastParameter));
				if (saturationParameter)
					subMenu->AddItem(new SliderMenuItem(saturationParameter));
				menu->AddItem(subMenu);
			}
			// the web should be destroyed only when the popup goes away, because of the sliders!
			menu->AddWebToDelete(web);
		}

		media_node	outNode;
		if (fRoster && fRoster->GetAudioOutput(&outNode) == B_OK) {
			BParameterWeb * web;
			if (fRoster->GetParameterWebFor(outNode, &web) == B_OK && web) {
				subMenu = new BMenu("Audio");
				subMenu->SetFont(be_plain_font);
				BMenu * subsubMenu = new BMenu("Source");
				subsubMenu->SetFont(be_plain_font);
				BString		groupName;
				BContinuousParameter * volumeParameter = NULL;
				bool		muted = false;
				for (int32 i = 0; i < web->CountParameters(); i++) {
					BParameter *parameter = web->ParameterAt(i);
					if (parameter) {
						BParameterGroup *group = parameter->Group();
						if (group) {
							if (groupName != group->Name()) {
								groupName = group->Name();
								message = new BMessage(SET_AUDIO_SOURCE);
								message->AddString("name", groupName);
								subsubMenu->AddItem(item = new BMenuItem(groupName.String(), message));
								item->SetMarked(gPrefs.AudioName == groupName);
								item->SetTarget(this);
							}
						} else
							groupName = "(no group)";
						if (gPrefs.AudioName == groupName) {
							if (strcmp(parameter->Kind(), B_MUTE) == 0) {
								// mute selector: read its state
								BDiscreteParameter * dp = dynamic_cast<BDiscreteParameter *> (parameter);
								if (dp) {
									int32 current_value;
									bigtime_t time;
									size_t size = sizeof(int32);
									if ((parameter->GetValue(reinterpret_cast<void *>(&current_value), &size, &time) == B_OK)
											 && (size <= sizeof(int32))) {
										muted = current_value == 1;	// cf ParameterWeb.h: 0 == thru, 1 == mute
									}
									message = new BMessage(MUTE_AUDIO_SWITCH);
									subMenu->AddItem(item = new BMenuItem("Mute", message, 'M'));
									item->SetMarked(muted);
									item->SetTarget(this);
								}
							} else if (strcmp(parameter->Kind(), B_GAIN) == 0)
								volumeParameter = dynamic_cast<BContinuousParameter *> (parameter);
						}
					} else
						groupName = "(no parameter)"; // don't know what that is: reset the name!
				}
				if (volumeParameter) {
					subMenu->AddItem(new SliderMenuItem(volumeParameter, true));
					item = new BMenuItem("Volume Up", new BMessage(VOLUME_UP), B_UP_ARROW, B_NO_COMMAND_KEY);
					subMenu->AddItem(item);
					item = new BMenuItem("Volume Down", new BMessage(VOLUME_DOWN), B_DOWN_ARROW, B_NO_COMMAND_KEY);
					subMenu->AddItem(item);
				}
				subMenu->AddItem(subsubMenu);
				menu->AddItem(subMenu);
				menu->AddWebToDelete(web);
			}
			fRoster->ReleaseNode(outNode);
		}
		
		menu->AddSeparatorItem();
	
		BMenu * options;
		if (fVideoConsumer && (options = fVideoConsumer->OptionsMenu()) != NULL) {
			menu->AddItem(options);
			menu->AddSeparatorItem();
		}
		
		options = new BMenu("Preferences");
		options->SetFont(be_plain_font);
		if (fWindow) {
			message = new BMessage(TAB_LESS);
			options->AddItem(item = new BMenuItem("No Tab", message));
			item->SetMarked(gPrefs.TabLess);
			
			message = new BMessage(STAY_ON_TOP);
			options->AddItem(item = new BMenuItem("On Top", message));
			item->SetMarked(gPrefs.StayOnTop);
				
			message = new BMessage(STAY_ON_SCREEN);
			options->AddItem(item = new BMenuItem("Window can't be Moved Offscreen", message));
			item->SetMarked(gPrefs.StayOnScreen);
	
			message = new BMessage(FULLSCREEN);
			options->AddItem(item = new PresetMenuItem("Full Screen", "Tab", message));
			item->SetMarked(IsFullScreen());
			
			if (!IsFullScreen()) {
				message = new BMessage(USE_ALL_WORKSPACES);
				options->AddItem(item = new BMenuItem("Occupy All Workspaces", message));
				item->SetMarked(gPrefs.AllWorkspaces);
			}
		}
		message = new BMessage(DISABLE_SCREEN_SAVER);
		options->AddItem(item = new BMenuItem("Disable Screen Saver", message));
		item->SetMarked(gPrefs.DisableScreenSaver);

		options->SetTargetForItems(this);
		menu->AddItem(options);

		options = new BMenu("User Services");
		options->SetFont(be_plain_font);

		message = new BMessage(SHOW_NOTICE);
		options->AddItem(item = new BMenuItem("Read stampTV's Notice", message, fWindow ? 'H' : 0));

		message = new BMessage(GOTO_WEBSITE);
		options->AddItem(item = new BMenuItem("Visit stampTV's Web Site at BeBits", message));
		
		message = new BMessage(FIND_PLUGINS);
		options->AddItem(item = new BMenuItem("Search for plugins in BeBits", message));

		message = new BMessage(RATE_STAMPTV);
		options->AddItem(item = new BMenuItem("Rate stampTV at BeBits", message));

		message = new BMessage(BUG_REPORT);
		options->AddItem(item = new BMenuItem("Write a Bug Report", message));

		options->SetTargetForItems(this);
		menu->AddItem(options);

		menu->AddSeparatorItem();
		menu->SetTargetForItems(this);

		message = new BMessage(B_QUIT_REQUESTED);
		menu->AddItem(item = new BMenuItem("Quit", message, 'Q'));
		item->SetTarget(be_app_messenger);

		ConvertToScreen(&point);
		point -= BPoint(5.0, 5.0);
		fWhilePopup = true;
		menu->Go(point, true, true, true);
		menu->SetAsyncAutoDestruct(true);
		while (be_app->IsCursorHidden())
			be_app->ShowCursor();
	}
}

BMenu *
stampView::BuildModesSubmenu() {
	BMenu *menu = new BMenu("Full Screen Mode");
	menu->SetFont(be_plain_font);
	BMenuItem *item;
	char buf[512];
	
	display_mode *list;
	uint32 count;
	display_mode current_mode;
	{
		BScreen scr(Window());
		scr.GetMode(&current_mode);
		scr.GetModeList(&list, &count);
	}
	
	uint16	h = 0, v = 0;
	BMenu	*submenu = NULL;
	for (uint32 i = 0;  i < count;  ++i)
	{
		if (list[i].timing.h_display != h || list[i].timing.v_display != v || !submenu) {
			h = list[i].timing.h_display;
			v = list[i].timing.v_display;
			sprintf(buf, "%dx%d", h, v);
			submenu = new BMenu(buf);
			submenu->SetFont(be_plain_font);
			item = new BMenuItem(submenu);
			if (list[i].timing.h_display == gPrefs.FullScreenX && list[i].timing.v_display == gPrefs.FullScreenY)
				item->SetMarked(true);
			menu->AddItem(item);
		}
		if (list[i].space == current_mode.space) {
			float f = (float)list[i].timing.pixel_clock * 1000. /
						list[i].timing.h_total / list[i].timing.v_total;
			sprintf(buf, "%.1f Hz", f);
			BMessage *msg = new BMessage(SET_PREFERRED_MODE);
			msg->AddInt32("clock", list[i].timing.pixel_clock);
			msg->AddInt32("h", h);
			msg->AddInt32("v", v);
			submenu->AddItem(item = new BMenuItem(buf, msg));
			item->SetTarget(this);
			item->SetMarked(gPrefs.PreferredMode == list[i].timing.pixel_clock);
		}
	}

	free(list);
	return menu;
}

bool
stampView::IsFullScreen() const
{
	if (fWindow)
		return fWindow->IsFullScreen();
	return false;
}

BRect
stampView::ScreenSize()
{
	if (fWindow)
		return fWindow->ScreenSize();
	return BScreen(Window()).Frame();
}

void
stampView::Pulse()
{
	if (gPrefs.DisableScreenSaver && idle_time() > 29000000) {
		BPoint where;
		uint32 buttons;
		GetMouse(&where, &buttons, false);
		ConvertToScreen(&where);
		set_mouse_position((int32) where.x, (int32) where.y);
	}
	if (fSuspend && system_time() > fSuspendTimeout)
		Resume();
}
