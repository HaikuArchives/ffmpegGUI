/*
 * Copyright 2003-2023, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Zach Dykstra,2003
 * Humdinger, humdingerb@gmail.com, 2022-2023
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022-2023
*/


#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <Invoker.h>
#include <Path.h>
#include <Window.h>

#include <vector>


class BAlert;
class BButton;
class BCheckBox;
class BDecimalSpinner;
class BFilePanel;
class BMenuBar;
class BMenuField;
class BPath;
class BPopUpMenu;
class BSpinner;
class BStatusBar;
class BString;
class BStringView;
class BTabView;
class BTab;
class BTextControl;
class BTextView;
class BView;


class CodecOption;
class ContainerOption;
class CommandLauncher;
class Spinner;
class DecSpinner;
class JobWindow;
class CropView;


class MainWindow : public BWindow {
public:
					MainWindow(BRect, const char* name, window_type type, ulong mode);

	virtual bool 	QuitRequested();
	virtual void 	MessageReceived(BMessage* message);

private:
	status_t		_LoadSettings(BMessage& settings);
	status_t		_SaveSettings();

	BMenuBar*		_BuildMenu();
	BView* 			_BuildFileOptions();
	BView*  		_BuildMainOptions();
	BView*			_BuildCroppingOptions();
	BView*  		_BuildAdvancedOptions();
	void	 		_BuildLogView();
	BView*  		_BuildEncodeProgress();

	void 			_BuildLine();

	void 			_GetMediaInfo();
	void 			_UpdateMediaInfo();
	void 			_ParseMediaOutput();

	void 			_ExtractPreviewImage();
	void			_DeleteTempFiles();

	void 			_AdoptDefaults();
	void			_SetDefaults();
	void			_PopulateCodecOptions();

	bool			_FileExists(const char* filepath);
	void 			_SetFileExtension();
	void 			_SetFiletype(entry_ref* ref);

	int32 			_GetSeconds(BString& time_string);
	void 			_RemoveOverPrecision(BString& float_string);

	void 			_ReadyToEncode();
	void 			_PlayVideo(const char* filepath);

	void 			_SetPlaybuttonsState();
	void 			_ToggleVideo();
	void 			_ToggleCropping();
	void 			_ToggleAudio();

	// text views
	BTextView* 		fLogView;
	BStringView*	fMediaInfoView;
	BStringView* 	fOutputCheckView;

	// misc views
	CropView*		fCropView;
	int32 			fCurrentCropImageIndex;
	int32 			fCropImageCount;

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
	BButton* 		fResetCroppingButton;
	BButton* 		fNewPreviewButton;

	// spin buttons
	Spinner* 	fVideoBitrateSpinner;
	DecSpinner* fFramerate;
	Spinner* 	fXres;
	Spinner* 	fYres;
	Spinner* 	fTopCrop;
	Spinner* 	fBottomCrop;
	Spinner* 	fLeftCrop;
	Spinner* 	fRightCrop;
	Spinner* 	fChannelCount;

	// advanced spinbuttons
	Spinner* 	fFixedQuantizer;
	Spinner* 	fMinQuantizer;
	Spinner* 	fMaxQuantizer;
	Spinner* 	fQuantDiff;
	Spinner* 	fQuantBlur;
	Spinner* 	fQuantCompression;
	Spinner* 	fBFrames;
	Spinner* 	fGop;

	// advanced checkboxes
	BCheckBox* 		fHighQualityBox;
	BCheckBox* 		fFourMotionBox;
	BCheckBox* 		fDeinterlaceBox;
	BCheckBox* 		fCalcNpsnrBox;

	// checkboxes
	BCheckBox* 		fEnableVideoBox;
	BCheckBox* 		fEnableAudioBox;
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

	// tab view and tabs
	BTabView*		fTabView;
	BTab*			fOptionsTab;
	BTab*			fCroppingTab;
	BTab*			fAdvancedTab;
	BTab*			fLogTab;

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
	BMenuItem* 		fMenuAddJob;
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

	BPath			fPreviewPath;
	// alerts
	BAlert* 		fStopAlert;
	BInvoker 		fAlertInvoker;

	// menu options for container format, video and audio codecs
	std::vector<ContainerOption> fContainerFormats;
	std::vector<CodecOption> fVideoCodecs;
	std::vector<CodecOption> fAudioCodecs;

	CommandLauncher* fCommandLauncher;
	JobWindow*		fJobWindow;
};

#endif // MAINWINDOW_H
