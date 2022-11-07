/*
 * Copyright 2003, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	ffgui-application.cpp , 1/06/03
	Zach Dykstra
*/

// new app object

#include <stdio.h>
#include "ffgui-application.h"
#include "ffgui-window.h"
#include "messages.h"
#include <AboutWindow.h>
#include <Resources.h>
#include <AppFileInfo.h>


const char *kAppSignature = "application/x-vnd.HaikuArchives-ffgui";

ffguiapp::ffguiapp(char *id)
	: BApplication(id)
{
	ffguiwin *window;
	window = new ffguiwin(BRect(0,0,0,0),"ffmpeg GUI",B_TITLED_WINDOW,0);
	window->Show();
}



//message received
void ffguiapp::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
		default:
			printf("recieved by app:\n");
			message->PrintToStream();
			printf("\n");
			BApplication::MessageReceived(message);
			break;
	}
}


void
ffguiapp::AboutRequested()
{

	BAboutWindow *aboutwindow = new BAboutWindow("ffmpeg GUI", kAppSignature);

	const char *authors[] =
	{
		"Zach Dykstra",
		NULL
	};

	BString extra_info;
	extra_info << 	"  Thanks to mmu_man, Jeremy, DeadYak, Marco, etc...\n"
					"  md@geekport.com\n"
					"  made more or less usable by reds <reds@sakamoto.pl> - have fun! ";
	/*
	BResources *appresource = BApplication::AppResources();
	size_t size;
	version_info *appversion = (version_info *)appresource->LoadResource('APPV',1,&size);
	BString version_string;
	version_string<<appversion->major;
	version_string+=".";
	version_string<<appversion->middle;
	version_string+=".";
	version_string<<appversion->minor;
	*/

	aboutwindow->AddCopyright(2003, "Zach Dykstra");
	aboutwindow->AddAuthors(authors);
	//aboutwindow->SetVersion(version_string.String());
	aboutwindow->AddDescription("a GUI frontend for ffmpeg");
	aboutwindow->AddExtraInfo(extra_info.String());
	aboutwindow->Show();

}

