/*

	VideoWindow.h

	© 1991-1999, Be Incorporated, All rights reserved, was Be's CodyCam Sample Code
	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#ifndef _VIDEO_WINDOW_H_
#define _VIDEO_WINDOW_H_

#include "stampView.h"

#include <Window.h>
#include <Accelerant.h>

class VideoWindow : public BWindow
{
public:
						VideoWindow (
							BRect frame,
							const char* title,
							window_look	look,
							window_feel feel,
							uint32		flags,
							uint32 		workspace = B_ALL_WORKSPACES);

	virtual	bool		QuitRequested();
	virtual void		FrameMoved(BPoint where);
	virtual void		FrameResized(float width, float height);
	virtual void		Zoom(BPoint rec_position, float rec_width, float rec_height);
	virtual	void		WindowActivated(bool state);
	virtual void		WorkspaceActivated(int32 workspace, bool active);
	virtual void		WorkspacesChanged(uint32 old_ws, uint32 new_ws);
	virtual void		ScreenChanged(BRect screen_size, color_space depth);
	
	void				SwitchFullScreen(bool NewFullScreenSetting = false);
	bool				IsFullScreen() const { return fFullScreen; }
	void				CheckWindowPosition();
	void				SetMaxSizes(int maxX, int maxY);
	BRect				ScreenSize() { return fScreenSize; }

	stampView *			VideoView() const { return fVideoView; }

private:
	stampView *			fVideoView;
	display_mode		fSaveMode;
	int32				fSingleWorkspace;
	int					fSaveX, fSaveY, fSaveWinWidth, fSaveWinHeight, fSaveVideoWidth, fSaveVideoHeight;
	bool				fFullScreen;
	color_space			fColorSpace;
	BRect				fScreenSize, fSaveScreenSize;
};



#endif // _VIDEO_WINDOW_H_
