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

ffguiapp::ffguiapp(char *id)
	: BApplication(id)
{
	ffguiwin *window;
	window = new ffguiwin(BRect(0,0,0,0),"ffmpeg GUI",B_TITLED_WINDOW,B_AUTO_UPDATE_SIZE_LIMITS);
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
