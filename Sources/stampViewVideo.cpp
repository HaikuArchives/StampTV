/*

	stampViewVideo.cpp
	Sub part of stamView implementation. Deals with nodes.

	© 1991-1999, Be Incorporated, All rights reserved, was Be's CodyCam Sample Code
	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#include "stampTV.h"
#include "stampView.h"
#include "VideoConsumer.h"
#include "VideoWindow.h"
#include "Preferences.h"
#include "DisplayViews.h"
#include "PluginsHandler.h"

#include <Application.h>
#include <Screen.h>
#include <Directory.h>
#include <NodeInfo.h>
#include <stdio.h>

#include <MediaRoster.h>
#include <ParameterWeb.h>
#include <TimeSource.h>
#include <TranslationKit.h>
#include <scheduler.h>
#include <Bitmap.h>

status_t 
stampView::SetUpNodes()
{
	status_t status = B_OK;

	/* find the media roster */
	fRoster = BMediaRoster::Roster(&status);
	if (!fRoster || status != B_OK) {
		ErrorAlert("Can't find the media roster", status);
		return status;
	}

//	//Debug code
//	media_node	outNode;
//	status = fRoster->GetAudioOutput(&outNode);
//	if (status == B_OK) {
//		BParameterWeb * web;
//		status_t status = fRoster->GetParameterWebFor(outNode, &web);
//		if (status != B_OK || !web) {
//			ErrorAlert("Can't get parameter web", status);
//		} else {
//			for (int32 i = 0; i < web->CountParameters(); i++) {
//				BParameter *parameter = web->ParameterAt(i);
//				printf("Name: %s Kind: %s Unit: %s ID:%d Type:%d Group:", parameter->Name(), parameter->Kind(), parameter->Unit(), int(parameter->ID()), int(parameter->Type()));
//				BParameterGroup * group = parameter->Group();
//				if (group)
//					printf("%s\n", group->Name());
//				else
//					printf("(No group)\n");
//				BDiscreteParameter * discret = dynamic_cast<BDiscreteParameter *> (parameter);
//				if (discret) {
//					for (int32 k = 0; k < discret->CountItems(); k++) {
//						printf("  %d: %s   Value: %d\n", int(k), discret->ItemNameAt(k), int(discret->ItemValueAt(k)));
//					}
//				} else if (dynamic_cast<BContinuousParameter *> (parameter) != NULL)
//					printf("  Continuous parameter\n");
//				else
//					printf("  (don't know)\n");
//			}
//			delete web;
//		}
//	}

	media_node			fTimeSourceNode;

	/* find the time source */
	status = fRoster->GetTimeSource(&fTimeSourceNode);
	if (status != B_OK) {
		ErrorAlert("Can't get a time source", status);
		return status;
	}

	/* find a video producer node */
	status = fRoster->GetVideoInput(&fVideoInputNode);
	if (status != B_OK) {
		ErrorAlert("Can't find a video input", status);
		return status;
	}
	
	fParameterWebCache.Init(fVideoInputNode, fRoster);

//	//Debug code
//	printf("\n\nChannel kind: %s\n", B_TUNER_CHANNEL);
//	printf("Resolution kind: %s\n", B_RESOLUTION);
//	printf("Video format kind: %s\n", B_VIDEO_FORMAT);
//	BParameterWeb	*web = GetParameterWeb();
//	if (!web)
//		return status;
//	
//	for (int32 i = 0; i < web->CountParameters(); i++) {
//		BParameter *parameter = web->ParameterAt(i);
//		type_code tp = parameter->ValueType();
//		printf("Name: %s Kind: %s Unit: %s Type: %4s ID:%d\n", parameter->Name(), parameter->Kind(), parameter->Unit(), (const char*) &tp, int(parameter->ID()));
//		BDiscreteParameter * discret = dynamic_cast<BDiscreteParameter *> (parameter);
//		BContinuousParameter * continuous = dynamic_cast<BContinuousParameter *> (parameter);
//		if (discret) {
//			for (int32 k = 0; k < discret->CountItems(); k++) {
//				printf("  %d: %s   Value: %d\n", int(k), discret->ItemNameAt(k), int(discret->ItemValueAt(k)));
//			}
//		} else if (continuous) {
//			printf("  Continuous parameter\n");
//		} else
//			printf("  (don't know)\n");
//	}

	delete[] fTVx;
	delete[] fTVy;
	fTVx = fTVy = NULL;
	fTVResolutions = 0;
	BDiscreteParameter * resolution = fParameterWebCache.GetDiscreteParameter(kResolutionParameter);
	if (resolution) {
		// Determine max window size: Max supported resolution of TV input card
		display_mode *list;
		uint32 count;
		BScreen scr(Window());
		display_mode current_mode;
		scr.GetMode(&current_mode);
		scr.GetModeList(&list, &count);
		int	mwx, mwy;
		mwx = mwy = 0;
		int maxResolutions = resolution->CountItems();
		fTVx = new int[maxResolutions];
		fTVy = new int[maxResolutions];
		int32	format = GetDiscreteParameterValue(kFormatParameter);
		fTVResolutions = 0;
		for (int32 k = 0; k < maxResolutions; k++) {
			// find each "natural" resolution of the video card (TV)
			int	x, y;
			sscanf(resolution->ItemNameAt(k), "%dx%d", &x, &y);
			if ((format == 1 || format == 2 || format == 4) && (x > 720 || y > 480))
				continue;	// NTSC-M NTSC-J and PAL-M don't support more than 720*480!
			fTVx[fTVResolutions] = x;
			fTVy[fTVResolutions++] = y;
			if (x >= mwx && y >= mwy) {
				// biggest window size
				mwx = x;
				mwy = y;
			}
		}
		if (mwx < 160 || mwx > 2048 || mwy < 120 || mwy > 2048) {
			fTVmaxX = 640;
			fTVmaxY = 480;
		} else {
			fTVmaxX = mwx;
			fTVmaxY = mwy;
		}
		if (fWindow)
			fWindow->SetMaxSizes(fTVmaxX, fTVmaxY);
		free(list);
	}
	if (fTVResolutions < 1) {
		// default choices...
		fTVResolutions = 2;
		fTVx = new int[fTVResolutions];
		fTVy = new int[fTVResolutions];
		fTVx[0] = 640; fTVy[0] = 480;
		fTVx[1] = 320; fTVy[1] = 240;
	}

	/* create the video consumer node */
	fVideoConsumer = new VideoConsumer("stampTV", this, fWindow != NULL, fRoster);
	if (!fVideoConsumer) {
		ErrorAlert("Can't create a video window", B_ERROR);
		return B_ERROR;
	}

	/* register the node */
	status = fRoster->RegisterNode(fVideoConsumer);
	if (status != B_OK) {
		ErrorAlert("Can't register the video window", status);
		return status;
	}
	
	status = fVideoConsumer->SetupVideoNode(fVideoInputNode);
	if (status != B_OK)
		return status;

	/* Connect The Nodes!!! */
	status = Connect();
	if (status != B_OK) {
		ErrorAlert("Can't connect the video source to the consumer node!", status);
		return status;
	}
	fVideoFrame = Bounds();
	fVideoConsumer->SetWhere(B_ORIGIN);

	/* set time sources */
	status = fRoster->SetTimeSourceFor(fVideoInputNode.node, fTimeSourceNode.node);
	if (status != B_OK) {
		ErrorAlert("Can't set the timesource for the video source", status);
		return status;
	}
	
	status = fRoster->SetTimeSourceFor(fVideoConsumer->ID(), fTimeSourceNode.node);
	if (status != B_OK) {
		ErrorAlert("Can't set the timesource for the video window", status);
		return status;
	}
	
	/* figure out what delay to use */
	bigtime_t latency = 0;
	status = fRoster->GetLatencyFor(fVideoInputNode, &latency);
	status = fRoster->SetProducerRunModeDelay(fVideoInputNode, latency);

	/* start the nodes */
	bigtime_t initLatency = 0;
	status = fRoster->GetInitialLatencyFor(fVideoInputNode, &initLatency);
	if (status < B_OK) {
		ErrorAlert("Can't get the initial latency for producer node", status);	
	}
	initLatency += estimate_max_scheduling_latency();
	
	BTimeSource *timeSource = fRoster->MakeTimeSourceFor(fVideoInputNode);
	bool running = timeSource->IsRunning();
	
	/* workaround for people without sound cards */
	/* because the system time source won't be running */
	bigtime_t real = BTimeSource::RealTime();
	if (!running)
	{
		status = fRoster->StartTimeSource(fTimeSourceNode, real);
		if (status != B_OK) {
			timeSource->Release();
			ErrorAlert("Can't start time source", status);
			return status;
		}
		status = fRoster->SeekTimeSource(fTimeSourceNode, 0, real);
		if (status != B_OK) {
			timeSource->Release();
			ErrorAlert("Can't seek time source", status);
			return status;
		}
	}

	bigtime_t perf = timeSource->PerformanceTimeFor(real + latency + initLatency);
	timeSource->Release();

	/* start the nodes */
	status = fRoster->StartNode(fVideoInputNode, perf);
	if (status != B_OK) {
		ErrorAlert("Can't start the video source", status);
		return status;
	}
	status = fRoster->StartNode(fVideoConsumer->Node(), perf);
	if (status != B_OK) {
		ErrorAlert("Can't start the video window", status);
		return status;
	}
	
	fAllOK = (status == B_OK);
	
	return status;
}

void 
stampView::TearDownNodes()
{
	if (fRoster && fVideoConsumer) {
		LockConnection();
		fParameterWebCache.Release();
		/* stop */
		if (fAllOK) {
			fRoster->StopNode(fVideoConsumer->Node(), 0, true);
			// we don't stop the video node anymore, because there *might* be other users
			//fRoster->StopNode(fVideoInputNode, 0, true);
		}
	
		/* disconnect */
		fVideoConsumer->Disconnect(true);
								
		if (fVideoInputNode != media_node::null) {
			fRoster->ReleaseNode(fVideoInputNode);
			fVideoInputNode = media_node::null;
		}
		fRoster->ReleaseNode(fVideoConsumer->Node());		
		fVideoConsumer = NULL;
	} else
		be_app->PostMessage(B_QUIT_REQUESTED);
}

void
stampView::SetChannel(int32 channel)
{
	fLastChannelValue = SetDiscreteParameterValue(kChannelParameter, channel);
	UpdateWindowTitle();
}

void
stampView::AdjustVolume(int v)
{
	media_node	outNode;
	if (fRoster && fRoster->GetAudioOutput(&outNode) == B_OK) {
		BParameterWeb * web;
		if (fRoster->GetParameterWebFor(outNode, &web) == B_OK && web) {
			BContinuousParameter *	gain = NULL;
			bool					mute = false;
			for (int32 i = 0; i < web->CountParameters(); i++) {
				BParameter *parameter = web->ParameterAt(i);
				if (parameter) {
					BParameterGroup *group = parameter->Group();
					if (group && gPrefs.AudioName == group->Name()) {
						if (strcmp(parameter->Kind(), B_GAIN) == 0)
							gain = dynamic_cast<BContinuousParameter *> (parameter);
						else if (strcmp(parameter->Kind(), B_MUTE) == 0) {
							BDiscreteParameter * dp = dynamic_cast<BDiscreteParameter *> (parameter);
							if (dp) {
								int32 current_value;
								bigtime_t time;
								size_t size = sizeof(int32);
								if ((parameter->GetValue(reinterpret_cast<void *>(&current_value), &size, &time) == B_OK)
										 && (size <= sizeof(int32))) {
									if (current_value == 1) {
										current_value = 0;
										dp->SetValue(reinterpret_cast<void *>(&current_value), sizeof(int32), BTimeSource::RealTime());
										fMuteDisplay->Set(false);
									}
								}
							}
							mute = true;
						}
						if (mute && gain)
							break;
					}
				}
			}
			if (gain) {
				float	min = gain->MinValue();
				float	max = gain->MaxValue();
				float	step = gain->ValueStep() * float(v);
				int		channelsCount = gain->CountChannels();
				if (channelsCount > 0) {
					float		*channels = new float[channelsCount];
					size_t		size = sizeof(float) * channelsCount;
					bigtime_t	when;
					bool		check = false;
					float		average = 0;
					float		check_average = 0;
					float		dilate = 1.0;
					// 'step' is not always big enough (media kit bug?)
					// So we loop increasing the used step more and more,
					// until we see a change of at least step / 4.
					while (gain->GetValue(channels, &size, &when) == B_OK && size > 0 && dilate < 20.) {
						int		count = size / sizeof(float);
						check_average = 0;
						for (int k = 0; k < count; k++) {
							float	v = channels[k];
							if (check)
								check_average += v;
							else
								average += v;
							v += step * dilate;
							if (v > max)
								v = max;
							else if (v < min)
								v = min;
							channels[k] = v;
						}
						if (check)
							check_average /= count;
						else
							average /= count;
						//printf("Average: %f Check: %f Dilate: %f step: %f min: %f max: %f\n", average, check_average, dilate, step, min, max);
						if (check) {
							if (fabs(average - check_average) > fabs(step * .25))
								break;
							if (v < 0 && check_average <= min || v > 0 && check_average >= max)
								break;
						}
						dilate *= 1.2;
						gain->SetValue(channels, size, BTimeSource::RealTime());
						check = true;
					}
					if (check)
						fVolumeDisplay->Set((check_average - min) / (max - min));
					delete [] channels;
				}
			}
			delete web;
		}
		fRoster->ReleaseNode(outNode);
	}
}

void
stampView::UpdateMute(bool switchMute)
{
	media_node	outNode;
	if (fRoster && fRoster->GetAudioOutput(&outNode) == B_OK) {
		BParameterWeb * web;
		if (fRoster->GetParameterWebFor(outNode, &web) == B_OK && web) {
			for (int32 i = 0; i < web->CountParameters(); i++) {
				BParameter *parameter = web->ParameterAt(i);
				if (parameter) {
					BParameterGroup *group = parameter->Group();
					if (group && gPrefs.AudioName == group->Name() && strcmp(parameter->Kind(), B_MUTE) == 0) {
						// mute selector: read its state
						BDiscreteParameter * dp = dynamic_cast<BDiscreteParameter *> (parameter);
						if (dp) {
							int32 current_value;
							bigtime_t time;
							size_t size = sizeof(int32);
							if ((parameter->GetValue(reinterpret_cast<void *>(&current_value), &size, &time) == B_OK)
									 && (size <= sizeof(int32))) {
								if (switchMute) {
									current_value = (current_value == 0);	// switch state
									dp->SetValue(reinterpret_cast<void *>(&current_value), sizeof(int32), BTimeSource::RealTime());
									if (current_value == 1 && !fVolumeDisplay->IsHidden())
										fVolumeDisplay->Hide();
								}
								fMuteDisplay->Set(current_value != 0);
							}
						}
					}
				}
			}
			delete web;
		}
		fRoster->ReleaseNode(outNode);
	}
}

void
stampView::ResizeVideo(int x, int y, BScreen * screen = NULL, display_mode *mode = NULL)
{
	if (!fAllOK)
		return;
	int	bestx = x, besty = y;
	if (IsFullScreen()) {
		// Look for the best size!
		bestx = 0; besty = 0;
		for (int r = 0; r < fTVResolutions; r++) {
			if (fTVx[r] >= bestx && fTVx[r] <= x && fTVy[r] >= besty && fTVy[r] <= y) {
				bestx = fTVx[r];
				besty = fTVy[r];
			}
		}
	}
	gPrefs.VideoSizeX = bestx;
	gPrefs.VideoSizeY = besty;
	LockConnection();
	if (fWindow)
		fWindow->BeginViewTransaction();
	fVideoConsumer->Disconnect();
	if (screen)
		screen->SetMode(mode);
	Connect();
	if (fWindow)
		fWindow->EndViewTransaction();
	UnlockConnection();
	// Connection might have failed and falled back to an other size!
	bestx = gPrefs.VideoSizeX;
	besty = gPrefs.VideoSizeY;
	if (!IsFullScreen() && fWindow)
		fWindow->ResizeTo(bestx - 1, besty - 1);
	else
		Invalidate();	// Update the possible black frame
	BRect	f = Bounds();
	BPoint	p(floorf((f.right + 1 - bestx) / 2), floorf((f.bottom + 1 - besty) / 2));
	fVideoConsumer->SetWhere(p);
	fVideoFrame.Set(p.x, p.y, p.x + bestx - 1, p.y + besty - 1);
	fVolumeDisplay->SetFrame(fVideoFrame);
	fChannelDisplay->SetFrame(fVideoFrame);
	fMuteDisplay->SetFrame(fVideoFrame);
}

status_t
stampView::Connect()
{
	status_t	s = B_MEDIA_BAD_FORMAT;
	int			x = gPrefs.VideoSizeX;
	int			y = gPrefs.VideoSizeY;

	bool		PALMWorkAround = false;
	if (GetDiscreteParameterValue(kFormatParameter) == 4) { //PAL-M
		SetDiscreteParameterValue(kFormatParameter, 1); // NTSC-M
		PALMWorkAround = true;
	}
	
	BScreen(this->Window()).GetMode(&fDisplayMode);
	s = fVideoConsumer->Connect((color_space) fDisplayMode.space, x, y);

	if (PALMWorkAround)
		SetDiscreteParameterValue(kFormatParameter, 4); //PAL-M

	if (s < B_OK) {
		// sometimes, too big a size might be the cause. Try one size smaller...
		int	maxX = 0, maxY = 0;
		for (int r = 0; r < fTVResolutions; r++)
			if (fTVx[r] >= maxX && fTVy[r] >= maxY && (fTVx[r] < x && fTVy[r] <= y || fTVx[r] <= x && fTVy[r] < y)) {
				maxX = fTVx[r];
				maxY = fTVy[r];
			}
		if (maxX > 0 && maxY > 0) {
			fVideoConsumer->InitColorSpaceSupport();
			gPrefs.VideoSizeX = maxX;
			gPrefs.VideoSizeY = maxY;
			printf("Retrying to smaller size %dx%d because %s\n", maxX, maxY, strerror(s));
			s = Connect();
		}
	}
	return s;
}

#if 1
// Use a benaphore style to lock the connection (init semaphore 0)
bool
stampView::TryLockConnection()
{
	if (atomic_add(&fConnectionLock, 1) > 0) {
		atomic_add(&fConnectionLock, -1); // no one can be waiting. Two threads scheme.
		return false;
	}
	return true;
}

void
stampView::LockConnection()
{
	if (atomic_add(&fConnectionLock, 1) > 0)
		while (acquire_sem(fConnectionLockSem) == B_INTERRUPTED)
			;
}

void
stampView::UnlockConnection()
{
	if (atomic_add(&fConnectionLock, -1) > 1)
		release_sem(fConnectionLockSem);
}

#else
// Use a semaphore style to lock the connection (init semaphore 1)

bool
stampView::TryLockConnection()
{
	status_t r;
	while ((r = acquire_sem_etc(fConnectionLockSem, 1, B_RELATIVE_TIMEOUT, 0)) == B_INTERRUPTED)
			;
	return r == B_OK;
}

void
stampView::LockConnection()
{
	while (acquire_sem(fConnectionLockSem) == B_INTERRUPTED)
			;
}

void
stampView::UnlockConnection()
{
	release_sem(fConnectionLockSem);
}

#endif

void
stampView::SetVisible(bool visible)
{
	if (fAllOK)
		fVideoConsumer->SetVisible(visible);
	if (fWindow && fWindow->IsFullScreen())
		ScreenChanged();
}

void
stampView::ScreenChanged()
{
	display_mode		dm;
	BScreen(this->Window()).GetMode(&dm);
	rgb_color	backcolor;
	bool	overlay = fVideoConsumer->GetOverlayColor(&backcolor);
	if (overlay ? memcmp(&fDisplayMode, &dm, sizeof(display_mode)) : dm.space != fDisplayMode.space)
		ResizeVideo(gPrefs.VideoSizeX, gPrefs.VideoSizeY);
}

int32
stampView::GetCurrentPreset()
{
	int32	channel = GetChannel();
	if (channel >= 0) {
		for (int p = 0; p < kMaxPresets; p++)
			if (gPrefs.presets[p].channel == channel)
				return p;
	}
	return -1;
}

int32
stampView::GetNextPreset(int direction)
{
	int32	current = GetCurrentPreset();
	int32	p = current;
	int d = direction > 0 ? +1 : -1;
	int c = direction * d;	// always > 0
	while (c-- > 0) {
		int loop = kMaxPresets;
		do {
			p = (p + d) % kMaxPresets;
			if (p < 0)
				p += kMaxPresets;
			if (loop-- < 0)
				return p; // avoid looping for ever!
		} while (gPrefs.presets[p].channel < 0);
	}
	return p;
}

void
stampView::UpdateWindowTitle(const char * new_title = NULL)
{
	if (fWindow) {
		if (new_title)
			fWindowTitle = new_title;
		bool	forced_title = (*fWindowTitle.String() != 0);
		if (forced_title)
			fWindow->SetTitle(fWindowTitle.String());
		if (!forced_title || fChannelDisplay) {
			BString title;
			int32	channel = GetChannel();
			int p;
			for (p = 0; p < kMaxPresets; p++) {
				if (gPrefs.presets[p].channel == channel) {
					title << gPrefs.presets[p].name;
					break;
				}
			}
			if (p >= kMaxPresets) {
				BDiscreteParameter	*tuner = fParameterWebCache.GetDiscreteParameter(kChannelParameter);
				if (tuner)
					title << tuner->ItemNameAt(channel);
				else
					title << "???";
			}
			if (fChannelDisplay)
				fChannelDisplay->Set(title.String());
			if (!forced_title) {
				BString	windowTitle = "stampTV - ";
				windowTitle << title;
				fWindow->SetTitle(windowTitle.String());
			}
		}
	}
}

long
stampView::save_frame_thread(void *p)
{
	BMessage * m = (BMessage*) p;
	stampView * view = (stampView*) m->what;
	view->SaveFrameThread(m);
	return B_OK;
}

inline uchar clip8(int a)
{
	uchar c;
	if (a > 255)
		c = 255;
	else if (a < 0)
		c = 0;
	else
		c = (uchar) a;
		
	return c;
} 

inline uchar YCbCr_red(uchar y, uchar, uchar cr)
{
	return clip8((int)(1.164 * (y - 16) + 1.596 * (cr - 128)));
}

inline uchar YCbCr_green(uchar y, uchar cb, uchar cr)
{
	return clip8((int)(1.164 * (y - 16) - 0.813 * (cr - 128) -
		0.392 * (cb - 128)));
}

inline uchar YCbCr_blue(uchar y, uchar cb, uchar)
{
	return clip8((int)(1.164 * (y - 16) + 2.017 * (cb - 128)));
}

void
stampView::SaveFrameThread(BMessage *msg)
{
	if (!fAllOK)
		return;
	const char * types;
	const char * mime;
	const char * name;
	entry_ref	 ref;
	if (msg->FindString("be:types", &types) == B_OK
			&& msg->FindString("be:filetypes", &mime) == B_OK
			&& msg->FindString("name", &name) == B_OK
			&& msg->FindRef("directory", &ref) == B_OK
			&& strcmp(types, B_FILE_MIME_TYPE) == 0)
	{
		BBitmap *bm = fVideoConsumer->GetFrame();
		if (bm) {
			if (bm->ColorSpace() == B_YCbCr422) {
				// The translation kit doesn't want to translate... Let's do it!
				BRect bounds = bm->Bounds();
				BBitmap * rgb = new BBitmap(bounds, 0, B_RGB32);
				uchar *srcRowStart = (uchar*) bm->Bits();
				uchar *destRowStart = (uchar*) rgb->Bits();
				uint height = (uint) bounds.Height() + 1;
				uint width = (uint) bounds.Width() + 1;
				for (uint row = 0; row < height; row++) {
					uchar *src = srcRowStart;
					uchar *dest = destRowStart;
					for (uint col = 0; col < width; col += 2) {
						uint y1 = *src++;
						uint cb = *src++;
						uint y2 = *src++;
						uint cr = *src++;

						*dest++ = YCbCr_blue(y1, cb, cr);
						*dest++ = YCbCr_green(y1, cb, cr);
						*dest++ = YCbCr_red(y1, cb, cr);
						dest++;

						if (col + 1 < width) {
							*dest++ = YCbCr_blue(y2, cb, cr);
							*dest++ = YCbCr_green(y2, cb, cr);
							*dest++ = YCbCr_red(y2, cb, cr);
							dest++;
						}
					}

					destRowStart += rgb->BytesPerRow();
					srcRowStart += bm->BytesPerRow();
				}
				delete bm;
				bm = rgb;
			} else if (bm->ColorSpace() == B_RGB16) {
				// The translation kit is really lazy... Let's try to help it...
				BRect		bounds = bm->Bounds();
				BBitmap *	rgb = new BBitmap(bounds, 0, B_RGB32);
				short *		srcRowStart = (short*) bm->Bits();
				uint32 *	destRowStart = (uint32*) rgb->Bits();
				int			height = (int) bounds.Height() + 1;
				int			width = (int) bounds.Width() + 1;
				for (int row = 0; row < height; row++) {
					short *src = srcRowStart;
					uint32 *dest = destRowStart;
					for (int col = 0; col < width; col ++) {
						int		s = *src ++;
						uint	r = (s >> 8) & 0xf8;
						uint	g = (s >> 3) & 0xfc;
						uint	b = (s & 0x1f) << 3;
						*dest ++ = (r << 16) | (g << 8) | (b << 0);
					}

					destRowStart += rgb->BytesPerRow() / 4;
					srcRowStart += bm->BytesPerRow() / 2;
				}
				delete bm;
				bm = rgb;
			}
			uint32 type = 0;
			int32				tr_count;
			translator_id *		translators;
			BTranslatorRoster *	roster = BTranslatorRoster::Default();
			roster->GetAllTranslators(&translators, &tr_count);
			
			for (int i = 0;  i < tr_count;  ++i) {
				const translation_format *formats;
				int32 fmt_count;
				
				roster->GetOutputFormats(translators[i], &formats, &fmt_count);
				
				for (int32 j = 0;  j < fmt_count;  ++j) {
					if (	formats[j].group == B_TRANSLATOR_BITMAP  &&
							formats[j].type != B_TRANSLATOR_BITMAP) {
							
						if (strcmp(formats[j].MIME, mime) == 0) {
							type = formats[j].type;
							break;
						}
					}
				}
			}
			
			delete [] translators;

			if (type != 0) {
				BDirectory dir;
				dir.SetTo(&ref);
	
				BFile file;
				file.SetTo(&dir, name, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
				
				BBitmapStream source(bm);
				
				roster->Translate(&source, 0, 0, &file, type, 0);
	
				BNodeInfo ni(&file);
				ni.SetType(mime);
			}
		}
	}
}

void
stampView::InitiateDrag(BPoint point)
{
	BTranslatorRoster *roster = BTranslatorRoster::Default();
	int32 tr_count;
	translator_id *translators;
	roster->GetAllTranslators(&translators, &tr_count);
	
	BMessage msg(B_SIMPLE_DATA);
	msg.AddInt32("be:actions", B_COPY_TARGET);
	msg.AddString("be:types", B_FILE_MIME_TYPE);
	BString str("stampTV frame");
	int	p = GetCurrentPreset();
	if (p >= 0) {
		str = gPrefs.presets[p].name;
		str += " frame";
	}
	msg.AddString("be:clip_name", str.String());
	
	for (int i = 0;  i < tr_count;  ++i) {
		const translation_format *formats;
		int32 fmt_count;
		
		roster->GetOutputFormats(translators[i], &formats, &fmt_count);
		
		for (int32 j = 0;  j < fmt_count;  ++j) {
			if (	formats[j].group == B_TRANSLATOR_BITMAP  &&
					formats[j].type != B_TRANSLATOR_BITMAP) {
					
				msg.AddString("be:filetypes", formats[j].MIME);
				msg.AddString("be:types", formats[j].MIME);
				msg.AddString("be:type_descriptions", formats[j].name);
				msg.AddInt32("types", formats[j].type);
			}
		}
	}
	
	if (modifiers() & B_CONTROL_KEY)
		msg.AddInt32("buttons", B_SECONDARY_MOUSE_BUTTON);	// cheat the system
	
	BRect	rect(0, 0, 50, 50);
	rect.OffsetBy(point);
	rect.OffsetBy(-25, -25);
	DragMessage(&msg, rect, this);
	
	delete [] translators;
}
