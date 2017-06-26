/*

	PassFilter.h

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video plugin License for usage restrictions.

*/

#ifndef _PASS_FILTER_H_
#define _PASS_FILTER_H_

#include "VideoPlugin.h"

class PassFilterPluginType : public VideoPluginType
{
	public:
							PassFilterPluginType(VideoPluginsHost * host, image_id id);
		virtual				~PassFilterPluginType();

		virtual const char*	ID();
		virtual uint32		Flags();
		virtual uint32		Version();
		virtual const char*	Name();
		virtual const char*	Author();
		virtual int32		GetVerticesProperties(const vertice_properties ** ap);
		virtual VideoPluginEngine * InstanciateVideoPluginEngine();
};

class PassFilterEngine : public VideoPluginEngine
{
	public:
							PassFilterEngine(VideoPluginType * plugin);
		virtual				~PassFilterEngine();
		friend class PassFilterPluginType;

		virtual status_t	RestoreConfig(BMessage & config);
		virtual status_t	StoreConfig(BMessage & config);
		virtual BView*		ConfigureView();

		// UseFormat will be called before ApplyEffect is first called and when the image format changes.
		// No need to monitor the BBitmap format.
		virtual status_t	UseFormat(media_raw_video_format & format, int which_work_mode);
		virtual status_t	ApplyEffect(BBitmap * frame_in, BBitmap * frame_out, int64 frame_count, bool skipped_frames);

	private:
		int					fWidth, fHeight;
		bool				f32, f16;
};

#endif // _PASS_FILTER_H_