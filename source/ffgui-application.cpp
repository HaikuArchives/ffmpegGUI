/*
 * Copyright 2003-2022, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Zach Dykstra,2003
 * Humdinger, humdingerb@gmail.com, 2022-2023
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022
*/


#include "ffgui-application.h"
#include "messages.h"

#include <AboutWindow.h>
#include <AppFileInfo.h>
#include <Catalog.h>
#include <Resources.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Application"


const char* kAppSignature = "application/x-vnd.HaikuArchives-ffmpegGUI";


ffguiapp::ffguiapp()
	:
	BApplication(kAppSignature)
{
	fWindow = new ffguiwin(BRect(0, 0, 0, 0), B_TRANSLATE_SYSTEM_NAME("ffmpegGUI"), B_TITLED_WINDOW,
		B_NOT_V_RESIZABLE);
	fWindow->Show();
}


void
ffguiapp::RefsReceived(BMessage* message)
{
	fWindow->PostMessage(message);
}


void
ffguiapp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
ffguiapp::AboutRequested()
{
	BAboutWindow* aboutwindow
		= new BAboutWindow(B_TRANSLATE_SYSTEM_NAME("ffmpegGUI"), kAppSignature);

	const char* authors[] = {
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
	extra_info << B_TRANSLATE(
		"Thanks to:\n"
		"mmu_man, Jeremy, DeadYak, Marco, etc.\n"
		"md@geekport.com\n"
		"reds <reds@sakamoto.pl> for making it more or less usable.\n"
		"Have fun!");

	aboutwindow->AddCopyright(2003, "Zach Dykstra");
	aboutwindow->AddAuthors(authors);
	aboutwindow->AddDescription(B_TRANSLATE("A GUI frontend for ffmpeg"));
	aboutwindow->AddExtraInfo(extra_info.String());
	aboutwindow->Show();
}
