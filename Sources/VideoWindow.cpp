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

#define TRACE(x)	//printf x

VideoWindow::VideoWindow(BRect frame, const char *title, window_look look, window_feel feel, uint32 flags, uint32 workspace):
	BWindow(frame, title, look, feel, flags, workspace), fVideoView(NULL), fFullScreen(false)
{
	{
		BScreen	screen(this);
		fScreenSize = screen.Frame();
		fColorSpace = screen.ColorSpace();
	}
	frame.OffsetTo(B_ORIGIN);
	fVideoView	= new stampView(frame);
	AddChild(fVideoView);
	// Work around an interface kit bug (?) where clicking the zoom box doesn't call Zoom...
	ResizeTo(gPrefs.WindowWidth - 1, gPrefs.WindowHeight - 1);
	Show();
	if (gPrefs.FullScreen)
		PostMessage(stampView::FULLSCREEN, fVideoView);
}

bool
VideoWindow::QuitRequested()
{
	if (fFullScreen) {
		BScreen(this).SetMode(fSingleWorkspace, &fSaveMode);
		// those are the value which will be saved...
		gPrefs.X			= fSaveX;
		gPrefs.Y			= fSaveY;
		gPrefs.VideoSizeX	= fSaveVideoWidth;
		gPrefs.VideoSizeY	= fSaveVideoHeight;
		gPrefs.WindowWidth	= fSaveWinWidth;
		gPrefs.WindowHeight	= fSaveWinHeight;
	}
	gPrefs.FullScreen = fFullScreen;
	return true;
}

void
VideoWindow::FrameMoved(BPoint where)
{
	gPrefs.X = (int) where.x;
	gPrefs.Y = (int) where.y;
	CheckWindowPosition();
}

void
VideoWindow::FrameResized(float width, float height)
{
	gPrefs.WindowWidth = int32(width + 1.5);
	gPrefs.WindowHeight = int32(height + 1.5);
	if (!IsFullScreen())
		CheckWindowPosition();
}

void
VideoWindow::CheckWindowPosition()
{
	if (gPrefs.ShouldStayOnScreen() && !IsFullScreen()) {
		const float b = 4.;
	
		// Keep the window in the screen
		BRect frame = Frame();
		if (fScreenSize.Width() > frame.Width() + 2 * b && fScreenSize.Height() > frame.Height() + 2 * b) {
			BPoint	where = frame.LeftTop();
		
			bool move = false;
			if (where.x < fScreenSize.left + b)		{ where.x = fScreenSize.left + b; 	move = true; }
			if (where.y < fScreenSize.top + b)		{ where.y = fScreenSize.top + b;		move = true; }
			if (frame.right > fScreenSize.right - b)	{ where.x = fScreenSize.right - b - frame.Width();	move = true; }
			if (frame.bottom > fScreenSize.bottom - b)	{ where.y =fScreenSize.bottom - b - frame.Height();	move = true; }
			if (move) MoveTo(where);
		}
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
			fSaveX				= gPrefs.X;
			fSaveY				= gPrefs.Y;
			fSaveWinWidth		= gPrefs.WindowWidth;
			fSaveWinHeight		= gPrefs.WindowHeight;
			fSaveVideoWidth		= gPrefs.VideoSizeX;
			fSaveVideoHeight	= gPrefs.VideoSizeY;
			fSaveScreenSize		= fScreenSize;
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
			fSingleWorkspace = current_workspace();
			SetWorkspaces(1 << fSingleWorkspace);
			fFullScreen = true; // do this first to avoid side effects (FrameResized won't react)
			SetSizeLimits(79, 10000., 59, 10000.);
			ResizeTo(gPrefs.FullScreenX - 1, gPrefs.FullScreenY - 1);
			if (gPrefs.VideoSizeIsWindowSize)
				fVideoView->ResizeVideo(gPrefs.FullScreenX, gPrefs.FullScreenY, &theScreen, &mode_list[preferred]);
			else
				fVideoView->ResizeVideo(fSaveVideoWidth, fSaveVideoHeight, &theScreen, &mode_list[preferred]);
			while (!be_app->IsCursorHidden())
				be_app->HideCursor();
			MoveTo(0, 0);
		}
		free(mode_list);
	}
	else
	{
		BScreen theScreen(this);

		ResizeTo(fSaveWinWidth - 1, fSaveWinHeight - 1);
		// Restore window size and position
		fFullScreen = false;
		fScreenSize = fSaveScreenSize;	// restore manually to get CheckWindowPosition to work ASAP!
		fColorSpace = (color_space) fSaveMode.space;
		fVideoView->ResizeVideo(fSaveVideoWidth, fSaveVideoHeight, &theScreen, &fSaveMode);
		SetMaxSizes(fVideoView->fTVmaxX, fVideoView->fTVmaxY); // for window size
		gPrefs.X = fSaveX;
		gPrefs.Y = fSaveY;
		MoveTo(fSaveX, fSaveY);
		if (gPrefs.AllWorkspaces)
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
	int maxX = int(fScreenSize.Width() - 8);
	int maxY = int(fScreenSize.Height() - 8 - 20);
	if (fVideoView->fTVResolutions > 0) {
		bool	set = false;
		if (x == maxX && y == maxY) {
			MoveTo(fSaveX, fSaveY);
			int k = fVideoView->fTVResolutions - 1;
			x = fVideoView->fTVx[k];
			y = fVideoView->fTVy[k];
			set = true;
		}
		for (int k = 0; !set && k < fVideoView->fTVResolutions; k++) {
			if (fVideoView->fTVx[k] == x && fVideoView->fTVy[k] == y) {
				k--;
				if (k < 0 && !gPrefs.VideoSizeIsWindowSize) {
					fSaveX = gPrefs.X;
					fSaveY = gPrefs.Y;
					MoveTo(rec_position);
					x = maxX;
					y = maxY;
				} else {
					if (k < 0)
						k = fVideoView->fTVResolutions - 1;
					x = fVideoView->fTVx[k];
					y = fVideoView->fTVy[k];
				}
				set = true;
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
	ResizeTo(x - 1, y - 1);
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
VideoWindow::WorkspaceActivated(int32 workspace, bool active)
{
	TRACE(("WorkspaceActivated: %d Active: %d\n", int(workspace), int(active)));
	if (IsFullScreen() || !gPrefs.AllWorkspaces)
	{
		if (active)
			fVideoView->Resume();
		else
			fVideoView->Suspend();
	}
}

void
VideoWindow::WorkspacesChanged(uint32 old_ws, uint32 new_ws)
{
	fSingleWorkspace = current_workspace();
}

void
VideoWindow::ScreenChanged(BRect screen, color_space space)
{
	TRACE(("ScreenChanged: Color:%d\n", int(space)));
	if (fScreenSize != screen || fColorSpace != space) {
		fScreenSize = screen;
		fColorSpace = space;
		if (!fFullScreen)
			fVideoView->Suspend(system_time() + 500000);
	}
}

void
VideoWindow::SetMaxSizes(int maxX, int maxY)
{
	if (!gPrefs.VideoSizeIsWindowSize) {
		maxX = 1024 * 4;
		maxY = 1024 * 3;
	}
	SetZoomLimits(maxX - 1, maxY - 1);
	SetSizeLimits(79, maxX - 1, 59, maxY - 1);
}
