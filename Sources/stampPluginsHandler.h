/*

	stampPluginsHandler.h

	© 2001, Georges-Edouard Berenger, All Rights Reserved.
	See the StampTV Video Plugin License for usage restrictions.

*/

#ifndef _STAMP_PLUGINS_HANDLER_H_
#define _STAMP_PLUGINS_HANDLER_H_

#include <Handler.h>

#include "PluginsHandler.h"
#include "ParameterWebCache.h"

class VideoConsumer;
class stampView;

class stampPluginsHandler : public PluginsHandler, public BHandler
{
	public:
							stampPluginsHandler(stampView * stamp, VideoConsumer * videoConsumer);
		virtual				~stampPluginsHandler();

		virtual	const char* Name();
		virtual uint32		Version();

		virtual bool		OverlayCompatible(color_space cspace);
		virtual status_t	GetChannel(int32 & channel);
		virtual status_t	SwitchToChannel(int32 channel);
		virtual status_t	GetPresetCount(int32 & count);
		virtual status_t	GetPreset(int32 & slot, BString & name, int32 & channel);
		virtual status_t	GetNthPreset(int32 index, BString & name, int32 & preset);
		virtual status_t	SwitchToPreset(BString & name);
		virtual status_t	SwitchToPreset(int32 slot);
		virtual status_t	SwitchToNext(int how_many, bool preset, int32 & index);
				BMenu *		PluginsMenu();
		void				InitParameterWeb(media_node & node, BMediaRoster * roster);
		int32				GetDiscreteParameterValue(const char * parameterName);
		int32				SetDiscreteParameterValue(const char * parameterName, int32 value);
		virtual status_t	SetWindowTitle(const char* name = NULL);
		virtual status_t	OpenConfigWindow(VideoPluginEngine * engine);

		// BHandler part
		virtual	void		MessageReceived(BMessage *message);
	private:
		stampView *			fStamp;
		VideoConsumer *		fVideoConsumer;
		ParameterWebCache	fParameterWebCache;
};

inline int32 stampPluginsHandler::GetDiscreteParameterValue(const char * parameterName)
{
	return fParameterWebCache.GetDiscreteParameterValue(parameterName);
}

inline int32 stampPluginsHandler::SetDiscreteParameterValue(const char * parameterName, int32 value)
{
	return fParameterWebCache.SetDiscreteParameterValue(parameterName, value);
}

#endif // _STAMP_PLUGINS_HANDLER_H_