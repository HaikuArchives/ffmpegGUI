/*
 * Copyright 2003, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 	ffgui-application.h , 1/06/03
 	Zach Dykstra
	Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022
*/

#include "Application.h"


class ffguiapp : public BApplication
{
	public: ffguiapp();
			virtual void MessageReceived(BMessage* message);
			void AboutRequested();
};

