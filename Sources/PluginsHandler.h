/*

	PluginsHandler.h

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video Plugin License for usage restrictions.


*/

#ifndef _PLUGINS_HANDLER_H_
#define _PLUGINS_HANDLER_H_

#include "VideoPlugin.h"

class BMenu;

const uint32 PLUGIN_HANDLER_REQUEST = 'Plug';

typedef struct {
	color_space				in_cspace;
	color_space				out_cspace;
	float					quality;
	uint32					flags;	// intersection of all flags of the path
	const vertice_properties *	vertice[1];	// Actual count not known there
} Tpath;

class PluginsHandler : public VideoPluginsHost
{
	public:
							PluginsHandler();
		virtual				~PluginsHandler();

		virtual	void		ScanPluginsFolder();
				//	saves the current configuration & settings in a BMessage
		virtual status_t	StoreConfig(BMessage & config);
				//	restores a plugin network configuration
		virtual status_t	RestoreConfig(BMessage & config);
				
				// Iterates over the screen modes allowed by the active engines
				// Set cookie to 0 the first time, and loop (if necessary) until
				// B_ERROR is returned.
				// firstFlags applies for the first plugin
				// lastFlags for the last plugin
				// allFlags is the intersection of all plugins.
				status_t	GetNextPath(int32 & cookie, color_space & in, color_space & out,
								bool & draw_in_buffer, uint32 & firstFlags,
								uint32 & allFlags, uint32 & lastFlags);
				int			GetPathCount() { return fPathCount; }
				// Use the path last returned by GetNextPath (doesn't change the cookie)
				// Notifies each engine of the format in use via their UseFormat method
				// Allocates buffers (bitmaps) as see fit
				// Pass the overlay bitmap if one can & should be used.
				status_t	UsePath(int32 cookie, media_raw_video_format & format, BBitmap * overlay);
				// Applies the effects on the given frame. Returns an image to display
				BBitmap *	Filter(BBitmap * frame, int64 frame_count, bool skipped_frames);

	protected:
		TList<VideoPluginType>		fTypeList;
		TList<VideoPluginEngine>	fEngineList;
		TList<BBitmap>				fBitmaps;			// Storage of *created* bitmaps

		// To help iterate over a chain of plugins
		
		static const int	kAllocateChunk;
				int			BuildPaths(	color_space in = ANY_COLOR_SPACE,
										color_space out = ANY_COLOR_SPACE,
										float quality = 1.0,
										uint32 flags = ~0,
										int depth = 0);
				void		ScanOnePluginsFolder(const char * rootdir);
				
				Tpath &		Path(int which = 0);	// Will extend the array if necessary!
													// default return 'work' copy

				void *		fPathsPool;			// one Tpath every 'fSizeOnePath' bytes
				int			fSizeOnePath;		// Size of on Tpath object for this count of engines
				int			fAllocatedCount;	// Number of paths allocated
				int			fPathCount;			// Count of valid paths
				bool		fReadyToFilter;		// Says if Filter should do anything!
};

#endif // _PLUGINS_HANDLER_H_
