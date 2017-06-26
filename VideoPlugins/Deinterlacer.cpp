/*

	Deinterlacer.cpp

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video plugin License for usage restrictions.

*/

#include "Deinterlacer.h"

#include <Bitmap.h>
#include <stdio.h>

status_t GetVideoPlugins(TList<VideoPluginType> & plugin_list, VideoPluginsHost * host, image_id id)
{
	status_t compatible = host->APIVersionCompatible();
	if (compatible != B_OK)
		return compatible;

	plugin_list.AddItem(new DeinterlacerPluginType(host, id));
	return B_OK;
}

DeinterlacerPluginType::DeinterlacerPluginType(VideoPluginsHost * host, image_id id) :
	VideoPluginType(host, id)
{
}

DeinterlacerPluginType::~DeinterlacerPluginType()
{
}

const char* DeinterlacerPluginType::ID()
{
	return "Deinterlacer";
}

uint32 DeinterlacerPluginType::Flags()
{
	return 0;
}

uint32 DeinterlacerPluginType::Version()
{
	return 001;
}

const char* DeinterlacerPluginType::Name()
{
	return "Deinterlacer";
}

const char* DeinterlacerPluginType::Author()
{
	return "Geb";
}

static const vertice_properties support[] =
{
	{	ANY_COLOR_SPACE,
		ANY_COLOR_SPACE,
		1.0,
		VAPI_PROCESS_TO_DISTINCT | VAPI_PROCESS_TO_OVERLAY,
	},
};

int32 DeinterlacerPluginType::GetVerticesProperties(const vertice_properties ** ap)
{
	*ap = support;
	return sizeof(support) / sizeof(vertice_properties);
}

VideoPluginEngine * DeinterlacerPluginType::InstanciateVideoPluginEngine()
{
	return new DeinterlacerEngine(this);
}

//-------------------------------------------------------------------------

DeinterlacerEngine::DeinterlacerEngine(VideoPluginType * plugin) : VideoPluginEngine(plugin)
{
}

DeinterlacerEngine::~DeinterlacerEngine()
{
}

status_t DeinterlacerEngine::RestoreConfig(BMessage & config)
{
	return B_OK;
}

status_t DeinterlacerEngine::StoreConfig(BMessage & config)
{
	return B_OK;
}

BView* DeinterlacerEngine::ConfigureView()
{
	return NULL;
}

status_t DeinterlacerEngine::UseFormat(media_raw_video_format & format, int which_work_mode)
{
	fLines = format.display.line_count;
	return B_OK;
}

status_t DeinterlacerEngine::ApplyEffect(BBitmap * frame_in, BBitmap * frame_out, int64 frame_count, bool skipped_frames)
{
	if (frame_in == frame_out)
		return B_OK;	// nothing to do...
	if (fLines > 240) {
		char *	s = (char*) frame_in->Bits();
		char *	d = (char*) frame_out->Bits();
		int		sbpr = frame_in->BytesPerRow();
		int		dbpr = frame_out->BytesPerRow();
		int		bpr = sbpr > dbpr ? dbpr : sbpr;
		int		wpr	= bpr / 8;	// how many 8 bytes words to copy
		int		rest = bpr % 8;
		for (int f = 0; f < fLines; f +=2) {
			int64 *	S = (int64*) (s + sbpr * f);
			int64 *	D = (int64*) (d + dbpr * f);
			int64 *	DD = (int64*) (((char*)D) + dbpr);
			int l = wpr;
			while (l >= 16) {
				*D = *S; D[1] = S[1]; D[2] = S[2]; D[3] = S[3]; D[4] = S[4]; D[5] = S[5]; D[6] = S[6]; D[7] = S[7];
				D[8] = S[8]; D[9] = S[9]; D[10] = S[10]; D[11] = S[11]; D[12] = S[12]; D[13] = S[13]; D[14] = S[14]; D[15] = S[15];
				*DD = *S; DD[1] = S[1]; DD[2] = S[2]; DD[3] = S[3]; DD[4] = S[4]; DD[5] = S[5]; DD[6] = S[6]; DD[7] = S[7];
				DD[8] = S[8]; DD[9] = S[9]; DD[10] = S[10]; DD[11] = S[11]; DD[12] = S[12]; DD[13] = S[13]; DD[14] = S[14]; DD[15] = S[15];
				S += 16; D += 16; DD +=16; l -= 16;
			}
			while (l >= 4) {
				*D = *S; D[1] = S[1]; D[2] = S[2]; D[3] = S[3];
				*DD = *S; DD[1] = S[1]; DD[2] = S[2]; DD[3] = S[3];
				S += 4; D += 4; DD +=4; l -= 4;
			}
			while (l >= 1) {
				int64	v = *S; *D = v; *DD = v;
				S++; D++; DD++; l--;
			}
			if (rest > 0) {
				int r = rest;
				int32 * s4 = (int32*) S;
				int32 * d4 = (int32*) D;
				int32 * dd4 = (int32*) DD;
				if (r >= 4) {
					*d4++ = *s4;
					*dd4++ = *s4++;
					r -= 4;
				}
				if (r >= 2) {
					int16 * s2 = (int16*) s4;
					int16 * d2 = (int16*) d4;
					int16 * dd2 = (int16*) dd4;
					*d2 = *s2;
					*dd2 = *s2;
				}
			}
		}
	} else {
		// Nothing needed for less than 240 lines. So just do a "normal" write in the target image
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
	}
	return B_OK;
}
