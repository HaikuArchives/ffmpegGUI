/*
 * Copyright 2003, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	ffgui-window.h , 1/06/03
	Zach Dykstra
*/
#include "commandlauncher.h"

#include <View.h>
#include <Window.h>
#include <TextView.h>
#include <OutlineListView.h>
#include <TextControl.h>
#include <Button.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <Spinner.h>
#include <String.h>
#include <FilePanel.h>
#include <TabView.h>
#include <StringList.h>
#include <StatusBar.h>
#include <MenuBar.h>

#include <string.h>
#include <stdio.h>


class ffguiwin : public BWindow
{
	public:
			ffguiwin(BRect, const char*,window_type,ulong);
			void BuildLine();
			virtual bool	QuitRequested();
			virtual void MessageReceived(BMessage*);


	private:
			void set_encodebutton_state();
			void set_outputfile_extension();
			int32 get_seconds(BString& time_string);
			void set_spinner_minsize(BSpinner *spinner);

			//main view
			BView *topview;
			// text views
			BTextView *outputtext;
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
			BCheckBox *customres;
			// popup menus
			BPopUpMenu *outputfileformatpopup;
			BMenuField *outputfileformat;
			BPopUpMenu *outputvideoformatpopup;
			BMenuField *outputvideoformat;
			BPopUpMenu *outputaudioformatpopup;
			BMenuField *outputaudioformat;
			// tab view
			BTabView	*tabview;

			//progress bar
			int32 encode_duration;
			int32 encode_time;
			bool duration_detected;
			BStatusBar *fStatusBar;

			//menu bar
			BMenuBar *fTopMenuBar;

			// bools
			bool benablevideo, benableaudio, benablecropping, bdeletesource,bcustomres;
			// bstrings
			BString commandline;
			// file panels
			BFilePanel *fSourceFilePanel;
			BFilePanel *fOutputFilePanel;

			CommandLauncher *fCommandLauncher;

};
