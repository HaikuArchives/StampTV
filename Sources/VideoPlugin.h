/*

	VideoPlugin.h

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video Plugin License for usage restrictions.

	C++ Video plugin API for stampTV (and others?).

	Simply export the function:

		extern "C" status_t _EXPORT GetVideoPlugins(BList *plugin_list, const VideoHost * host);

	The function should fill plugin_list with pointers to VideoPluginType objects.
	Thus, one binary can bundle more than one single effect (say a Blur & one Sharpen effect).
	The VideoHost object is *not* owned by the plugin (don't delete it!)
	but may be cached throughout the live of the plugin to query the host app.

*/

#ifndef _VIDEO_PLUGIN_H_
#define _VIDEO_PLUGIN_H_

#include "TemplateList.h"

#include <MediaDefs.h>
#include <image.h>
#include <Rect.h>
#include <Errors.h>
#include <String.h>

//	Error codes
enum {
	VAPI_HOST_TOO_OLD = B_ERRORS_END + 1970,	// try to avoid conflict with programmer's error codes...
	VAPI_NOT_SUPPORTED,							// function not supported (by the this host for instance).
	VAPI_NOT_IMPLEMENTED = VAPI_NOT_SUPPORTED,	// Same thing as VAPI_NOT_SUPPORTED
	VAPI_HOST_TOO_NEW,							// host requires newer version of plugin
};

//	flags for a plugin type
enum {
	// if the plugin has a configuration view
	VAPI_CONFIGURABLE			= 1,
};

//	flags for an vertice property
enum {
	// Can work from one buffer to a distinct one (not overlay)
	VAPI_PROCESS_TO_DISTINCT	= 1,
	// frame_in & frame_out may point to the same bitmap (not overlay, implies in_cspace == out_cspace)
	VAPI_PROCESS_IN_PLACE 		= 1 << 1,
	// may the ouput bitmap be an overlay? This might create a performance issue (not "in place")
	VAPI_PROCESS_TO_OVERLAY 	= 1 << 2,
	// do we accept to process overlays in place?
	VAPI_PROCESS_OVERLAY_IN_PLACE = 1 << 3,
	// will attach a view to the bitmap to draw. Requires VAPI_PROCESS_IN_PLACE
	// This flag *must* be honored by the host.
	VAPI_BVIEW_DRAWING			= (1 << 4) | VAPI_PROCESS_IN_PLACE,
};

// 	Improve readability of the code...
#define ANY_COLOR_SPACE B_NO_COLOR_SPACE

typedef struct {
	color_space		in_cspace;
	color_space		out_cspace;
	float			quality;		// a value between 0+ & 1.0, 1.0 means no quality loss.
	uint32			flags;			// not yet defined flags should be cleared
	int32			future[4];
} vertice_properties;

// IMPORTANT!!! You may *NOT* use ANY_COLOR_SPACE for only input or output!!!
// If you use ANY_COLOR_SPACE, then it must be for *both* input AND output!!!
// and input color_space will *always* be *equal* to the output color_space!
// Therefore you *can't* use ANY_COLOR_SPACE to create a converter from any
// color space to a specific one (or reverse): You *must* enumerate them all!

inline bool supports_each(uint32 flags, uint32 test)
{
	return (flags & test) == test;
}

inline bool supports_one(uint32 flags, uint32 test)
{
	return (flags & test) != 0;
}

class BBitmap;
class BMessage;
class VideoPluginEngine;
class VideoPluginsHost;

class VideoPluginType
{
	// One object will be created by plugin type, that is one for a blur effect,
	// one for a sharpen effect, etc. It allows the application to list what effects are available.
	// This object should be as light as possible, and allows to instanciate
	// VideoPluginEngine objects which do the "real" processing work.
	public:
							VideoPluginType(VideoPluginsHost * host, image_id id);
		virtual				~VideoPluginType();

		// Those may be called anytime!
		virtual const char*	ID() = 0;
		virtual uint32		Flags() = 0;
		virtual uint32		Version() = 0;
		virtual const char*	Name() = 0;
		virtual const char*	Author() = 0;
		virtual const char*	AboutString();
		virtual status_t	About();
		// to tell which color spaces the plugin supports
		// Use quality to tell how good the support is: 1.0 perfect down to 0.0 (next to not supported!)
		// Sort them in preference order, best first!
		// Return a pointer to a *static*array*.
		// If this list is not hardcoded, then it *must* be calculated *once*
		// when it is *first* called! May *never* change!!!
		virtual int32		GetVerticesProperties(const vertice_properties ** ap) = 0;
		virtual VideoPluginEngine * InstanciateVideoPluginEngine() = 0;
		
		// FBC expansion possibilities...
		virtual status_t	Perform(perform_code d, void *arg);
		virtual void		_reserved_engine_1();
		virtual void		_reserved_engine_2();
		virtual void		_reserved_engine_3();
		virtual void		_reserved_engine_4();
		virtual void		_reserved_engine_5();
		virtual void		_reserved_engine_6();
		virtual void		_reserved_engine_7();
		virtual void		_reserved_engine_8();
		int32				_reserved[16];

		// General
		VideoPluginsHost *	Host() 		{ return fHost; }
		image_id			ImageID() 	{ return fImageID; }	// For resources

	private:
		VideoPluginsHost * 	fHost;
		image_id			fImageID;
		BString				fAboutString;
};

class VideoPluginEngine
{
	public:
							VideoPluginEngine(VideoPluginType * plugin);
		virtual				~VideoPluginEngine();
		// always Close() an engine before you delete it. Can't be done at destruction
		// because of virtual destruction problems
		virtual void		Close();

				//	saves the current configuration in a BMessage
		virtual status_t	StoreConfig(BMessage & config);
				//	restores a plugin configuration
		virtual status_t	RestoreConfig(BMessage & config);
		virtual BView*		ConfigureView();

		// UseFormat will be called before ApplyEffect is first called and when the image format changes.
		// No need to monitor the BBitmap format.
		virtual status_t	UseFormat(media_raw_video_format & format, int which_work_mode);
		
		// Where the "real" image processing is done.
		virtual status_t	ApplyEffect(BBitmap * frame_in, BBitmap * frame_out, int64 frame_count, bool skipped_frames) = 0;

		// You should not change those methods, nor use the "Set..." versions, unless you know what you do!
				uint32		Flags();						// Flags of the active path. Set before UseFormat
				void		SetFlags(uint32 flags);
				BBitmap *	Output();						// Pointer to the intermediate bitmap, if any
				void		SetOutput(BBitmap * output);	// (you shouldn't use those)
				BWindow *	ConfigWindow();
				void		SetConfigWindow(BWindow * window);
				BRect		ConfigWindowFrame();
				void		SetConfigWindowFrame(BRect frame);
				bool		WritingInOverlay();
				void		SetWritingInOverlay(bool writingInOverlay);
				bool		ReadingFromOverlay();
				void		SetReadingFromOverlay(bool readingFromOverlay);

		// FBC expansion possibilities...
		virtual status_t	Perform(perform_code d, void *arg);
		virtual void		_reserved_engine_1();
		virtual void		_reserved_engine_2();
		virtual void		_reserved_engine_3();
		virtual void		_reserved_engine_4();
		virtual void		_reserved_engine_5();
		virtual void		_reserved_engine_6();
		virtual void		_reserved_engine_7();
		virtual void		_reserved_engine_8();
		int32				_reserved[16];		

		// General
		VideoPluginType *	Plugin() 	{ return fPlugin; }
		VideoPluginsHost *	Host()		{ return Plugin()->Host(); }

	private:
		VideoPluginType	* 	fPlugin;
		BBitmap *			fOutput;	// For PluginsHandler needs!
		BWindow *			fConfigWindow;
		BRect				fConfigWindowRect;
		uint32				fFlags;		// current path flags
		bool				fWritingInOverlay;
		bool				fReadingFromOverlay;
};

class VideoPluginsHost
{
	public:
							VideoPluginsHost();
		virtual				~VideoPluginsHost();

		enum {
			// This level will tell which layout format is the host assuming
			VAPI_VERSION = 002,
			// This tells which minor binary compatible revision is in used
			VAPI_SUB_VERSION = 002
		};

		//	What version of the Video Plugins API
		virtual	uint32		APIVersion();
		//	Additions to layout are ok (BFC compatible)
		virtual	uint32		APISubVersion();
		
		//	Call this from GetVideoPlugins() to check if the host is compatible!
		inline	status_t	APIVersionCompatible();

		//	Name of the Host program (stampTV?...)
		virtual	const char* Name() = 0;
		//	What version of the host program
		virtual uint32		Version() = 0;
		//	What color modes are supported for overlays (default implementation with no memory)
		virtual bool		OverlayCompatible(color_space cspace);
		//	Channel handling. Empty default implementations.
			//	what is the current channel?
		virtual status_t	GetChannel(int32 & channel);
			//	switch to a specific channel
		virtual status_t	SwitchToChannel(int32 channel);
			//	how many presets are there? (maximal number of presets)
		virtual status_t	GetPresetCount(int32 & count);
			//	get info on the current channel (B_ERROR if current channel isn't a preset)
		virtual status_t	GetPreset(int32 & slot, BString & name, int32 & channel);
			//	get the value of a specific preset (B_ERROR if the slot is empty)
		virtual status_t	GetNthPreset(int32 slot, BString & name, int32 & channel);
			//	switch to a specific preset by name (B_ERROR if preset isn't found)
		virtual status_t	SwitchToPreset(BString & name);
			//	switch to a specific preset by slot number (B_ERROR if the slot is empty)
		virtual status_t	SwitchToPreset(int32 slot);
			//	switch to the 'how_many' next preset or channel. (may be negative for previous)
			//	Fills 'slot' with the index of the new slot for preset, of the new channel otherwise.
		virtual status_t	SwitchToNext(int how_many, bool preset, int32 & index);
			//	Set the window's title until it is set again for some reason.
			//	Default value restores default value.
		virtual status_t	SetWindowTitle(const char* name = NULL);
			//	Opens a config window for this engine
		virtual status_t	OpenConfigWindow(VideoPluginEngine * engine);
			//	Controls audio Mute
		virtual status_t	SetMuteAudio(bool mute);
		
		// FBC expansion possibilities...
		virtual status_t	Perform(perform_code d, void *arg);
		virtual void		_reserved_host_1();
		virtual void		_reserved_host_2();
		virtual void		_reserved_host_3();
		virtual void		_reserved_host_4();
		virtual void		_reserved_host_5();
		virtual void		_reserved_host_6();
		virtual void		_reserved_host_7();
		virtual void		_reserved_host_8();
		int32				_reserved[16];
};

inline status_t VideoPluginsHost::APIVersionCompatible()
{
	// Test that the layouts are compatible (what the believes must match the host)
	uint32	host_version = APIVersion();
	if (host_version < VAPI_VERSION)
		return VAPI_HOST_TOO_OLD;
	else if (host_version > VAPI_VERSION)
		return VAPI_HOST_TOO_NEW;
	// Host must provide all the services that the plugin requires
	if (VAPI_SUB_VERSION > APISubVersion())
		return VAPI_HOST_TOO_OLD;
	return B_OK;
}

extern "C" status_t _EXPORT GetVideoPlugins(TList<VideoPluginType> & plugin_list, VideoPluginsHost * host, image_id id);

#endif // _VIDEO_PLUGIN_H_
