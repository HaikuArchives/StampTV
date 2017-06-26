/*

	PluginApplication.h

	© 2001, Georges-Edouard Berenger, All Rights Reserved.

*/

#ifndef _PLUGIN_APPLICATION_H_
#define _PLUGIN_APPLICATION_H_

#include <Application.h>

class PluginApplication : public BApplication {

public:
				PluginApplication();
virtual	void	ReadyToRun();
};

#endif // _PLUGIN_APPLICATION_H_
