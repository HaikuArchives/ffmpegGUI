#ifndef FFGUI_WINDOW_H
#define FFGUI_WINDOW_H

/*
 * Copyright 2003, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	ffgui-window.h , 1/06/03
	Zach Dykstra
	Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022
*/

#include <Window.h>


class BView;
class BTextView;
class BTextControl;
class BButton;
class BSpinner;
class BCheckBox;
class BPopUpMenu;
class BMenuField;
class BMenuBar;
class BTabView;
class BStatusBar;
class BFilePanel;
class BString;
class CommandLauncher;
class ffguispinner;


class ffguiwin : public BWindow
{
	public:
			ffguiwin(BRect, const char* name,window_type type,ulong mode);
			void BuildLine();
			virtual bool	QuitRequested();
			virtual void MessageReceived(BMessage* message);


	private:
			void set_playbuttons_state();
			bool file_exists(const char* filepath);
			void set_filetype(entry_ref* ref);
			void set_encodebutton_state();
			void set_outputfile_extension();
			int32 get_seconds(BString& time_string);
			void set_spinner_minsize(BSpinner *spinner);
			void play_video(const char* filepath);
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
			BButton *sourceplaybutton;
			BButton *outputplaybutton;
			BButton *encodebutton;
			// spin buttons
			ffguispinner *vbitrate;
			ffguispinner *framerate;
			ffguispinner *xres;
			ffguispinner *yres;
			ffguispinner *topcrop;
			ffguispinner *bottomcrop;
			ffguispinner *leftcrop;
			ffguispinner *rightcrop;
			ffguispinner *ac;
			// advanced spinbuttons
			ffguispinner *fixedquant;
			ffguispinner *minquant;
			ffguispinner *maxquant;
			ffguispinner *quantdifference;
			ffguispinner *quantblur;
			ffguispinner *quantcompression;
			ffguispinner *bframes;
			ffguispinner *gop;
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
			BPopUpMenu *abpopup;
			BMenuField *ab;
			BPopUpMenu *arpopup;
			BMenuField *ar;

			// tab view
			BTabView	*tabview;

			//progress bar
			int32 encode_duration;
			int32 encode_time;
			bool duration_detected;
			BCheckBox *fPlayCheck;
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

#endif
