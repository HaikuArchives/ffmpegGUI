/*
 * Copyright 2003-2023, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Zach Dykstra,2003
 * Humdinger, humdingerb@gmail.com, 2022-2023
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022-2023
*/


#include "ffgui-window.h"
#include "DurationToString.h"
#include "commandlauncher.h"
#include "ffgui-application.h"
#include "ffgui-spinner.h"
#include "messages.h"

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <Clipboard.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <Notification.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <Spinner.h>
#include <StatusBar.h>
#include <String.h>
#include <StringList.h>
#include <StringView.h>
#include <TabView.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>

#include <cstdlib>
#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Window"

static const char* kIdleText = B_TRANSLATE_MARK("Waiting to start encoding" B_UTF8_ELLIPSIS);
static const char* kEmptySource = B_TRANSLATE_MARK("Please select a source file.");
static const char* kSourceDoesntExist = B_TRANSLATE_MARK("There's no file with that name.");
static const char* kOutputExists
	= B_TRANSLATE_MARK("This file already exists. It will be overwritten!");
static const char* kOutputIsSource
	= B_TRANSLATE_MARK("Cannot overwrite the source file. Please choose "
					   "another output file name.");


ContainerOption::ContainerOption(const BString& option, const BString& extension,
	const BString& description, format_capability capability)
	:
	Option(option),
	Extension(extension),
	Description(description),
	Capability(capability)
{
}


CodecOption::CodecOption(const BString& option, const BString& description)
	:
	Option(option),
	Description(description)
{
}


ffguiwin::ffguiwin(BRect r, const char* name, window_type type, ulong mode)
	:
	BWindow(r, name, type, mode)
{
	// Invoker for the Alerts to use to send their messages to the timer
	fAlertInvoker.SetMessage(new BMessage(M_STOP_ALERT_BUTTON));
	fAlertInvoker.SetTarget(this);

	fEncodeStartTime = 0; // 0 means: no encoding in progress

	// initialize GUI elements
	fSourceButton = new BButton(B_TRANSLATE("Source file"), new BMessage(M_SOURCE));
	fSourceButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	fSourceTextControl = new BTextControl("", "", new BMessage('srcf'));
	fSourceTextControl->SetModificationMessage(new BMessage(M_SOURCEFILE));

	fMediaInfoView = new BStringView("mediainfo", B_TRANSLATE_NOCOLLECT(kEmptySource));
	fMediaInfoView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	BFont font(be_plain_font);
	font.SetSize(ceilf(font.Size() * 0.9));
	fMediaInfoView->SetFont(&font, B_FONT_SIZE);

	fOutputButton = new BButton(B_TRANSLATE("Output file"), new BMessage(M_OUTPUT));
	fOutputButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	fOutputTextControl = new BTextControl("", "", new BMessage('outf'));
	fOutputTextControl->SetModificationMessage(new BMessage(M_OUTPUTFILE));

	fOutputCheckView = new BStringView("outputcheck", "");
	fOutputCheckView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fOutputCheckView->SetFont(&font, B_FONT_SIZE);

	fSourcePlayButton = new BButton("⯈", new BMessage(M_PLAY_SOURCE));
	fOutputPlayButton = new BButton("⯈", new BMessage(M_PLAY_OUTPUT));
	float height;
	fSourceTextControl->GetPreferredSize(NULL, &height);
	BSize size(height, height);
	fSourcePlayButton->SetExplicitSize(size);
	fOutputPlayButton->SetExplicitSize(size);
	fSourcePlayButton->SetEnabled(false);
	fOutputPlayButton->SetEnabled(false);

	PopulateCodecOptions();
	fFileFormatPopup = new BPopUpMenu("");
	std::vector<ContainerOption>::iterator container_iter;
	for (container_iter = fContainerFormats.begin(); container_iter != fContainerFormats.end();
		++container_iter) {
		fFileFormatPopup->AddItem(
			new BMenuItem(container_iter->Description.String(), new BMessage(M_OUTPUTFILEFORMAT)));
	}

	fFileFormatPopup->ItemAt(0)->SetMarked(true);
	fFileFormat = new BMenuField(NULL, fFileFormatPopup);

	fVideoFormatPopup = new BPopUpMenu("");
	std::vector<CodecOption>::iterator codec_iter;
	for (codec_iter = fVideoCodecs.begin(); codec_iter != fVideoCodecs.end(); ++codec_iter) {
		fVideoFormatPopup->AddItem(
			new BMenuItem(codec_iter->Description.String(), new BMessage(M_OUTPUTVIDEOFORMAT)));
	}
	fVideoFormatPopup->ItemAt(0)->SetMarked(true);
	fVideoFormat = new BMenuField(B_TRANSLATE("Video codec:"), fVideoFormatPopup);

	float popup_width;
	fVideoFormatPopup->GetPreferredSize(&popup_width, nullptr);
	fVideoFormat->CreateMenuBarLayoutItem()->SetExplicitMinSize(
		BSize(popup_width, B_SIZE_UNSET));

	fAudioFormatPopup = new BPopUpMenu("");
	for (codec_iter = fAudioCodecs.begin(); codec_iter != fAudioCodecs.end(); ++codec_iter) {
		fAudioFormatPopup->AddItem(
			new BMenuItem(codec_iter->Description.String(), new BMessage(M_OUTPUTAUDIOFORMAT)));
	}
	fAudioFormatPopup->ItemAt(0)->SetMarked(true);
	fAudioFormat = new BMenuField(B_TRANSLATE("Audio codec:"), fAudioFormatPopup);
	fAudioFormatPopup->GetPreferredSize(&popup_width, nullptr);
	fAudioFormat->CreateMenuBarLayoutItem()->SetExplicitMinSize(
		BSize(popup_width, B_SIZE_UNSET));

	fEnabelVideoBox
		= new BCheckBox("", B_TRANSLATE("Enable video encoding"), new BMessage(M_ENABLEVIDEO));
	fEnabelVideoBox->SetValue(B_CONTROL_ON);
	fVideoBitrateSpinner = new ffguispinner("", B_TRANSLATE("Bitrate (Kbit/s):"), new BMessage(M_VBITRATE));
	fFramerate = new ffguidecspinner("", B_TRANSLATE("Framerate (fps):"), new BMessage(M_FRAMERATE));
	fCustomResolutionBox = new BCheckBox("", B_TRANSLATE("Use custom resolution"), new BMessage(M_CUSTOMRES));
	fXres = new ffguispinner("", B_TRANSLATE("Width:"), new BMessage(M_XRES));
	fYres = new ffguispinner("", B_TRANSLATE("Height:"), new BMessage(M_YRES));

	fEnabelCropBox
		= new BCheckBox("", B_TRANSLATE("Enable video cropping"), new BMessage(M_ENABLECROPPING));
	fEnabelCropBox->SetValue(B_CONTROL_OFF);
	fTopCrop = new ffguispinner("", B_TRANSLATE("Top:"), new BMessage(M_TOPCROP));
	fBottomCrop = new ffguispinner("", B_TRANSLATE("Bottom:"), new BMessage(M_BOTTOMCROP));
	fLeftCrop = new ffguispinner("", B_TRANSLATE("Left:"), new BMessage(M_LEFTCROP));
	fRightCrop = new ffguispinner("", B_TRANSLATE("Right:"), new BMessage(M_RIGHTCROP));

	fEnabelAudioBox
		= new BCheckBox("", B_TRANSLATE("Enable audio encoding"), new BMessage(M_ENABLEAUDIO));
	fEnabelAudioBox->SetValue(B_CONTROL_ON);

	fAudioBitsPopup = new BPopUpMenu("");
	fAudioBitsPopup->AddItem(new BMenuItem("48", new BMessage(M_AB)));
	fAudioBitsPopup->AddItem(new BMenuItem("96", new BMessage(M_AB)));
	fAudioBitsPopup->AddItem(new BMenuItem("128", new BMessage(M_AB)));
	fAudioBitsPopup->AddItem(new BMenuItem("160", new BMessage(M_AB)));
	fAudioBitsPopup->AddItem(new BMenuItem("196", new BMessage(M_AB)));
	fAudioBitsPopup->AddItem(new BMenuItem("320", new BMessage(M_AB)));
	fAudioBitsPopup->AddItem(new BMenuItem("625", new BMessage(M_AB)));
	fAudioBitsPopup->AddItem(new BMenuItem("1411", new BMessage(M_AB)));
	fAudioBitsPopup->ItemAt(1)->SetMarked(true);
	fAudioBits = new BMenuField(B_TRANSLATE("Bitrate (Kbit/s):"), fAudioBitsPopup);
	fSampleratePopup = new BPopUpMenu("");
	fSampleratePopup->AddItem(new BMenuItem("22050", new BMessage(M_AR)));
	fSampleratePopup->AddItem(new BMenuItem("44100", new BMessage(M_AR)));
	fSampleratePopup->AddItem(new BMenuItem("48000", new BMessage(M_AR)));
	fSampleratePopup->AddItem(new BMenuItem("96000", new BMessage(M_AR)));
	fSampleratePopup->AddItem(new BMenuItem("192000", new BMessage(M_AR)));
	fSampleratePopup->ItemAt(1)->SetMarked(true);
	fSamplerate = new BMenuField(B_TRANSLATE("Sampling rate (Hz):"), fSampleratePopup);
	fChannelCount = new ffguispinner("", B_TRANSLATE("Audio channels:"), new BMessage(M_AC));

	fBFrames = new ffguispinner("", B_TRANSLATE("'B' frames:"), nullptr);
	fGop = new ffguispinner("", B_TRANSLATE("GOP size:"), nullptr);
	fHighQualityBox
		= new BCheckBox("", B_TRANSLATE("Use high quality settings"), new BMessage(M_HIGHQUALITY));
	fFourMotionBox
		= new BCheckBox("", B_TRANSLATE("Use four motion vector"), new BMessage(M_FOURMOTION));
	fDeinterlaceBox
		= new BCheckBox("", B_TRANSLATE("Deinterlace pictures"), new BMessage(M_DEINTERLACE));
	fCalcNpsnrBox = new BCheckBox(
		"", B_TRANSLATE("Calculate PSNR of compressed frames"), new BMessage(M_CALCPSNR));

	fFixedQuantizer = new ffguispinner("", B_TRANSLATE("Use fixed video quantizer scale:"), nullptr);
	fMinQuantizer = new ffguispinner("", B_TRANSLATE("Min video quantizer scale:"), nullptr);
	fMaxQuantizer = new ffguispinner("", B_TRANSLATE("Max video quantizer scale:"), nullptr);
	fQuantDiff
		= new ffguispinner("", B_TRANSLATE("Max difference between quantizer scale:"), nullptr);
	fQuantBlur = new ffguispinner("", B_TRANSLATE("Video quantizer scale blur:"), nullptr);
	fQuantCompression
		= new ffguispinner("", B_TRANSLATE("Video quantizer scale compression:"), nullptr);

	fLogView = new BTextView("");
	fLogView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fLogView->MakeEditable(false);

	fStartAbortButton = new BButton(B_TRANSLATE("Start"), new BMessage(M_ENCODE));
	fStartAbortButton->SetEnabled(false);
	fCommandlineTextControl = new BTextControl("", "", nullptr);

	fSourceFilePanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false,
		new BMessage(M_SOURCEFILE_REF));

	fOutputFilePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false,
		new BMessage(M_OUTPUTFILE_REF));

	fPlayFinishedBox = new BCheckBox("play_finished", B_TRANSLATE("Play when finished"), NULL);
	fPlayFinishedBox->SetValue(B_CONTROL_OFF);

	fStatusBar = new BStatusBar("");
	fStatusBar->SetText(B_TRANSLATE_NOCOLLECT(kIdleText));

	// set the min and max values for the spin controls
	fVideoBitrateSpinner->SetMinValue(64);
	fVideoBitrateSpinner->SetMaxValue(50000);
	fFramerate->SetMinValue(1);
	fFramerate->SetMaxValue(120);
	fXres->SetMinValue(160);
	fXres->SetMaxValue(7680);
	fYres->SetMinValue(120);
	fYres->SetMaxValue(4320);

	// set the initial values
	fVideoBitrateSpinner->SetValue(1000);
	fFramerate->SetValue(30);
	fXres->SetValue(1280);
	fYres->SetValue(720);
	fChannelCount->SetValue(2);

	// set minimum size for the spinners
	SetSpinnerMinsize(fVideoBitrateSpinner);
	SetSpinnerMinsize(fFramerate);
	SetSpinnerMinsize(fXres);
	SetSpinnerMinsize(fYres);
	SetSpinnerMinsize(fChannelCount);
	SetSpinnerMinsize(fTopCrop);
	SetSpinnerMinsize(fBottomCrop);
	SetSpinnerMinsize(fLeftCrop);
	SetSpinnerMinsize(fRightCrop);

	// set step values for the spinners
	fVideoBitrateSpinner->SetStep(100);

	// set the initial command line
	SetDefaults();
	BuildLine();

	// create tabs and boxes
	BView* fileoptionsview = new BView("fileoptions", B_SUPPORTS_LAYOUT);
	BLayoutBuilder::Group<>(fileoptionsview, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 0)
		.AddGrid(B_USE_SMALL_SPACING, 0.0)
			.Add(fSourceButton, 0, 0)
			.Add(fSourceTextControl, 1, 0, 2, 1)
			.Add(fSourcePlayButton, 3, 0)
			.Add(fMediaInfoView, 1, 1, 2, 1)
			.Add(fOutputButton, 0, 3)
			.Add(fOutputTextControl, 1, 3)
			.Add(fFileFormat, 2, 3)
			.Add(fOutputPlayButton, 3, 3)
			.Add(fOutputCheckView, 1, 4, 3, 1)
			.SetColumnWeight(0, 0)
			.SetColumnWeight(1, 1)
			.SetColumnWeight(2, 0)
			.SetColumnWeight(3, 0)
		.End();

	BView* encodeview = new BView("encodeview", B_SUPPORTS_LAYOUT);
	BLayoutBuilder::Group<>(encodeview, B_VERTICAL)
		.AddGroup(B_HORIZONTAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 0)
			.Add(fStartAbortButton)
			.Add(fCommandlineTextControl)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL));

	BBox* videobox = new BBox("");
	videobox->SetLabel(B_TRANSLATE("Video"));
	BGroupLayout* videolayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fEnabelVideoBox)
		.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
			.Add(fVideoFormat->CreateLabelLayoutItem(), 0, 0)
			.Add(fVideoFormat->CreateMenuBarLayoutItem(), 1, 0)
			.Add(fVideoBitrateSpinner->CreateLabelLayoutItem(), 0, 1)
			.Add(fVideoBitrateSpinner->CreateTextViewLayoutItem(), 1, 1)
			.Add(fFramerate->CreateLabelLayoutItem(), 0, 2)
			.Add(fFramerate->CreateTextViewLayoutItem(), 1, 2)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(fCustomResolutionBox)
		.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
			.Add(fXres->CreateLabelLayoutItem(), 0, 0)
			.Add(fXres->CreateTextViewLayoutItem(), 1, 0)
			.Add(fYres->CreateLabelLayoutItem(), 0, 1)
			.Add(fYres->CreateTextViewLayoutItem(), 1, 1)
		.End();
	videobox->AddChild(videolayout->View());

	BBox* croppingoptionsbox = new BBox("");
	croppingoptionsbox->SetLabel(B_TRANSLATE("Cropping options"));
	BGroupLayout* croppingoptionslayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
		  B_USE_DEFAULT_SPACING)
		.Add(fEnabelCropBox)
		.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
			.Add(fTopCrop->CreateLabelLayoutItem(), 0, 0)
			.Add(fTopCrop->CreateTextViewLayoutItem(), 1, 0)
			.Add(fBottomCrop->CreateLabelLayoutItem(), 0, 1)
			.Add(fBottomCrop->CreateTextViewLayoutItem(), 1, 1)
			.Add(fLeftCrop->CreateLabelLayoutItem(), 0, 2)
			.Add(fLeftCrop->CreateTextViewLayoutItem(), 1, 2)
			.Add(fRightCrop->CreateLabelLayoutItem(), 0, 3)
			.Add(fRightCrop->CreateTextViewLayoutItem(), 1, 3)
		.End()
		.AddGlue();
	croppingoptionsbox->AddChild(croppingoptionslayout->View());

	BBox* audiobox = new BBox("");
	audiobox->SetLabel(B_TRANSLATE("Audio"));
	BGroupLayout* audiolayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fEnabelAudioBox)
		.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
			.Add(fAudioFormat->CreateLabelLayoutItem(), 0, 0)
			.Add(fAudioFormat->CreateMenuBarLayoutItem(), 1, 0)
			.Add(fAudioBits->CreateLabelLayoutItem(), 0, 1)
			.Add(fAudioBits->CreateMenuBarLayoutItem(), 1, 1)
			.Add(fSamplerate->CreateLabelLayoutItem(), 0, 2)
			.Add(fSamplerate->CreateMenuBarLayoutItem(), 1, 2)
			.Add(fChannelCount->CreateLabelLayoutItem(), 0, 3)
			.Add(fChannelCount->CreateTextViewLayoutItem(), 1, 3)
		.End()
		.AddGlue();
	audiobox->AddChild(audiolayout->View());

	BView* mainoptionsview = new BView("", B_SUPPORTS_LAYOUT);
	BView* advancedoptionsview = new BView("", B_SUPPORTS_LAYOUT);
	BView* outputview = new BScrollView("", fLogView, B_SUPPORTS_LAYOUT, true, true);

	BLayoutBuilder::Group<>(mainoptionsview, B_HORIZONTAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING)
		.Add(videobox)
		.Add(croppingoptionsbox)
		.Add(audiobox)
		.Layout();

	BLayoutBuilder::Group<>(advancedoptionsview, B_HORIZONTAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING)
		.AddGroup(B_VERTICAL)
			.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
				.Add(fBFrames->CreateLabelLayoutItem(), 0, 0)
				.Add(fBFrames->CreateTextViewLayoutItem(), 1, 0)
				.Add(fGop->CreateLabelLayoutItem(), 0, 1)
				.Add(fGop->CreateTextViewLayoutItem(), 1, 1)
			.End()
			.Add(new BSeparatorView(B_HORIZONTAL))
			.Add(fHighQualityBox)
			.Add(fFourMotionBox)
			.Add(fDeinterlaceBox)
			.Add(fCalcNpsnrBox)
		.End()
		.Add(new BSeparatorView(B_VERTICAL))
		.AddGroup(B_VERTICAL)
			.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
				.Add(fFixedQuantizer->CreateLabelLayoutItem(), 0, 0)
				.Add(fFixedQuantizer->CreateTextViewLayoutItem(), 1, 0)
				.Add(fMinQuantizer->CreateLabelLayoutItem(), 0, 1)
				.Add(fMinQuantizer->CreateTextViewLayoutItem(), 1, 1)
				.Add(fMaxQuantizer->CreateLabelLayoutItem(), 0, 2)
				.Add(fMaxQuantizer->CreateTextViewLayoutItem(), 1, 2)
				.Add(fQuantDiff->CreateLabelLayoutItem(), 0, 3)
				.Add(fQuantDiff->CreateTextViewLayoutItem(), 1, 3)
				.Add(fQuantBlur->CreateLabelLayoutItem(), 0, 4)
				.Add(fQuantBlur->CreateTextViewLayoutItem(), 1, 4)
				.Add(fQuantCompression->CreateLabelLayoutItem(), 0, 5)
				.Add(fQuantCompression->CreateTextViewLayoutItem(), 1, 5)
			.End()
		.AddGlue()
		.End();

	fTabView = new BTabView("");
	BTab* mainoptionstab = new BTab();
	BTab* advancedoptionstab = new BTab();
	BTab* outputtab = new BTab();

	fTabView->AddTab(mainoptionsview, mainoptionstab);
	// fTabView->AddTab(advancedoptionsview, advancedoptionstab); //don´t remove,
	// will be needed later
	fTabView->AddTab(outputview, outputtab);
	mainoptionstab->SetLabel(B_TRANSLATE("Main options"));
	advancedoptionstab->SetLabel(B_TRANSLATE("Advanced options"));
	outputtab->SetLabel(B_TRANSLATE("Log"));

	// menu bar
	BMenuBar* menuBar = new BMenuBar("menubar");
	BMenu* menu;
	BMenuItem* item;

	menu = new BMenu(B_TRANSLATE("File"));
	item = new BMenuItem(
		B_TRANSLATE("Open source file" B_UTF8_ELLIPSIS), new BMessage(M_SOURCE), 'O');
	menu->AddItem(item);
	item = new BMenuItem(
		B_TRANSLATE("Select output file" B_UTF8_ELLIPSIS), new BMessage(M_OUTPUT), 'S');
	menu->AddItem(item);
	menu->AddSeparatorItem();
	fMenuPlaySource
		= new BMenuItem(B_TRANSLATE("Play source file"), new BMessage(M_PLAY_SOURCE), 'P');
	fMenuPlaySource->SetEnabled(false);
	menu->AddItem(fMenuPlaySource);
	fMenuPlayOutput = new BMenuItem(
		B_TRANSLATE("Play output file"), new BMessage(M_PLAY_OUTPUT), 'P', B_SHIFT_KEY);
	fMenuPlayOutput->SetEnabled(false);
	menu->AddItem(fMenuPlayOutput);
	menu->AddSeparatorItem();
	item = new BMenuItem(B_TRANSLATE("About ffmpegGUI"), new BMessage(B_ABOUT_REQUESTED));
	menu->AddItem(item);
	item = new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q');
	menu->AddItem(item);
	menuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Encoding"));
	fMenuStartEncode = new BMenuItem(B_TRANSLATE("Start encoding"), new BMessage(M_ENCODE), 'E');
	fMenuStartEncode->SetEnabled(false);
	menu->AddItem(fMenuStartEncode);
	fMenuStopEncode
		= new BMenuItem(B_TRANSLATE("Abort encoding"), new BMessage(M_STOP_ENCODING), 'A');
	fMenuStopEncode->SetEnabled(false);
	menu->AddItem(fMenuStopEncode);
	menu->AddSeparatorItem();
	item = new BMenuItem(B_TRANSLATE("Copy fCommand"), new BMessage(M_COPY_COMMAND), 'C');
	menu->AddItem(item);
	menuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Options"));
	fMenuDefaults = new BMenuItem(B_TRANSLATE("Default options"), new BMessage(M_DEFAULTS), 'D');
	menu->AddItem(fMenuDefaults);
	menuBar->AddItem(menu);

	// main layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(-2, 0, -2, 0)
		.Add(menuBar)
		.Add(fileoptionsview)
		.Add(fTabView)
		.Add(encodeview)
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING)
			.Add(fStatusBar)
			.AddGroup(B_VERTICAL)
				.AddGlue()
				.Add(fPlayFinishedBox)
			.End()
		.End()
		.AddGlue()
		.Layout();

	ResizeToPreferred();
	float min_width, min_height, max_width, max_height;
	GetSizeLimits(&min_width, &max_width, &min_height, &max_height);
	BSize window_size = Size();
	min_width = window_size.width;
	min_height = window_size.height;
	SetSizeLimits(min_width, max_width, min_height, max_height);
	MoveOnScreen();

	// initialize command launcher
	fCommandLauncher = new CommandLauncher(new BMessenger(this));
}


bool
ffguiwin::QuitRequested()
{
	// encoding in progress
	if (fEncodeStartTime > 0) {
		fAlertInvoker.SetMessage(new BMessage(M_QUIT_ALERT_BUTTON));
		fStopAlert = new BAlert("abort",
			B_TRANSLATE("Are you sure, that you want to abort the encoding?\n"),
			B_TRANSLATE("Cancel"), B_TRANSLATE("Stop encoding"));
		fStopAlert->SetShortcut(0, B_ESCAPE);
		fStopAlert->Go(&fAlertInvoker);
		return false;
	}

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
ffguiwin::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_ABOUT_REQUESTED:
		{
			be_app->PostMessage(B_ABOUT_REQUESTED);
			break;
		}
		case M_COPY_COMMAND:
		{
			BString text(fCommandlineTextControl->Text());
			ssize_t textLen = text.Length();
			BMessage* clip = (BMessage*) NULL;

			if (be_clipboard->Lock()) {
				be_clipboard->Clear();
				if ((clip = be_clipboard->Data())) {
					clip->AddData("text/plain", B_MIME_TYPE, text.String(), textLen);
					be_clipboard->Commit();
				}
				be_clipboard->Unlock();
			}
			break;
		}
		case M_DEFAULTS:
		{
			SetDefaults();
			if (FileExists(fSourceTextControl->Text()))
				AdoptDefaults();
			break;
		}
		case M_SOURCEFILE:
		{
			GetMediaInfo();
		} // intentional fall-though
		case M_OUTPUTFILE:
		{
			BuildLine();
			ReadyToEncode();
			SetPlaybuttonsState();

			break;
		}
		case M_OUTPUTFILEFORMAT:
		{
			BString outputfilename(fOutputTextControl->Text());
			outputfilename = outputfilename.Trim();

			if (!outputfilename.IsEmpty()) {
				SetFileExtension();
				ReadyToEncode();
				SetPlaybuttonsState();
			}

			int32 option_index = fFileFormatPopup->FindMarkedIndex();
			if (fContainerFormats[option_index].Capability == CAP_AUDIO_ONLY)
				fEnabelVideoBox->SetEnabled(false);
			else
				fEnabelVideoBox->SetEnabled(true);

			ToggleVideo();
			BuildLine();
			break;
		}
		case M_OUTPUTVIDEOFORMAT:
		{
			ToggleVideo();
			ToggleCropping();
			BuildLine();
			break;
		}
		case M_OUTPUTAUDIOFORMAT:
		{
			ToggleAudio();
			BuildLine();
			break;
		}
		case M_VBITRATE:
		case M_FRAMERATE:
		case M_XRES:
		case M_YRES:
		case M_TOPCROP:
		case M_BOTTOMCROP:
		case M_LEFTCROP:
		case M_RIGHTCROP:
		case M_AB:
		case M_AR:
		case M_AC:
		{
			BuildLine();
			break;
		}

		case M_ENABLEVIDEO:
		{
			ToggleVideo();
			ToggleCropping();
			BuildLine();
			break;
		}

		case M_CUSTOMRES:
		{
			ToggleVideo();
			BuildLine();
			break;
		}

		case M_ENABLECROPPING:
		{
			ToggleCropping();
			BuildLine();
			break;
		}

		case M_ENABLEAUDIO:
		{
			ToggleAudio();
			BuildLine();
			break;
		}
		case M_SOURCE:
		{
			fSourceFilePanel->Show();
			break;
		}
		case M_OUTPUT:
		{
			BPath path = fOutputTextControl->Text();
			if (path.InitCheck() == B_OK) {
				fOutputFilePanel->SetSaveText(path.Leaf());
				path.GetParent(&path);
				fOutputFilePanel->SetPanelDirectory(path.Path());
			}
			fOutputFilePanel->Show();
			break;
		}
		case M_SOURCEFILE_REF:
		{
			entry_ref ref;
			message->FindRef("refs", &ref);
			BEntry file_entry(&ref, true);
			BPath file_path(&file_entry);
			fSourceTextControl->SetText(file_path.Path());
			fOutputTextControl->SetText(file_path.Path());
			SetFileExtension();
			break;
		}
		case M_OUTPUTFILE_REF:
		{
			entry_ref directory_ref;
			message->FindRef("directory", &directory_ref);
			BEntry directory_entry(&directory_ref, true);
			BPath directory_path(&directory_entry);
			BString filename;
			message->FindString("name", &filename);
			filename.Prepend("/");
			filename.Prepend(directory_path.Path());

			fOutputTextControl->SetText(filename);
			SetFileExtension();
			break;
		}
		case M_INFO_OUTPUT:
		{
			BString info_data;
			message->FindString("data", &info_data);
			fMediainfo << info_data;
			break;
		}
		case M_INFO_FINISHED:
		{
			fEncodeDuration = 0;
			ParseMediaOutput();
			UpdateMediaInfo();
			AdoptDefaults();
			break;
		}
		case M_ENCODE:
		{
			fEncodeStartTime = real_time_clock();
			fStartAbortButton->SetLabel(B_TRANSLATE("Abort"));
			fStartAbortButton->SetMessage(new BMessage(M_STOP_ENCODING));
			fMenuStartEncode->SetEnabled(false);
			fMenuStopEncode->SetEnabled(true);

			fLogView->SelectAll();
			fLogView->Clear();
			fCommand.SetTo(fCommandlineTextControl->Text());
			fCommand.Append(" -y");

			BString files_string(B_TRANSLATE("Encoding: %source%   →   %output%"));
			BString name;
			BString filename = fSourceTextControl->Text();
			int32 position = filename.FindLast("/") + 1;
			filename.CopyInto(name, position, filename.Length() - position);
			files_string.ReplaceFirst("%source%", name);
			filename = fOutputTextControl->Text();
			position = filename.FindLast("/") + 1;
			filename.CopyInto(name, position, filename.Length() - position);
			files_string.ReplaceFirst("%output%", name);

			fStatusBar->SetText(files_string.String());

			BMessage start_encode_message(M_ENCODE_COMMAND);
			start_encode_message.AddString("cmdline", fCommand);
			fCommandLauncher->PostMessage(&start_encode_message);
			fEncodeTime = 0;
			break;
		}
		case M_STOP_ENCODING:
		{
			time_t now = (time_t) real_time_clock();
			// Only show if encoding has been running for more than 30s
			if (now - fEncodeStartTime > 30) {
				fStopAlert = new BAlert("abort",
					B_TRANSLATE("Are you sure, that you want to abort the encoding?\n"),
					B_TRANSLATE("Cancel"), B_TRANSLATE("Abort encoding"));
				fStopAlert->SetShortcut(0, B_ESCAPE);
				fStopAlert->Go(&fAlertInvoker);
			} else {
				BMessage stop_encode_message(M_STOP_COMMAND);
				fCommandLauncher->PostMessage(&stop_encode_message);
				fEncodeStartTime = 0; // 0 means: no encoding in progress
			}
			break;
		}
		case M_STOP_ALERT_BUTTON:
		{
			fStopAlert = NULL;
			int32 selection = -1;
			message->FindInt32("which", &selection);
			if (selection == 1) {
				BMessage stop_encode_message(M_STOP_COMMAND);
				fCommandLauncher->PostMessage(&stop_encode_message);
				fEncodeStartTime = 0; // 0 means: no encoding in progress
			}
			break;
		}
		case M_QUIT_ALERT_BUTTON:
		{
			fStopAlert = NULL;
			int32 selection = -1;
			message->FindInt32("which", &selection);
			if (selection == 1) {
				BMessage stop_encode_message(M_STOP_COMMAND);
				fCommandLauncher->PostMessage(&stop_encode_message);
				fEncodeStartTime = 0; // 0 means: no encoding in progress
				be_app->PostMessage(B_QUIT_REQUESTED);
			} else
				fAlertInvoker.SetMessage(new BMessage(M_STOP_ALERT_BUTTON));
			break;
		}
		case M_ENCODE_PROGRESS:
		{
			BString progress_data;
			message->FindString("data", &progress_data);
			progress_data << "\n";
			fLogView->Insert(progress_data.String());
			fLogView->ScrollTo(0.0, 1000000.0);

			// calculate progress percentage
			int32 time_startpos = progress_data.FindFirst("time=");
			if (time_startpos > -1) {
				time_startpos += 5;
				int32 time_endpos = progress_data.FindFirst(".", time_startpos);
				BString time_string;
				progress_data.CopyInto(time_string, time_startpos, time_endpos - time_startpos);
				fEncodeTime = GetSeconds(time_string);

				int32 encode_percentage;
				if (fEncodeDuration > 0)
					encode_percentage = (fEncodeTime * 100) / fEncodeDuration;
				else
					encode_percentage = 0;

				BMessage progress_update_message(B_UPDATE_STATUS_BAR);
				progress_update_message.AddFloat(
					"delta", encode_percentage - fStatusBar->CurrentValue());
				BString percentage_string;
				percentage_string << encode_percentage << "%";
				progress_update_message.AddString("trailing_text", percentage_string.String());
				PostMessage(&progress_update_message, fStatusBar);
			}
			break;
		}
		case M_ENCODE_FINISHED:
		{
			fEncodeStartTime = 0; // 0 means: no encoding in progress

			fStartAbortButton->SetLabel(B_TRANSLATE("Start"));
			fStartAbortButton->SetMessage(new BMessage(M_ENCODE));
			fMenuStartEncode->SetEnabled(true);
			fMenuStopEncode->SetEnabled(false);

			fStatusBar->Reset();
			fStatusBar->SetText(B_TRANSLATE_NOCOLLECT(kIdleText));

			if (FileExists(fOutputTextControl->Text()))
				fOutputCheckView->SetText(B_TRANSLATE_NOCOLLECT(kOutputExists));
			else
				fOutputCheckView->SetText("");

			status_t exit_code;
			message->FindInt32("exitcode", &exit_code);

			if (exit_code == ABORTED)
				break;

			BNotification encodeFinished(B_INFORMATION_NOTIFICATION);
			encodeFinished.SetGroup(B_TRANSLATE_SYSTEM_NAME("ffmpeg GUI"));
			BString title(B_TRANSLATE("Encoding"));

			if (exit_code == SUCCESS) {
				encodeFinished.SetContent(B_TRANSLATE("Encoding finished successfully!"));

				if (fPlayFinishedBox->Value() == B_CONTROL_ON)
					PlayVideo(fOutputTextControl->Text());
				else {
					BPath path = fOutputTextControl->Text();

					if (path.InitCheck() == B_OK) {
						title = path.Leaf();

						entry_ref ref;
						get_ref_for_path(path.Path(), &ref);
						SetFiletype(&ref);
						encodeFinished.SetOnClickFile(&ref);
					}
				}
				SetPlaybuttonsState();
			} else {
				encodeFinished.SetContent(B_TRANSLATE("Encoding failed."));
				fTabView->Select(1);
				fLogView->ScrollTo(0.0, 1000000.0);
			}
			encodeFinished.SetTitle(title);
			encodeFinished.Send();

			if (fStopAlert != NULL) {
				fStopAlert->Lock();
				fStopAlert->TextView()->SetText(
					B_TRANSLATE("Too late! Encoding has already finished.\n"));
				BButton* button = fStopAlert->ButtonAt(1);
				button->SetLabel(B_TRANSLATE_COMMENT("Duh!",
					"Button label of stop-encoding-alert if you're too "
					"late..."));
				fStopAlert->Unlock();
			}
			break;
		}
		case M_PLAY_SOURCE:
		{
			PlayVideo(fSourceTextControl->Text());
			break;
		}
		case M_PLAY_OUTPUT:
		{
			PlayVideo(fOutputTextControl->Text());
			break;
		}
		case B_REFS_RECEIVED:
		{
			entry_ref file_ref;
			if (message->FindRef("refs", &file_ref) == B_OK) {
				BEntry file_entry(&file_ref, true);
				BPath file_path(&file_entry);
				fSourceTextControl->SetText(file_path.Path());
				fOutputTextControl->SetText(file_path.Path());
				SetFileExtension();
			}
			break;
		}
		case B_SIMPLE_DATA:
		{
			BPoint drop_point;
			entry_ref file_ref;
			message->FindPoint("_drop_point_", &drop_point);
			message->FindRef("refs", &file_ref);
			BEntry file_entry(&file_ref, true);
			BPath file_path(&file_entry);

			BRect sourcefile_rect = fSourceTextControl->Bounds();
			fSourceTextControl->ConvertToScreen(&sourcefile_rect);
			BRect outputfile_rect = fOutputTextControl->Bounds();
			fOutputTextControl->ConvertToScreen(&outputfile_rect);

			// add padding around the text controls for a larger target
			float padding = (outputfile_rect.top - sourcefile_rect.bottom) / 2;
			sourcefile_rect.InsetBy(-padding, -padding);
			outputfile_rect.InsetBy(-padding, -padding);

			if (sourcefile_rect.Contains(drop_point)) {
				fSourceTextControl->SetText(file_path.Path());
				fOutputTextControl->SetText(file_path.Path());
			} else if (outputfile_rect.Contains(drop_point))
				fOutputTextControl->SetText(file_path.Path());
			else
				break;

			SetFileExtension();
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
ffguiwin::BuildLine() // ask all the views what they hold, reset the command string
{
	BString source_filename(fSourceTextControl->Text());
	BString output_filename(fOutputTextControl->Text());
	source_filename.Trim();
	output_filename.Trim();
	BString fCommand("ffmpeg -i ");
	fCommand << "\"" << source_filename << "\""; // append the input file
													// name

	// file format
	int32 option_index = fFileFormatPopup->FindMarkedIndex();
	BString fileformat_option = fContainerFormats[option_index].Option;
	fCommand << " -f " << fileformat_option; // grab and set the file format

	// is video enabled, add options
	if ((fEnabelVideoBox->Value() == B_CONTROL_ON) and (fEnabelVideoBox->IsEnabled())) {
		option_index = fVideoFormatPopup->FindMarkedIndex();
		fCommand << " -vcodec " << fVideoCodecs[option_index].Option;
		if (option_index != 0) {
			fCommand << " -b:v " << fVideoBitrateSpinner->Value() << "k";
			fCommand << " -r " << fFramerate->Value();
			if (fCustomResolutionBox->IsEnabled() && fCustomResolutionBox->Value())
				fCommand << " -s " << fXres->Value() << "x" << fYres->Value();

			// cropping options
			if (fEnabelCropBox->IsEnabled() && fEnabelCropBox->Value()) {
				fCommand << " -vf crop=iw-" << fLeftCrop->Value() + fRightCrop->Value() << ":ih-"
							<< fTopCrop->Value() + fBottomCrop->Value() << ":" << fLeftCrop->Value()
							<< ":" << fTopCrop->Value();
			}
		}
	} else
		fCommand << " -vn";

	// audio encoding enabled, grab the values
	if (fEnabelAudioBox->Value() == B_CONTROL_ON) {
		option_index = fAudioFormatPopup->FindMarkedIndex();
		fCommand << " -acodec " << fAudioCodecs[option_index].Option;
		if (option_index != 0) {
			fCommand << " -b:a " << std::atoi(fAudioBitsPopup->FindMarked()->Label()) << "k";
			fCommand << " -ar " << std::atoi(fSampleratePopup->FindMarked()->Label());
			fCommand << " -ac " << fChannelCount->Value();
		}
	} else
		fCommand << (" -an");

	fCommand << " \"" << output_filename << "\"";
	fCommand << " -loglevel error -stats";
	fCommandlineTextControl->SetText(fCommand.String());
}


void
ffguiwin::GetMediaInfo()
{
	// Reset media info and video/audio tags
	fMediainfo = fVideoCodec = fAudioCodec = fVideoWidth = fVideoHeight = fVideoFramerate
		= fDuration = fVideoBitrate = fAudioBitrate = fAudioSamplerate = fAudioChannelLayout = "";
	BString command;
	command << "ffprobe -v error -show_entries format=duration,bit_rate:"
			   "stream=codec_name,width,height,r_frame_rate,sample_rate,"
			   "channel_layout "
			   "-of default=noprint_wrappers=1 -select_streams v:0 "
			<< "\"" << fSourceTextControl->Text() << "\" ; ";
	command << "ffprobe -v error -show_entries format=duration:"
			   "stream=codec_name,sample_rate,channels,channel_layout,bit_rate "
			   "-of default=noprint_wrappers=1 -select_streams a:0 "
			<< "\"" << fSourceTextControl->Text() << "\"";

	BMessage get_info_message(M_INFO_COMMAND);
	get_info_message.AddString("cmdline", command);
	fCommandLauncher->PostMessage(&get_info_message);
}


void
ffguiwin::UpdateMediaInfo()
{
	if (fVideoFramerate != "" && fVideoFramerate != "N/A") {
		// Convert fractional representation (e.g. 50/1) to floating number
		BStringList calclist;
		bool status = fVideoFramerate.Split("/", true, calclist);
		if (calclist.StringAt(1) != "0") {
			float rate = atof(calclist.StringAt(0)) / atof(calclist.StringAt(1));
			fVideoFramerate.SetToFormat("%.3f", rate);
			RemoveOverPrecision(fVideoFramerate);
		}
	}
	if (fVideoBitrate != "" && fVideoFramerate != "N/A") {
		// Convert bits/s to kBit/s
		int32 vrate = int32(ceil(atof(fVideoBitrate) / 1024));
		fVideoBitrate.SetToFormat("%" B_PRId32, vrate);
	}
	if (fAudioSamplerate != "" && fAudioSamplerate != "N/A") {
		// Convert Hz to kHz
		float samplerate = atof(fAudioSamplerate) / 1000;
		fAudioSamplerate.SetToFormat("%.2f", samplerate);
		RemoveOverPrecision(fAudioSamplerate);
	}
	if (fAudioBitrate != "" && fAudioBitrate != "N/A") {
		// Convert bits/s to kBit/s
		int32 abitrate = ceil(atoi(fAudioBitrate) / 1024);
		fAudioBitrate.SetToFormat("%" B_PRId32, abitrate);
	}
	if (fDuration != "N/A") {
		fEncodeDuration = atoi(fDuration); // Also used to calculate progress bar
		// Convert seconds to HH:MM:SS
		char durationText[64];
		duration_to_string(fEncodeDuration, durationText, sizeof(durationText));
		fDuration = durationText;
	}
	BString text;
	text << "📺: ";
	if (fVideoCodec == "")
		text << B_TRANSLATE("No video track");
	else {
		text << fVideoCodec << ", " << fVideoWidth << "x" << fVideoHeight << ", ";
		text << fVideoFramerate << " " << B_TRANSLATE("fps") << ", ";
		text << fVideoBitrate << " "
			 << "Kbit/s";
	}
	text << "    🔈: ";
	if (fAudioCodec == "")
		text << B_TRANSLATE("No audio track");
	else {
		text << fAudioCodec << ", " << fAudioSamplerate << " "
			 << "kHz, ";
		text << fAudioChannelLayout << ", " << fAudioBitrate << " "
			 << "Kbits/s";
	}
	text << "    🕛: " << fDuration;

	fMediaInfoView->SetText(text.String());
	ReadyToEncode();
}


void
ffguiwin::ParseMediaOutput()
{
	BStringList list;
	fMediainfo.ReplaceAll("\n", "=");
	fMediainfo.Split("=", true, list);

	if (list.HasString("width")) { // video stream is always first
		int32 index = list.IndexOf("codec_name");
		fVideoCodec = list.StringAt(index + 1);
		// Remove in case of 2nd "codec_name" of audio stream
		list.Remove(index);

		fVideoWidth = list.StringAt(list.IndexOf("width") + 1);
		fVideoHeight = list.StringAt(list.IndexOf("height") + 1);
		fVideoFramerate = list.StringAt(list.IndexOf("r_frame_rate") + 1);

		fDuration = list.StringAt(list.IndexOf("duration") + 1);

		index = list.IndexOf("bit_rate");
		fVideoBitrate = list.StringAt(index + 1);
		// Remove in case of 2nd "bit_rate" of audio stream
		list.Remove(index);
	}
	if (list.HasString("sample_rate")) { // audio stream is second
		fAudioCodec = list.StringAt(list.IndexOf("codec_name") + 1);
		fAudioSamplerate = list.StringAt(list.IndexOf("sample_rate") + 1);
		fAudioChannels = list.StringAt(list.IndexOf("channels") + 1);
		fAudioChannelLayout = list.StringAt(list.IndexOf("channel_layout") + 1);

		if (fDuration == "") // if audio-only (not filled by video ffprobe above)
			fDuration = list.StringAt(list.IndexOf("duration") + 1);

		int32 index = list.IndexOf("bit_rate");
		fAudioBitrate = list.StringAt(index + 1);
	}
}


void
ffguiwin::AdoptDefaults()
{
	if (!fVideoBitrate.IsEmpty() && fVideoBitrate != "N/A")
		fVideoBitrateSpinner->SetValue(atoi(fVideoBitrate));

	if (!fVideoFramerate.IsEmpty() && fVideoFramerate != "N/A") {
		int32 point = fVideoFramerate.FindFirst(".");
		int32 places = fVideoFramerate.CountChars() - (point + 1);
		fFramerate->SetPrecision((point == B_ERROR) ? 0 : places);
		fFramerate->TextView()->SetText(fVideoFramerate);
		fFramerate->SetValueFromText();
	}

	if (!fAudioSamplerate.IsEmpty() && fAudioSamplerate != "N/A") {
		BString rate;
		rate.SetToFormat("%.f", atof(fAudioSamplerate) * 1000);
		BMenuItem* item = fSampleratePopup->FindItem(rate);
		if (item != NULL)
			item->SetMarked(true);
	}

	if (!fAudioChannels.IsEmpty() && fAudioChannels != "N/A")
		fChannelCount->SetValue(atoi(fAudioChannels));
}


void
ffguiwin::SetDefaults()
{
	// set the initial values
	fVideoBitrateSpinner->SetValue(1000);
	fFramerate->SetValue(30);
	fXres->SetValue(1280);
	fYres->SetValue(720);

	fTopCrop->SetValue(0);
	fBottomCrop->SetValue(0);
	fLeftCrop->SetValue(0);
	fRightCrop->SetValue(0);

	fAudioBitsPopup->ItemAt(2)->SetMarked(true);
	fSampleratePopup->ItemAt(1)->SetMarked(true);
	fChannelCount->SetValue(2);

	// set the default status
	fEnabelVideoBox->SetValue(true);
	fEnabelVideoBox->SetEnabled(B_CONTROL_ON);
	fEnabelAudioBox->SetValue(true);
	fEnabelAudioBox->SetEnabled(B_CONTROL_ON);

	fEnabelCropBox->SetValue(false);
	fEnabelCropBox->SetEnabled(B_CONTROL_OFF);
	fCustomResolutionBox->SetValue(false);
	fCustomResolutionBox->SetEnabled(B_CONTROL_OFF);
	fXres->SetEnabled(B_CONTROL_OFF);
	fYres->SetEnabled(B_CONTROL_OFF);

	// create internal logic
	ToggleVideo();
	ToggleCropping();
	ToggleAudio();
}


void
ffguiwin::PopulateCodecOptions()
{
	//	container formats
	fContainerFormats.push_back(
		ContainerOption("avi", "avi", "AVI (Audio Video Interleaved)", CAP_AUDIO_VIDEO));
	fContainerFormats.push_back(
		ContainerOption("matroska", "mkv", "Matroska", CAP_AUDIO_VIDEO));
	fContainerFormats.push_back(
		ContainerOption("mp4", "mp4", "MPEG-4 Part 14", CAP_AUDIO_VIDEO));
	fContainerFormats.push_back(
		ContainerOption("mpeg", "mpg", "MPEG-1 Systems/MPEG Program Stream", CAP_AUDIO_VIDEO));
	fContainerFormats.push_back(
		ContainerOption("ogg", "ogg", "Ogg", CAP_AUDIO_VIDEO));
	fContainerFormats.push_back(
		ContainerOption("webm", "webm", "WebM", CAP_AUDIO_VIDEO));
	fContainerFormats.push_back(
		ContainerOption("flac", "flac", "FLAC", CAP_AUDIO_ONLY));
	fContainerFormats.push_back(
		ContainerOption("mp3", "mp3", "MPEG audio layer 3", CAP_AUDIO_ONLY));
	fContainerFormats.push_back(
		ContainerOption("oga", "oga", "Ogg Audio", CAP_AUDIO_ONLY));
	fContainerFormats.push_back(
		ContainerOption("wav", "wav", "WAV/WAVE (Waveform Audio)", CAP_AUDIO_ONLY));

	// video codecs
	fVideoCodecs.push_back(CodecOption("copy", "1:1 copy"));
	fVideoCodecs.push_back(CodecOption("mpeg4", "MPEG-4 part 2"));
	fVideoCodecs.push_back(CodecOption("theora", "Theora"));
	fVideoCodecs.push_back(CodecOption("vp8", "On2 VP8"));
	fVideoCodecs.push_back(CodecOption("vp9", "Google VP9"));
	fVideoCodecs.push_back(CodecOption("wmv1", "Windows Media Video 7"));
	fVideoCodecs.push_back(CodecOption("wmv2", "Windows Media Video 8"));
	fVideoCodecs.push_back(CodecOption("mjpeg", "Motion JPEG"));

	// audio codecs
	fAudioCodecs.push_back(CodecOption("copy", "1:1 copy"));
	fAudioCodecs.push_back(CodecOption("aac", "AAC (Advanced Audio Coding)"));
	fAudioCodecs.push_back(CodecOption("ac3", "ATSC A/52A (AC-3)"));
	fAudioCodecs.push_back(CodecOption("libvorbis", "Vorbis"));
	fAudioCodecs.push_back(CodecOption("flac", "FLAC (Free Lossless Audio Codec)"));
	fAudioCodecs.push_back(CodecOption("dts", "DCA (DTS Coherent Acoustics)"));
	fAudioCodecs.push_back(CodecOption("mp3", "MPEG audio layer 3"));
	fAudioCodecs.push_back(CodecOption("pcm_s16le", "PCM signed 16-bit little endian"));
}


bool
ffguiwin::FileExists(const char* filepath)
{
	BEntry entry(filepath);
	bool status = entry.Exists();

	return status;
}


void
ffguiwin::SetFileExtension()
{
	BString output_filename(fOutputTextControl->Text());
	if (output_filename == "")
		output_filename = fSourceTextControl->Text();

	int32 begin_ext = output_filename.FindLast(".");
	// cut away extension if it already exists
	if (begin_ext != B_ERROR) {
		++begin_ext;
		output_filename.RemoveChars(begin_ext, output_filename.Length() - begin_ext);
	} else
		output_filename.Append(".");

	int32 option_index = fFileFormatPopup->FindMarkedIndex();
	output_filename.Append(fContainerFormats[option_index].Extension);
	fOutputTextControl->SetText(output_filename);
}


void
ffguiwin::SetFiletype(entry_ref* ref)
{
	BFile file(ref, B_READ_ONLY);
	BNodeInfo nodeInfo(&file);
	char mimeString[B_MIME_TYPE_LENGTH];

	if (nodeInfo.GetType(mimeString) != B_OK) {
		BMimeType type;
		if (BMimeType::GuessMimeType(ref, &type) == B_OK) {
			strlcpy(mimeString, type.Type(), B_MIME_TYPE_LENGTH);
			nodeInfo.SetType(type.Type());
		}
	}
}


int32
ffguiwin::GetSeconds(BString& time_string)
{
	int32 hours = 0;
	int32 minutes = 0;
	int32 seconds = 0;
	BStringList time_list;

	time_string.Trim().Split(":", true, time_list);
	hours = std::atoi(time_list.StringAt(0).String());
	minutes = std::atoi(time_list.StringAt(1).String());
	seconds = std::atoi(time_list.StringAt(2).String());

	seconds += minutes * 60;
	seconds += hours * 3600;

	return seconds;
}


void
ffguiwin::RemoveOverPrecision(BString& float_string)
{
	// Remove trailing "0" and "."
	while (true) {
		if (float_string.EndsWith("0")) {
			float_string.Truncate(float_string.CountChars() - 1);
			if (float_string.EndsWith(".")) {
				float_string.Truncate(float_string.CountChars() - 1);
				break;
			}
		} else
			break;
	}
}


void
ffguiwin::SetSpinnerMinsize(BSpinner* spinner)
{
	BSize textview_prefsize = spinner->TextView()->PreferredSize();
	textview_prefsize.width += 20;
	textview_prefsize.height = B_SIZE_UNSET;
	spinner->CreateTextViewLayoutItem()->SetExplicitMinSize(textview_prefsize);
}


void
ffguiwin::SetSpinnerMinsize(BDecimalSpinner* spinner)
{
	BSize textview_prefsize = spinner->TextView()->PreferredSize();
	textview_prefsize.width += 20;
	textview_prefsize.height = B_SIZE_UNSET;
	spinner->CreateTextViewLayoutItem()->SetExplicitMinSize(textview_prefsize);
}


void
ffguiwin::ReadyToEncode()
{
	BString source_filename(fSourceTextControl->Text());
	BString output_filename(fOutputTextControl->Text());
	source_filename.Trim();
	output_filename.Trim();

	bool ready = true;
	fSourceTextControl->MarkAsInvalid(false);
	fOutputTextControl->MarkAsInvalid(false);

	if (source_filename.IsEmpty()) {
		fMediaInfoView->SetText(B_TRANSLATE_NOCOLLECT(kEmptySource));
		fOutputCheckView->SetText("");
		fSourcePlayButton->SetEnabled(false);
		fMenuPlaySource->SetEnabled(false);
		ready = false;
	}

	if (!FileExists(source_filename)) {
		fMediaInfoView->SetText(B_TRANSLATE_NOCOLLECT(kSourceDoesntExist));
		fSourceTextControl->MarkAsInvalid(true);
		fOutputCheckView->SetText("");
		fSourcePlayButton->SetEnabled(false);
		fMenuPlaySource->SetEnabled(false);
		ready = false;
	}

	if (FileExists(output_filename))
		fOutputCheckView->SetText(B_TRANSLATE_NOCOLLECT(kOutputExists));
	else
		fOutputCheckView->SetText("");

	if (output_filename == source_filename) {
		fOutputCheckView->SetText(B_TRANSLATE_NOCOLLECT(kOutputIsSource));
		fOutputTextControl->MarkAsInvalid(true);
		ready = false;
	}

	if (output_filename.IsEmpty())
		ready = false;

	fStartAbortButton->SetEnabled(ready);
	fMenuStartEncode->SetEnabled(ready);
}


void
ffguiwin::PlayVideo(const char* filepath)
{
	BEntry video_entry(filepath);
	entry_ref video_ref;
	video_entry.GetRef(&video_ref);
	be_roster->Launch(&video_ref);
}


void
ffguiwin::SetPlaybuttonsState()
{
	bool valid = FileExists(fSourceTextControl->Text());
	fSourcePlayButton->SetEnabled(valid);
	fMenuPlaySource->SetEnabled(valid);

	valid = FileExists(fOutputTextControl->Text());
	fOutputPlayButton->SetEnabled(valid);
	fMenuPlayOutput->SetEnabled(valid);
}


void
ffguiwin::ToggleVideo()
{
	bool video_options_enabled;

	if ((fEnabelVideoBox->Value() == B_CONTROL_ON) and (fEnabelVideoBox->IsEnabled())) {
		fVideoFormat->SetEnabled(true);
		if (fVideoFormatPopup->FindMarkedIndex() != 0)
			video_options_enabled = true;
		else
			video_options_enabled = false;
	} else {
		fVideoFormat->SetEnabled(false);
		video_options_enabled = false;
	}

	fVideoBitrateSpinner->SetEnabled(video_options_enabled);
	fFramerate->SetEnabled(video_options_enabled);
	fCustomResolutionBox->SetEnabled(video_options_enabled);

	bool customres_options_enabled;
	if ((fCustomResolutionBox->IsEnabled()) and (fCustomResolutionBox->Value() == B_CONTROL_ON))
		customres_options_enabled = true;
	else
		customres_options_enabled = false;

	fXres->SetEnabled(customres_options_enabled);
	fYres->SetEnabled(customres_options_enabled);
}


void
ffguiwin::ToggleCropping()
{

	// disable cropping if video options are not enabled;
	if ((fEnabelVideoBox->IsEnabled()) and (fEnabelVideoBox->Value() == B_CONTROL_ON)
		and (fVideoFormatPopup->FindMarkedIndex() != 0))
			fEnabelCropBox->SetEnabled(true);
	else
		fEnabelCropBox->SetEnabled(false);

	bool cropping_options_enabled;
	if ((fEnabelCropBox->IsEnabled()) and (fEnabelCropBox->Value() == B_CONTROL_ON))
		cropping_options_enabled = true;
	else
		cropping_options_enabled = false;

	fTopCrop->SetEnabled(cropping_options_enabled);
	fBottomCrop->SetEnabled(cropping_options_enabled);
	fLeftCrop->SetEnabled(cropping_options_enabled);
	fRightCrop->SetEnabled(cropping_options_enabled);
}


void
ffguiwin::ToggleAudio()
{
	bool audio_options_enabled;
	if (fEnabelAudioBox->Value() == B_CONTROL_ON) {
		fAudioFormat->SetEnabled(true);

		if (fAudioFormatPopup->FindMarkedIndex() != 0)
			audio_options_enabled = true;
		else
			audio_options_enabled = false;
	} else {
		fAudioFormat->SetEnabled(false);
		audio_options_enabled = false;
	}

	fAudioBits->SetEnabled(audio_options_enabled);
	fChannelCount->SetEnabled(audio_options_enabled);
	fSamplerate->SetEnabled(audio_options_enabled);
}
