/*

	Preset.cpp

	© 2001, Georges-Edouard Berenger, All Rights Reserved.

*/

#include "Preset.h"
#include "Preferences.h"
#include "ParameterWebCache.h"
#include <stdio.h>
#include <ParameterWeb.h>

void Preset::Unset(bool clearName = true)
{
	if (clearName)
		name = "";
	channel = LONG_MIN;
	videoFormat = LONG_MIN;
	videoInput = LONG_MIN;
	audioInput = LONG_MIN;
	tunerLocale = LONG_MIN;
}

void Preset::SetToCurrent(ParameterWebCache & pwc)
{
	name = "";
	channel = pwc.GetDiscreteParameterValue(kChannelParameter);
	videoFormat = pwc.GetDiscreteParameterValue(kVideoFormatParameter);
	videoInput = pwc.GetDiscreteParameterValue(kVideoInputParameter);
	audioInput = pwc.GetDiscreteParameterValue(kAudioInputParameter);
	tunerLocale = pwc.GetDiscreteParameterValue(kTunerLocaleParameter);
}

bool Preset::SwitchTo(ParameterWebCache & pwc, Preset* previous)
{
	if (IsValid()) {
		Preset	temp;
		if (previous == NULL)
			previous = &temp;
		else
			previous->name = "";
		previous->tunerLocale = pwc.SetDiscreteParameterValue(kTunerLocaleParameter, tunerLocale);
		bool force = tunerLocale != previous->tunerLocale;
		previous->videoFormat = pwc.SetDiscreteParameterValue(kVideoFormatParameter, videoFormat, force);
		force = force || videoFormat != previous->videoFormat;
		previous->channel = pwc.SetDiscreteParameterValue(kChannelParameter, channel, force);
		previous->videoInput = pwc.SetDiscreteParameterValue(kVideoInputParameter, videoInput);
		previous->audioInput = pwc.SetDiscreteParameterValue(kAudioInputParameter, audioInput);
		return true;
	}
	return false;
}

bool Preset::operator==(Preset& preset) const
{
	//	Don't compare names!
	return (preset.channel == channel
		&& preset.videoFormat == videoFormat
		&& preset.videoInput == videoInput
		&& preset.audioInput == audioInput
		&& preset.tunerLocale == tunerLocale);
}

void Preset::FindName(ParameterWebCache & pwc)
{
	if (name.Length() < 1)
		for (int k = 0; k < kMaxPresets; k++)
			if (*this == gPrefs.presets[k]) {
				name = gPrefs.presets[k].name;
				break;
			}
	if (name.Length() < 1 && channel != LONG_MIN) {
		BDiscreteParameter	*tuner = pwc.GetDiscreteParameter(kChannelParameter);
		if (tuner)
			name = tuner->ItemNameAt(channel);
	}
	if (name.Length() < 1)
		name = "???";
}

void Preset::Dump(ParameterWebCache & pwc)
{
	FindName(pwc);
	printf("Preset \"%s\", Channel=%d, VideoFormat=%d, VideoInput=%d, AudioInput=%d, TunerLocale=%d\n",
		name.String(), int(channel), int(videoFormat), int(videoInput), int(audioInput), int(tunerLocale));
		
}
