/*

	DisplayViews.h

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#ifndef _DISPLAY_VIEWS_H_
#define _DISPLAY_VIEWS_H_

#include <View.h>
#include <String.h>
#include "Preferences.h"

class displayView : public BView
{
public:
						displayView(const BRect & frame, float h, float s);

	virtual void		AttachedToWindow();
	virtual	void		Draw(BRect updateRect);
	virtual	void		MouseDown(BPoint where);
	virtual	void		Pulse();
			void		SetFrame(const BRect & frame);
			void		SetBackColor(const rgb_color & color);
	virtual void		Resize();
			void		Changed();
			void		UseTimer(bool timer);

protected:
	BRect				fPictureFrame;
	float				fHeight, fSize;
	BString				fText;
	float				fDescent;
	bigtime_t			fTime;
};

class volumeDisplay : public displayView
{
public:
						volumeDisplay(const BRect & frame);
	virtual	void		Draw(BRect updateRect);
			void		Set(float v);
	virtual void		Resize();
private:
	float				fVolume;
};

class muteDisplay : public displayView
{
public:
						muteDisplay(const BRect & frame);
	virtual	void		Draw(BRect updateRect);
	virtual	void		Pulse();
	virtual void		Resize();
			void		Set(bool mute);
};

class channelDisplay : public displayView
{
public:
						channelDisplay(const BRect & frame, Preset * pendingPresetStorage);
			void		Hide();
			void		Set(const char* name);
			Preset&		SetPendingPreset(Preset* preset);
private:
	Preset *			fPendingPresetStorage;
	Preset				fPendingPreset;
	Preset				fPreviousPendingPreset;
};

#endif // _DISPLAY_VIEWS_H_