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

class VideoConsumer;
class VideoWindow;
class BMediaRoster;
class BScreen;
class volumeDisplay;
class channelDisplay;
class muteDisplay;
class PluginsHandler;

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
				POPUP_END,
				SET_AUDIO_SOURCE,
				DISABLE_SCREEN_SAVER,
				UPDATE_WINDOW_TITLE,
				// For BeInControl
				VOLUME_UP = 'bica',
				VOLUME_DOWN,
				CHANNEL_UP,
				CHANNEL_DOWN,
				PRESET_UP,
				PRESET_DOWN,
				FULLSCREEN,
				MUTE_AUDIO,
				LAST_CHANNEL,
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

		status_t		SetUpNodes();
		void			TearDownNodes();

		BDiscreteParameter	*GetParameter(const char * parameterName)
			{	return fParameterWebCache.GetDiscreteParameter(parameterName);	}
		int32			GetDiscreteParameterValue(const char * parameterName)
			{	return fParameterWebCache.GetDiscreteParameterValue(parameterName); }
		int32			SetDiscreteParameterValue(const char * parameterName, int32 value)
			{	return fParameterWebCache.SetDiscreteParameterValue(parameterName, value);	}
		int32			GetChannel()
			{	return GetDiscreteParameterValue(kChannelParameter);	}
		void			SetChannel(int32 channel);
		void			AdjustVolume(int v);
		void			UpdateMute(bool switchMute = false);

		void			ResizeVideo(int x, int y, BScreen * screen = NULL, display_mode *mode = NULL);
		status_t		Connect();
		bool			TryLockConnection();
		void			LockConnection();
		void			UnlockConnection();
		void			SetVisible(bool visible);
		void			ScreenChanged();
		bool			IsFullScreen() const;

		int32			GetCurrentPreset();
		int32			GetNextPreset(int direction);

		void			UpdateWindowTitle(const char * title = NULL);

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
		media_node			fVideoInputNode;
		VideoConsumer		*fVideoConsumer;
		bool				fAllOK;

		BPoint				fClickPoint;
		bigtime_t			fLastFirstClick;
		VideoWindow			*fWindow;
		BWindow		  		*fVideoPreferences;
		bool				fWhilePopup;
		BRect				fVideoFrame;
		display_mode		fDisplayMode;
		BMediaRoster		*fRoster;
		ParameterWebCache	fParameterWebCache;
		int32				fLastChannelValue;
		volumeDisplay		*fVolumeDisplay;
		channelDisplay		*fChannelDisplay;
		muteDisplay			*fMuteDisplay;
		BString				fWindowTitle;
		int32				fConnectionLock;
		sem_id				fConnectionLockSem;
		friend class		stampPluginsHandler;
};

#endif // _STAMP_VIEW_H_
