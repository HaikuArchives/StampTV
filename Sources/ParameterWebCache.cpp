/*

	ParameterWebCache.cpp

	© 1991-1999, Be Incorporated, All rights reserved, was Be's CodyCam Sample Code
	© 2000, Frank Olivier, All Rights Reserved.
	© 2000-2001, Georges-Edouard Berenger, All Rights Reserved.
	See stampTV's License for usage restrictions.

*/

#include "ParameterWebCache.h"

#include "stampTV.h"

#include <MediaRoster.h>
#include <ParameterWeb.h>
#include <TimeSource.h>
#include <stdio.h>

// Should we be R5.0.3 compatible?
#define R5_COMPATIBLE 1

const char * kBrightnessParameterKind = "BRIGHTNESS";
const char * kContrastParameterKind = "CONTRAST";
const char * kSaturationParameterKind = "SATURATION";

#define SET_WHEN BTimeSource::RealTime() - 10 * 1000000

ParameterWebCache::ParameterWebCache(BParameterWeb * web):
	fCachedParameterWeb(web)
{
	memset(fCachedParameters, 0, sizeof(fCachedParameters));
}

ParameterWebCache::~ParameterWebCache()
{
	Release();
}

void ParameterWebCache::Init(media_node & node, BMediaRoster * roster)
{
	Release();
	fCachedParameterWeb = NewVideoParameterWeb(node, roster);
}

void ParameterWebCache::Release()
{
	delete fCachedParameterWeb;
	fCachedParameterWeb = NULL;
	memset(fCachedParameters, 0, sizeof(fCachedParameters));
}

BParameterWeb * ParameterWebCache::NewVideoParameterWeb(media_node & node, BMediaRoster * roster)
{
	BParameterWeb * newweb = NULL;
	status_t status = roster->GetParameterWebFor(node, &newweb);
	if (status != B_OK || !newweb) {
		ErrorAlert("Can't create new video parameter web", status);
		return NULL;
	}
	return newweb;
}

BDiscreteParameter * ParameterWebCache::GetDiscreteParameter(parameter_cache_parameters param)
{
	if (!fCachedParameterWeb)
		return NULL;
	if (fCachedParameters[param])
		return reinterpret_cast<BDiscreteParameter *> (fCachedParameters[param]);
	BDiscreteParameter * result = NULL;
	int32	last = fCachedParameterWeb->CountParameters();
	const char * name = ParameterName(param);
	for (int32 i = 0; i < last; i++) {
		BParameter *parameter = fCachedParameterWeb->ParameterAt(i);
#if R5_COMPATIBLE
		
		// This part of the code is *E*X*T*R*E*M*E*L*Y* *W*E*A*K* !
		// We shouldn't rely on the names used for the GUI!!!
		// Those may change at any update of the driver (don't even think about localisation!)

		if (strcmp(parameter->Name(), name) == 0)
#else
		if (strcmp(parameter->Kind(), name) == 0)
#endif
		{
			result = dynamic_cast<BDiscreteParameter *> (parameter);
			fCachedParameters[int(param)] = result;
			break;
		}
	}
	return result;
}

int32 ParameterWebCache::GetDiscreteParameterValue(parameter_cache_parameters param)
{
	BDiscreteParameter	*parameter = GetDiscreteParameter(param);
	if (!parameter)
		return -1;
	int32		value;
	size_t		size = sizeof(value);
	bigtime_t	lastChange;
	if (parameter->GetValue(reinterpret_cast<void *>(&value), &size, &lastChange) == B_OK
			 && size <= sizeof(value)) {
		return value;
	}
	return -1;
}

int32 ParameterWebCache::SetDiscreteParameterValue(parameter_cache_parameters param, int32 value, bool force = false)
{
	BDiscreteParameter	*parameter = GetDiscreteParameter(param);
	if (!parameter)
		return -1;

//	int32	lastValue = parameter->CountItems() - 1;
//	if (value > lastValue)
//		value = lastValue;
//	if (value < 0)
//		value = 0;
	int32 current_value;
	bigtime_t time;
	size_t size = sizeof(current_value);
	if ((parameter->GetValue(reinterpret_cast<void *>(&current_value), &size, &time) == B_OK)
			 && (size <= sizeof(current_value))) {
		if (force || current_value != value)
			parameter->SetValue(reinterpret_cast<void *>(&value), sizeof(current_value), SET_WHEN);
		return current_value;
	}
	return -1;
}

BContinuousParameter * ParameterWebCache::GetContinuousParameter(parameter_cache_parameters param)
{
	BContinuousParameter * result = NULL;
	if (!fCachedParameterWeb)
		return NULL;
	if (fCachedParameters[param])
		return reinterpret_cast<BContinuousParameter *> (fCachedParameters[param]);
	int32	last = fCachedParameterWeb->CountParameters();
	const char * name = ParameterName(param);
	for (int32 i = 0; i < last; i++) {
		BParameter *parameter = fCachedParameterWeb->ParameterAt(i);
		if (strcmp(parameter->Kind(), name) == 0)
		{
			result = dynamic_cast<BContinuousParameter *> (parameter);
			fCachedParameters[param] = result;
			break;
		}
	}
	return result;
}

float ParameterWebCache::SetContinuousParameterValue(parameter_cache_parameters param, float value, bool force = false)
{
	BContinuousParameter	*parameter = GetContinuousParameter(param);
	if (!parameter)
		return FLT_MIN;

//	if (value < parameter->MinValue())
//		value = parameter->MinValue();
//	else if (value > parameter->MaxValue())
//		value = parameter->MaxValue();
	float current_value;
	bigtime_t time;
	size_t size = sizeof(current_value);
	if ((parameter->GetValue(reinterpret_cast<void *>(&current_value), &size, &time) == B_OK)
			 && (size <= sizeof(current_value))) {
		if (force || current_value != value)
			parameter->SetValue(reinterpret_cast<void *>(&value), sizeof(current_value), SET_WHEN);
		return current_value;
	}
	return -1.0;
}

float ParameterWebCache::GetContinuousParameterValue(parameter_cache_parameters param)
{
	BContinuousParameter	*parameter = GetContinuousParameter(param);
	if (!parameter)
		return FLT_MIN;
	float		value;
	size_t		size = sizeof(value);
	bigtime_t	lastChange;
	if (parameter->GetValue(reinterpret_cast<void *>(&value), &size, &lastChange) == B_OK
			 && size <= sizeof(value)) {
		return value;
	}
	return FLT_MIN;
}

const char * ParameterWebCache::ParameterName(parameter_cache_parameters parameter)
{
	const char * gParameterNames[kParameterCacheParametersCount] = {
#if R5_COMPATIBLE
		"Channel:",
		"Video Format:",
		"Default Image Size:",
		"Video Input:",
		"Audio Input:",
		"Tuner Locale:",
		"Use Phase Lock Loop",
#else
		B_TUNER_CHANNEL,
		B_VIDEO_FORMAT,
		B_RESOLUTION,
		// these are not ok! We still miss definitions!
		"Video Input:",
		"Audio Input:",
		"Tuner Locale:",
		"Use Phase Lock Loop",
#endif
		kBrightnessParameterKind,
		kContrastParameterKind,
		kSaturationParameterKind,
	};

	return gParameterNames[int(parameter)];
}
