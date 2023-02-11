/*
 * Copyright 2003-2023, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Zach Dykstra,2003
 * Humdinger, humdingerb@gmail.com, 2022-2023
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022-2023
*/


#ifndef FFGUI_WINDOW_H
#define FFGUI_WINDOW_H


#include <Invoker.h>
#include <Window.h>
#include <vector>


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


enum format_capability {
	CAP_AUDIO_VIDEO,
	CAP_AUDIO_ONLY
};


class ContainerOption {
public:
					ContainerOption(const BString& option, const BString& extension,
						const BString& description, format_capability capability);
	BString 		Option;
	BString 		Extension;
	BString 		Description;
	format_capability Capability;
};


class CodecOption {
public:
					CodecOption(const BString& option, const BString& shortlabel,
						const BString& description);
	BString 		Option;
	BString Shortlabel;
	BString 		Description;
};


class ffguiwin : public BWindow {
public:
					ffguiwin(BRect, const char* name, window_type type, ulong mode);
	virtual bool 	QuitRequested();
	virtual void 	MessageReceived(BMessage* message);

private:
	void 			BuildLine();

	void 			GetMediaInfo();
	void 			UpdateMediaInfo();
	void 			ParseMediaOutput();

	void 			AdoptDefaults();
	void			SetDefaults();
	void			PopulateCodecOptions();

	bool			FileExists(const char* filepath);
	void 			SetFileExtension();
	void 			SetFiletype(entry_ref* ref);

	int32 			GetSeconds(BString& time_string);
	void 			RemoveOverPrecision(BString& float_string);

	void 			SetSpinnerMinsize(BSpinner* spinner);
	void 			SetSpinnerMinsize(BDecimalSpinner* spinner);

	void 			ReadyToEncode();
	void 			PlayVideo(const char* filepath);

	void 			SetPlaybuttonsState();
	void 			ToggleVideo();
	void 			ToggleCropping();
	void 			ToggleAudio();

	// text views
	BTextView* 		fLogView;
	BStringView*	fMediaInfoView;
	BStringView* 	fOutputCheckView;

	// text controls
	BTextControl* 	fSourceTextControl;
	BTextControl* 	fOutputTextControl;
	BTextControl* 	fCommandlineTextControl;

	// buttons
	BButton* 		fSourceButton;
	BButton* 		fOutputButton;
	BButton* 		fSourcePlayButton;
	BButton*		fOutputPlayButton;
	BButton* 		fStartAbortButton;

	// spin buttons
	ffguispinner* 	fVideoBitrateSpinner;
	ffguidecspinner* fFramerate;
	ffguispinner* 	fXres;
	ffguispinner* 	fYres;
	ffguispinner* 	fTopCrop;
	ffguispinner* 	fBottomCrop;
	ffguispinner* 	fLeftCrop;
	ffguispinner* 	fRightCrop;
	ffguispinner* 	fChannelCount;

	// advanced spinbuttons
	ffguispinner* 	fFixedQuantizer;
	ffguispinner* 	fMinQuantizer;
	ffguispinner* 	fMaxQuantizer;
	ffguispinner* 	fQuantDiff;
	ffguispinner* 	fQuantBlur;
	ffguispinner* 	fQuantCompression;
	ffguispinner* 	fBFrames;
	ffguispinner* 	fGop;

	// advanced checkboxes
	BCheckBox* 		fHighQualityBox;
	BCheckBox* 		fFourMotionBox;
	BCheckBox* 		fDeinterlaceBox;
	BCheckBox* 		fCalcNpsnrBox;

	// checkboxes
	BCheckBox* 		fEnableVideoBox;
	BCheckBox* 		fEnableAudioBox;
	BCheckBox* 		fEnableCropBox;
	BCheckBox* 		fCustomResolutionBox;

	// popup menus
	BPopUpMenu* 	fFileFormatPopup;
	BMenuField*	 	fFileFormat;
	BPopUpMenu* 	fVideoFormatPopup;
	BMenuField* 	fVideoFormat;
	BPopUpMenu* 	fAudioFormatPopup;
	BMenuField* 	fAudioFormat;
	BPopUpMenu* 	fAudioBitsPopup;
	BMenuField* 	fAudioBits;
	BPopUpMenu* 	fSampleratePopup;
	BMenuField* 	fSamplerate;

	// tab view
	BTabView*		fTabView;

	// progress bar
	int32 			fEncodeDuration;
	int32 			fEncodeTime;
	BCheckBox* 		fPlayFinishedBox;
	BStatusBar* 	fStatusBar;
	time_t 			fEncodeStartTime;

	// menu bar
	BMenuItem* 		fMenuPlaySource;
	BMenuItem* 		fMenuPlayOutput;
	BMenuItem* 		fMenuStartEncode;
	BMenuItem* 		fMenuStopEncode;
	BMenuItem* 		fMenuDefaults;

	// bstrings
	BString 		fCommand;
	BString 		fMediainfo;

	// ffprobe stream tags
	BString 		fVideoCodec;
	BString 		fAudioCodec;
	BString 		fVideoWidth;
	BString 		fVideoHeight;
	BString 		fVideoFramerate;
	BString 		fDuration;
	BString 		fVideoBitrate;
	BString 		fAudioBitrate;
	BString 		fAudioSamplerate;
	BString 		fAudioChannels;
	BString 		fAudioChannelLayout;

	// file panels
	BFilePanel* 	fSourceFilePanel;
	BFilePanel* 	fOutputFilePanel;

	// alerts
	BAlert* 		fStopAlert;
	BInvoker 		fAlertInvoker;

	// menu options for container format, video and audio codecs
	std::vector<ContainerOption> fContainerFormats;
	std::vector<CodecOption> fVideoCodecs;
	std::vector<CodecOption> fAudioCodecs;

	CommandLauncher* fCommandLauncher;
};

#endif // FFGUI_WINDOW_H
