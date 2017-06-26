/*

	DisplayViews.cpp

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#include "DisplayViews.h"
#include <stdio.h>
#include <Window.h>

#define DELAY 1500000

const rgb_color kDisplayColor = {0, 203, 0, 255};

displayView::displayView(const BRect & frame, float h, float s) :
	BView(BRect(0, 0, 10, 10), "", B_FOLLOW_NONE | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED, B_WILL_DRAW),
	fPictureFrame (frame), fHeight(h), fSize(s)
{
	fTime = system_time();
}

void displayView::AttachedToWindow()
{
	BView::AttachedToWindow();
	Hide();
	Resize();
	Window()->SetPulseRate(250000);
	SetHighColor(kDisplayColor);
}

void displayView::Draw(BRect updateRect)
{
	DrawString(fText.String(), Bounds().LeftBottom() + BPoint(1.0, 1.0 - fDescent));
}

void displayView::MouseDown(BPoint where)
{
	BView * parent = Parent();
	if (parent) {
		ConvertToParent(&where);
		parent->MouseDown(where);
	}
}

void displayView::Pulse()
{
	if (!IsHidden()) {
		if (system_time() > fTime)
			Hide();
	}
}

void displayView::SetFrame(const BRect & frame)
{
	fPictureFrame = frame;
	Resize();
}

void displayView::SetBackColor(const rgb_color & color)
{
	rgb_color	current = ViewColor();
	if (memcmp(&current, &color, sizeof(rgb_color))) {
		SetViewColor(color);
		Invalidate();
	}
}

void displayView::Resize()
{
	BRect	frame(fPictureFrame);
	float h = frame.Height();

	BFont	font(be_bold_font);
	font.SetSize(floorf(h * fSize));
	font.SetFlags(font.Flags() | B_DISABLE_ANTIALIASING);
	SetFont(&font);
	font_height	fheight;
	font.GetHeight(&fheight);
	fDescent = ceilf(fheight.descent);

	frame.left = floorf(fPictureFrame.left + fPictureFrame.Width() * 0.05);
	frame.right = floorf(frame.left + StringWidth(fText.String()) + 2.0);
	frame.top = floorf(fPictureFrame.top + h * fHeight);
	frame.bottom = floorf(frame.top + h * fSize + 1.0);
	MoveTo(frame.left, frame.top);
	ResizeTo(frame.Width(), frame.Height());
}

void displayView::Changed()
{
	Invalidate();
	fTime = system_time() + DELAY;
	if (IsHidden())
		Show();
}

// Volume display class
volumeDisplay::volumeDisplay(const BRect & frame) : displayView(frame, 0.84, 0.12)
{
}

void volumeDisplay::Draw(BRect updateRect)
{
	SetHighColor(ViewColor());
	FillRect(updateRect);
	SetHighColor(kDisplayColor);
	BRect	f = Bounds();
	float	h = f.Height();
	float	w = f.Width();
	SetPenSize(h / 12.0);
	SetLineMode(B_ROUND_CAP, B_ROUND_JOIN);
	BPoint	points[3] = {	BPoint(0.02 * w, h / 2.0), BPoint(0.98 * w, 0.15 * h), BPoint(0.98 * w, 0.85 * h) };
	StrokePolygon(points, 3, true);
	SetPenSize(h / 5);
	float	v = w * (fVolume * 0.95 + 0.03);
	StrokeLine(BPoint(v, 0.08 * h), BPoint(v, 0.92 * h));
}

void volumeDisplay::Resize()
{
	BRect	frame(fPictureFrame);
	float	h = frame.Height();

	frame.left = floorf(fPictureFrame.left + fPictureFrame.Width() * 0.05);
	frame.right = floorf(frame.left + fPictureFrame.Width() * 0.75);
	frame.top = floorf(fPictureFrame.top + h * fHeight);
	frame.bottom = floorf(frame.top + h * fSize + 1.0);
	MoveTo(frame.left, frame.top);
	ResizeTo(frame.Width(), frame.Height());
}

void volumeDisplay::Set(float v)
{
	fVolume = v;
	Resize();
	Changed();
}

// Mute display class
muteDisplay::muteDisplay(const BRect & frame) : displayView(frame, 0.85, 0.10)
{
}

void muteDisplay::Draw(BRect updateRect)
{
	SetHighColor(ViewColor());
	FillRect(updateRect);
	SetHighColor(kDisplayColor);
	BRect	f = Bounds();
	float	x = f.Width() / 4.;
	float	y = f.Height() / 5.;
	SetPenSize(x / 3.5);
	SetLineMode(B_ROUND_CAP, B_ROUND_JOIN);
	BPoint	points[6] = {	BPoint(1. * x, 2. * y), BPoint(1.8 * x, 2. * y), BPoint(3. * x, 1. * y),
							BPoint(3. * x, 4. * y), BPoint(1.8 * x, 3. * y), BPoint(1. * x, 3. * y)};
	StrokePolygon(points, 6, true);
	SetPenSize(x / 4.5);
	StrokeLine(BPoint(1.2 * x, 4.2 * y), BPoint(2.8 * x, 0.5 * y));
}

void muteDisplay::Pulse()
{
}

void muteDisplay::Resize()
{
	BRect	frame(fPictureFrame);
	float	h = frame.Height();
	float	s = h * fSize;

	frame.right = floorf(fPictureFrame.right - fPictureFrame.Width() * 0.05);
	frame.left = floorf(frame.right - s);
	frame.top = floorf(fPictureFrame.top + h * fHeight);
	frame.bottom = floorf(frame.top + s + 1.0);
	MoveTo(frame.left, frame.top);
	ResizeTo(frame.Width(), frame.Height());
}

void muteDisplay::Set(bool mute)
{
	if (mute) {
		if (IsHidden())
			Show();
	} else {
		if (!IsHidden())
			Hide();
	}
}

// Preset display class
channelDisplay::channelDisplay(const BRect & frame, Preset * pendingPresetStorage) :
	displayView(frame, 0.05, 0.10), fPendingPresetStorage(pendingPresetStorage)
{
}

void channelDisplay::Set(const char* name)
{
	fText = name;
	Resize();
	Changed();
}

Preset& channelDisplay::SetPendingPreset(Preset* preset)
{
	fPreviousPendingPreset = fPendingPreset;
	if (preset)
		fPendingPreset = *preset;
	else
		fPendingPreset.Unset();
	return fPreviousPendingPreset;
}

void channelDisplay::Hide()
{
	displayView::Hide();
	if (fPendingPresetStorage && fPendingPreset.IsValid())
		*fPendingPresetStorage = fPendingPreset;
}
