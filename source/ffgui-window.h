/*
	ffgui-window.h , 1/06/03
	Zach Dykstra
*/
#include "View.h"
#include "MWindow.h"
#include "VGroup.h"
#include "HGroup.h"
#include "TabGroup.h"
#include "LayeredGroup.h"
#include "MRadioGroup.h"
#include "MTextView.h"
#include "MOutlineListView.h"
#include "MTextControl.h"
#include "MSplitter.h"
#include "MButton.h"
#include "MCheckBox.h"
#include "Space.h"
#include "MPopup.h"
#include "MBorder.h"
#include "MDragBar.h"
#include "MProgressBar.h"
#include "PropGadget.h"
#include "MScrollView.h"
#include "SpinButton.h"
#include "String.h"

#include <string.h>
#include <stdio.h>

class ffguiwin : public MWindow
{
	public: 
			ffguiwin(BRect,char*,window_type,ulong);
			ffguiwin(BRect, char*, window_look, window_feel, ulong);
			void BuildLine();
			virtual bool	QuitRequested();
			virtual void MessageReceived(BMessage*);


//	private:
			//main view
			MView *topview; 
			// text views
			MTextView *abouttext;
			// text controls
			MTextControl *sourcefile; 
			MTextControl *outputfile;
			MTextControl *encode;
			// spin buttons 
			SpinButton *vbitrate; 
			SpinButton *framerate;
			SpinButton *xres;
			SpinButton *yres;
			SpinButton *topcrop;
			SpinButton *bottomcrop;
			SpinButton *leftcrop;
			SpinButton *rightcrop;
			SpinButton *ab;
			SpinButton *ar;
			SpinButton *ac;
			// advanced spinbuttons
			SpinButton *fixedquant;
			SpinButton *minquant;
			SpinButton *maxquant;
			SpinButton *quantdifference;
			SpinButton *quantblur;
			SpinButton *quantcompression;
			SpinButton *bframes;
			SpinButton *gop;
			// advanced checkboxes
			MCheckBox *highquality;
			MCheckBox *fourmotion;
			MCheckBox *deinterlace;
			MCheckBox *calcpsnr;
			// checkboxes
			MCheckBox *enablevideo;
			MCheckBox *enableaudio;
			MCheckBox *enablecropping;
			MCheckBox *deletesource;
			MCheckBox *customres;
			// popup menus
			MPopup *outputfileformat;
			MPopup *outputvideoformat;
			MPopup *outputaudioformat;
			//progress bar
			MProgressBar *encodestatus;
			// bools
			bool benablevideo, benableaudio, benablecropping, bdeletesource,bcustomres;
			// bstrings
			BString *commandline;
};
