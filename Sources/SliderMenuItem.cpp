/*

	SliderMenuItem.cpp

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#include "SliderMenuItem.h"
#include "Preferences.h"

#include <ParameterWeb.h>
#include <MediaRoster.h>
#include <TimeSource.h>
#include <String.h>
#include <string.h>
#include <stdio.h>
#include <Window.h>

const float kSliderWidth = 100.;
const float kSmallMargin = 15.;
const float kBigMargin = 45.;

static long
track_thread(void *p)
{
	// you can't use filters: almost no event come to them!
	// So we create a thread which polls... BAAAAHHHHH!!!!
	// But it works!
	SliderMenuItem * vmi = (SliderMenuItem*) p;
	vmi->Tracking();
	return B_OK;
}

SliderMenuItem::SliderMenuItem(BContinuousParameter * parameter, bool bigMargin)
			:BMenuItem(parameter->Name(), NULL), fGain(parameter), fWindow(NULL), fChannels(NULL), fChannelCount(0), fTrack_sem(-1), fTrack_thread(-1)
{
	fRightMargin = bigMargin ? kBigMargin : kSmallMargin;
	if (fGain && (fChannelCount = fGain->CountChannels()) > 0) {
		fChannels = new float[fChannelCount];
		fTrack_sem = create_sem(0, "Track semaphore");
		resume_thread(fTrack_thread = spawn_thread(track_thread, "Slider Tracker", B_NORMAL_PRIORITY, this));
	}
}

SliderMenuItem::~SliderMenuItem()
{
	delete_sem(fTrack_sem);
	status_t	r;
	wait_for_thread(fTrack_thread, &r);
	delete [] fChannels;
}

void SliderMenuItem::DrawContent()
{
	BMenu * menu = Menu();
	if (menu)
		fWindow = menu->Window();
	Menu()->MovePenTo(ContentLocation());
	BMenuItem::DrawContent();
	DrawSlider();
}

void SliderMenuItem::GetContentSize(float* width, float* height)
{
	BMenuItem::GetContentSize(width, height);
	*width += kSliderWidth + kSmallMargin;
}

void SliderMenuItem::Tracking()
{
	fValue = FLT_MIN;
	// be extremely carefull here! each object maybe gone!
	while (acquire_sem_etc(fTrack_sem, 1, B_RELATIVE_TIMEOUT, 10000) == B_TIMED_OUT) {
		if (fWindow->Lock()) {
			BMenu * menu = Menu();
			if (fGain && menu && menu->Window() == fWindow && IsSelected()) {
				// everything should be ok now...
				BPoint	where;
				uint32	buttons;
				menu->GetMouse(&where, &buttons, false);
				BRect	zone = Frame();
				zone.right -= fRightMargin;
				zone.left = zone.right - kSliderWidth;
				if (zone.Contains(where) && buttons) {
					float	min = fGain->MinValue();
					float	max = fGain->MaxValue();
					float	v = min + (max - min) * (where.x - zone.left) / kSliderWidth;
					if (fabsf(v - fValue) * kSliderWidth > 1. && fChannelCount > 0) {
						fValue = v;
						size_t		size = sizeof(float) * fChannelCount;
						for (int c = 0; c < fChannelCount; c++)
							fChannels[c] = v;
						fGain->SetValue(fChannels, size, BTimeSource::RealTime());
						DrawSlider();
						menu->Flush();
					}
				}
			}
			fWindow->Unlock();
		}
	}
}

void SliderMenuItem::DrawSlider()
{
	BMenu * menu = Menu();
	rgb_color	high = menu->HighColor();
	BRect	zone = Frame();
	zone.top = floorf((zone.top + zone.bottom) / 2.) - 1.;
	zone.bottom = zone.top + 1.;
	zone.right -= fRightMargin;
	zone.left = zone.right - kSliderWidth;
	rgb_color back = ui_color(B_MENU_BACKGROUND_COLOR);
	if (IsSelected())
		back = tint_color(back, B_HIGHLIGHT_BACKGROUND_TINT);
	rgb_color dark_1_back = tint_color(back, B_DARKEN_1_TINT);
	rgb_color dark_2_back = tint_color(back, B_DARKEN_2_TINT);
	rgb_color dark_max_back = tint_color(back, B_DARKEN_MAX_TINT);
	rgb_color light_1_back = tint_color(back, B_LIGHTEN_1_TINT);
	rgb_color lighter_back = tint_color(back, IsSelected() ? B_LIGHTEN_2_TINT : B_LIGHTEN_MAX_TINT);
	BRect	full = zone;
	full.InsetBy(-5, -5);
	menu->BeginLineArray(32);
	menu->AddLine(BPoint(zone.left, zone.top), BPoint(zone.right, zone.top), dark_max_back);
	menu->AddLine(BPoint(zone.left, zone.bottom), BPoint(zone.right, zone.bottom), dark_max_back);
	zone.InsetBy(-1, -1);
	menu->AddLine(BPoint(zone.left,		zone.top), 		BPoint(zone.left,	zone.bottom),	dark_2_back);
	menu->AddLine(BPoint(zone.right,	zone.top),		BPoint(zone.right,	zone.bottom),	light_1_back);
	menu->AddLine(BPoint(zone.left,		zone.top),		BPoint(zone.right,	zone.top),		dark_2_back);
	menu->AddLine(BPoint(zone.left,		zone.bottom),	BPoint(zone.right,	zone.bottom),	light_1_back);
	zone.InsetBy(-1, -1);
	menu->AddLine(BPoint(zone.left,		zone.top), 		BPoint(zone.left,	zone.bottom),	dark_1_back);
	menu->AddLine(BPoint(zone.right,	zone.top),		BPoint(zone.right,	zone.bottom),	lighter_back);
	menu->AddLine(BPoint(zone.left,		zone.top),		BPoint(zone.right,	zone.top),		dark_1_back);
	menu->AddLine(BPoint(zone.left,		zone.bottom),	BPoint(zone.right,	zone.bottom),	lighter_back);
	//menu->AddLine(BPoint(zone.left - 1., zone.top),		BPoint(zone.left - 1., zone.bottom), tint_color(back, B_LIGHTEN_2_TINT));
	menu->EndLineArray();
	menu->SetHighColor(back);
	zone.InsetBy(-1, -1);
	menu->FillRect(BRect(zone.left, zone.top - 1. , zone.right, zone.top));
	menu->FillRect(BRect(zone.left, zone.bottom, zone.right, zone.bottom + 2.));
	menu->FillRect(BRect(zone.left - 6, zone.top - 2, zone.left, zone.bottom + 2.));
	menu->FillRect(BRect(zone.right, zone.top - 2, zone.right + 5., zone.bottom + 2.));
	if (fGain) {
		size_t		size = sizeof(float) * fChannelCount;
		bigtime_t	when;
		fGain->GetValue(fChannels, &size, &when);
		float		volume = 0;
		for (int c = 0; c < fChannelCount; c++)
			volume += fChannels[c];
		volume /= fChannelCount;
		float	min = fGain->MinValue();
		float	max = fGain->MaxValue();
		float	p = floorf(full.left + (volume - min) * kSliderWidth / (max - min) + 6.);
		menu->SetHighColor(tint_color(back, B_LIGHTEN_2_TINT * B_LIGHTEN_1_TINT));
		BRect	knob(p - 5., zone.top, p + 5., zone.bottom);
		menu->FillRect(knob);
		knob.InsetBy(-1, -1);
		rgb_color black = { 0, 0, 0, 0};
		menu->BeginLineArray(32);
		menu->AddLine(BPoint(knob.left + 1., knob.top), 	BPoint(knob.right - 1.,	knob.top), black);
		menu->AddLine(BPoint(knob.left + 1., knob.bottom),	BPoint(knob.right - 1.,	knob.bottom), black);
		menu->AddLine(BPoint(knob.right, knob.top + 1.),	BPoint(knob.right,	knob.bottom - 1.), black);
		menu->AddLine(BPoint(knob.left, knob.top + 1.),		BPoint(knob.left,	knob.bottom - 1.), black);
		rgb_color wine = { 152, 51, 0, 0};
		menu->AddLine(BPoint(p, zone.top + 2.), BPoint(p, zone.bottom - 2.), wine);
		menu->AddLine(BPoint(knob.right, knob.bottom),	BPoint(knob.right, knob.bottom), dark_2_back);
		knob.OffsetBy(1., 1.);
		menu->AddLine(BPoint(knob.left + 2., knob.bottom),	BPoint(knob.right - 1.,	knob.bottom), dark_2_back);
		menu->AddLine(BPoint(knob.right, knob.top + 2.),	BPoint(knob.right,	knob.bottom - 1.), dark_2_back);
		menu->EndLineArray();
	}
	menu->SetHighColor(high);
}

