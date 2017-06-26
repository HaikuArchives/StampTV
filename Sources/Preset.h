/*

	Preset.h

	© 2001, Georges-Edouard Berenger, All Rights Reserved.

*/

#ifndef _PRESET_H_
#define _PRESET_H_

#include <String.h>

const int kMaxPresets = 38; // F1 - F12 + 'A' - 'Z'

class ParameterWebCache;

class Preset
{
public:
	BString	name;
	int32	channel;
	int32	videoFormat;
	int32	videoInput;
	int32	audioInput;
	int32	tunerLocale;
	
			Preset()	{ Unset(); }
	void	Unset(bool clearName = true);
	bool	IsValid()	{ return channel >= 0; }
	void	SetToCurrent(ParameterWebCache & pwc);
	bool	SwitchTo(ParameterWebCache & pwc, Preset* previous = NULL);
	void	FindName(ParameterWebCache & pwc);
	bool	operator==(Preset& preset) const;
	bool	operator!=(Preset& preset) const { return !(*this == preset); }
	void	Dump(ParameterWebCache & pwc);
};


#endif // _PRESET_H_