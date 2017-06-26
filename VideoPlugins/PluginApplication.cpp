/*

	PluginApplication.cpp

	© 2001, Georges-Edouard Berenger, All Rights Reserved.

*/

#include "PluginApplication.h"
#include <Path.h>
#include <FindDirectory.h>
#include <Alert.h>
#include <Directory.h>
#include <Roster.h>
#include <stdio.h>

//#define show(x) (new BAlert("", (x), "Ok!"))->Go()
const char *kTrackerSig = "application/x-vnd.Be-TRAK";


PluginApplication::PluginApplication() : BApplication("application/x-vnd.Geb-StampTV-Plugin")
{
}

void PluginApplication::ReadyToRun()
{
	app_info		appInfo;
	GetAppInfo(&appInfo);
	BEntry			addon(&appInfo.ref);
	char			addonName[B_FILE_NAME_LENGTH];
	BPath			parent_path, target_path;
	BEntry			parent;
	char			texte[10240];
	addon.GetParent(&parent);
	parent.GetPath(&parent_path);
	find_directory(B_USER_ADDONS_DIRECTORY, &target_path, true);
	target_path.Append("Video Plugins", true);
	create_directory(target_path.Path(), 777);
	addon.GetName(addonName);
	if (target_path==parent_path) {
		sprintf(texte, "\"%s\" is a plugin for stampTV, not an application!\n\n"
			"You should leave it where it is, and use it through stampTV\n", addonName);
		(new BAlert("", texte, "OK"))->Go();
	} else {
		BPath		duplicate_path(target_path);
		duplicate_path.Append(addonName);
		BEntry		duplicate(duplicate_path.Path());
		if (duplicate.Exists()) {
			BMessage	openFolder(B_REFS_RECEIVED);
			BEntry		target(target_path.Path());
			entry_ref	target_ref;
			target.GetRef(&target_ref);
			openFolder.AddRef("refs", &target_ref);
			be_roster->Launch(kTrackerSig, &openFolder);
			sprintf(texte, "\"%s\" is a plugin for stampTV, not an application!\n\n"
				"You should move it in the folder:\n%s\n\n"
				"I won't do that for you, because there is already a plugin with this name there.",
				addonName, target_path.Path());
			(new BAlert("", texte, "OK"))->Go();
		} else {
			sprintf(texte, "\"%s\" is a plugin for stampTV, not an application!\n\n"
				"You should move it in the folder:\n%s\n\nShall I do this for you?",
				addonName, target_path.Path());
			int32 reponse=(new BAlert("", texte, "No", "Yes, please!"))->Go();
			if (reponse==1) {
				BDirectory	target(target_path.Path());
				addon.MoveTo(&target);
			}
		}
	}
	Quit();
}

int main()
{ 
	PluginApplication addon;
	addon.Run();
	return 0;
}