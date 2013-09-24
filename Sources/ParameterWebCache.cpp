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

// Should we be R5.0.3 compatible?
#define R5_COMPATIBLE 1

#if R5_COMPATIBLE
const char * kChannelParameter = "Channel:";
const char * kFormatParameter = "Video Format:";
const char * kResolutionParameter = "Default Image Size:";
#else
const char * kChannelParameter = B_TUNER_CHANNEL;
const char * kFormatParameter = B_VIDEO_FORMAT;
const char * kResolutionParameter = B_RESOLUTION;
#endif

const char * kBrightnessParameter = "BRIGHTNESS";
const char * kContrastParameter = "CONTRAST";
const char * kSaturationParameter = "SATURATION";

ParameterWebCache::ParameterWebCache(BParameterWeb * web):
	fCachedParameterWeb(web), fCachedChannelParameter(NULL), fCachedFormatParameter(NULL)
{
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
	fCachedChannelParameter = NULL;
	fCachedFormatParameter = NULL;
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

BDiscreteParameter * ParameterWebCache::GetDiscreteParameter(const char * parameterName)
{
	if (!fCachedParameterWeb)
		return NULL;
	if (parameterName == kChannelParameter && fCachedChannelParameter)
		return fCachedChannelParameter;
	if (parameterName == kFormatParameter && fCachedFormatParameter)
		return fCachedFormatParameter;
	BDiscreteParameter * result = NULL;
	int32	last = fCachedParameterWeb->CountParameters();
	for (int32 i = 0; i < last; i++) {
		BParameter *parameter = fCachedParameterWeb->ParameterAt(i);
#if R5_COMPATIBLE
		
		// This part of the code is *E*X*T*R*E*M*E*L*Y* *W*E*A*K* !
		// We shouldn't rely on the names used for the GUI!!!
		// Those may change at any update of the driver (don't even think about localisation!)

		if (strcmp(parameter->Name(), parameterName) == 0)
#else
		if (strcmp(parameter->Kind(), parameterName) == 0)
#endif
		{
			result = dynamic_cast<BDiscreteParameter *> (parameter);
			if (parameterName == kChannelParameter)
				fCachedChannelParameter = result;
			if (parameterName == kFormatParameter)
				fCachedFormatParameter = result;
			break;
		}
	}
	return result;
}

int32 ParameterWebCache::GetDiscreteParameterValue(const char * parameterName)
{
	BDiscreteParameter	*parameter = GetDiscreteParameter(parameterName);
	if (!parameter)
		return -1;
	int32		value;
	size_t		size = sizeof(int32);
	bigtime_t	lastChange;
	if (parameter->GetValue(reinterpret_cast<void *>(&value), &size, &lastChange) == B_OK
			 && size <= sizeof(int32)) {
		return value;
	}
	return -1;
}

int32 ParameterWebCache::SetDiscreteParameterValue(const char * parameterName, int32 value)
{
	BDiscreteParameter	*parameter = GetDiscreteParameter(parameterName);
	if (!parameter)
		return -1;

	int32	lastValue = parameter->CountItems() - 1;
	if (value > lastValue)
		value = lastValue;
	if (value < 0)
		value = 0;

	int32 current_value;
	bigtime_t time;
	size_t size = sizeof(int32);
	if ((parameter->GetValue(reinterpret_cast<void *>(&current_value), &size, &time) == B_OK)
			 && (size <= sizeof(int32))) {
		if (current_value == value)
			return current_value;
	}
	parameter->SetValue(reinterpret_cast<void *>(&value), sizeof(int32), BTimeSource::RealTime());
	return current_value;
}

BContinuousParameter * ParameterWebCache::GetContinuousParameter(const char * parameterName)
{
	BContinuousParameter * result = NULL;
	if (!fCachedParameterWeb)
		return NULL;
	int32	last = fCachedParameterWeb->CountParameters();
	for (int32 i = 0; i < last; i++) {
		BParameter *parameter = fCachedParameterWeb->ParameterAt(i);
		if (strcmp(parameter->Kind(), parameterName) == 0)
		{
			result = dynamic_cast<BContinuousParameter *> (parameter);
			break;
		}
	}
	return result;
}

float ParameterWebCache::SetContinuousParameterValue(const char * parameterName, float value)
{
	BContinuousParameter	*parameter = GetContinuousParameter(parameterName);
	if (!parameter)
		return FLT_MIN;

	if (value < parameter->MinValue())
		value = parameter->MinValue();
	else if (value > parameter->MaxValue())
		value = parameter->MaxValue();
	float current_value;
	bigtime_t time;
	size_t size = sizeof(float);
	if ((parameter->GetValue(reinterpret_cast<void *>(&current_value), &size, &time) == B_OK)
			 && (size <= sizeof(float))) {
		if (current_value == value)
			return current_value;
	}
	parameter->SetValue(reinterpret_cast<void *>(&value), sizeof(float), BTimeSource::RealTime());
	return current_value;
}

float ParameterWebCache::GetContinuousParameterValue(const char * parameterName)
{
	BContinuousParameter	*parameter = GetContinuousParameter(parameterName);
	if (!parameter)
		return FLT_MIN;
	float		value;
	size_t		size = sizeof(float);
	bigtime_t	lastChange;
	if (parameter->GetValue(reinterpret_cast<void *>(&value), &size, &lastChange) == B_OK
			 && size <= sizeof(float)) {
		return value;
	}
	return FLT_MIN;
}
