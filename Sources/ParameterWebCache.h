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

#include "Preset.h"

class BMediaRoster;

enum parameter_cache_parameters {
	kChannelParameter,
	kVideoFormatParameter,
	kResolutionParameter,

	kVideoInputParameter,
	kAudioInputParameter,
	kTunerLocaleParameter,
	kUsePhaseLockParameter,

	kBrightnessParameter,
	kContrastParameter,
	kSaturationParameter,

	kParameterCacheParametersCount
};

class ParameterWebCache
{
public:
								ParameterWebCache(BParameterWeb * web = NULL);
								~ParameterWebCache();
		void					Init(media_node & node, BMediaRoster * roster);
		void					Release();
static	BParameterWeb *			NewVideoParameterWeb(media_node & node, BMediaRoster * roster);	// creates a new one

		BDiscreteParameter *	GetDiscreteParameter(parameter_cache_parameters parameter);
		int32					GetDiscreteParameterValue(parameter_cache_parameters parameter);
		int32					SetDiscreteParameterValue(parameter_cache_parameters parameter, int32 value, bool force = false);

		BContinuousParameter *	GetContinuousParameter(parameter_cache_parameters parameter);
		float					GetContinuousParameterValue(parameter_cache_parameters parameter);
		float					SetContinuousParameterValue(parameter_cache_parameters parameter, float value, bool force = false);

private:
		BParameterWeb *			fCachedParameterWeb;
		BParameter *			fCachedParameters[kParameterCacheParametersCount];
		
		const char *			ParameterName(parameter_cache_parameters parameter);
};

// Definitions missing from Be Headers
extern const char * kBrightnessParameterKind;
extern const char * kContrastParameterKind;
extern const char * kSaturationParameterKind;

#endif // _PARAMETER_WEB_CACHE_H_
