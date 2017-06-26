/*

	Overlayer.cpp

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video plugin License for usage restrictions.

*/

#include "Overlayer.h"

#include <Bitmap.h>
#include <stdio.h>

status_t GetVideoPlugins(TList<VideoPluginType> & plugin_list, VideoPluginsHost * host, image_id id)
{
	status_t compatible = host->APIVersionCompatible();
	if (compatible != B_OK)
		return compatible;

	plugin_list.AddItem(new OverlayerPluginType(host, id));
	return B_OK;
}

OverlayerPluginType::OverlayerPluginType(VideoPluginsHost * host, image_id id) :
	VideoPluginType(host, id)
{
}

OverlayerPluginType::~OverlayerPluginType()
{
}

const char* OverlayerPluginType::ID()
{
	return "Overlayer";
}

uint32 OverlayerPluginType::Flags()
{
	return 0;
}

uint32 OverlayerPluginType::Version()
{
	return 001;
}

const char* OverlayerPluginType::Name()
{
	return "Overlayer";
}

const char* OverlayerPluginType::Author()
{
	return "Geb";
}

static const vertice_properties support[] =
{
	{	ANY_COLOR_SPACE,
		ANY_COLOR_SPACE,
		1.0,
		VAPI_PROCESS_IN_PLACE | VAPI_PROCESS_TO_OVERLAY | VAPI_PROCESS_OVERLAY_IN_PLACE,
	},
};

int32 OverlayerPluginType::GetVerticesProperties(const vertice_properties ** ap)
{
	*ap = support;
	return sizeof(support) / sizeof(vertice_properties);
}

VideoPluginEngine * OverlayerPluginType::InstanciateVideoPluginEngine()
{
	return new OverlayerEngine(this);
}

//-------------------------------------------------------------------------

OverlayerEngine::OverlayerEngine(VideoPluginType * plugin) : VideoPluginEngine(plugin)
{
}

OverlayerEngine::~OverlayerEngine()
{
}

status_t OverlayerEngine::RestoreConfig(BMessage & config)
{
	return B_OK;
}

status_t OverlayerEngine::StoreConfig(BMessage & config)
{
	return B_OK;
}

BView* OverlayerEngine::ConfigureView()
{
	return NULL;
}

status_t OverlayerEngine::UseFormat(media_raw_video_format & format, int which_work_mode)
{
	return B_OK;
}

status_t OverlayerEngine::ApplyEffect(BBitmap * frame_in, BBitmap * frame_out, int64 frame_count, bool skipped_frames)
{
	if (frame_in == frame_out)
		return B_OK;	// nothing to do...
	// DO NOT USE memcpy!!! IT  *H*O*R*R*R*R*I*B*L*Y*  SLOW WITH OVERLAYS!!!
	int64 * S = (int64*) frame_in->Bits();
	int64 * D = (int64*) frame_out->Bits();
	int l = frame_in->BitsLength();
	while (l >= 16 * 8) {
		*D = *S; D[1] = S[1]; D[2] = S[2]; D[3] = S[3]; D[4] = S[4]; D[5] = S[5]; D[6] = S[6]; D[7] = S[7];
		D[8] = S[8]; D[9] = S[9]; D[10] = S[10]; D[11] = S[11]; D[12] = S[12]; D[13] = S[13]; D[14] = S[14]; D[15] = S[15];
		S += 16; D += 16; l -= 16 * 8;
	}
	while (l >= 4 * 8) {
		*D = *S; D[1] = S[1]; D[2] = S[2]; D[3] = S[3];
		S += 4; D += 4; l -= 4 * 8;
	}
	while (l >= 8) {
		*D = *S;
		S += 1; D += 1; l -= 8;
	}
	if (l > 0) {
		int32 * s4 = (int32*) S;
		int32 * d4 = (int32*) D;
		if (l >= 4) {
			*d4++ = *s4++;
			l -= 4;
		}
		if (l >= 2) {
			int16 * s2 = (int16*) s4;
			int16 * d2 = (int16*) d4;
			*d2 = *s2;
		}
	}
	return B_OK;
}
