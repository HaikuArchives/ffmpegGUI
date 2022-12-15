/*
 * Copyright 2003-2022, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	ffgui-application.cpp , 1/06/03
	Zach Dykstra
	Humdinger, humdingerb@gmail.com, 2022
*/

// new app object

#include "ffgui-application.h"
#include "ffgui-window.h"
#include "messages.h"
#include <stdio.h>

#include <AboutWindow.h>
#include <AppFileInfo.h>
#include <Catalog.h>
#include <Resources.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Application"


const char *kAppSignature = "application/x-vnd.HaikuArchives-ffmpegGUI";

ffguiapp::ffguiapp()
	: BApplication(kAppSignature)
{
	ffguiwin *window;
	window = new ffguiwin(BRect(0,0,0,0),B_TRANSLATE_SYSTEM_NAME("ffmpeg GUI"),B_TITLED_WINDOW,B_NOT_V_RESIZABLE);
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
	BAboutWindow *aboutwindow = new BAboutWindow(B_TRANSLATE_SYSTEM_NAME("ffmpeg GUI"), kAppSignature);

	const char *authors[] =
	{
		"2003 Zach Dykstra",
		"2015,2018 diversys",
		"2020 Dominika Liberda (sdomi)",
		"2020 waddlesplash",
		"2020 Jacob Secunda (CoderforEvolution)",
		"2018,2021 Scott McCreary (scottmc)",
		"2022 Andi Machovec (BlueSky)",
		NULL
	};

	BString extra_info;
	extra_info << 	B_TRANSLATE("Thanks to mmu_man, Jeremy, DeadYak, Marco, etc...\n"
					"md@geekport.com\n"
					"made more or less usable by reds <reds@sakamoto.pl> - have fun!");

	aboutwindow->AddCopyright(2003, "Zach Dykstra");
	aboutwindow->AddAuthors(authors);
	aboutwindow->AddDescription(B_TRANSLATE("A GUI frontend for ffmpeg"));
	aboutwindow->AddExtraInfo(extra_info.String());
	aboutwindow->Show();
}
