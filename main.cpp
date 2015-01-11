/*
	ffgui-main.cpp , 1/06/03
	Zach Dykstra
*/
#include <stdio.h>

#include "ffgui-application.h"
#include "ffgui-window.h"

int main(void)
{
	ffguiapp app("application/x-md-ffguiapp");
	app.Run();
	return(0);
}
