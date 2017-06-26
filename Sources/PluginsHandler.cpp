/*

	PluginsHandler.cpp

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video Plugin License for usage restrictions.

	Handling of a set of plugin. Alows acces to the plugin services
	in a global way: what plugins are available, what is the active plugin list,
	how to change it, how to preprare the engines for work (creation of intermediate
	bitmaps if necessary), can that combinaison of plugins work, what color spaces
	are supported, handling of the plugin process flow...
	
	This level is 100% host independent. Host implementers should be able to reuse this
	file as is and benefit from bug fixes/improvements.
	(Hopefully, without changes to the interface, but there is no waranty for that!)

	This level contains very intimate implementation details on the image processing engine.
	Those details should remain hidden here!
	
	Here is *no* knowledge on the host. In particular, concurence problems are not handled:
	- do not modify the engine list *while* processing (you'll hear about it if you do!),
	- if you change the engine list (directly manipulating the fEngineList):
		* rebuild the paths with BuildPaths(),
		* iterate of the available paths with GetPathCount()/GetNextPath(),
		* select which path you will use with UsePath() (this creates intermediate bitmaps),
		* to the video work with Filter().

*/

#include "PluginsHandler.h"

#include <stdlib.h>
#include <string.h>
#include <Bitmap.h>
#include <View.h>
#include <Node.h>
#include <Mime.h>
#include <Path.h>
#include <Application.h>
#include <Roster.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Alert.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#define	WARNING(x)	printf x
#define	PROGRESS(x)	printf x

const char * kPluginsDirectoryName = "Video Plugins";

PluginsHandler::PluginsHandler() : fPathsPool(NULL), fSizeOnePath(-1), fAllocatedCount(0),
	fPathCount(0), fReadyToFilter(false)
{
}

PluginsHandler::~PluginsHandler()
{
	fReadyToFilter = false;
	fBitmaps.MakeEmpty();
	// Be sure to close & delete all engines before their master!
	while (fEngineList.CountItems() > 0) {
		VideoPluginEngine * engine = fEngineList.RemoveItem(fEngineList.CountItems() - 1);
		engine->Close();
		delete engine;
	}
	fTypeList.MakeEmpty();
	if (fPathsPool)
		free(fPathsPool);
}

void PluginsHandler::ScanPluginsFolder()
{
	app_info	infos;
	BPath		path;
	if (be_app->GetAppInfo(&infos) == B_OK && path.SetTo(&infos.ref) == B_OK && path.GetParent(&path) == B_OK) {
		path.Append("Plugins", true);
		ScanOnePluginsFolder(path.Path());
	}
	if (find_directory(B_COMMON_ADDONS_DIRECTORY, &path, true) == B_OK) {
		BDirectory	dir(path.Path());
		if (!dir.Contains(kPluginsDirectoryName, B_DIRECTORY_NODE))
			dir.CreateDirectory(kPluginsDirectoryName, NULL);
		path.Append(kPluginsDirectoryName);
		ScanOnePluginsFolder(path.Path());
	}
	// Let's remove duplicate plugins (same ID).
	for (int t = 0; t < fTypeList.CountItems(); t++) {
		for (int tt = t+1; tt < fTypeList.CountItems(); tt++) {
			if (strcmp(fTypeList.ItemAt(t)->ID(), fTypeList.ItemAt(tt)->ID()) == 0) {
				VideoPluginType * type = fTypeList.RemoveItem(tt);
				if (fTypeList.ItemAt(t)->Version() < type->Version()) {
					// we would rather keep the second in the list: permute them
					fTypeList.AddItem(type, t);
					type = fTypeList.RemoveItem(t + 1);
				}
				delete type;
				tt--;	// continue at the same index, since the remaining items shifted left one place...
			}
		}
	}
}

status_t PluginsHandler::StoreConfig(BMessage & config)
{
	config.MakeEmpty();
	for (int e = 0; e < fEngineList.CountItems(); e++) {
		VideoPluginEngine * engine = fEngineList.ItemAt(e);
		bool	configOpen = (engine->ConfigWindow() != NULL);
		engine->Close();
		config.AddString("ID", engine->Plugin()->ID());
		BMessage	plugConfig;
		engine->StoreConfig(plugConfig);
		config.AddMessage("Config", &plugConfig);
		config.AddRect("Frame", engine->ConfigWindowFrame());
		config.AddBool("ConfigOpen", configOpen);
	}
	return B_OK;
}

status_t PluginsHandler::RestoreConfig(BMessage & config)
{
	if (modifiers() & B_CONTROL_KEY) {
		BAlert * alert = new BAlert("", "Do you want to reset the plugins settings?", "Cancel", "Reset", NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		if (alert->Go())
			config.MakeEmpty();
	}
	const char * ID;
	for (int e = 0; config.FindString("ID", e, &ID) == B_OK; e++) {
		for (int t = 0; t < fTypeList.CountItems(); t++) {
			if (strcmp(fTypeList.ItemAt(t)->ID(), ID) == 0) {
				VideoPluginEngine * engine = fTypeList.ItemAt(t)->InstanciateVideoPluginEngine();
				fEngineList.AddItem(engine);
				BMessage	plugConfig;
				if (config.FindMessage("Config", e, &plugConfig) != B_OK)
					plugConfig.MakeEmpty();
				engine->RestoreConfig(plugConfig);
				BRect		frame;
				if (config.FindRect("Frame", e, &frame) == B_OK)
					engine->SetConfigWindowFrame(frame);
				else
					engine->SetConfigWindowFrame(BRect(-1, -1, -10, -10));
				bool		configOpen = false;
				if (config.FindBool("ConfigOpen", &configOpen) == B_OK && configOpen)
					OpenConfigWindow(engine);
				break;
			}
		}
	}
	return B_OK;
}

void PluginsHandler::ScanOnePluginsFolder(const char * rootdir)
{
	DIR* dir = opendir (rootdir);
	if (dir) {
		struct dirent * entry = readdir (dir);
		while (entry) {
			if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
				char path[PATH_MAX];
				strcpy(path, rootdir);
				strcat(path, "/");
				strcat(path, entry->d_name);
				struct stat st;
				if (stat(path, &st) == 0) {
					if (S_ISREG(st.st_mode)) {
						BNode	node (path);
						char	type[B_MIME_TYPE_LENGTH];
						if (node.InitCheck() == B_OK
							&& node.ReadAttr("BEOS:TYPE", B_MIME_STRING_TYPE, 0, type, B_MIME_TYPE_LENGTH) > 0
							&& strcmp(type, B_APP_MIME_TYPE) == 0) {
							// To help debugging if a crash occurs...
							PROGRESS(("Loading \"%s\" as an addon... ", path));
							fflush(stdout);
							char	name[B_OS_NAME_LENGTH];
							strncpy(name, entry->d_name, B_OS_NAME_LENGTH);
							rename_thread(find_thread(NULL), name);
							// an executable has been found. Try to load it as a VST plugin
							image_id videoplugin = load_add_on(path);
							if (videoplugin > 0)
							{	// the file is indeed an addon, but is it a VST plugin?
								PROGRESS(("OK! Video Plugin?... "));
								fflush(stdout);
								status_t (*video_plugin)(TList<VideoPluginType> & plugin_list, VideoPluginsHost * host, image_id id);
								if (get_image_symbol (videoplugin, "GetVideoPlugins", B_SYMBOL_TYPE_TEXT, (void**) &video_plugin) == B_OK)
								{	// Chances are now that this is a VST plugin, but is it supported?...
									PROGRESS(("Yes!\n"));
									(*video_plugin)(fTypeList, this, videoplugin);
								} else
									PROGRESS(("No!\n"));
							} else
								PROGRESS(("Not an addon!\n"));
						}
					}
					else if (S_ISDIR(st.st_mode))
						ScanOnePluginsFolder(path);
				}
			}
			entry = readdir(dir);
		}
		closedir(dir);
	}
}

status_t PluginsHandler::GetNextPath(int32 & cookie, color_space & in, color_space & out,
	bool & draw_in_buffer, uint32 & firstFlags, uint32 & allFlags, uint32 & lastFlags)
{
	cookie++;
	if (cookie < 1 || cookie > fPathCount)
		return B_ERROR;
	color_space	given_in = in;
	color_space given_out = out;
	int	engineCount = fEngineList.CountItems();
	if (engineCount < 1) {
		// no engine: empty filter
		if (in == ANY_COLOR_SPACE)
			in = out;
		else if (out == ANY_COLOR_SPACE)
			out = in;
		if (in != out)
			return B_ERROR;
		draw_in_buffer = false;
		firstFlags = lastFlags = allFlags = VAPI_PROCESS_IN_PLACE | VAPI_PROCESS_OVERLAY_IN_PLACE;
		return B_OK;
	}
	do {
		Tpath & path = Path(cookie);
		in = path.in_cspace;
		out = path.out_cspace;
		if (in == ANY_COLOR_SPACE)
			in = given_in;
		if (out == ANY_COLOR_SPACE)
			out = given_out;
		if ((in == given_in || given_in == ANY_COLOR_SPACE) && (out == given_out || given_out == ANY_COLOR_SPACE)) {
			// should the incoming buffers accept bitmaps?
			draw_in_buffer = false;
			for (int a = 0; a < engineCount; a++) {
				draw_in_buffer = supports_each(path.vertice[a]->flags, VAPI_BVIEW_DRAWING);
				if (draw_in_buffer || !supports_one(path.vertice[a]->flags, VAPI_PROCESS_IN_PLACE))
					break;
			}
			firstFlags = path.vertice[0]->flags;
			allFlags = path.flags;
			lastFlags = path.vertice[engineCount - 1]->flags;
			return B_OK;
		}
	} while (++cookie <= fPathCount);
	return B_ERROR;
}

status_t PluginsHandler::UsePath(int32 cookie, media_raw_video_format & format, BBitmap * overlay)
{
	fReadyToFilter = false;
	if (cookie < 1 || cookie > fPathCount)
		return B_ERROR;
	fBitmaps.MakeEmpty();
	int	engine_count = fEngineList.CountItems();
	if (engine_count > 0) {
		Tpath & path = Path(cookie);
		// clear the settings
		for (int e = 0; e < engine_count; e ++) {
			VideoPluginEngine * engine = fEngineList.ItemAt(e);
			engine->SetOutput(NULL);
			engine->SetReadingFromOverlay(false);
			engine->SetWritingInOverlay(false);
		}
		// use the end-overlay as much as possible
		if (overlay) {
			for (int e = engine_count - 1; e >= 0; e--) {
				VideoPluginEngine * engine = fEngineList.ItemAt(e);
				engine->SetOutput(overlay);
				engine->SetWritingInOverlay(true);
				PROGRESS(("Engine %d '%s' writes in the overlay\n", e+1, engine->Plugin()->Name()));
				// can this vertice let an overlay go through?
				if (!supports_each(path.vertice[e]->flags, VAPI_PROCESS_OVERLAY_IN_PLACE))
					break;
				// can the previous vertice write to an overlay?
				if (e > 0 && !supports_one(path.vertice[e - 1]->flags, VAPI_PROCESS_OVERLAY_IN_PLACE | VAPI_PROCESS_TO_OVERLAY))
					break;
				engine->SetReadingFromOverlay(true);
				PROGRESS(("Engine %d '%s' reads from the overlay\n", e+1, engine->Plugin()->Name()));
			}
		}
		// use each bitmap as much as possible. NULL means output = input.
		BBitmap *	b = NULL;	// initial in-bitmap is given by the caller. It respects the plugins requirements for views
		bool		views_allowed = false;
		for (int a = 0; a < engine_count; a++) {
			views_allowed = supports_each(path.vertice[a]->flags, VAPI_BVIEW_DRAWING);
			if (views_allowed || !supports_one(path.vertice[a]->flags, VAPI_PROCESS_IN_PLACE | VAPI_PROCESS_OVERLAY_IN_PLACE))
				break;
		}
		BRect				frame(0, 0, format.display.line_width - 1.0, format.display.line_count - 1.0);
		color_space			last_cspace = format.display.format;
		for (int e = 0; e < engine_count; e ++) {
			VideoPluginEngine * engine = fEngineList.ItemAt(e);
			const	vertice_properties * ap;
			int32	propertyCount = engine->Plugin()->GetVerticesProperties(&ap);
			int	property = path.vertice[e] - ap;
			if (property < 0 || property >= propertyCount) {
				WARNING(("PluginsHandler::UsePath: vertice #%d wasn't valid in path #%d\n", e, int(cookie)));
				return B_ERROR;
			}
			uint32 flags = path.vertice[e]->flags;
			engine->SetFlags(flags);
			status_t r = engine->UseFormat(format, property);
			if (r != B_OK) {
				WARNING(("Engine '%s' didn't accept property %d\n", engine->Plugin()->Name(), e));
				return r;
			}
			if (engine->Output() == NULL) {
				color_space out_cspace = path.vertice[e]->out_cspace;
				if (out_cspace == ANY_COLOR_SPACE)
					out_cspace = last_cspace;
				// no need to check color space compatibility: VAPI_PROCESS_IN_PLACE implies it!
				if (supports_one(flags, VAPI_PROCESS_IN_PLACE | VAPI_PROCESS_OVERLAY_IN_PLACE)
					&& (views_allowed || !supports_each(flags, VAPI_BVIEW_DRAWING)))
				{	// we can reuse the in bitmap for the output
				} else {	// current bitmap doesn't meet requirements. Create a new one!
					views_allowed = false;
					for (int a = e + 1; a < engine_count; a++) {
						views_allowed = supports_each(path.vertice[a]->flags, VAPI_BVIEW_DRAWING);
						if (views_allowed || !supports_one(path.vertice[a]->flags, VAPI_PROCESS_IN_PLACE | VAPI_PROCESS_OVERLAY_IN_PLACE))
							break;
					}
					b = new BBitmap(frame, out_cspace, views_allowed);
					fBitmaps.AddItem(b);
					WARNING(("Created intermediate bitmap format %d (views: %d) for engine %d '%s'\n", int(out_cspace), int(views_allowed), int(e + 1), engine->Plugin()->Name()));
				}
				engine->SetOutput(b);
				last_cspace = out_cspace;
			}
		}
	}
	fReadyToFilter = true;
	return B_OK;
}

BBitmap * PluginsHandler::Filter(BBitmap * frame, int64 frame_count, bool skipped_frames)
{
	if (fReadyToFilter) {
		int	engine_count = fEngineList.CountItems();
		if (engine_count < 1)
			return frame;
		BBitmap *	out;
		BView *		view = NULL;
		for (int e = 0; e < engine_count; e ++) {
			VideoPluginEngine * engine = fEngineList.ItemAt(e);
			out = engine->Output();
			if (out == NULL)
				out = frame;
			if (!view && supports_each(engine->Flags(), VAPI_BVIEW_DRAWING)) {
				if (frame->Lock()) {
					view = frame->ChildAt(0);
					if (!view) {
						view = new BView(frame->Bounds(), B_EMPTY_STRING, 0, 0);
						frame->AddChild(view);
					}
				} else {
					WARNING(("Couldn't lock bitmap for engine '%s' (#%d)\n", engine->Plugin()->Name(), e + 1));
					return frame;
				}
			} else if (frame != out && view) {
				view->Sync();
				frame->Unlock();
				view = NULL;
			}
			if (view && !supports_each(engine->Flags(), VAPI_BVIEW_DRAWING)) {
				view->Sync();
			}
			engine->ApplyEffect(frame, out, frame_count, skipped_frames);
			frame = out;
		}
		if (view) {
			view->Sync();
			frame->Unlock();
		}
	}
	return frame;
}

//-------------------------------------------------------------------------
// Private functions to generate a path in plugins configuration

const int PluginsHandler::kAllocateChunk = 32;

Tpath & PluginsHandler::Path(int which)
{
	if (which >= fAllocatedCount) {
		fAllocatedCount = ((which / kAllocateChunk) + 1) * kAllocateChunk;
		fPathsPool = realloc(fPathsPool, fSizeOnePath * fAllocatedCount);
	}
	Tpath * paths = (Tpath *) ((char*) fPathsPool + fSizeOnePath * which);
	return *paths;
}

static int qsort_float_compare(const void* a, const void* b)
{
	float fa = ((const vertice_properties*) a)->quality;
	float fb = ((const vertice_properties*) b)->quality;
	if (fabs(fa - fb) < 0.000001)
		return 0;
	if (fa > fb)
		return -1;
	return 1;
}

int PluginsHandler::BuildPaths(color_space in, color_space out, float quality, uint32 flags, int depth)
{
	fReadyToFilter = false;
	if (depth == 0) {
		// prepare storing path of the right size!
		fPathCount = 0;
		int c = fEngineList.CountItems() - 1;
		if (c < 0)
			c = 0;
		int	newSize = sizeof(Tpath) + sizeof(vertice_properties *) * c;
		if (newSize != fSizeOnePath) {
			if (fPathsPool)
				free(fPathsPool);
			fSizeOnePath = newSize;
			fAllocatedCount = kAllocateChunk;
			fPathsPool = malloc (fSizeOnePath * fAllocatedCount);
		}
		// special case if there is no plugin at all!
		if (fEngineList.CountItems() < 1) {
			if (in == ANY_COLOR_SPACE)
				in = out;
			else if (out == ANY_COLOR_SPACE)
				out = in;
			if (in == out)
				fPathCount = 1;
			return fPathCount;
		}
	}
	// Tries to build all paths between the engine 'depth' and the last engine,
	// respecting the 'in' & 'out' color spaces. Quality so far is 'quality'.
	const	vertice_properties * ap;
	int32	propertyCount = fEngineList.ItemAt(depth)->Plugin()->GetVerticesProperties(&ap);
	bool	lastEngine = (depth + 1 == fEngineList.CountItems());
	for (int property = 0; property < propertyCount; property++) {
		color_space this_out = (lastEngine ? out : ANY_COLOR_SPACE); // what should be produced at this level! Could be improved!
		if ((in == ANY_COLOR_SPACE || ap[property].in_cspace == ANY_COLOR_SPACE || ap[property].in_cspace == in)
			&& (this_out == ANY_COLOR_SPACE || this_out == ap[property].out_cspace || ap[property].out_cspace == ANY_COLOR_SPACE))
		{	// this combinaison works (so far)!
			Tpath & path = Path();
			path.vertice[depth] = &ap[property];
			float q = quality * ap[property].quality;
			uint32 f = flags & ap[property].flags;
			this_out = ap[property].out_cspace;
			if (this_out == ANY_COLOR_SPACE)
				this_out = in;
			if (lastEngine) {
				path.quality = q;
				path.flags = f;
				path.out_cspace = this_out;
				// ANY_COLOR_SPACE Input may be forced by later vertice: make sure we see that!
				for (int a = 0; a <= depth && (path.in_cspace = path.vertice[a]->in_cspace) == ANY_COLOR_SPACE; a++)
					;
				// 'useless' operation to force possible extension of the array!
				Path(++fPathCount).quality = q;
				memcpy(&Path(fPathCount), &Path(), fSizeOnePath);
//				path = Path(fPathCount);
//				printf("Q: %f i: %d o: %d\n", path.quality, int(path.in_cspace), int(path.out_cspace));
			} else
				BuildPaths(this_out, out, q, f, depth + 1);
		}
	}
	if (depth == 0 && fPathCount > 1) {
		// We now sort the results so that they are presented in the "best" paths first
		qsort(&Path(1), fPathCount, fSizeOnePath, qsort_float_compare);
	}
	return fPathCount;
}
