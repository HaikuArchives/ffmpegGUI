/*
 * Copyright 2003-2022, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Zach Dykstra,2003
 * Humdinger, humdingerb@gmail.com, 2022-2023
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022
*/


#ifndef FFGUI_APPLICATION_H
#define FFGUI_APPLICATION_H


#include "ffgui-window.h"

#include <Application.h>


class ffguiapp : public BApplication
{
public:
					ffguiapp();
	virtual void 	RefsReceived(BMessage* message);
	virtual void 	MessageReceived(BMessage* message);
			void 	AboutRequested();

private:
	ffguiwin* 		fWindow;
};

#endif // FFGUI_APPLICATION_H
