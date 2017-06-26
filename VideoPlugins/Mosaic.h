/*

	Mosaic.h

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video plugin License for usage restrictions.

*/

#ifndef _MOSAIC_H_
#define _MOSAIC_H_

#include "VideoPlugin.h"
#include <Message.h>
#include <Path.h>

class MosaicPluginType : public VideoPluginType
{
	public:
							MosaicPluginType(VideoPluginsHost * host, image_id id);
		virtual				~MosaicPluginType();

		virtual const char*	ID();
		virtual uint32		Flags();
		virtual uint32		Version();
		virtual const char*	Name();
		virtual const char*	Author();
		virtual int32		GetVerticesProperties(const vertice_properties ** ap);
		virtual VideoPluginEngine * InstanciateVideoPluginEngine();
	private:
};

class MosaicEngine : public VideoPluginEngine
{
	public:
							MosaicEngine(VideoPluginType * plugin);
		virtual				~MosaicEngine();
		friend class 		MosaicPluginType;

		virtual status_t	RestoreConfig(BMessage & config);
		void 				RestoreConfigWithDefault(BMessage & config, BMessage & defaults);
		virtual status_t	StoreConfig(BMessage & config);
		virtual BView*		ConfigureView();

		// UseFormat will be called before ApplyEffect is first called and when the image format changes.
		// No need to monitor the BBitmap format.
		virtual status_t	UseFormat(media_raw_video_format & format, int which_work_mode);
		virtual status_t	ApplyEffect(BBitmap * frame_in, BBitmap * frame_out, int64 frame_count, bool skipped_frames);

	private:
		void				UpdateMute();
		friend class ConfigView;
		BPath				mDefaultsPath;
		BMessage			mDefaults;
		BBitmap *			mMosaic;
		BView	*			mView;
		float				mWidth, mHeight;
		int64				mLastFrame;
		int					mPos;
		int32				mLastCount;
		int32				mCount;
		int32				mDelay;
		float				mFSize;
		bool				mDisplayTitle;
		bool				mThrough;
		int32				mScanType;
};

#endif // _MOSAIC_H_
