/*

	VideoWindow.cpp

	© 1991-1999, Be Incorporated, All rights reserved, was Be's CodyCam Sample Code
	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#include "VideoWindow.h"
#include "stampTV.h"
#include "Preferences.h"

#include <Screen.h>
#include <Application.h>
#include <stdio.h>
#include <stdlib.h>

VideoWindow::VideoWindow(BRect frame, const char *title, window_look look, window_feel feel, uint32 flags, uint32 workspace):
	BWindow(frame, title, look, feel, flags, workspace), fVideoView(NULL), fFullScreen(false)
{
	frame.OffsetTo(B_ORIGIN);
	fVideoView	= new stampView(frame);
	AddChild(fVideoView);
	// Work around an interface kit bug (?) where clicking the zoom box doesn't call Zoom...
	ResizeTo(gPrefs.VideoSizeX - 1, gPrefs.VideoSizeY - 1);
	Show();
	if (gPrefs.FullScreen)
		PostMessage(stampView::FULLSCREEN, fVideoView);
}

bool
VideoWindow::QuitRequested()
{
	if (fFullScreen) {
		BScreen(this).SetMode(fFullScreenWorkspace, &fSaveMode);
		// those are the value which will be saved...
		gPrefs.X			= fSaveX;
		gPrefs.Y			= fSaveY;
		gPrefs.VideoSizeX	= fSaveWidth;
		gPrefs.VideoSizeY	= fSaveHeight;
	}
	gPrefs.FullScreen = fFullScreen;
	return true;
}

void
VideoWindow::FrameMoved(BPoint where)
{
	gPrefs.X = (int)where.x;
	gPrefs.Y = (int)where.y;
	CheckWindowPosition();
}

void
VideoWindow::FrameResized(float width, float height)
{
	if (IsFullScreen())
		return;
	width += 1.;
	height += 1.;
	if (width != gPrefs.VideoSizeX || height != gPrefs.VideoSizeY)
		fVideoView->ResizeVideo(width, height);
	CheckWindowPosition();
}

void
VideoWindow::CheckWindowPosition()
{
	if (gPrefs.StayOnScreen && !IsFullScreen()) {
		const float b = 4.;
	
		// Keep the window in the screen
		BRect screen = BScreen(this).Frame();
		BRect frame = Frame();
		BPoint	where = frame.LeftTop();
	
		bool move = false;
		if (where.x < screen.left + b)		{ where.x = screen.left + b; 	move = true; }
		if (where.y < screen.top + b)		{ where.y = screen.top + b;		move = true; }
		if (frame.right > screen.right - b)	{ where.x = screen.right - b - frame.Width();	move = true; }
		if (frame.bottom > screen.bottom - b)	{ where.y =screen.bottom - b - frame.Height();	move = true; }
		if (move) MoveTo(where);
	}
}

void
VideoWindow::SwitchFullScreen(bool NewFullScreenSetting)
{
	if (!fFullScreen || NewFullScreenSetting)
	{
		// Go fullscreen
		BScreen theScreen(this);
		if (!fFullScreen) {
			// Save the current settings
			theScreen.GetMode(&fSaveMode);
			fSaveX		= gPrefs.X;
			fSaveY		= gPrefs.Y;
			fSaveWidth	= gPrefs.VideoSizeX;
			fSaveHeight	= gPrefs.VideoSizeY;
		}

		// Search for a fTVmaxX*fTVmaxY mode, with the same color space as the current mode
		display_mode *mode_list;
		int32 mode_count, preferred = -1, possible = -1;
		theScreen.GetModeList(&mode_list, (uint32 *)&mode_count);
		for (int32 i=0; i<mode_count; i++)
		{
			if ((mode_list[i].timing.h_display == gPrefs.FullScreenX) && 
				(mode_list[i].timing.v_display == gPrefs.FullScreenY) &&
				(mode_list[i].space == fSaveMode.space))
			{
				if (possible == -1)
					possible = i;
				
				if (mode_list[i].timing.pixel_clock == gPrefs.PreferredMode)
					preferred = i;
			}
		}
		if (preferred == -1)
			preferred = possible;
		
		if (preferred != -1) // Resize to fullscreen size
		{
			fFullScreenWorkspace = current_workspace();
			SetWorkspaces(1 << fFullScreenWorkspace);
			fFullScreen = true; // do this first to avoid side effects (FrameResized won't react)
			SetSizeLimits(79, 10000., 59, 10000.);
			ResizeTo(gPrefs.FullScreenX - 1, gPrefs.FullScreenY - 1);
			fVideoView->ResizeVideo(gPrefs.FullScreenX, gPrefs.FullScreenY, &theScreen, &mode_list[preferred]);
			while (!be_app->IsCursorHidden())
				be_app->HideCursor();
			MoveTo(0, 0);
		}
		free(mode_list);
	}
	else
	{
		BScreen theScreen(this);

		// Restore window size and position
		fFullScreen = false;
		fVideoView->ResizeVideo(fSaveWidth, fSaveHeight, &theScreen, &fSaveMode);
		SetMaxSizes(fVideoView->fTVmaxX, fVideoView->fTVmaxY); // for window size
		gPrefs.X = fSaveX;
		gPrefs.Y = fSaveY;
		MoveTo(fSaveX, fSaveY);
		SetWorkspaces(B_ALL_WORKSPACES);
		
		while (be_app->IsCursorHidden())
			be_app->ShowCursor();
	}
}

void
VideoWindow::Zoom(BPoint rec_position, float rec_width, float rec_height)
{
	BRect	b = Bounds();
	int x = b.IntegerWidth() + 1;
	int y = b.IntegerHeight() + 1;
	if (fVideoView->fTVResolutions > 0) {
		bool	set = false;
		for (int k = 0; k < fVideoView->fTVResolutions; k++) {
			if (fVideoView->fTVx[k] == x && fVideoView->fTVy[k] == y) {
				k--;
				if (k < 0)
					k = fVideoView->fTVResolutions - 1;
				x = fVideoView->fTVx[k];
				y = fVideoView->fTVy[k];
				set = true;
				break;
			}
		}
		if (!set) {
			int	diff = LONG_MAX;
			int	best = 0;
			for (int k = 0; k < fVideoView->fTVResolutions; k++) {
				int d = x - fVideoView->fTVx[k];
				if (d < 0) d = -d;
				int e = y - fVideoView->fTVy[k];
				if (e < 0) e = -e;
				d += e;
				if (d < diff) {
					best = k;
					diff = d;
				}
			}
			x = fVideoView->fTVx[best];
			y = fVideoView->fTVy[best];
		}
	}
	fVideoView->ResizeVideo(x, y);
}

void
VideoWindow::WindowActivated(bool state)
{
	if (IsFullScreen()) {
		if (state) {
			while (be_app->IsCursorHidden())
				be_app->ShowCursor();
		} else {
			while (!be_app->IsCursorHidden())
				be_app->HideCursor();
		}
	}
}

void
VideoWindow::WorkspaceActivated(int32, bool active)
{
	fVideoView->SetVisible(active);
}

void
VideoWindow::ScreenChanged(BRect, color_space)
{
	fVideoView->ScreenChanged();
}

void
VideoWindow::SetMaxSizes(int maxX, int maxY)
{
	SetZoomLimits(maxX - 1, maxY - 1);
	SetSizeLimits(79, maxX - 1, 59, maxY - 1);
}
