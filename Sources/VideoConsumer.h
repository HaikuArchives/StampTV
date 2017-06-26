/*

	VideoConsumer.h

	© 1991-1999, Be Incorporated, All rights reserved, was Be's CodyCam Sample Code
	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#ifndef _VIDEO_CONSUMER_H_
#define _VIDEO_CONSUMER_H_

#include <BufferConsumer.h>
#include <MediaEventLooper.h>

class stampView;
class stampPluginsHandler;

#define kMaxBufferCount 4

extern const int kBufferCount;
extern const int kBufferCountOverlay;

class VideoConsumer : 
	public BMediaEventLooper,
	public BBufferConsumer
{
public:
						VideoConsumer( const char * name, stampView * stampView,
							bool quit_app, BMediaRoster * roster);
						~VideoConsumer();
	
/*	BMediaNode */
public:
	
	virtual	BMediaAddOn	*AddOn(long *cookie) const;
	
protected:

	virtual void		NodeRegistered();
							
	virtual	status_t 	HandleMessage(
							int32 message,
							const void * data,
							size_t size);

/*  BMediaEventLooper */
protected:
	virtual void		HandleEvent(
							const media_timed_event *event,
							bigtime_t lateness,
							bool realTimeEvent);
/*	BBufferConsumer */
public:
	
	virtual	status_t	AcceptFormat(
							const media_destination &dest,
							media_format * format);
	virtual	status_t	GetNextInput(
							int32 * cookie,
							media_input * out_input);
							
	virtual	void		DisposeInputCookie(
							int32 cookie);
protected:

	virtual	void		BufferReceived(BBuffer * buffer);
			void		ProcessBuffer(BBuffer * buffer);
	
private:

	virtual	void		ProducerDataStatus(
							const media_destination &for_whom,
							int32 status,
							bigtime_t at_media_time);									
	virtual	status_t	GetLatencyFor(
							const media_destination &for_whom,
							bigtime_t * out_latency,
							media_node_id * out_id);	
	virtual	status_t	Connected(
							const media_source &producer,
							const media_destination &where,
							const media_format & with_format,
							media_input * out_input);							
	virtual	void		Disconnected(
							const media_source &producer,
							const media_destination &where);							
	virtual	status_t	FormatChanged(
							const media_source & producer,
							const media_destination & consumer, 
							int32 from_change_count,
							const media_format & format);
							
/*	implementation */

public:
			BMenu *		OptionsMenu();
			BBitmap *	GetFrame();	// will return a 'new BBitmap'
			bool		OverlayActive()	{ return mOverlayBitmap != NULL; }

/*	Connection */
	
			void		InitColorSpaceSupport();
			bool		OverlayCompatible(color_space cspace);
			bool		CheckOverlayRestrictions(int vWidth, int vHeight, float wWidth, float wHeight);
			bool		OverlayWouldNotScale()	{ return mOverlayWouldNotScale; }
			status_t	CreateBuffers(media_format & format, color_space overlay_space,
							bool draw_in_buffer, bool createOverlay, bool overlayBuffer);
			void		DeleteBuffers();
			status_t	SetupVideoNode(media_node & video_node);
			status_t	Connect(color_space current, int vWidth, int vHeight, float wWidth, float wHeight);
			void		Disconnect(bool final = false);
private:
			status_t	TryConnect(color_space buffer_space, color_space overlay_space,
							int width, int height, bool draw_in_buffer,
							bool createOverlay = false, bool overlayBuffer = false);
		
private:
	stampView			*mStamp;
	bool				mQuitApp;

	BMediaRoster		*fRoster;
	media_input			mIn;
	media_destination	mDestination;
	media_output		fProducerOut;
	media_input			fConsumerIn;
	BBitmap				*mBitmap[kMaxBufferCount];
	BBufferGroup		*mBuffers;
	media_buffer_id		mBufferMap[kMaxBufferCount];
	int					mBufferCount;
	BBitmap				*mOverlayBitmap;
	BList				mOverlaySpaces;
	overlay_restrictions	mRestrictions;
	bool				mOverlayWouldNotScale;
	stampPluginsHandler	*fPluginsHandler;
	int					mLastFrame;
	int64				mFrameCount;
	bool				mFrameDropped;
	bool				mFrameRequested;
	port_id				mFrameRequestPort;
};

#endif // _VIDEO_CONSUMER_H_