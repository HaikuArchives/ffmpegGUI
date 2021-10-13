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
	: MApplication(id)
{
	ffguiwin *window;
	window = new ffguiwin(BRect(20,20,150,150),"ffmpeg GUI",B_TITLED_WINDOW,0);
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
			MApplication::MessageReceived(message);
			break;
	}
}
