/*

	Overlayer.h

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video plugin License for usage restrictions.

*/

#ifndef _OVERLAYER_H_
#define _OVERLAYER_H_

#include "VideoPlugin.h"

class OverlayerPluginType : public VideoPluginType
{
	public:
							OverlayerPluginType(VideoPluginsHost * host, image_id id);
		virtual				~OverlayerPluginType();

		virtual const char*	ID();
		virtual uint32		Flags();
		virtual uint32		Version();
		virtual const char*	Name();
		virtual const char*	Author();
		virtual int32		GetVerticesProperties(const vertice_properties ** ap);
		virtual VideoPluginEngine * InstanciateVideoPluginEngine();
	private:
};

class OverlayerEngine : public VideoPluginEngine
{
	public:
							OverlayerEngine(VideoPluginType * plugin);
		virtual				~OverlayerEngine();
		friend class OverlayerPluginType;

		virtual status_t	RestoreConfig(BMessage & config);
		virtual status_t	StoreConfig(BMessage & config);
		virtual BView*		ConfigureView();

		// UseFormat will be called before ApplyEffect is first called and when the image format changes.
		// No need to monitor the BBitmap format.
		virtual status_t	UseFormat(media_raw_video_format & format, int which_work_mode);
		virtual status_t	ApplyEffect(BBitmap * frame_in, BBitmap * frame_out, int64 frame_count, bool skipped_frames);

	private:
};

#endif // _OVERLAYER_H_
