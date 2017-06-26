/*

	DrawerPlugin.h

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video plugin License for usage restrictions.

*/

#ifndef _DRAWER_PLUGIN_H_
#define _DRAWER_PLUGIN_H_

#include "VideoPlugin.h"
#include "Benaphore.h"
#include <Font.h>
#include <String.h>

class DrawerPluginType : public VideoPluginType
{
	public:
							DrawerPluginType(VideoPluginsHost * host, image_id id);
		virtual				~DrawerPluginType();

		virtual const char*	ID();
		virtual uint32		Flags();
		virtual uint32		Version();
		virtual const char*	Name();
		virtual const char*	Author();
		virtual int32		GetVerticesProperties(const vertice_properties ** ap);
		virtual VideoPluginEngine * InstanciateVideoPluginEngine();
};

class DrawerEngine : public VideoPluginEngine
{
	public:
							DrawerEngine(VideoPluginType * plugin);
		virtual				~DrawerEngine();
		friend class DrawerPluginType;

		virtual status_t	RestoreConfig(BMessage & config);
		virtual status_t	StoreConfig(BMessage & config);
		virtual BView*		ConfigureView();
				void		SetText(const char * text);
				void		GetText(BString	& text);

		// UseFormat will be called before ApplyEffect is first called and when the image format changes.
		// No need to monitor the BBitmap format.
		virtual status_t	UseFormat(media_raw_video_format & format, int which_work_mode);
		virtual status_t	ApplyEffect(BBitmap * frame_in, BBitmap * frame_out, int64 frame_count, bool skipped_frames);

	private:
		BString				fText;
		BFont				fFont;
		Benaphore			fLock;
};

#endif // _DRAWER_PLUGIN_H_