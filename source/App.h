/*
 * Copyright 2003-2022, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Zach Dykstra,2003
 * Humdinger, humdingerb@gmail.com, 2022-2023
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022
*/


#ifndef APPLICATION_H
#define APPLICATION_H


#include "MainWindow.h"

#include <Application.h>


class App : public BApplication {
public:
					App();

	virtual void 	RefsReceived(BMessage* message);
	virtual void 	MessageReceived(BMessage* message);
			void 	AboutRequested();

private:
	MainWindow* 		fMainWindow;
};

#endif // APPLICATION_H
