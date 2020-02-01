/* 
 	ffgui-application.h , 1/06/03
 	Zach Dykstra
*/

#include "MApplication.h"


class ffguiapp : public MApplication
{
	public: ffguiapp(char *);
			virtual void MessageReceived(BMessage*);
};

