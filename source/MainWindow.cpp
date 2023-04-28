/*
 * Copyright 2003-2023, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Zach Dykstra,2003
 * Humdinger, humdingerb@gmail.com, 2022-2023
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022-2023
*/


#include "MainWindow.h"

#include "App.h"
#include "CropView.h"
#include "CodecContainerOptions.h"
#include "CommandLauncher.h"
#include "JobWindow.h"
#include "Messages.h"
#include "Spinner.h"
#include "Utilities.h"

#include <Alert.h>
#include <BeBuild.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <Clipboard.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <Notification.h>
#include <Path.h>
#include <PathFinder.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <Spinner.h>
#include <StatusBar.h>
#include <String.h>
#include <StringFormat.h>
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
static const char* kOutputExists = B_TRANSLATE_MARK(
	"This file already exists. It will be overwritten!");
static const char* kOutputIsSource = B_TRANSLATE_MARK(
	"Cannot overwrite the source file. Please choose another output file name.");

// Use ffmpeg for 2ndary architecture (gcc11+) on 32bit Haiku
// because vp8 and vp9 codecs are not available on gcc2 builds of ffmpeg_tools
#ifdef B_HAIKU_32_BIT
static const char* kFFMpeg = "ffmpeg-x86";
static const char* kFFProbe = "ffprobe-x86";
#else
static const char* kFFMpeg = "ffmpeg";
static const char* kFFProbe = "ffprobe";
#endif


MainWindow::MainWindow(BRect r, const char* name, window_type type, ulong mode)
	:
	BWindow(r, name, type, mode),
	fStopAlert(NULL)
{
	// Invoker for the Alerts to use to send their messages to the timer
	fAlertInvoker.SetMessage(new BMessage(M_STOP_ALERT_BUTTON));
	fAlertInvoker.SetTarget(this);

	fEncodeStartTime = 0; // 0 means: no encoding in progress

	fSourceFilePanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false,
		new BMessage(M_SOURCEFILE_REF));

	fOutputFilePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL, B_FILE_NODE, false,
		new BMessage(M_OUTPUTFILE_REF));

	// _Building layouts
	BMenuBar* menuBar = _BuildMenu();
	BView* fileoptionsview = _BuildFileOptions();
	BView* mainoptionsview = _BuildMainOptions();
	BView* croppingoptionsview = _BuildCroppingOptions();
//	BView* advancedoptionsview = _BuildAdvancedOptions();
	_BuildLogView();
	BView* encodeprogressview = _BuildEncodeProgress();

	BView* logview = new BScrollView("", fLogView, B_SUPPORTS_LAYOUT, true, true);

	// _Building tab view
	fTabView = new BTabView("");
	fOptionsTab = new BTab();
	fCroppingTab = new BTab();
	fAdvancedTab = new BTab();
	fLogTab = new BTab();

	fTabView->AddTab(mainoptionsview, fOptionsTab);
	fTabView->AddTab(croppingoptionsview, fCroppingTab);
	// fTabView->AddTab(advancedoptionsview, advancedoptionstab); //don´t remove,
	// will be needed later
	fTabView->AddTab(logview, fLogTab);

	fOptionsTab->SetLabel(B_TRANSLATE("Options"));
	fCroppingTab->SetLabel(B_TRANSLATE("Cropping"));
	// advancedoptionstab->SetLabel(B_TRANSLATE("Advanced options"));
	fLogTab->SetLabel(B_TRANSLATE("Log"));

	fPlayFinishedBox = new BCheckBox("play_finished", B_TRANSLATE("Play when finished"), NULL);
	fPlayFinishedBox->SetValue(B_CONTROL_OFF);

	fStatusBar = new BStatusBar("");
	fStatusBar->SetText(B_TRANSLATE_NOCOLLECT(kIdleText));

	// main layout
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(-2, 0, -2, 0)
		.Add(menuBar)
		.Add(fileoptionsview)
		.Add(fTabView)
		.Add(encodeprogressview)
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

	BMessage settings;
	_LoadSettings(settings);

	BRect frame = Frame();
	if (settings.FindRect("main_window", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), Bounds().Height());
	}
	MoveOnScreen();

	// create job window
	frame = Frame();
	frame.InsetBySelf(10, 200);
	fJobWindow = new JobWindow(frame, &settings, new BMessenger(this));
	fJobWindow->Show();
	fJobWindow->Hide();

	// initialize command launcher
	fCommandLauncher = new CommandLauncher(new BMessenger(this));

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
	fVideoBitrateSpinner->SetWithoutInvoke(1000);
	fFramerate->SetWithoutInvoke(int32(30));
	fXres->SetWithoutInvoke(1280);
	fYres->SetWithoutInvoke(720);
	fChannelCount->SetWithoutInvoke(2);

	// set step values for the spinners
	fVideoBitrateSpinner->SetStep(100);

	// set the initial command line
	_SetDefaults();
}


bool
MainWindow::QuitRequested()
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

	if (fJobWindow->IsJobRunning()) {
		BAlert* alert = new BAlert("abort",
			B_TRANSLATE("Are you sure, that you want to abort the encoding?\n"),
			B_TRANSLATE("Cancel"), B_TRANSLATE("Stop encoding"));
		alert->SetShortcut(0, B_ESCAPE);

		int32 choice = alert->Go();
		switch (choice) {
			case 0:
				return false;
			case 1:
				fJobWindow->PostMessage(M_JOB_ABORT);
		}
	}
	_SaveSettings();
	_DeleteTempFiles();

	fJobWindow->LockLooper();
	fJobWindow->Quit();

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
MainWindow::MessageReceived(BMessage* message)
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
		case M_JOB_ARCHIVE:
		{
			_UnarchiveJob(*message);
			Activate(true);
			break;
		}
		case M_ADD_JOB:
		{
			BString filename(fOutputTextControl->Text());
			filename.Trim();

			BString command(fCommandlineTextControl->Text());
			command << " -y";

			BMessage jobMessage(_ArchiveJob());
			if (fJobWindow->Lock())
				fJobWindow->AddJob(
					filename.String(), fDuration.String(), command.String(), jobMessage);
			fJobWindow->Unlock();
			break;
		}
		case M_JOB_MANAGER:
		{
			if (fJobWindow->IsHidden())
				fJobWindow->Show();
			else
				fJobWindow->Activate(true);
			break;
		}
		case M_JOB_COUNT:
		{
			int32 count;
			message->FindInt32("jobcount", &count);
			BString title(B_TRANSLATE_SYSTEM_NAME("ffmpegGUI"));
			if (count > 0) {
				BString jobs("  •  ");
				static BStringFormat format(B_TRANSLATE("{0, plural,"
					"one{(# Job)}"
					"other{(# Jobs)}}"));
				format.Format(jobs, count);
				title << jobs;
			}
			SetTitle(title);
			break;
		}
		case M_DEFAULTS:
		{
			_SetDefaults();
			if (_FileExists(fSourceTextControl->Text()))
				_AdoptDefaults();
			break;
		}
		case M_SOURCEFILE:
		{
			_GetMediaInfo();
		} // intentional fall-though
		case M_OUTPUTFILE:
		{
			_BuildLine();
			_ReadyToEncode();
			_SetPlaybuttonsState();

			break;
		}
		case M_OUTPUTFILEFORMAT:
		{
			int32 marked = fFileFormatPopup->FindMarkedIndex();
			BMenuItem* item = fFileFormatPopup->Superitem();
			if (item != NULL)
				item->SetLabel(fContainerFormats[marked].Extension);

			if (fContainerFormats[marked].Capability == CAP_AUDIO_ONLY)
				fEnableVideoBox->SetEnabled(false);
			else
				fEnableVideoBox->SetEnabled(true);

			_ToggleVideo();

			BString outputfilename(fOutputTextControl->Text());
			outputfilename = outputfilename.Trim();

			if (!outputfilename.IsEmpty()) {
				_SetFileExtension();
				_ReadyToEncode();
				_SetPlaybuttonsState();
			}

			_BuildLine();
			break;
		}
		case M_OUTPUTVIDEOFORMAT:
		{
			int32 marked = fVideoFormatPopup->FindMarkedIndex();
			BMenuItem* item = fVideoFormatPopup->Superitem();
			if (item != NULL)
				item->SetLabel(fVideoCodecs[marked].Shortlabel);

			_ToggleVideo();
			_ToggleCropping();
			_BuildLine();
			break;
		}
		case M_OUTPUTAUDIOFORMAT:
		{
			int32 marked = fAudioFormatPopup->FindMarkedIndex();
			BMenuItem* item = fAudioFormatPopup->Superitem();
			if (item != NULL)
				item->SetLabel(fAudioCodecs[marked].Shortlabel);

			_ToggleAudio();
			_BuildLine();
			break;
		}
		case M_TOPCROP:
		{
			fCropView->SetTopCrop(fTopCrop->Value());
			_BuildLine();
			break;
		}
		case M_BOTTOMCROP:
		{
			fCropView->SetBottomCrop(fBottomCrop->Value());
			_BuildLine();
			break;
		}
		case M_LEFTCROP:
		{
			fCropView->SetLeftCrop(fLeftCrop->Value());
			_BuildLine();
			break;
		}
		case M_RIGHTCROP:
		{
			fCropView->SetRightCrop(fRightCrop->Value());
			_BuildLine();
			break;
		}
		case M_RESET_CROPPING:
		{
			fTopCrop->SetValue(0);
			fBottomCrop->SetValue(0);
			fLeftCrop->SetValue(0);
			fRightCrop->SetValue(0);

			break;
		}
		case M_VBITRATE:
		case M_FRAMERATE:
		case M_XRES:
		case M_YRES:
		case M_AUDIOBITRATE:
		case M_SAMPLERATE:
		case M_CHANNELS:
		{
			_BuildLine();
			break;
		}

		case M_ENABLEVIDEO:
		{
			_ToggleVideo();
			_ToggleCropping();
			_BuildLine();
			break;
		}

		case M_CUSTOMRES:
		{
			_ToggleVideo();
			_BuildLine();
			break;
		}
		case M_ENABLEAUDIO:
		{
			_ToggleAudio();
			_BuildLine();
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
			_SetFileExtension();
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
			_SetFileExtension();
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
			_ParseMediaOutput();
			_UpdateMediaInfo();
			_AdoptDefaults();
			_ExtractPreviewImage();
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
			fCommand.Append(" -y"); // Overwrite output files without asking

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

			int32 seconds;
			message->FindInt32("time", &seconds);
			// calculate progress percentage
			if (seconds > -1) {
				fEncodeTime = seconds;
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

			if (_FileExists(fOutputTextControl->Text()))
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
					_PlayVideo(fOutputTextControl->Text());
				else {
					BPath path = fOutputTextControl->Text();

					if (path.InitCheck() == B_OK) {
						title = path.Leaf();

						entry_ref ref;
						get_ref_for_path(path.Path(), &ref);
						_SetFiletype(&ref);
						encodeFinished.SetOnClickFile(&ref);
					}
				}
				_SetPlaybuttonsState();
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
			_PlayVideo(fSourceTextControl->Text());
			break;
		}
		case M_PLAY_OUTPUT:
		{
			_PlayVideo(fOutputTextControl->Text());
			break;
		}
		case M_EXTRACTIMAGE_FINISHED:
		{
			fCropView->LoadImage(fPreviewPath.Path());
			fNewPreviewButton->SetEnabled(true);
			break;
		}
		case M_NEW_PREVIEW:
		{
			fNewPreviewButton->SetEnabled(false);
			_ExtractPreviewImage();
			break;
		}
		case M_HELP:
		{
			_OpenHelp();
			break;
		}
		case M_WEBSITE:
		{
			BString url(B_TRANSLATE_COMMENT("https://ffmpeg.org/ffmpeg.html",
				"You can change to a good localized version of an ffmpeg ressource"));
			_OpenURL(url);
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
				_SetFileExtension();
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

			_SetFileExtension();
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


status_t
MainWindow::_LoadSettings(BMessage& settings)
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	status = path.Append("ffmpegGUI");
	if (status != B_OK)
		return status;

	status = path.Append("settings");
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK)
		return status;

	return settings.Unflatten(&file);
}


status_t
MainWindow::_SaveSettings()
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	status = path.Append("ffmpegGUI");
	if (status != B_OK)
		return status;

	status = create_directory(path.Path(), 0777);
	if (status != B_OK)
		return status;

	status = path.Append("settings");
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	BMessage settings('fmpg');
	status = settings.AddRect("main_window", Frame());
	status = settings.AddRect("job_window", fJobWindow->Frame());
	status = settings.AddMessage("column settings", fJobWindow->GetColumnState());

	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}


BMessage
MainWindow::_ArchiveJob()
{
	BMessage jobMessage(M_JOB_ARCHIVE);

	BString text;
	text = fSourceTextControl->Text();
	text.Trim();
	jobMessage.AddString("source", text);

	text = fOutputTextControl->Text();
	text.Trim();
	jobMessage.AddString("output", text);

	jobMessage.AddString("mediainfo", fMediaInfoView->Text());

	jobMessage.AddInt32("format", fFileFormatPopup->FindMarkedIndex());

	jobMessage.AddBool("v_box_enabled", fEnableVideoBox->IsEnabled());
	jobMessage.AddInt32("v_box_ticked", fEnableVideoBox->Value());
	jobMessage.AddInt32("v_codec", fVideoFormatPopup->FindMarkedIndex());
	jobMessage.AddInt32("v_bitrate", fVideoBitrateSpinner->Value());
	jobMessage.AddString("framerate", fFramerate->TextView()->Text());

	jobMessage.AddBool("res_box_enabled", fCustomResolutionBox->IsEnabled());
	jobMessage.AddInt32("res_box_ticked", fCustomResolutionBox->Value());
	jobMessage.AddInt32("xres", fXres->Value());
	jobMessage.AddInt32("yres", fYres->Value());

	jobMessage.AddInt32("lcrop", fLeftCrop->Value());
	jobMessage.AddInt32("rcrop", fRightCrop->Value());
	jobMessage.AddInt32("tcrop", fTopCrop->Value());
	jobMessage.AddInt32("bcrop", fBottomCrop->Value());

	jobMessage.AddBool("a_box_enabled", fEnableAudioBox->IsEnabled());
	jobMessage.AddInt32("a_box_ticked", fEnableAudioBox->Value());
	jobMessage.AddInt32("a_codec", fAudioFormatPopup->FindMarkedIndex());
	jobMessage.AddInt32("a_bitrate", fAudioBitsPopup->FindMarkedIndex());
	jobMessage.AddInt32("samplerate", fSampleratePopup->FindMarkedIndex());
	jobMessage.AddInt32("channels", fChannelCount->Value());

	jobMessage.AddString("commandline", fCommandlineTextControl->Text());

	return jobMessage;
}


void
MainWindow::_UnarchiveJob(BMessage jobMessage)
{
	BString text;
	bool onoff;
	int32 value;

	// Don't trigger the message leading to _GetMediaInfo()
	// which would overwrite the settings.
	fSourceTextControl->SetModificationMessage(NULL);
	if (jobMessage.FindString("source", &text) == B_OK)
		fSourceTextControl->SetText(text);
	fSourceTextControl->SetModificationMessage(new BMessage(M_SOURCEFILE));

	// Don't trigger the message leading to _BuildLine()
	// which would overwrite the commandline.
	fOutputTextControl->SetModificationMessage(NULL);
	if (jobMessage.FindString("output", &text) == B_OK)
		fOutputTextControl->SetText(text);
	fOutputTextControl->SetModificationMessage(new BMessage(M_OUTPUTFILE));

	if (jobMessage.FindString("mediainfo", &text) == B_OK)
		fMediaInfoView->SetText(text);

	if (jobMessage.FindInt32("format", &value) == B_OK) {
		BMenuItem* item = fFileFormatPopup->ItemAt(value);
		item->SetMarked(true);
		item = fFileFormatPopup->Superitem();
		if (item != NULL)
			item->SetLabel(fContainerFormats[value].Extension);
	}

	if (jobMessage.FindBool("v_box_enabled", &onoff) == B_OK)
		fEnableVideoBox->SetEnabled(onoff);
	if (jobMessage.FindInt32("v_box_ticked", &value) == B_OK)
		fEnableVideoBox->SetValue(value);
	if (jobMessage.FindInt32("v_codec", &value) == B_OK) {
		BMenuItem* item = fVideoFormatPopup->ItemAt(value);
		item->SetMarked(true);
		item = fVideoFormatPopup->Superitem();
		if (item != NULL)
			item->SetLabel(fVideoCodecs[value].Shortlabel);
	}
	if (jobMessage.FindInt32("v_bitrate", &value) == B_OK)
		fVideoBitrateSpinner->SetWithoutInvoke(value);
	if (jobMessage.FindString("framerate", &text) == B_OK) {
		int precision = _Precision(text);
		fFramerate->SetPrecision(precision);
		fFramerate->TextView()->SetText(text);
		fFramerate->SetFromTextWithoutInvoke();
	}

	if (jobMessage.FindBool("res_box_enabled", &onoff) == B_OK)
		fCustomResolutionBox->SetEnabled(onoff);
	if (jobMessage.FindInt32("res_box_ticked", &value) == B_OK)
		fCustomResolutionBox->SetValue(value);
	if (jobMessage.FindInt32("xres", &value) == B_OK)
		fXres->SetWithoutInvoke(value);
	if (jobMessage.FindInt32("yres", &value) == B_OK)
		fYres->SetWithoutInvoke(value);

	if (jobMessage.FindInt32("lcrop", &value) == B_OK)
		fLeftCrop->SetWithoutInvoke(value);
	if (jobMessage.FindInt32("rcrop", &value) == B_OK)
		fRightCrop->SetWithoutInvoke(value);
	if (jobMessage.FindInt32("tcrop", &value) == B_OK)
		fTopCrop->SetWithoutInvoke(value);
	if (jobMessage.FindInt32("bcrop", &value) == B_OK)
		fBottomCrop->SetWithoutInvoke(value);

	if (jobMessage.FindBool("a_box_enabled", &onoff) == B_OK)
		fEnableAudioBox->SetEnabled(onoff);
	if (jobMessage.FindInt32("a_box_ticked", &value) == B_OK)
		fEnableAudioBox->SetValue(value);
	if (jobMessage.FindInt32("a_codec", &value) == B_OK) {
		BMenuItem* item = fAudioFormatPopup->ItemAt(value);
		item->SetMarked(true);
		item = fAudioFormatPopup->Superitem();
		if (item != NULL)
			item->SetLabel(fAudioCodecs[value].Shortlabel);
	}
	if (jobMessage.FindInt32("a_bitrate", &value) == B_OK) {
		BMenuItem* item = fAudioBitsPopup->ItemAt(value);
		item->SetMarked(true);
	}
	if (jobMessage.FindInt32("samplerate", &value) == B_OK) {
		BMenuItem* item = fSampleratePopup->ItemAt(value);
		item->SetMarked(true);
	}
	if (jobMessage.FindInt32("channels", &value) == B_OK)
		fChannelCount->SetWithoutInvoke(value);

	if (jobMessage.FindString("commandline", &text) == B_OK)
		fCommandlineTextControl->SetText(text);

	_ToggleVideo();
	_ToggleCropping();
	_ToggleAudio();
	_ExtractPreviewImage();
}


BMenuBar*
MainWindow::_BuildMenu()
{
	// menu bar
	BMenuBar* menuBar = new BMenuBar("menubar");
	BMenu* menu;
	BMenuItem* item;

	// ffmpegGUI menu
	menu = new BMenu(B_TRANSLATE_SYSTEM_NAME("ffmpegGUI"));
	item = new BMenuItem(
		B_TRANSLATE("Help" B_UTF8_ELLIPSIS), new BMessage(M_HELP), 'H');
	menu->AddItem(item);
	item = new BMenuItem(
		B_TRANSLATE("FFmpeg documentation" B_UTF8_ELLIPSIS), new BMessage(M_WEBSITE), 'F');
	menu->AddItem(item);
	menu->AddSeparatorItem();
	item = new BMenuItem(B_TRANSLATE("About ffmpegGUI"), new BMessage(B_ABOUT_REQUESTED));
	menu->AddItem(item);
	item = new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q');
	menu->AddItem(item);
	menuBar->AddItem(menu);

	// File menu
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
	menuBar->AddItem(menu);

	// Encoding menu
	menu = new BMenu(B_TRANSLATE("Encoding"));
	fMenuStartEncode = new BMenuItem(B_TRANSLATE("Start encoding"), new BMessage(M_ENCODE), 'E');
	fMenuStartEncode->SetEnabled(false);
	menu->AddItem(fMenuStartEncode);
	fMenuStopEncode
		= new BMenuItem(B_TRANSLATE("Abort encoding"), new BMessage(M_STOP_ENCODING), 'A');
	fMenuStopEncode->SetEnabled(false);
	menu->AddItem(fMenuStopEncode);
	menu->AddSeparatorItem();
	item = new BMenuItem(B_TRANSLATE("Copy commandline"), new BMessage(M_COPY_COMMAND), 'L');
	menu->AddItem(item);
	menuBar->AddItem(menu);

	// Jobs menu
	menu = new BMenu(B_TRANSLATE("Jobs"));
	fMenuAddJob = new BMenuItem(B_TRANSLATE("Add as new job"), new BMessage(M_ADD_JOB), 'J');
	fMenuAddJob->SetEnabled(false);
	menu->AddItem(fMenuAddJob);
	item = new BMenuItem(B_TRANSLATE("Open job manager" B_UTF8_ELLIPSIS),
		new BMessage(M_JOB_MANAGER), 'M');
	menu->AddItem(item);
	menuBar->AddItem(menu);

	// Options menu
	menu = new BMenu(B_TRANSLATE("Options"));
	fMenuDefaults = new BMenuItem(B_TRANSLATE("Default options"), new BMessage(M_DEFAULTS), 'D');
	menu->AddItem(fMenuDefaults);
	menuBar->AddItem(menu);

	return menuBar;
}


BView*
MainWindow::_BuildFileOptions()
{
	// Source file
	fSourceButton = new BButton(B_TRANSLATE("Source file"), new BMessage(M_SOURCE));
	fSourceButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	fSourceTextControl = new BTextControl("", "", new BMessage('srcf'));
	fSourceTextControl->SetModificationMessage(new BMessage(M_SOURCEFILE));

	fMediaInfoView = new BStringView("mediainfo", B_TRANSLATE_NOCOLLECT(kEmptySource));
	fMediaInfoView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	BFont font(be_plain_font);
	font.SetSize(ceilf(font.Size() * 0.9));
	fMediaInfoView->SetFont(&font, B_FONT_SIZE);

	// Output file
	fOutputButton = new BButton(B_TRANSLATE("Output file"), new BMessage(M_OUTPUT));
	fOutputButton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	fOutputTextControl = new BTextControl("", "", new BMessage('outf'));
	fOutputTextControl->SetModificationMessage(new BMessage(M_OUTPUTFILE));

	fOutputCheckView = new BStringView("outputcheck", "");
	fOutputCheckView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fOutputCheckView->SetFont(&font, B_FONT_SIZE);

	// Play buttons
	fSourcePlayButton = new BButton("⯈", new BMessage(M_PLAY_SOURCE));
	fOutputPlayButton = new BButton("⯈", new BMessage(M_PLAY_OUTPUT));
	float height;
	fSourceTextControl->GetPreferredSize(NULL, &height);
	BSize size(height, height);
	fSourcePlayButton->SetExplicitSize(size);
	fOutputPlayButton->SetExplicitSize(size);
	fSourcePlayButton->SetEnabled(false);
	fOutputPlayButton->SetEnabled(false);

	_PopulateCodecOptions();

	// File format pop-up menu
	fFileFormatPopup = new BPopUpMenu("");
	bool separator = false;
	std::vector<ContainerOption>::iterator container_iter;
	container_iter = fContainerFormats.begin();
	fFileFormatPopup = new BPopUpMenu(container_iter->Extension.String(), false, false);
	fFileFormatPopup->SetRadioMode(true);

	for (container_iter = fContainerFormats.begin(); container_iter != fContainerFormats.end();
		++container_iter) {
		if ((container_iter->Capability == CAP_AUDIO_ONLY) and (separator == false)) {
			fFileFormatPopup->AddSeparatorItem();
			separator = true;
		} else {
			fFileFormatPopup->AddItem(new BMenuItem(container_iter->Description.String(),
				new BMessage(M_OUTPUTFILEFORMAT)));
		}
	}
	fFileFormatPopup->ItemAt(0)->SetMarked(true);
	fFileFormat = new BMenuField(NULL, fFileFormatPopup);

	// Build File Options layout
	BView* fileoptionsview = new BView("fileoptions", B_SUPPORTS_LAYOUT);
	BLayoutBuilder::Group<>(fileoptionsview, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 4)
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

	return fileoptionsview;
}


BView*
MainWindow::_BuildMainOptions()
{
	// Video codec pop-up menu
	std::vector<CodecOption>::iterator codec_iter;
	codec_iter = fVideoCodecs.begin();
	fVideoFormatPopup = new BPopUpMenu(codec_iter->Shortlabel.String(), false, false);
	fVideoFormatPopup->SetRadioMode(true);

	for (codec_iter = fVideoCodecs.begin(); codec_iter != fVideoCodecs.end(); ++codec_iter) {
		fVideoFormatPopup->AddItem(
			new BMenuItem(codec_iter->Description.String(), new BMessage(M_OUTPUTVIDEOFORMAT)));
	}
	fVideoFormatPopup->ItemAt(0)->SetMarked(true);
	fVideoFormat = new BMenuField(B_TRANSLATE("Video codec:"), fVideoFormatPopup);

	// Video options
	fEnableVideoBox
		= new BCheckBox("", B_TRANSLATE("Enable video encoding"), new BMessage(M_ENABLEVIDEO));
	fEnableVideoBox->SetValue(B_CONTROL_ON);
	fVideoBitrateSpinner = new Spinner("", B_TRANSLATE("Bitrate (Kbit/s):"),
		new BMessage(M_VBITRATE));
	fFramerate = new DecSpinner("", B_TRANSLATE("Framerate (fps):"),
		new BMessage(M_FRAMERATE));
	fCustomResolutionBox = new BCheckBox("", B_TRANSLATE("Use custom resolution"),
		new BMessage(M_CUSTOMRES));
	fXres = new Spinner("", B_TRANSLATE("Width:"), new BMessage(M_XRES));
	fYres = new Spinner("", B_TRANSLATE("Height:"), new BMessage(M_YRES));

	// _Build Video Options layout
	BBox* videobox = new BBox("");
	videobox->SetLabel(B_TRANSLATE("Video"));
	BGroupLayout* videolayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fEnableVideoBox)
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
		.End()
		.AddGlue();
	videobox->AddChild(videolayout->View());

	// Audio codec pop-up menu
	codec_iter = fAudioCodecs.begin();
	fAudioFormatPopup = new BPopUpMenu(codec_iter->Shortlabel.String(), false, false);
	fAudioFormatPopup->SetRadioMode(true);
	for (codec_iter = fAudioCodecs.begin(); codec_iter != fAudioCodecs.end(); ++codec_iter) {
		fAudioFormatPopup->AddItem(
			new BMenuItem(codec_iter->Description.String(), new BMessage(M_OUTPUTAUDIOFORMAT)));
	}
	fAudioFormatPopup->ItemAt(0)->SetMarked(true);
	fAudioFormat = new BMenuField(B_TRANSLATE("Audio codec:"), fAudioFormatPopup);

	// Audio options
	fEnableAudioBox
		= new BCheckBox("", B_TRANSLATE("Enable audio encoding"), new BMessage(M_ENABLEAUDIO));
	fEnableAudioBox->SetValue(B_CONTROL_ON);

	fAudioBitsPopup = new BPopUpMenu("");
	fAudioBitsPopup->AddItem(new BMenuItem("48", new BMessage(M_AUDIOBITRATE)));
	fAudioBitsPopup->AddItem(new BMenuItem("96", new BMessage(M_AUDIOBITRATE)));
	fAudioBitsPopup->AddItem(new BMenuItem("128", new BMessage(M_AUDIOBITRATE)));
	fAudioBitsPopup->AddItem(new BMenuItem("160", new BMessage(M_AUDIOBITRATE)));
	fAudioBitsPopup->AddItem(new BMenuItem("196", new BMessage(M_AUDIOBITRATE)));
	fAudioBitsPopup->AddItem(new BMenuItem("320", new BMessage(M_AUDIOBITRATE)));
	fAudioBitsPopup->AddItem(new BMenuItem("625", new BMessage(M_AUDIOBITRATE)));
	fAudioBitsPopup->AddItem(new BMenuItem("1411", new BMessage(M_AUDIOBITRATE)));
	fAudioBitsPopup->ItemAt(1)->SetMarked(true);
	fAudioBits = new BMenuField(B_TRANSLATE("Bitrate (Kbit/s):"), fAudioBitsPopup);

	fSampleratePopup = new BPopUpMenu("");
	fSampleratePopup->AddItem(new BMenuItem("22050", new BMessage(M_SAMPLERATE)));
	fSampleratePopup->AddItem(new BMenuItem("44100", new BMessage(M_SAMPLERATE)));
	fSampleratePopup->AddItem(new BMenuItem("48000", new BMessage(M_SAMPLERATE)));
	fSampleratePopup->AddItem(new BMenuItem("96000", new BMessage(M_SAMPLERATE)));
	fSampleratePopup->AddItem(new BMenuItem("192000", new BMessage(M_SAMPLERATE)));
	fSampleratePopup->ItemAt(1)->SetMarked(true);
	fSamplerate = new BMenuField(B_TRANSLATE("Sampling rate (Hz):"), fSampleratePopup);
	fChannelCount = new Spinner("", B_TRANSLATE("Audio channels:"), new BMessage(M_CHANNELS));

	// _Build Audio Options layout
	BBox* audiobox = new BBox("");
	audiobox->SetLabel(B_TRANSLATE("Audio"));
	BGroupLayout* audiolayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fEnableAudioBox)
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

	// Build Main Options tab layout
	BView* mainoptionsview = new BView("", B_SUPPORTS_LAYOUT);
	BLayoutBuilder::Group<>(mainoptionsview, B_HORIZONTAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING)
		.Add(videobox)
		.Add(audiobox)
		.Layout();

	return mainoptionsview;
}


BView*
MainWindow::_BuildCroppingOptions()
{
	fTopCrop = new Spinner("", B_TRANSLATE("Top:"), new BMessage(M_TOPCROP));
	fBottomCrop = new Spinner("", B_TRANSLATE("Bottom:"), new BMessage(M_BOTTOMCROP));
	fLeftCrop = new Spinner("", B_TRANSLATE("Left:"), new BMessage(M_LEFTCROP));
	fRightCrop = new Spinner("", B_TRANSLATE("Right:"), new BMessage(M_RIGHTCROP));

	fTopCrop->SetMinValue(0);
	fBottomCrop->SetMinValue(0);
	fLeftCrop->SetMinValue(0);
	fRightCrop->SetMinValue(0);

	fCropView = new CropView();
	fResetCroppingButton = new BButton("", B_TRANSLATE("Reset"), new BMessage(M_RESET_CROPPING));
	fNewPreviewButton = new BButton("", B_TRANSLATE("New preview"), new BMessage(M_NEW_PREVIEW));

	BView* croppingoptionsview = new BView("", B_SUPPORTS_LAYOUT);
	BLayoutBuilder::Group<>(croppingoptionsview, B_HORIZONTAL)
		.AddGroup(B_VERTICAL)
			.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 0)
			.AddGlue()
			.Add(fNewPreviewButton)
			.AddGlue()
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
			.Add(fResetCroppingButton)
			.AddGlue()
		.End()
		.Add(fCropView)
		.Layout();

	return croppingoptionsview;
}


BView*
MainWindow::_BuildAdvancedOptions()
{
	// Advanced options (currently ignored / hidden)
	fBFrames = new Spinner("", B_TRANSLATE("'B' frames:"), nullptr);
	fGop = new Spinner("", B_TRANSLATE("GOP size:"), nullptr);
	fHighQualityBox
		= new BCheckBox("", B_TRANSLATE("Use high quality settings"), new BMessage(M_HIGHQUALITY));
	fFourMotionBox
		= new BCheckBox("", B_TRANSLATE("Use four motion vector"), new BMessage(M_FOURMOTION));
	fDeinterlaceBox
		= new BCheckBox("", B_TRANSLATE("Deinterlace pictures"), new BMessage(M_DEINTERLACE));
	fCalcNpsnrBox = new BCheckBox(
		"", B_TRANSLATE("Calculate PSNR of compressed frames"), new BMessage(M_CALCPSNR));

	fFixedQuantizer = new Spinner("", B_TRANSLATE("Use fixed video quantizer scale:"), nullptr);
	fMinQuantizer = new Spinner("", B_TRANSLATE("Min video quantizer scale:"), nullptr);
	fMaxQuantizer = new Spinner("", B_TRANSLATE("Max video quantizer scale:"), nullptr);
	fQuantDiff
		= new Spinner("", B_TRANSLATE("Max difference between quantizer scale:"), nullptr);
	fQuantBlur = new Spinner("", B_TRANSLATE("Video quantizer scale blur:"), nullptr);
	fQuantCompression
		= new Spinner("", B_TRANSLATE("Video quantizer scale compression:"), nullptr);

	// Build Advanced Options layout
	BView* advancedoptionsview = new BView("", B_SUPPORTS_LAYOUT);
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

	return advancedoptionsview;
}


void
MainWindow::_BuildLogView()
{
	fLogView = new BTextView("");
	fLogView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fLogView->MakeEditable(false);
}


BView*
MainWindow::_BuildEncodeProgress()
{
	// Start/Stop, commandline, status bar
	fStartAbortButton = new BButton(B_TRANSLATE("Start"), new BMessage(M_ENCODE));
	fStartAbortButton->SetEnabled(false);
	fCommandlineTextControl = new BTextControl("", "", nullptr);

	// Build Encode/Progress view layout
	BView* encodeprogressview = new BView("encodeview", B_SUPPORTS_LAYOUT);
	BLayoutBuilder::Group<>(encodeprogressview, B_VERTICAL)
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, 0)
			.Add(fStartAbortButton)
			.Add(fCommandlineTextControl)
			.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.End();

	return encodeprogressview;
}


void
MainWindow::_BuildLine() // update the ffmpeg commandline
{
	// split existing commandline into tokens
	fCommand = fCommandlineTextControl->Text();
	fCommand.Trim();
	BStringList tokens_raw;
	fCommand.Split(" ", true, tokens_raw);

	// consolidate parts of quoted strings into single tokens
	BStringList tokens;
	for (int32 token_idx=0; token_idx<tokens_raw.CountStrings(); ++token_idx) {
		BString token = tokens_raw.StringAt(token_idx);
		if (token.StartsWith("\"")) {
			if (!token.EndsWith("\"")) {
				for (int32 part_idx=token_idx+1; part_idx<tokens_raw.CountStrings(); ++part_idx) {
					BString token_part = tokens_raw.StringAt(part_idx);
					token << " " << token_part;
					if (token_part.EndsWith("\"")) {
						token_idx = part_idx;
						break;
					}
				}
			}
		}

		tokens.Add(token);
	}

	BString value;

	// make sure ffmpeg is at the start of the command
	if (tokens.StringAt(0) != kFFMpeg) {
		tokens.Add(kFFMpeg, 0);
	}

	// input file
	value = (fSourceTextControl->Text());
	value.Trim();
	value.Prepend("\"");
	value.Append("\"");
	_SetParameter(tokens, "-i", value);

	// container format
	int32 option_index = fFileFormatPopup->FindMarkedIndex();
	_SetParameter(tokens, "-f", fContainerFormats[option_index].Option);

	// video options
	if ((fEnableVideoBox->Value() == B_CONTROL_ON) and (fEnableVideoBox->IsEnabled())) {
		option_index = fVideoFormatPopup->FindMarkedIndex();
		_RemoveParameter(tokens, "-vn");
		_SetParameter(tokens, "-vcodec", fVideoCodecs[option_index].Option);

		if (option_index != 0) {
			value = "";
			value << fVideoBitrateSpinner->Value() << "k";
			_SetParameter(tokens, "-b:v", value);
			value = "";
			value << fFramerate->Value();
			_SetParameter(tokens, "-r", value);
			if (fCustomResolutionBox->IsEnabled() && fCustomResolutionBox->Value()) {
				value = "";
				value << fXres->Value() << "x" << fYres->Value();
				_SetParameter(tokens, "-s", value);
			}
			else {
				_RemoveParameter(tokens, "-s");
			}

			// cropping options
			int32 topcrop = fTopCrop->Value();
			int32 bottomcrop = fBottomCrop->Value();
			int32 leftcrop = fLeftCrop->Value();
			int32 rightcrop = fRightCrop->Value();

			if ((topcrop + bottomcrop + leftcrop + rightcrop) > 0) {
				value = "";
				value << "crop=iw-" << leftcrop + rightcrop << ":ih-"
						<< topcrop + bottomcrop << ":" << leftcrop
						<< ":" << topcrop;
				_SetParameter(tokens, "-vf", value);
			}
			else {
				_RemoveParameter(tokens, "-vf");
			}
		}
		else {
			_RemoveParameter(tokens, "-b:v");
			_RemoveParameter(tokens, "-r");
			_RemoveParameter(tokens, "-s");
			_RemoveParameter(tokens, "-vf");
		}
	}
	else {
		_RemoveParameter(tokens, "-vcodec");
		_SetParameter(tokens, "-vn", "");
	}

	//audio options
	if (fEnableAudioBox->Value() == B_CONTROL_ON) {
		option_index = fAudioFormatPopup->FindMarkedIndex();
		_RemoveParameter(tokens, "-an");
		value = fAudioCodecs[option_index].Option;
		_SetParameter(tokens, "-acodec", value);

		if (option_index != 0) {
			value = fAudioBitsPopup->FindMarked()->Label();
			value << "k";
			_SetParameter(tokens, "-b:a", value);
			value = fSampleratePopup->FindMarked()->Label();
			_SetParameter(tokens, "-ar", value);
			value = (fChannelCount->Value());
			_SetParameter(tokens, "-ac", value);
			_SetParameter(tokens, "-strict", "-2"); // enable 'experimental codecs' needed for dca (DTS)
		}
		else {
			_RemoveParameter(tokens, "-b:a");
			_RemoveParameter(tokens, "-ar");
			_RemoveParameter(tokens, "-ac");
		}
	}
	else {
		_RemoveParameter(tokens, "-acodec");
		_RemoveParameter(tokens, "-b:a");
		_RemoveParameter(tokens, "-ar");
		_RemoveParameter(tokens, "-ac");
		_SetParameter(tokens, "-an", "");

		fCommand << (" -an");

	}

	//logging and output formatting

	// output file


	// assemble the commandline from the token list and put it in the textcontrol
	fCommand.SetTo("");

	for (int32 i=0; i<tokens.CountStrings(); ++i)
	{
		printf("%d: %s\n", i, tokens.StringAt(i).String());
		fCommand << tokens.StringAt(i) << " ";
	}

	fCommand.Trim();
	fCommandlineTextControl->SetText(fCommand);


/*
	BString source_filename(fSourceTextControl->Text());
	BString output_filename(fOutputTextControl->Text());
	source_filename.Trim();
	output_filename.Trim();
	fCommand = kFFMpeg;
	fCommand << " -i \"" << source_filename << "\""; // append the input file name

	// file format
	int32 option_index = fFileFormatPopup->FindMarkedIndex();
	BString fileformat_option = fContainerFormats[option_index].Option;
	fCommand << " -f " << fileformat_option; // grab and set the file format

	// is video enabled, add options
	if ((fEnableVideoBox->Value() == B_CONTROL_ON) and (fEnableVideoBox->IsEnabled())) {
		option_index = fVideoFormatPopup->FindMarkedIndex();
		fCommand << " -vcodec " << fVideoCodecs[option_index].Option;
		if (option_index != 0) {
			fCommand << " -b:v " << fVideoBitrateSpinner->Value() << "k";
			fCommand << " -r " << fFramerate->Value();
			if (fCustomResolutionBox->IsEnabled() && fCustomResolutionBox->Value())
				fCommand << " -s " << fXres->Value() << "x" << fYres->Value();

			// cropping options
			int32 topcrop = fTopCrop->Value();
			int32 bottomcrop = fBottomCrop->Value();
			int32 leftcrop = fLeftCrop->Value();
			int32 rightcrop = fRightCrop->Value();

			if ((topcrop + bottomcrop + leftcrop + rightcrop) > 0) {
				fCommand << " -vf crop=iw-" << leftcrop + rightcrop << ":ih-"
						<< topcrop + bottomcrop << ":" << leftcrop
						<< ":" << topcrop;
			}
		}
	} else
		fCommand << " -vn";

	// audio encoding enabled, grab the values
	if (fEnableAudioBox->Value() == B_CONTROL_ON) {
		option_index = fAudioFormatPopup->FindMarkedIndex();
		fCommand << " -acodec " << fAudioCodecs[option_index].Option;
		if (option_index != 0) {
			fCommand << " -b:a " << std::atoi(fAudioBitsPopup->FindMarked()->Label()) << "k";
			fCommand << " -ar " << std::atoi(fSampleratePopup->FindMarked()->Label());
			fCommand << " -ac " << fChannelCount->Value();
			fCommand << " -strict -2 "; // enable 'experimental codecs' needed for dca (DTS)
		}
	} else
		fCommand << (" -an");

	fCommand << " \"" << output_filename << "\"";
	fCommand << " -loglevel error -stats";
	*/

	fCommandlineTextControl->SetText(fCommand.String());
}


void
MainWindow::_SetParameter(BStringList& param_list, const BString& name, const BString& value)
{
	int32 param_index;
	if (param_list.HasString(name)) {
		param_index = param_list.IndexOf(name);
	}
	else {
		param_index = param_list.CountStrings() - 1;
		param_list.Add(name, param_index);
	}

	if (param_list.StringAt(param_index + 1).StartsWith("-")) { // no parameter value
		param_list.Add(value, param_index+1);
	}
	else {
		if (!param_list.Replace(param_index+1, value))
			param_list.Add(value, param_index+1);
	}
}


void
MainWindow::_RemoveParameter(BStringList& param_list, const BString& name)
{
	if (param_list.HasString(name)) {
		int32 param_index = param_list.IndexOf(name);
		if (!param_list.StringAt(param_index+1).StartsWith("-")) {
			param_list.Remove(param_index+1);
		}
		param_list.Remove(param_index);
	}
}


void
MainWindow::_GetMediaInfo()
{
	// Reset media info and video/audio tags
	fMediainfo = fVideoCodec = fAudioCodec = fVideoWidth = fVideoHeight = fVideoFramerate
		= fDuration = fVideoBitrate = fAudioBitrate = fAudioSamplerate = fAudioChannelLayout = "";
	BString command;
	command << kFFProbe << " -v error -show_entries format=duration,bit_rate:"
			   "stream=codec_name,width,height,r_frame_rate,sample_rate,"
			   "channel_layout "
			   "-of default=noprint_wrappers=1 -select_streams v:0 "
			<< "\"" << fSourceTextControl->Text() << "\" ; ";
	command << kFFProbe << " -v error -show_entries format=duration:"
			   "stream=codec_name,sample_rate,channels,channel_layout,bit_rate "
			   "-of default=noprint_wrappers=1 -select_streams a:0 "
			<< "\"" << fSourceTextControl->Text() << "\"";

	BMessage get_info_message(M_INFO_COMMAND);
	get_info_message.AddString("cmdline", command);
	fCommandLauncher->PostMessage(&get_info_message);
}


void
MainWindow::_UpdateMediaInfo()
{
	if (fVideoFramerate != "" && fVideoFramerate != "N/A") {
		// Convert fractional representation (e.g. 50/1) to floating number
		BStringList calclist;
		bool status = fVideoFramerate.Split("/", true, calclist);
		if (calclist.StringAt(1) != "0") {
			float rate = atof(calclist.StringAt(0)) / atof(calclist.StringAt(1));
			fVideoFramerate.SetToFormat("%.3f", rate);
			remove_over_precision(fVideoFramerate);
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
		remove_over_precision(fAudioSamplerate);
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
		seconds_to_string(fEncodeDuration, durationText, sizeof(durationText));
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
	_ReadyToEncode();
}


void
MainWindow::_ParseMediaOutput()
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
MainWindow::_ExtractPreviewImage()
{
	BMessage extract_image_message(M_EXTRACTIMAGE_COMMAND);
	BPath source_path(fSourceTextControl->Text());
	find_directory(B_SYSTEM_TEMP_DIRECTORY, &fPreviewPath);
	BString preview_filename(source_path.Leaf());
	preview_filename.Append("_preview.jpg");
	preview_filename.Prepend("ffmpegGUI_");
	fPreviewPath.Append(preview_filename);

	// skip first and last second of the clip (often black)
	int32 min = 1;
	int32 max = fEncodeDuration - 1;
	// Generate random time
	int32 randomSecond = min + (rand() % static_cast<int>(max - min + 1));
	// Convert seconds to HH:MM:SS
	char randomTime[64];
	seconds_to_string(randomSecond, randomTime, sizeof(randomTime));

	BString extract_image_cmd;
	extract_image_cmd	<< "ffmpeg -y -ss " << randomTime << " -i \"" << source_path.Path()
						<< "\" -qscale:v 2 -vframes 1 \"" << fPreviewPath.Path() << "\"";
	extract_image_message.AddString("cmdline", extract_image_cmd);
	fCommandLauncher->PostMessage(&extract_image_message);
}


void
MainWindow::_DeleteTempFiles()
{
	BPath temp_path;
	find_directory(B_SYSTEM_TEMP_DIRECTORY, &temp_path);
	BDirectory temp_dir(temp_path.Path());
	BEntry current_entry;
	BPath current_path;
	while (temp_dir.GetNextEntry(&current_entry) == B_OK) {
		current_entry.GetPath(&current_path);
		BString current_filename(current_path.Leaf());
		if (current_filename.StartsWith("ffmpegGUI_"))
			current_entry.Remove();
	}
}


void
MainWindow::_AdoptDefaults()
{
	if (!fVideoBitrate.IsEmpty() && fVideoBitrate != "N/A")
		fVideoBitrateSpinner->SetWithoutInvoke(atoi(fVideoBitrate));

	if (!fVideoFramerate.IsEmpty() && fVideoFramerate != "N/A") {
		int precision = _Precision(fVideoFramerate);
		fFramerate->SetPrecision(precision);
		fFramerate->TextView()->SetText(fVideoFramerate);
		fFramerate->SetFromTextWithoutInvoke();
	}

	if (!fAudioSamplerate.IsEmpty() && fAudioSamplerate != "N/A") {
		BString rate;
		rate.SetToFormat("%.f", atof(fAudioSamplerate) * 1000);
		BMenuItem* item = fSampleratePopup->FindItem(rate);
		if (item != NULL)
			item->SetMarked(true);
	}

	if (!fAudioChannels.IsEmpty() && fAudioChannels != "N/A")
		fChannelCount->SetWithoutInvoke(atoi(fAudioChannels));

	_BuildLine();
}


int
MainWindow::_Precision(BString& float_string)
{
	int32 point = float_string.FindFirst(".");
	int32 places = float_string.CountChars() - (point + 1);
	return ((point == B_ERROR) ? 0 : places);
}


void
MainWindow::_SetDefaults()
{
	// set the initial values
	fVideoBitrateSpinner->SetWithoutInvoke(1000);
	fFramerate->SetWithoutInvoke(int32(30));
	fXres->SetWithoutInvoke(1280);
	fYres->SetWithoutInvoke(720);

	fTopCrop->SetWithoutInvoke(0);
	fBottomCrop->SetWithoutInvoke(0);
	fLeftCrop->SetWithoutInvoke(0);
	fRightCrop->SetWithoutInvoke(0);

	fAudioBitsPopup->ItemAt(2)->SetMarked(true);
	fSampleratePopup->ItemAt(1)->SetMarked(true);
	fChannelCount->SetWithoutInvoke(2);

	// set the default status
	fEnableVideoBox->SetValue(true);
	fEnableVideoBox->SetEnabled(B_CONTROL_ON);
	fEnableAudioBox->SetValue(true);
	fEnableAudioBox->SetEnabled(B_CONTROL_ON);

	fCustomResolutionBox->SetValue(false);
	fCustomResolutionBox->SetEnabled(B_CONTROL_OFF);
	fXres->SetEnabled(B_CONTROL_OFF);
	fYres->SetEnabled(B_CONTROL_OFF);

	// create internal logic
	_ToggleVideo();
	_ToggleCropping();
	_ToggleAudio();

	_BuildLine();
}


void
MainWindow::_PopulateCodecOptions()
{
	//	container formats (ffmpeg option, extension, description)
	fContainerFormats.push_back(
		ContainerOption("avi", "avi", "avi - AVI (Audio Video Interleaved)", CAP_AUDIO_VIDEO));
	fContainerFormats.push_back(
		ContainerOption("matroska", "mkv", "mkv - Matroska", CAP_AUDIO_VIDEO));
	fContainerFormats.push_back(
		ContainerOption("mp4", "mp4", "mp4 - MPEG-4 Part 14", CAP_AUDIO_VIDEO));
	fContainerFormats.push_back(
		ContainerOption("mpeg", "mpg", "mpg - MPEG-1 Systems/MPEG Program Stream", CAP_AUDIO_VIDEO));
	fContainerFormats.push_back(
		ContainerOption("ogg", "ogg", "ogg", CAP_AUDIO_VIDEO));
	fContainerFormats.push_back(
		ContainerOption("webm", "webm", "webm", CAP_AUDIO_VIDEO));
	fContainerFormats.push_back(
		ContainerOption("flac", "flac", "flac", CAP_AUDIO_ONLY));
	fContainerFormats.push_back(
		ContainerOption("mp3", "mp3", "mp3 - MPEG audio layer 3", CAP_AUDIO_ONLY));
	fContainerFormats.push_back(
		ContainerOption("oga", "oga", "oga - Ogg Audio", CAP_AUDIO_ONLY));
	fContainerFormats.push_back(
		ContainerOption("wav", "wav", "wav - WAVE (Waveform Audio)", CAP_AUDIO_ONLY));

	// video codecs (ffmpeg option, short label, description)
	fVideoCodecs.push_back(CodecOption("copy", B_TRANSLATE("1:1 copy"), B_TRANSLATE("1:1 copy")));
	fVideoCodecs.push_back(CodecOption("mjpeg", "mjpeg", "mjpeg - Motion JPEG"));
	fVideoCodecs.push_back(CodecOption("mpeg4", "mpeg4", "mpeg4 - MPEG-4 part 2"));
	fVideoCodecs.push_back(CodecOption("theora", "theora", "theora"));
	fVideoCodecs.push_back(CodecOption("vp8", "vp8", "vp8 - On2 VP8"));
	fVideoCodecs.push_back(CodecOption("vp9", "vp9", "vp9 - Google VP9"));
	fVideoCodecs.push_back(CodecOption("wmv1", "wmv1", "wmv1 - Windows Media Video 7"));
	fVideoCodecs.push_back(CodecOption("wmv2", "wmv2", "wmv2 - Windows Media Video 8"));

	// audio codecs (ffmpeg option, short label, description)
	fAudioCodecs.push_back(CodecOption("copy", B_TRANSLATE("1:1 copy"), B_TRANSLATE("1:1 copy")));
	fAudioCodecs.push_back(CodecOption("aac", "aac", "aac - AAC (Advanced Audio Coding)"));
	fAudioCodecs.push_back(CodecOption("ac3", "ac3", "ac3 - ATSC A/52A (AC-3)"));
	fAudioCodecs.push_back(CodecOption("dts", "dts", "dts - DCA (DTS Coherent Acoustics)"));
	fAudioCodecs.push_back(CodecOption("flac", "flac", "flac (Free Lossless Audio Codec)"));
	fAudioCodecs.push_back(CodecOption("mp3", "mp3", "mp3 - MPEG audio layer 3"));
	fAudioCodecs.push_back(CodecOption("pcm_s16be", "pcm16", "pcm - signed 16-bit"));
	fAudioCodecs.push_back(CodecOption("libvorbis", "vorbis", "vorbis"));
}


bool
MainWindow::_FileExists(const char* filepath)
{
	BEntry entry(filepath);
	bool status = entry.Exists();

	return status;
}


void
MainWindow::_SetFileExtension()
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
MainWindow::_SetFiletype(entry_ref* ref)
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


void
MainWindow::_ReadyToEncode()
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
		fOutputTextControl->SetText("");
		fOutputCheckView->SetText("");
		ready = false;
	} else if (!_FileExists(source_filename)) {
		fMediaInfoView->SetText(B_TRANSLATE_NOCOLLECT(kSourceDoesntExist));
		fSourceTextControl->MarkAsInvalid(true);
		fOutputCheckView->SetText("");
		ready = false;
	}

	if (_FileExists(output_filename))
		fOutputCheckView->SetText(B_TRANSLATE_NOCOLLECT(kOutputExists));
	else
		fOutputCheckView->SetText("");

	if (output_filename == source_filename && !source_filename.IsEmpty()) {
		fOutputCheckView->SetText(B_TRANSLATE_NOCOLLECT(kOutputIsSource));
		fOutputTextControl->MarkAsInvalid(true);
		ready = false;
	}

	if (output_filename.IsEmpty())
		ready = false;

	_SetPlaybuttonsState();
	fStartAbortButton->SetEnabled(ready);
	fMenuStartEncode->SetEnabled(ready);
	fMenuAddJob->SetEnabled(ready);
}


void
MainWindow::_PlayVideo(const char* filepath)
{
	BEntry video_entry(filepath);
	entry_ref video_ref;
	video_entry.GetRef(&video_ref);
	be_roster->Launch(&video_ref);
}


void
MainWindow::_OpenHelp()
{
	BPathFinder pathFinder;
	BStringList paths;
	BPath path;

	pathFinder.FindPaths(B_FIND_PATH_DOCUMENTATION_DIRECTORY,
		"packages/ffmpegGUI", paths);
	if (!paths.IsEmpty()) {
		if (path.SetTo(paths.StringAt(0)) == B_OK) {
			path.Append("ReadMe.html");
			BMessage message(B_REFS_RECEIVED);
			message.AddString("url", path.Path());
			be_roster->Launch("text/html", &message);
		}
	}
}


void
MainWindow::_OpenURL(BString url)
{
	if (url.IsEmpty())
		return;

	BMessage message(B_REFS_RECEIVED);
	message.AddString("url", url);
	be_roster->Launch("application/x-vnd.Be.URL.http", &message);
}


void
MainWindow::_SetPlaybuttonsState()
{
	bool valid = _FileExists(fSourceTextControl->Text());
	fSourcePlayButton->SetEnabled(valid);
	fMenuPlaySource->SetEnabled(valid);

	valid = _FileExists(fOutputTextControl->Text());
	fOutputPlayButton->SetEnabled(valid);
	fMenuPlayOutput->SetEnabled(valid);
}


void
MainWindow::_ToggleVideo()
{
	bool video_options_enabled;

	if ((fEnableVideoBox->Value() == B_CONTROL_ON) and (fEnableVideoBox->IsEnabled())) {
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
MainWindow::_ToggleCropping()
{
	// disable cropping if video options are not enabled;
	bool cropping_options_enabled;
	if ((fEnableVideoBox->IsEnabled()) and (fEnableVideoBox->Value() == B_CONTROL_ON)
		and (fVideoFormatPopup->FindMarkedIndex() != 0))
		cropping_options_enabled = true;
	else
		cropping_options_enabled = false;

	fTopCrop->SetEnabled(cropping_options_enabled);
	fBottomCrop->SetEnabled(cropping_options_enabled);
	fLeftCrop->SetEnabled(cropping_options_enabled);
	fRightCrop->SetEnabled(cropping_options_enabled);
	fCropView->SetEnabled(cropping_options_enabled);
	fResetCroppingButton->SetEnabled(cropping_options_enabled);
	fNewPreviewButton->SetEnabled(cropping_options_enabled);
	fCroppingTab->SetEnabled(cropping_options_enabled);
	fTabView->Invalidate();
}


void
MainWindow::_ToggleAudio()
{
	bool audio_options_enabled;
	if (fEnableAudioBox->Value() == B_CONTROL_ON) {
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
