#ifndef FFGUI_WINDOW_H
#define FFGUI_WINDOW_H

/*
 * Copyright 2003, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	ffgui-window.h , 1/06/03
	Zach Dykstra
	Humdinger, humdingerb@gmail.com, 2022
	Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022
*/

#include <Invoker.h>
#include <Window.h>


class BView;
class BTextView;
class BTextControl;
class BAlert;
class BButton;
class BDecimalSpinner;
class BSpinner;
class BCheckBox;
class BPopUpMenu;
class BMenuField;
class BMenuBar;
class BTabView;
class BStatusBar;
class BFilePanel;
class BString;
class BStringView;
class CommandLauncher;
class ffguispinner;
class ffguidecspinner;


class ffguiwin : public BWindow
{
	public:
			ffguiwin(BRect, const char* name,window_type type,ulong mode);
			void BuildLine();
			virtual bool	QuitRequested();
			virtual void 	MessageReceived(BMessage* message);


	private:
			void get_media_info();
			void parse_media_output();
			void adopt_defaults();
			void set_defaults();
			void update_media_info();
			void remove_over_precision(BString& float_string);
			void set_playbuttons_state();
			bool file_exists(const char* filepath);
			void set_filetype(entry_ref* ref);
			void is_ready_to_encode();
			void set_outputfile_extension();
			int32 get_seconds(BString& time_string);
			void set_spinner_minsize(BSpinner *spinner);
			void set_spinner_minsize(BDecimalSpinner *spinner);
			void play_video(const char* filepath);
			void toggle_video();
			void toggle_cropping();
			void toggle_audio();


			//main view
			BView *topview;
			// text views
			BTextView *outputtext;
			BStringView *mediainfo;
			BStringView *outputcheck;
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
			ffguidecspinner *framerate;
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
			BCheckBox *fPlayCheck;
			BStatusBar *fStatusBar;
			time_t encode_starttime;

			//menu bar
			BMenuItem* fMenuPlaySource;
			BMenuItem* fMenuPlayOutput;
			BMenuItem* fMenuStartEncode;
			BMenuItem* fMenuStopEncode;
			BMenuItem* fMenuDefaults;

			// bstrings
			BString commandline;
			BString fMediainfo;

			// ffprobe stream tags
			BString fVideoCodec;
			BString fAudioCodec;
			BString fVideoWidth;
			BString fVideoHeight;
			BString fVideoFramerate;
			BString fDuration;
			BString fVideoBitrate;
			BString fAudioBitrate;
			BString fAudioSamplerate;
			BString fAudioChannels;
			BString fAudioChannelLayout;

			// file panels
			BFilePanel *fSourceFilePanel;
			BFilePanel *fOutputFilePanel;

			// alerts
			BAlert* fStopAlert;
			BInvoker fAlertInvoker;

			CommandLauncher *fCommandLauncher;
};

#endif
