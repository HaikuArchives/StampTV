/*

	ParameterWebCache.h

	© 1991-1999, Be Incorporated, All rights reserved, was Be's CodyCam Sample Code
	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#ifndef _PARAMETER_WEB_CACHE_H_
#define _PARAMETER_WEB_CACHE_H_

#include <MediaNode.h>

class BMediaRoster;

class ParameterWebCache
{
public:
							ParameterWebCache(BParameterWeb * web = NULL);
							~ParameterWebCache();
		void				Init(media_node & node, BMediaRoster * roster);
		void				Release();
static	BParameterWeb		*NewVideoParameterWeb(media_node & node, BMediaRoster * roster);	// creates a new one

		BDiscreteParameter	*GetDiscreteParameter(const char * parameterName);
		int32				GetDiscreteParameterValue(const char * parameterName);
		int32				SetDiscreteParameterValue(const char * parameterName, int32 value);

		BContinuousParameter*GetContinuousParameter(const char * parameterName);
		float				GetContinuousParameterValue(const char * parameterName);
		float				SetContinuousParameterValue(const char * parameterName, float value);

private:
		BParameterWeb		*fCachedParameterWeb;
		BDiscreteParameter	*fCachedChannelParameter;
		BDiscreteParameter	*fCachedFormatParameter;
};

// *Only* use those constant, or the caching mechanism won't work!
// (we do direct pointer comparaison!)

extern const char * kChannelParameter;
extern const char * kFormatParameter;
extern const char * kResolutionParameter;

extern const char * kBrightnessParameter;
extern const char * kContrastParameter;
extern const char * kSaturationParameter;

#endif // _PARAMETER_WEB_CACHE_H_