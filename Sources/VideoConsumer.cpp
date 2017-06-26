/*

	VideoConsumer.cpp

	© 1991-1999, Be Incorporated, All rights reserved, was Be's CodyCam Sample Code
	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#include <View.h>
#include <stdio.h>
#include <fcntl.h>
#include <Buffer.h>
#include <unistd.h>
#include <string.h>
#include <NodeInfo.h>
#include <scheduler.h>
#include <TimeSource.h>
#include <StringView.h>
#include <MediaRoster.h>
#include <Application.h>
#include <BufferGroup.h>
#include <Window.h>
#include <Bitmap.h>
#include <TranslationKit.h>

#include "VideoConsumer.h"
#include "stampTV.h"
#include "stampView.h"
#include "stampPluginsHandler.h"

#define M1 ((double)1000000.0)
#define JITTER		20000

#define	FUNCTION(x)	printf x
#define PROGRESS(x)	printf x
#define LOOP(x)		//printf x

// Should we *try* to use overlay? (if we fail, then we will fall back to non-overlay mode)
#define TRY_OVERLAY 1

#define OVERLAY_BITMAP_FLAGS (B_BITMAP_WILL_OVERLAY | B_BITMAP_RESERVE_OVERLAY_CHANNEL)
#define BUFFER_BITMAP_FLAGS B_BITMAP_IS_CONTIGUOUS
#define OVERLAY_BUFFER_BITMAP_FLAGS (BUFFER_BITMAP_FLAGS | OVERLAY_BITMAP_FLAGS)

const int kBufferCount = 3;

const media_raw_video_format vid_format = { 25, 1, 0, 479, B_VIDEO_TOP_LEFT_RIGHT, 1, 1, {B_RGB32, 640, 480, 640*4, 0, 0}};

//---------------------------------------------------------------

VideoConsumer::VideoConsumer( const char * name, stampView * sView, bool quit_app, BMediaRoster * roster) :
	BMediaNode(name),
	BMediaEventLooper(),
	BBufferConsumer(B_MEDIA_RAW_VIDEO),
	mStamp(sView),
	mQuitApp(quit_app),
	fRoster(roster),
	mBuffers(NULL),
	mBufferCount(kBufferCount),
	mOverlayBitmap(NULL),
	mOverlayWouldNotScale(false),
	mLastFrame(0),
	mFrameRequested(false)
{
	FUNCTION(("VideoConsumer::VideoConsumer\n"));

//	AddNodeKind(B_PHYSICAL_OUTPUT); not really!
	SetEventLatency(0);
	for (uint32 j = 0; j < kMaxBufferCount; j++)
	{
		mBitmap[j] = NULL;
		mBufferMap[j] = 0;
	}
	SetPriority(B_DISPLAY_PRIORITY);
	InitColorSpaceSupport();
	fPluginsHandler = new stampPluginsHandler(mStamp, this);
	mStamp->Window()->AddHandler(fPluginsHandler);
	mFrameRequestPort = create_port(5, "Frame Requests");
}

//---------------------------------------------------------------

VideoConsumer::~VideoConsumer()
{
	FUNCTION(("VideoConsumer::~VideoConsumer\n"));
	
	Quit();
	DeleteBuffers();
	if (mQuitApp)
		be_app->PostMessage(B_QUIT_REQUESTED);
	FUNCTION(("VideoConsumer::~VideoConsumer - DONE\n"));
	delete_port(mFrameRequestPort);
}

/********************************
	From BMediaNode
********************************/

//---------------------------------------------------------------

BMediaAddOn *
VideoConsumer::AddOn(long *cookie) const
{
	FUNCTION(("VideoConsumer::AddOn\n"));
	// do the right thing if we're ever used with an add-on
	*cookie = 0;
	return NULL;
}


//---------------------------------------------------------------

void
VideoConsumer::NodeRegistered()
{
	FUNCTION(("VideoConsumer::NodeRegistered\n"));
	mIn.destination.port = ControlPort();
	mIn.destination.id = 0;
	mIn.source = media_source::null;
	mIn.format.type = B_MEDIA_RAW_VIDEO;
	mIn.format.u.raw_video = vid_format;

	Run();
}

//---------------------------------------------------------------

status_t
VideoConsumer::HandleMessage(int32 message, const void * data, size_t size)
{
	FUNCTION(("VideoConsumer::HandleMessage\n"));
		
	switch (message)
	{
		default: {
			status_t error = B_OK;
			if (((error = BBufferConsumer::HandleMessage(message, data, size)) != B_OK)
			 && ((error = BMediaNode::HandleMessage(message, data, size)) != B_OK)) {
				BMediaNode::HandleBadMessage(message, data, size);
			}
			return error;
		}
	}
}

//---------------------------------------------------------------

void
VideoConsumer::BufferReceived(BBuffer * buffer)
{
//	FUNCTION(("VideoConsumer::BufferReceived\n"));
	if (RunState() != B_STARTED)
	{
		buffer->Recycle();
		return;
	}
	if (1)
	{
		media_timed_event event(buffer->Header()->start_time, BTimedEventQueue::B_HANDLE_BUFFER,
							buffer, BTimedEventQueue::B_RECYCLE_BUFFER);
		EventQueue()->AddEvent(event);
	} else
		ProcessBuffer(buffer);
}


//---------------------------------------------------------------

void
VideoConsumer::ProcessBuffer(BBuffer * buffer)
{
	if (RunState() == B_STARTED)
	{
		mFrameCount++;
		// see if this is one of our buffers
		int	index = 0;
		bool ourBuffers = true;
		while (index < mBufferCount && buffer->ID() != mBufferMap[index])
			index++;
				
		if (index == mBufferCount)
		{
			// no, buffers belong to consumer
			ourBuffers = false;
			index = mLastFrame + 1;
			if (index >= mBufferCount)
				index = 0;
		}
									
		if (RunMode() == B_OFFLINE ||
			TimeSource()->Now() < buffer->Header()->start_time + JITTER)
		{
			BBitmap * currentBitmap = mBitmap[index];
			if (!ourBuffers)
				// not our buffers, so we need to copy
				memcpy(currentBitmap->Bits(), buffer->Data(), currentBitmap->BitsLength());
			mLastFrame = index;
			if (mStamp->TryLockConnection()) {
				currentBitmap = fPluginsHandler->Filter(currentBitmap, mFrameCount, mFrameDropped);
				mStamp->DrawFrame(currentBitmap, currentBitmap == mOverlayBitmap);
				mStamp->UnlockConnection();
			}
			mFrameDropped = false;
			if (mFrameRequested) {
				PROGRESS(("VideoConsumer::ProcessBuffer - Frame requested\n"));
				if (port_count(mFrameRequestPort) < 1) {
					//	Frame capture code
					BRect bounds = currentBitmap->Bounds();
					BBitmap * bm = new BBitmap(bounds, 0, currentBitmap->ColorSpace());
					if (bm->BitsLength() == currentBitmap->BitsLength())
						memcpy(bm->Bits(), currentBitmap->Bits(), currentBitmap->BitsLength());
					else {
						int b_bpr = currentBitmap->BytesPerRow();
						int bm_bpr = bm->BytesPerRow();
						int bpr = b_bpr < bm_bpr ? b_bpr : bm_bpr;
						char * bp = (char*) currentBitmap->Bits();
						char * bmp = (char*) bm->Bits();
						int count = int(bounds.bottom - bounds.top) + 1;
						while (count-- > 0) {
							memcpy(bmp, bp, bpr);
							bmp += bm_bpr;
							bp += b_bpr;
						}
					}
					write_port(mFrameRequestPort, 'grab', &bm, sizeof(bm));
				}
				mFrameRequested = false;
			}
		}
		else {
			PROGRESS(("VideoConsumer::ProcessBuffer - DROPPED FRAME\n"));
			mFrameDropped = true;
		}
	}
	buffer->Recycle();
}


//---------------------------------------------------------------

void
VideoConsumer::ProducerDataStatus(
	const media_destination &for_whom,
	int32 status,
	bigtime_t at_media_time)
{
	FUNCTION(("VideoConsumer::ProducerDataStatus: %d\n", int(status)));
}

//---------------------------------------------------------------

status_t
VideoConsumer::CreateBuffers(media_format & format, color_space overlay_space,
	bool draw_in_buffer, bool createOverlay, bool overlayBuffer)
{
	FUNCTION(("VideoConsumer::CreateBuffers\n"));
	
	// delete any old buffers
	DeleteBuffers();
	color_space mColorspace = format.u.raw_video.display.format;
	BRect		frame(0, 0, format.u.raw_video.display.line_width - 1, format.u.raw_video.display.line_count - 1);

	PROGRESS(("VideoConsumer::CreateBuffers - Colorspace: %d->%d Overlay: %d OverlayBuffer: %d -> ", int(mColorspace), int(overlay_space), int(createOverlay), int(overlayBuffer)));
	
	if (createOverlay) {
		uint32	flags = overlayBuffer ? OVERLAY_BUFFER_BITMAP_FLAGS : OVERLAY_BITMAP_FLAGS;
		BBitmap * bm = new BBitmap(frame, flags, overlay_space);
		if (!(bm && bm->InitCheck() == B_OK && bm->IsValid())) {
			PROGRESS(("Couldn't create overlay!\n"));
			delete bm;
			return B_ERROR;
		}
		PROGRESS(("Overlay created!\n"));
		mOverlayBitmap = bm;
	}
	mBufferCount = overlayBuffer ? 1 : kBufferCount;

	// create a buffer group
	mBuffers = new BBufferGroup();
	status_t status = mBuffers->InitCheck();
	if (status != B_OK)
	{
		delete mBuffers;
		mBuffers = NULL;
		ErrorAlert("VideoConsumer::CreateBuffers - ERROR CREATING BUFFER GROUP", status);
		return status;
	}

	// and attach the  bitmaps to the buffer group
	for (int j = 0; j < mBufferCount; j++)
	{
		BBitmap * bm;
		if (overlayBuffer)
			bm = mOverlayBitmap;
		else {
			bm = new BBitmap(frame, draw_in_buffer ? BUFFER_BITMAP_FLAGS | B_BITMAP_ACCEPTS_VIEWS : BUFFER_BITMAP_FLAGS, mColorspace);
			if (!(bm && bm->InitCheck() == B_OK && bm->IsValid())) {
				delete bm;
				PROGRESS(("Buffer Bitmap creation failed!\n"));
				return B_ERROR;
			}
			PROGRESS(("Bitmap #%d created. ", j + 1));
		}
		buffer_clone_info info;
		info.area = area_for(bm->Bits());
		area_info bm_info;
		status = get_area_info(info.area, &bm_info);
		info.offset = ((char *) bm->Bits()) - ((char *) bm_info.address);
		info.size = bm->BitsLength();
		info.flags = 0;
		info.buffer = 0;
		
		BBuffer * buffer = NULL;
		if ((status = mBuffers->AddBuffer(info, &buffer)) != B_OK)
		{
			ErrorAlert("VideoConsumer::CreateBuffers - ERROR ADDING BUFFER TO GROUP", status);
			return status;
		}
		mBufferMap[j] = buffer->ID();
		mBitmap[j] = bm;
	}
	format.u.raw_video.display.bytes_per_row = mBitmap[0]->BytesPerRow();
	format.deny_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	format.require_flags = B_MEDIA_MAUI_UNDEFINED_FLAGS;
	if (createOverlay && overlayBuffer)
		format.require_flags |= B_MEDIA_LINEAR_UPDATES;
	if (mBufferCount > 1) {
		format.require_flags |= B_MEDIA_MULTIPLE_BUFFERS;
		//format.deny_flags |= B_MEDIA_RETAINED_DATA;
	}
	PROGRESS(("\n"));
	return status;
}

//---------------------------------------------------------------

void
VideoConsumer::DeleteBuffers()
{
	if (mBuffers)
	{
		FUNCTION(("VideoConsumer::DeleteBuffers\n"));
		
		for (int j = 0; j < mBufferCount; j++)
			if (mBitmap[j] != NULL){
				if (mBitmap[j]->IsValid())
				{

// Experimental code to save the current bitmaps... Very interesting!
//					BEntry		entry;
//					char		name[B_FILE_NAME_LENGTH];
//					int	n = 1;
//					do {
//						sprintf(name, "/boot/home/Desktop/Frame %d - %d", j+1, n++);
//						entry.SetTo(name);
//					} while (entry.Exists());
//							
//					BTranslatorRoster * roster = BTranslatorRoster::Default();
//					BBitmapStream stream(mBitmap[j]);
//					BFile file(name, B_CREATE_FILE | B_WRITE_ONLY | B_ERASE_FILE);
//					if (file.InitCheck() == B_OK) {
//						uint32 type = B_TGA_FORMAT;
//						roster->Translate(&stream, NULL, NULL, &file, type);
//						stream.DetachBitmap(&mBitmap[j]);
//					}

					delete mBitmap[j];
					if (mBitmap[j] == mOverlayBitmap)
						mOverlayBitmap = NULL;
					mBitmap[j] = NULL;
				}
			}
		delete mBuffers;
		mBuffers = NULL;
		delete mOverlayBitmap;
		mOverlayBitmap = NULL;
	}
}

//---------------------------------------------------------------

void
VideoConsumer::InitColorSpaceSupport()
{
#if TRY_OVERLAY
	DeleteBuffers();	// this can't happen anytime. Some cards accept only one overlay at a time...
	mOverlaySpaces.MakeEmpty();
	memset(&mRestrictions, 0, sizeof(mRestrictions));

	BRect	frame(0, 0, 15, 15);

	color_space	possible_overlays[] = { B_RGB32, B_RGB16, B_RGB15, B_YCbCr422, B_YCbCr411, B_YCbCr444, B_YCbCr420 };
	int		tries = sizeof (possible_overlays) / sizeof (color_space);
	bool	restrictionsFound = false;
	for (int t = 0; t < tries; t++) {
		BBitmap * bm = new BBitmap(frame, OVERLAY_BUFFER_BITMAP_FLAGS, possible_overlays[t]);
		if (bm && bm->InitCheck() == B_OK && bm->IsValid()) {
			mOverlaySpaces.AddItem((void*) possible_overlays[t]);
			if (!restrictionsFound && bm->GetOverlayRestrictions(&mRestrictions) == B_OK)
				restrictionsFound = true;
		}
		delete bm;
	}
#endif
}

bool
VideoConsumer::OverlayCompatible(color_space cspace)
{
	return mOverlaySpaces.HasItem((void *)cspace);
}

bool
VideoConsumer::CheckOverlayRestrictions(int vWidth, int vHeight, float wWidth, float wHeight)
{
	return (mOverlaySpaces.CountItems() > 0
		&& vWidth * mRestrictions.min_width_scale <= wWidth
		&& vWidth * mRestrictions.max_width_scale >= wWidth
		&& vHeight * mRestrictions.min_height_scale <= wHeight
		&& vHeight * mRestrictions.max_height_scale >= wHeight
		// These are not well implemented by the Voodoo3 driver... What about others?... :-(
//		&& mRestrictions.source.min_width <= vWidth
//		&& mRestrictions.source.max_width >= vWidth
//		&& mRestrictions.source.min_height <= vHeight
//		&& mRestrictions.source.max_height >= vHeight
//		&& mRestrictions.destination.min_width <= wWidth
//		&& mRestrictions.destination.max_width >= wWidth
//		&& mRestrictions.destination.min_height <= wHeight
//		&& mRestrictions.destination.max_height >= wHeight
		);
}

//---------------------------------------------------------------

status_t
VideoConsumer::SetupVideoNode(media_node & video_node)
{
	FUNCTION(("VideoConsumer::SetupVideoNode\n"));

	/* find free producer output */
	int32 cnt = 0;
	status_t status = fRoster->GetFreeOutputsFor(video_node, &fProducerOut, 1,  &cnt, B_MEDIA_RAW_VIDEO);
	if (status != B_OK || cnt < 1) {
		ErrorAlert("Can't find an available video stream", status);
		if (cnt < 1)
			status = B_RESOURCE_UNAVAILABLE;
		return status;
	}

	/* find free consumer input */
	cnt = 0;
	status = fRoster->GetFreeInputsFor(Node(), &fConsumerIn, 1, &cnt, B_MEDIA_RAW_VIDEO);
	if (status != B_OK || cnt < 1) {
		ErrorAlert("Can't find an available connection to the video window", status);
		if (cnt < 1)
			status = B_RESOURCE_UNAVAILABLE;
	}
	return status;
}

//---------------------------------------------------------------

status_t
VideoConsumer::Connect(color_space current, int vWidth, int vHeight, float wWidth, float wHeight)
{
	FUNCTION(("VideoConsumer::Connect %d\n", int(current)));

	int32		cookie = 0;
	color_space	in, out;
	uint32		firstFlags, allFlags, lastFlags;
	bool		drawInBuffer;
	in = ANY_COLOR_SPACE;
	out = ANY_COLOR_SPACE;
	while (fPluginsHandler->GetNextPath(cookie, in, out, drawInBuffer, firstFlags, allFlags, lastFlags) == B_OK) {
		bool	overlayPossible = CheckOverlayRestrictions(vWidth, vHeight, wWidth, wHeight);
		bool	overlayBuffer = supports_each(allFlags, VAPI_PROCESS_OVERLAY_IN_PLACE);
		bool	overlayOutput = (overlayBuffer || supports_one(lastFlags, VAPI_PROCESS_TO_OVERLAY | VAPI_PROCESS_OVERLAY_IN_PLACE));
		bool	normalBuffers = supports_one(firstFlags, VAPI_PROCESS_TO_DISTINCT | VAPI_PROCESS_IN_PLACE);
		mOverlayWouldNotScale =  mOverlaySpaces.CountItems() > 0 && (overlayBuffer || overlayOutput) && !overlayPossible;
		if (!overlayPossible)
			overlayBuffer = overlayOutput = false;
		if (out == ANY_COLOR_SPACE) {
			// we can choose freely the color mode
			if (overlayOutput) {
				int m = mOverlaySpaces.CountItems();
				for (int c = 0; c < m; c++) {
					color_space cs = (color_space) int(mOverlaySpaces.ItemAt(c));
					if (TryConnect(cs, cs, vWidth, vHeight, drawInBuffer, true, overlayBuffer) == B_OK)
						goto mode_found;
				}
			}
			if (normalBuffers) {
				// Overlay connections didn't work out, try without overlay
				if (TryConnect(current, current, vWidth, vHeight, drawInBuffer) == B_OK)	// first try current color_space: the most efficient to blit!
					goto mode_found;
				color_space	spaces[] = { B_RGB32, B_RGB16, B_RGB15 };
				int m = sizeof (spaces) / sizeof (color_space);
				for (int t = 0; t < m; t++) {
					if (spaces[t] != current) {
						if (TryConnect(spaces[t], spaces[t], vWidth, vHeight, drawInBuffer) == B_OK)
							goto mode_found;
					}
				}
			}
			goto no_way;	// this path didn't work out. Try the next one...
		} else {
			// we can *not* choose freely the color mode
			if (overlayOutput && TryConnect(in, out, vWidth, vHeight, drawInBuffer, true, overlayBuffer) == B_OK)
				goto mode_found;
			if (!normalBuffers || TryConnect(in, out, vWidth, vHeight, drawInBuffer) != B_OK)
				goto no_way;
		}
mode_found:
		fPluginsHandler->UsePath(cookie, mIn.format.u.raw_video, mOverlayBitmap);
		return B_OK;
no_way:		
		in = ANY_COLOR_SPACE;
		out = ANY_COLOR_SPACE;
	}
	return B_ERROR;
}

//---------------------------------------------------------------

status_t
VideoConsumer::TryConnect(color_space buffer_space, color_space overlay_space, int width, int height,
	bool draw_in_buffer, bool createOverlay, bool overlayBuffer)
{
	FUNCTION(("VideoConsumer::TryConnect %d overlay:%d\n", int(buffer_space), int(createOverlay)));
	media_format format;
	format.type = B_MEDIA_RAW_VIDEO;
	format.u.raw_video.field_rate = 0;
	format.u.raw_video.interlace = 0;
	format.u.raw_video.first_active = 0;
	format.u.raw_video.last_active = height - 1;
	format.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	format.u.raw_video.pixel_width_aspect = 1;
	format.u.raw_video.pixel_height_aspect = 1;
	format.u.raw_video.display.format = buffer_space;
	format.u.raw_video.display.line_width = width;
	format.u.raw_video.display.line_count = height;
	format.u.raw_video.display.pixel_offset = 0;
	format.u.raw_video.display.line_offset = 0;
	format.u.raw_video.display.flags = 0;
	
	/* connect producer to consumer */
	status_t s = CreateBuffers(format, overlay_space, draw_in_buffer, createOverlay, overlayBuffer);
	if (s == B_OK) {
		s = fRoster->Connect(fProducerOut.source, fConsumerIn.destination, 
							&format, &fProducerOut, &fConsumerIn);
		FUNCTION(("VideoConsumer::TryConnect -> %s\n", strerror(s)));
	}
	return s;
}

//---------------------------------------------------------------

status_t
VideoConsumer::Connected(
	const media_source & producer,
	const media_destination & where,
	const media_format & with_format,
	media_input * out_input)
{
	FUNCTION(("VideoConsumer::Connected\n"));
	
	mIn.source = producer;
	mIn.format = with_format;
	mIn.node = Node();
	sprintf(mIn.name, "Video Consumer");
	*out_input = mIn;
	if (mIn.format.u.raw_video.field_rate < 1.0)
		mIn.format.u.raw_video.field_rate = 25.;	// defensive programming!

	uint32 user_data = 0;
	int32 change_tag = 1;
	BBufferConsumer::SetOutputBuffersFor(producer, mDestination, mBuffers, (void *)&user_data, &change_tag, true);

	return B_OK;
}

//---------------------------------------------------------------

void
VideoConsumer::Disconnect(bool final)
{
	fRoster->Disconnect(fProducerOut.node.node, fProducerOut.source, fConsumerIn.node.node, fConsumerIn.destination);
	if (final) {
		delete fPluginsHandler;
		fPluginsHandler = NULL;
	}
}

//---------------------------------------------------------------

void
VideoConsumer::Disconnected(
	const media_source & producer,
	const media_destination & where)
{
	FUNCTION(("VideoConsumer::Disconnected\n"));

	if (where == mIn.destination && producer == mIn.source) {
		// disconnect the connection
		mIn.source = media_source::null;
	}
}

//---------------------------------------------------------------

status_t
VideoConsumer::AcceptFormat(
	const media_destination & dest,
	media_format * format)
{
	FUNCTION(("VideoConsumer::AcceptFormat \n"));
	
	if (dest != mIn.destination)
	{
		ErrorAlert("VideoConsumer::AcceptFormat - BAD DESTINATION");
		return B_MEDIA_BAD_DESTINATION;	
	}
	
	if (format->type == B_MEDIA_NO_TYPE)
		format->type = B_MEDIA_RAW_VIDEO;
	
	if (format->type != B_MEDIA_RAW_VIDEO)
	{
		ErrorAlert("VideoConsumer::AcceptFormat - BAD FORMAT");
		return B_MEDIA_BAD_FORMAT;
	}
	
	if (format->u.raw_video.display.format != mBitmap[0]->ColorSpace())
	{
		PROGRESS(("Refused format: %d Overlay: %d\n", int(format->u.raw_video.display.format), int(mOverlayBitmap != 0)));
		return B_MEDIA_BAD_FORMAT;
	}
		
	char format_string[256];		
	string_for_format(*format, format_string, 256);
	FUNCTION(("VideoConsumer::AcceptFormat: %s\n", format_string));

	return B_OK;
}

//---------------------------------------------------------------

status_t
VideoConsumer::GetNextInput(
	int32 * cookie,
	media_input * out_input)
{
	FUNCTION(("VideoConsumer::GetNextInput\n"));

	// custom build a destination for this connection
	// put connection number in id

	if (*cookie < 1)
	{
		mIn.node = Node();
		mIn.destination.id = *cookie;
		sprintf(mIn.name, "Video Consumer");
		*out_input = mIn;
		(*cookie)++;
		return B_OK;
	}
	else
		return B_MEDIA_BAD_DESTINATION;
}

//---------------------------------------------------------------

void
VideoConsumer::DisposeInputCookie(int32 /*cookie*/)
{
}

//---------------------------------------------------------------

status_t
VideoConsumer::GetLatencyFor(
	const media_destination &for_whom,
	bigtime_t * out_latency,
	media_node_id * out_timesource)
{
	FUNCTION(("VideoConsumer::GetLatencyFor\n"));
	
	if (for_whom != mIn.destination)
		return B_MEDIA_BAD_DESTINATION;
	
	*out_latency = 10000;
	*out_timesource = TimeSource()->ID();
	return B_OK;
}

//---------------------------------------------------------------

status_t
VideoConsumer::FormatChanged(
				const media_source & producer,
				const media_destination & consumer, 
				int32 from_change_count,
				const media_format &format)
{
	FUNCTION(("VideoConsumer::FormatChanged\n"));
	
	if (consumer != mIn.destination)
		return B_MEDIA_BAD_DESTINATION;

	if (producer != mIn.source)
		return B_MEDIA_BAD_SOURCE;

	mIn.format = format;
	
	if (mBitmap[0] && format.u.raw_video.display.format == mBitmap[0]->ColorSpace())
		return B_OK;
	
	return B_MEDIA_BAD_FORMAT;
}
//---------------------------------------------------------------

void
VideoConsumer::HandleEvent(
	const media_timed_event *event,
	bigtime_t lateness,
	bool realTimeEvent)

{
	LOOP(("VideoConsumer::HandleEvent\n"));
	
	switch (event->type)
	{
		case BTimedEventQueue::B_START:
			PROGRESS(("VideoConsumer::HandleEvent - START\n"));
			mFrameCount = 0;
			mFrameDropped = false;
			break;
		case BTimedEventQueue::B_STOP:
			PROGRESS(("VideoConsumer::HandleEvent - STOP\n"));
			EventQueue()->FlushEvents(event->event_time, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
			break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
			LOOP(("VideoConsumer::HandleEvent - HANDLE BUFFER\n"));
			ProcessBuffer((BBuffer *) event->pointer);
			break;
		default:
			ErrorAlert("VideoConsumer::HandleEvent - BAD EVENT");
			break;
	}			
	LOOP(("VideoConsumer::HandleEvent - DONE\n"));
}

BBitmap *
VideoConsumer::GetFrame()
{
	FUNCTION(("VideoConsumer::GetFrame %d\n", int(mFrameRequested)));
	if (!mFrameRequested) {
		mFrameRequested = true;
		BBitmap * 	b;
		int32		code;
		//	make sure we don't deadlock: timeout if necessary!
		if (read_port_etc(mFrameRequestPort, &code, &b, sizeof(b), B_TIMEOUT, 1000000) >= 0)
			return b;
	}
	return NULL;
}

BMenu *
VideoConsumer::OptionsMenu()
{
	return fPluginsHandler ? fPluginsHandler->PluginsMenu() : NULL;
}
