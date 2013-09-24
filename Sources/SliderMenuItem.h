/*

	SliderMenuItem.h

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#ifndef _SLIDER_MENU_ITEM_H_
#define _SLIDER_MENU_ITEM_H_

#include <MenuItem.h>

class SliderMenuItem : public BMenuItem {

public:
						SliderMenuItem(BContinuousParameter * parameter, bool bigMargin = false);
virtual					~SliderMenuItem();
virtual	void			DrawContent();
virtual	void			GetContentSize(float* width, float* height);
		void			Tracking();
		void			DrawSlider();
private:
	float				fValue;
	BContinuousParameter * fGain;
	float				fRightMargin;
	BWindow				* fWindow;
	float				* fChannels;
	int					fChannelCount;
	sem_id				fTrack_sem;
	thread_id			fTrack_thread;
};

#endif // _SLIDER_MENU_ITEM_H_
