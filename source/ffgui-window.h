/*
 * Copyright 2003, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	ffgui-window.h , 1/06/03
	Zach Dykstra
*/

#include "View.h"
#include "Window.h"
#include "TextView.h"
#include "OutlineListView.h"
#include "TextControl.h"
#include "Splitter.h"
#include "Button.h"
#include "CheckBox.h"
#include <MenuField.h>
#include <PopUpMenu.h>
#include "Spinner.h"
#include "String.h"

#include <string.h>
#include <stdio.h>

//#include "VGroup.h"
//#include "HGroup.h"
//#include "TabGroup.h"
//#include "LayeredGroup.h"
//#include "MRadioGroup.h"
//#include "Space.h"
//#include "MBorder.h"
//#include "MDragBar.h"
//#include "MProgressBar.h"
//#include "PropGadget.h"
//#include "MScrollView.h"


class ffguiwin : public BWindow
{
	public: 
			ffguiwin(BRect,char*,window_type,ulong);
			ffguiwin(BRect, char*, window_look, window_feel, ulong);
			void BuildLine();
			virtual bool	QuitRequested();
			virtual void MessageReceived(BMessage*);


//	private:
			//main view
			BView *topview; 
			// text views
			BTextView *abouttext;
			// text controls
			BTextControl *sourcefile; 
			BTextControl *outputfile;
			BTextControl *encode;
			//buttons
			BButton *sourcefilebutton;
			BButton *outputfilebutton;
			BButton *encodebutton;
			// spin buttons 
			BSpinner *vbitrate; 
			BSpinner *framerate;
			BSpinner *xres;
			BSpinner *yres;
			BSpinner *topcrop;
			BSpinner *bottomcrop;
			BSpinner *leftcrop;
			BSpinner *rightcrop;
			BSpinner *ab;
			BSpinner *ar;
			BSpinner *ac;
			// advanced spinbuttons
			BSpinner *fixedquant;
			BSpinner *minquant;
			BSpinner *maxquant;
			BSpinner *quantdifference;
			BSpinner *quantblur;
			BSpinner *quantcompression;
			BSpinner *bframes;
			BSpinner *gop;
			// advanced checkboxes
			BCheckBox *highquality;
			BCheckBox *fourmotion;
			BCheckBox *deinterlace;
			BCheckBox *calcpsnr;
			// checkboxes
			BCheckBox *enablevideo;
			BCheckBox *enableaudio;
			BCheckBox *enablecropping;
			BCheckBox *deletesource;
			BCheckBox *customres;
			// popup menus
			BPopUpMenu *outputfileformatpopup;
			BMenuField *outputfileformat;
			BPopUpMenu *outputvideoformatpopup;
			BMenuField *outputvideoformat;
			BPopUpMenu *outputaudioformatpopup;
			BMenuField *outputaudioformat;
			
			//progress bar
			//MProgressBar *encodestatus;
			// bools
			bool benablevideo, benableaudio, benablecropping, bdeletesource,bcustomres;
			// bstrings
			BString *commandline;
};
