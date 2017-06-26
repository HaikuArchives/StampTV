/*

	HorizontalFlip.cpp

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video plugin License for usage restrictions.

*/

#include "HorizontalFlip.h"

#include <Bitmap.h>
#include <stdio.h>

status_t GetVideoPlugins(TList<VideoPluginType> & plugin_list, VideoPluginsHost * host, image_id id)
{
	status_t compatible = host->APIVersionCompatible();
	if (compatible != B_OK)
		return compatible;

	plugin_list.AddItem(new HorizontalFlipPluginType(host, id));
	return B_OK;
}

HorizontalFlipPluginType::HorizontalFlipPluginType(VideoPluginsHost * host, image_id id) :
	VideoPluginType(host, id)
{
}

HorizontalFlipPluginType::~HorizontalFlipPluginType()
{
}

const char* HorizontalFlipPluginType::ID()
{
	return "Horizontal Flip";
}

uint32 HorizontalFlipPluginType::Flags()
{
	return 0;
}

uint32 HorizontalFlipPluginType::Version()
{
	return 001;
}

const char* HorizontalFlipPluginType::Name()
{
	return "Horizontal Flip";
}

const char* HorizontalFlipPluginType::Author()
{
	return "Geb";
}

static const vertice_properties support[] =
{
	{	B_RGB16,
		B_RGB16,
		1.0,
		VAPI_PROCESS_TO_DISTINCT | VAPI_PROCESS_IN_PLACE,
	},
};

int32 HorizontalFlipPluginType::GetVerticesProperties(const vertice_properties ** ap)
{
	*ap = support;
	return sizeof(support) / sizeof(vertice_properties);
}

VideoPluginEngine * HorizontalFlipPluginType::InstanciateVideoPluginEngine()
{
	return new HorizontalFlipEngine(this);
}

//-------------------------------------------------------------------------

HorizontalFlipEngine::HorizontalFlipEngine(VideoPluginType * plugin) : VideoPluginEngine(plugin)
{
}

HorizontalFlipEngine::~HorizontalFlipEngine()
{
}

status_t HorizontalFlipEngine::RestoreConfig(BMessage & config)
{
	return B_OK;
}

status_t HorizontalFlipEngine::StoreConfig(BMessage & config)
{
	return B_OK;
}

BView* HorizontalFlipEngine::ConfigureView()
{
	return NULL;
}

status_t HorizontalFlipEngine::UseFormat(media_raw_video_format & format, int which_work_mode)
{
	return B_OK;
}

status_t HorizontalFlipEngine::ApplyEffect(BBitmap * frame_in, BBitmap * frame_out, int64 frame_count, bool skipped_frames)
{
	BRect bounds = frame_in->Bounds();
	int		X = (int) bounds.right + 1;
	int		Y = (int) bounds.bottom + 1;
	int		W = frame_in->BytesPerRow();
	short * s = (short *) frame_in->Bits();
	short * d = (short *) frame_out->Bits();
	for (int y = 0; y < Y; y++)
	{
		short * sl = s;
		short * sr = s + X - 1;
		short * dl = d;
		short * dr = d + X - 1;
		while (sl < sr) {
			short v = *sl;
			*dl = *sr;
			*dr = v;	// in case in work in place!
			dl++;
			dr--;
			sl++;
			sr--;
		}
		s = (short *) ((char *) s + W);
		d = (short *) ((char *) d + W);
	}
	
	return B_OK;
}

