/*

	PassFilter.cpp

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video plugin License for usage restrictions.

*/

#include "PassFilter.h"

#include <Bitmap.h>
#include <stdio.h>

status_t GetVideoPlugins(TList<VideoPluginType> & plugin_list, VideoPluginsHost * host, image_id id)
{
	status_t compatible = host->APIVersionCompatible();
	if (compatible != B_OK)
		return compatible;

	plugin_list.AddItem(new PassFilterPluginType(host, id));
	return B_OK;
}

PassFilterPluginType::PassFilterPluginType(VideoPluginsHost * host, image_id id) :
	VideoPluginType(host, id)
{
}

PassFilterPluginType::~PassFilterPluginType()
{
}

const char* PassFilterPluginType::ID()
{
	return "Pass Filter";
}

uint32 PassFilterPluginType::Flags()
{
	return 0;
}

uint32 PassFilterPluginType::Version()
{
	return 001;
}

const char* PassFilterPluginType::Name()
{
	return "Pass Filter";
}

const char* PassFilterPluginType::Author()
{
	return "Geb";
}

static const vertice_properties support[] =
{
//	{	B_RGB32,
//		B_RGB32,
//		1.0,
//		VAPI_PROCESS_TO_DISTINCT,
//	},
	{	B_RGB16,
		B_RGB16,
		0.9,
		VAPI_PROCESS_TO_DISTINCT,
	},
//	{	B_RGB32,
//		B_RGB16,
//		1.0,
//		VAPI_PROCESS_TO_DISTINCT,
//	},
};

int32 PassFilterPluginType::GetVerticesProperties(const vertice_properties ** ap)
{
	*ap = support;
	return sizeof(support) / sizeof(vertice_properties);
}

VideoPluginEngine * PassFilterPluginType::InstanciateVideoPluginEngine()
{
	return new PassFilterEngine(this);
}

//-------------------------------------------------------------------------

PassFilterEngine::PassFilterEngine(VideoPluginType * plugin) : VideoPluginEngine(plugin)
{
}

PassFilterEngine::~PassFilterEngine()
{
}

status_t PassFilterEngine::RestoreConfig(BMessage & config)
{
	return B_OK;
}

status_t PassFilterEngine::StoreConfig(BMessage & config)
{
	return B_OK;
}

BView* PassFilterEngine::ConfigureView()
{
	return NULL;
}

status_t PassFilterEngine::UseFormat(media_raw_video_format & format, int which_work_mode)
{
	fWidth = format.display.line_width;
	fHeight = format.display.line_count;
	f32 = support[which_work_mode].in_cspace == B_RGB32 && support[which_work_mode].out_cspace == B_RGB32;
	f16 = support[which_work_mode].in_cspace == B_RGB16 && support[which_work_mode].out_cspace == B_RGB16;
	return B_OK;
}


inline int rgb32to16(long s)
{
	return ((s & 0xf8) >> 3) | ((s & 0xfc00) >> 5) | ((s & 0xf80000) >> 8);
}

inline long rgb16(int r, int g, int b)
{
	return ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | ((b & 0xf8) >> 3);
}

inline int rgb32(uchar r, uchar g, uchar b)
{
	return (r << 16) | (g << 8) | b;
}

inline int redFromRGB16(int s)
{
	return (s >> 8) & 0xf8;
}

inline int greenFromRGB16(int s)
{
	return (s >> 3) & 0xfc;
}

inline int blueFromRGB16(int s)
{
	return (s & 0x1f) << 3;
}

inline int redFromRGB32(long s)
{
	return (s >> 16) & 0xff;
}

inline int greenFromRGB32(long s)
{
	return (s >> 8) & 0xff;
}

inline int blueFromRGB32(long s)
{
	return (s) & 0xff;
}

inline int dist(int a, int b)
{
//	int d = (a - b);
//	d = ((d < 0 ? -d : d)) >> 2;
//	return d * d;
	return (a - b) & 0x80;
}

status_t PassFilterEngine::ApplyEffect(BBitmap * frame_in, BBitmap * frame_out, int64 frame_count, bool skipped_frames)
{
//	memcpy(frame_out->Bits(), frame_in->Bits(), frame_in->BitsLength());
	if (f32) {
		int		W = frame_in->BytesPerRow();
		char *	s = (char *) frame_in->Bits();
		char *	d = (char *) frame_out->Bits();
		int		r = 128, g = 128, b = 128;
		for (int y = 1; y < fHeight - 1; y++)
		{
			int x = fWidth - 1;
			long * S = (long *) (s + W * y) + 1;
			long * D = (long *) (d + W * y) + 1;
			while (x-- > 0) {
				int	v = *(S++);
				int R, G, B;
				R = redFromRGB32(v);
				G = greenFromRGB32(v);
				B = blueFromRGB32(v);
				short t = dist(r, R) + dist(g, G) + dist(b, B);
				if (t > 255)
					t = 255;
				*(D++) = rgb32(t, t, t);
				r = R; g = G; b = B;
			}
		}
	} else if (f16) {
		int		W = frame_in->BytesPerRow();
		char *	s = (char *) frame_in->Bits();
		char *	d = (char *) frame_out->Bits();
		int		r = 128, g = 128, b = 128;
		for (int y = 1; y < fHeight - 1; y++)
		{
			int x = fWidth - 1;
			short * S = (short *) (s + W * y) + 1;
			short * D = (short *) (d + W * y) + 1;
			while (x-- > 0) {
				int	v = *(S++);
				int R, G, B;
				R = redFromRGB16(v);
				G = greenFromRGB16(v);
				B = blueFromRGB16(v);
				short t = dist(r, R) + dist(g, G); + dist(b, B);
				if (t > 255)
					t = 255;
				*(D++) = rgb16(t, t, t);
//				*(D++) = rgb16(dist(r, R), dist(g, G), dist(b, B));
//				*(D++) = rgb16(dist(r, R), dist(g, G), dist(b, B));
				r = R; g = G; b = B;
			}
		}
	} else {
		int		SW = frame_in->BytesPerRow();
		int		DW = frame_out->BytesPerRow();
		char *	s = (char *) frame_in->Bits();
		char *	d = (char *) frame_out->Bits();
		int		r = 128, g = 128, b = 128;
		for (int y = 1; y < fHeight - 1; y++)
		{
			int x = fWidth - 1;
			long * S = (long *) (s + SW * y) + 1;
			short * D = (short *) (d + DW * y) + 1;
			while (x-- > 0) {
				int	v = *(S++);
				int R, G, B;
				R = redFromRGB32(v);
				G = greenFromRGB32(v);
				B = blueFromRGB32(v);
				short t = dist(r, R) + dist(g, G) + dist(b, B);
				if (t > 255)
					t = 255;
				*(D++) = rgb16(t, t, t);
				r = R; g = G; b = B;
			}
		}
	}
	return B_OK;
}

