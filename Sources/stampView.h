/*

	stampView.h

	© 1991-1999, Be Incorporated, All rights reserved, was Be's CodyCam Sample Code
	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#ifndef _STAMP_VIEW_H_
#define _STAMP_VIEW_H_

#include <View.h>
#include <Accelerant.h>
#include <String.h>

#include "ParameterWebCache.h"
#include "Preferences.h"

class VideoConsumer;
class VideoWindow;
class BMediaRoster;
class BScreen;
class volumeDisplay;
class channelDisplay;
class muteDisplay;

class stampView : public BView
{
public:
		enum messages
		{
				SET_CHANNEL = 'st_A',
				SUBDIVIDE_CHANNELS,
				TAB_LESS,
				STAY_ON_TOP,
				STAY_ON_SCREEN,
				VIDEO_PREFERENCES,
				SWITCH_TO_PRESET,
				CREATE_PRESET,
				REMOVE_PRESET,
				SET_PREFERRED_MODE,
				IDEAL_RESIZE,
				SHOW_NOTICE,
				GOTO_WEBSITE,
				FIND_PLUGINS,
				RATE_STAMPTV,
				BUG_REPORT,
				POPUP_END,
				SET_AUDIO_SOURCE,
				DISABLE_SCREEN_SAVER,
				UPDATE_WINDOW_TITLE,
				MUTE_AUDIO_ON,
				MUTE_AUDIO_OFF,
				VIDEO_SIZE_IS_WINDOW_SIZE,
				USE_ALL_WORKSPACES,
				// For BeInControl
				VOLUME_UP = 'bica',
				VOLUME_DOWN,
				CHANNEL_UP,
				CHANNEL_DOWN,
				PRESET_UP,
				PRESET_DOWN,
				FULLSCREEN,
				MUTE_AUDIO_SWITCH,
				LAST_CHANNEL,
		};
		
		enum muteCommand {
			eKeepMuteAsIs,
			eMuteOn,
			eMuteOff,
			eSwitchMute,
		};

						stampView(BRect frame);
	virtual				~stampView();

	virtual void		AttachedToWindow();
	virtual void		MessageReceived(BMessage *message);
	virtual void		KeyDown(const char *bytes, int32 numBytes);
	virtual void		MouseDown(BPoint point);
	virtual void		MouseUp(BPoint point);
	virtual	void		MouseMoved(BPoint, uint32, const BMessage *);
	virtual	void		Draw(BRect updateRect);
	virtual	void		Pulse();
	virtual void		FrameResized(float width = 0, float height = 0);

		status_t		SetUpNodes();
		void			TearDownNodes();

		BDiscreteParameter	*GetParameter(parameter_cache_parameters parameter)
			{	return fParameterWebCache.GetDiscreteParameter(parameter);	}
		int32			GetDiscreteParameterValue(parameter_cache_parameters parameter)
			{	return fParameterWebCache.GetDiscreteParameterValue(parameter); }
		int32			SetDiscreteParameterValue(parameter_cache_parameters parameter, int32 value)
			{	return fParameterWebCache.SetDiscreteParameterValue(parameter, value);	}
		int32			GetChannel()
			{	return GetDiscreteParameterValue(kChannelParameter);	}
		void			SetChannel(int32 channel, bool delay = true);

		int32			GetCurrentPreset(ParameterWebCache& pwc);
		int32			GetNextPreset(int direction, ParameterWebCache& pwc);
		void			SetPreset(int32 preset, bool delay = true)
			{	SetPreset(gPrefs.presets[preset], delay); }
		void			SetPreset(Preset& preset, bool delay = true);

		void			AdjustVolume(int v);
		void			UpdateMute(muteCommand change = eKeepMuteAsIs);

		void			ResizeVideo(int x, int y, BScreen * screen = NULL, display_mode *mode = NULL);
		status_t		Connect();
		void			DrawFrame(BBitmap * frame, bool overlay);
		bool			TryLockConnection();
		void			LockConnection();
		void			UnlockConnection();
		void			Suspend(bigtime_t timeout = LONGLONG_MAX);
		void			Resume();
		bool			IsFullScreen() const;
		BRect			ScreenSize();

		void			UpdateWindowTitle(const char * title = NULL);
		void			UpdateWindowTitle(Preset& preset);
		
		void			OpenURL(const char * url);
		void			WriteBugReport();

	static	long		save_frame_thread(void *);
		void			SaveFrameThread(BMessage *msg);
		void			InitiateDrag(BPoint start);
		
		BMenu			*BuildModesSubmenu();
		int				fTVResolutions;
		int				*fTVx;
		int				*fTVy;
		int				fTVmaxX;
		int				fTVmaxY;

private:
		void				ChangedPreset(Preset& newPreset, bool delay);
		media_node			fVideoInputNode;
		VideoConsumer		*fVideoConsumer;
		bool				fAllOK;
		bool				fSuspend;
		bigtime_t			fSuspendTimeout;

		BPoint				fClickPoint;
		bigtime_t			fLastFirstClick;
		VideoWindow			*fWindow;
		BWindow		  		*fVideoPreferences;
		bool				fWhilePopup;
		BRect				fVideoFrame;
		display_mode		fDisplayMode;
		BMediaRoster		*fRoster;
		ParameterWebCache	fParameterWebCache;
		Preset				fBounceLastPreset;
		Preset				fBounceCurrentPreset;
		volumeDisplay		*fVolumeDisplay;
		channelDisplay		*fChannelDisplay;
		muteDisplay			*fMuteDisplay;
		BString				fWindowTitle;
		int32				fConnectionLock;
		sem_id				fConnectionLockSem;
		BBitmap *			fCurrentOverlay;
		rgb_color			fOverlayKeyColor;
		friend class		stampPluginsHandler;
};

#endif // _STAMP_VIEW_H_
