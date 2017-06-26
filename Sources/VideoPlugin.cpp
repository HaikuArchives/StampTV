/*

	VideoPlugin.cpp

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video Plugin License for usage restrictions.

	Generic part of the video plugins.
	
	Define the minimal interface the plugins Type/Engines should implement.
	
	Also defines the minimal interface for the VideoPluginsHost class:
	ie, VideoPluginsHost shows the services that hosts *must* always implement
	and that any video plugin may always use.

*/

#include "VideoPlugin.h"

#include <Bitmap.h>
#include <Window.h>
#include <String.h>
#include <Alert.h>

//-------------------------------------------------------------------------

VideoPluginsHost::VideoPluginsHost()
{
}

VideoPluginsHost::~VideoPluginsHost()
{
}

uint32 VideoPluginsHost::APIVersion()
{
	return VAPI_VERSION;
}

uint32 VideoPluginsHost::APISubVersion()
{
	return VAPI_SUB_VERSION;
}

bool VideoPluginsHost::OverlayCompatible(color_space cspace)
{
	bool	compatible = false;
	BBitmap * bm = new BBitmap(BRect(0, 0, 15, 15), B_BITMAP_WILL_OVERLAY | B_BITMAP_RESERVE_OVERLAY_CHANNEL, cspace);
	if (bm && bm->InitCheck() == B_OK && bm->IsValid())
		compatible = true;
	delete bm;
	return compatible;
}

status_t VideoPluginsHost::GetChannel(int32 & channel)
{
	return VAPI_NOT_SUPPORTED;
}

status_t VideoPluginsHost::SwitchToChannel(int32 channel)
{
	return VAPI_NOT_SUPPORTED;
}

status_t VideoPluginsHost::GetPresetCount(int32 & count)
{
	return VAPI_NOT_SUPPORTED;
}

status_t VideoPluginsHost::GetPreset(int32 & slot, BString & name, int32 & channel)
{
	return VAPI_NOT_SUPPORTED;
}

status_t VideoPluginsHost::GetNthPreset(int32 slot, BString & name, int32 & channel)
{
	return VAPI_NOT_SUPPORTED;
}

status_t VideoPluginsHost::SwitchToPreset(BString & name)
{
	return VAPI_NOT_SUPPORTED;
}

status_t VideoPluginsHost::SwitchToPreset(int32 slot)
{
	return VAPI_NOT_SUPPORTED;
}

status_t VideoPluginsHost::SwitchToNext(int how_many, bool preset, int32 & index)
{
	return VAPI_NOT_SUPPORTED;
}

status_t VideoPluginsHost::Perform(perform_code d, void *arg)
{
	return VAPI_NOT_SUPPORTED;
}

status_t VideoPluginsHost::SetWindowTitle(const char* name = NULL)
{
	return VAPI_NOT_SUPPORTED;
}

status_t VideoPluginsHost::OpenConfigWindow(VideoPluginEngine * engine)
{
	return VAPI_NOT_SUPPORTED;
}

status_t VideoPluginsHost::SetMuteAudio(bool mute)
{
	return VAPI_NOT_SUPPORTED;
}

void VideoPluginsHost::_reserved_host_1() {}
void VideoPluginsHost::_reserved_host_2() {}
void VideoPluginsHost::_reserved_host_3() {}
void VideoPluginsHost::_reserved_host_4() {}
void VideoPluginsHost::_reserved_host_5() {}
void VideoPluginsHost::_reserved_host_6() {}
void VideoPluginsHost::_reserved_host_7() {}
void VideoPluginsHost::_reserved_host_8() {}

//-------------------------------------------------------------------------

VideoPluginType::VideoPluginType(VideoPluginsHost * host, image_id id) :
	fHost(host), fImageID(id)
{
}

VideoPluginType::~VideoPluginType()
{
}

const char* VideoPluginType::AboutString()
{
	if (fAboutString.Length() == 0) {
		fAboutString << Name() << "\nby " << Author();
	}
	return fAboutString.String();
}

status_t VideoPluginType::About()
{
	BAlert	* alert = new BAlert(Name(), AboutString(), "OK");
	alert->Go(NULL);
	return B_OK;
}

status_t VideoPluginType::Perform(perform_code d, void *arg)
{
	return VAPI_NOT_SUPPORTED;
}

void VideoPluginType::_reserved_engine_1() {}
void VideoPluginType::_reserved_engine_2() {}
void VideoPluginType::_reserved_engine_3() {}
void VideoPluginType::_reserved_engine_4() {}
void VideoPluginType::_reserved_engine_5() {}
void VideoPluginType::_reserved_engine_6() {}
void VideoPluginType::_reserved_engine_7() {}
void VideoPluginType::_reserved_engine_8() {}

//-------------------------------------------------------------------------

VideoPluginEngine::VideoPluginEngine(VideoPluginType * plugin) :
	fPlugin(plugin), fConfigWindow(NULL), fConfigWindowRect(-1, -1, -10, -10)
{
}

VideoPluginEngine::~VideoPluginEngine()
{
}

void VideoPluginEngine::Close()
{
	// if you put this in the destructor, then if the configuration view
	// tries to communicate with the engine to save some information,
	// it will use an object that is already mostly destroyed (and will crash...).
	if (fConfigWindow->Lock())
		fConfigWindow->Quit();
}

status_t VideoPluginEngine::RestoreConfig(BMessage & config)
{
	return B_OK;
}

status_t VideoPluginEngine::StoreConfig(BMessage & config)
{
	return B_OK;
}

BView* VideoPluginEngine::ConfigureView()
{
	return NULL;
}

status_t VideoPluginEngine::UseFormat(media_raw_video_format & format, int which_work_mode)
{
	return VAPI_NOT_SUPPORTED;
}

uint32 VideoPluginEngine::Flags()
{
	return fFlags;
}

void VideoPluginEngine::SetFlags(uint32 flags)
{
	fFlags = flags;
}

BBitmap * VideoPluginEngine::Output()
{
	return fOutput;
}

void VideoPluginEngine::SetOutput(BBitmap * output)
{
	fOutput = output;
}

BWindow * VideoPluginEngine::ConfigWindow()
{
	return fConfigWindow;
}

void VideoPluginEngine::SetConfigWindow(BWindow * window)
{
	fConfigWindow = window;
}

BRect VideoPluginEngine::ConfigWindowFrame()
{
	return fConfigWindowRect;
}

void VideoPluginEngine::SetConfigWindowFrame(BRect frame)
{
	fConfigWindowRect = frame;
}

bool VideoPluginEngine::WritingInOverlay()
{
	return fWritingInOverlay;
}

void VideoPluginEngine::SetWritingInOverlay(bool writingInOverlay)
{
	fWritingInOverlay = writingInOverlay;
}

bool VideoPluginEngine::ReadingFromOverlay()
{
	return fReadingFromOverlay;
}

void VideoPluginEngine::SetReadingFromOverlay(bool readingFromOverlay)
{
	fReadingFromOverlay = readingFromOverlay;
}

status_t VideoPluginEngine::Perform(perform_code d, void *arg)
{
	return VAPI_NOT_SUPPORTED;
}

void VideoPluginEngine::_reserved_engine_1() {}
void VideoPluginEngine::_reserved_engine_2() {}
void VideoPluginEngine::_reserved_engine_3() {}
void VideoPluginEngine::_reserved_engine_4() {}
void VideoPluginEngine::_reserved_engine_5() {}
void VideoPluginEngine::_reserved_engine_6() {}
void VideoPluginEngine::_reserved_engine_7() {}
void VideoPluginEngine::_reserved_engine_8() {}

