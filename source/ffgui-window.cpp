/*
 * Copyright 2003-2022, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	ffgui-window.cpp , 1/06/03
	Zach Dykstra
	Humdinger, humdingerb@gmail.com, 2022
	Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022
*/

#include "ffgui-window.h"
#include "ffgui-application.h"
#include "messages.h"
#include "commandlauncher.h"

#include <Alert.h>
#include <Box.h>
#include <Catalog.h>
#include <Entry.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <Path.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <View.h>
#include <Roster.h>
#include <TextView.h>
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

#include <cstdlib>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Window"


void ffguiwin::BuildLine() // ask all the views what they hold, reset the command string
{
	BString commandline("ffmpeg -i ");
	commandline << "\"" << sourcefile->Text() << "\"";  //append the input file name

	//this really is a hack to get mkv output working. Should and will be replaced by a proper formats class
	//that handles format name, commandline option and file extension in a proper way
	BString fileformat_option(outputfileformat->MenuItem()->Label());
	if (fileformat_option == "mkv")
	{
		fileformat_option = "matroska";
	}
	commandline << " -f " << fileformat_option; // grab and set the file format

	if (benablevideo == false) // is video enabled, add options
	{
		commandline << " -vcodec " << outputvideoformat->MenuItem()->Label(); // grab and set the video encoder
		commandline << " -b:v " << vbitrate->Value() << "k";
		commandline << " -r " << framerate->Value();
		if (bcustomres == true)
		{
			commandline << " -s " << xres->Value() << "x" << yres->Value();
		}

		// cropping options
		if (benablecropping == true)
		{

			commandline << " -vf crop=iw-" << leftcrop->Value() + rightcrop->Value()
						<< ":ih-" << topcrop->Value()+bottomcrop->Value() << ":"
						<< leftcrop->Value() << ":" << topcrop->Value();
		}
	}
	else //nope, add the no video encoding flag
	{
		commandline << " -vn";
	}

	if (benableaudio == true) // audio encoding enabled, grab the values
	{
		commandline << " -acodec " << outputaudioformat->MenuItem()->Label();
		commandline << " -b:a " << ab->Value() << "k";
		commandline << " -ar " << ar->Value();
		commandline << " -ac " << ac->Value();
	}
	else
	{
		commandline << (" -an");
	}
	commandline << " \"" << outputfile->Text() << "\"";
	encode->SetText(commandline.String());
}

// new window object
ffguiwin::ffguiwin(BRect r, const char *name, window_type type, ulong mode)
	: BWindow(r,name,type,mode)
{
	//initialize GUI elements
	fTopMenuBar = new BMenuBar("topmenubar");

	sourcefilebutton = new BButton(B_TRANSLATE("Source file"), new BMessage(M_SOURCE));
	sourcefilebutton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	sourcefile = new BTextControl("", "", new BMessage(M_SOURCEFILE));

	outputfilebutton = new BButton(B_TRANSLATE("Output file"), new BMessage(M_OUTPUT));
	outputfilebutton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	outputfile = new BTextControl("", "", new BMessage(M_OUTPUTFILE));

	outputfileformatpopup = new BPopUpMenu("");
	outputfileformatpopup->AddItem(new BMenuItem("avi", new BMessage(M_OUTPUTFILEFORMAT)));
	outputfileformatpopup->AddItem(new BMenuItem("vcd", new BMessage(M_OUTPUTFILEFORMAT)));
	outputfileformatpopup->AddItem(new BMenuItem("mp4", new BMessage(M_OUTPUTFILEFORMAT)));
	outputfileformatpopup->AddItem(new BMenuItem("mpeg", new BMessage(M_OUTPUTFILEFORMAT)));
	outputfileformatpopup->AddItem(new BMenuItem("mkv", new BMessage(M_OUTPUTFILEFORMAT)));
	outputfileformatpopup->AddItem(new BMenuItem("webm", new BMessage(M_OUTPUTFILEFORMAT)));
	outputfileformatpopup->ItemAt(0)->SetMarked(true);
	outputfileformat = new BMenuField(B_TRANSLATE("Output file format:"), outputfileformatpopup);
	BSize menuWidth = outputfileformat->PreferredSize();
	menuWidth.height=B_SIZE_UNSET;
	outputfileformat->SetExplicitMinSize(menuWidth);

	outputvideoformatpopup = new BPopUpMenu("");
	outputvideoformatpopup->AddItem(new BMenuItem("mpeg4", new BMessage(M_OUTPUTVIDEOFORMAT)));
	outputvideoformatpopup->AddItem(new BMenuItem("vp7", new BMessage(M_OUTPUTVIDEOFORMAT)));
	outputvideoformatpopup->AddItem(new BMenuItem("vp8", new BMessage(M_OUTPUTVIDEOFORMAT)));
	outputvideoformatpopup->AddItem(new BMenuItem("vp9", new BMessage(M_OUTPUTVIDEOFORMAT)));
	outputvideoformatpopup->AddItem(new BMenuItem("wmv1", new BMessage(M_OUTPUTVIDEOFORMAT)));
	outputvideoformatpopup->ItemAt(0)->SetMarked(true);
	outputvideoformat = new BMenuField(B_TRANSLATE("Output video format:"), outputvideoformatpopup);
	menuWidth = outputvideoformat->PreferredSize();
	menuWidth.height=B_SIZE_UNSET;
	outputvideoformat->SetExplicitMinSize(menuWidth);

	outputaudioformatpopup = new BPopUpMenu("");
	outputaudioformatpopup->AddItem(new BMenuItem("ac3", new BMessage(M_OUTPUTAUDIOFORMAT)));
	outputaudioformatpopup->AddItem(new BMenuItem("aac", new BMessage(M_OUTPUTAUDIOFORMAT)));
	outputaudioformatpopup->AddItem(new BMenuItem("opus", new BMessage(M_OUTPUTAUDIOFORMAT)));
	outputaudioformatpopup->AddItem(new BMenuItem("vorbis", new BMessage(M_OUTPUTAUDIOFORMAT)));
	outputaudioformatpopup->ItemAt(0)->SetMarked(true);
	outputaudioformat = new BMenuField(B_TRANSLATE("Output audio format:"), outputaudioformatpopup);
	menuWidth = outputaudioformat->PreferredSize();
	menuWidth.height=B_SIZE_UNSET;
	outputaudioformat->SetExplicitMinSize(menuWidth);

	enablevideo = new BCheckBox("", B_TRANSLATE("Enable video encoding"), new BMessage(M_ENABLEVIDEO));
	enablevideo->SetValue(B_CONTROL_ON);
	vbitrate = new BSpinner("", B_TRANSLATE("Bitrate (Kbit/s):"), new BMessage(M_VBITRATE));
	framerate = new BSpinner("", B_TRANSLATE("Framerate (fps):"), new BMessage(M_FRAMERATE));
	customres = new BCheckBox("", B_TRANSLATE("Use custom resolution"), new BMessage(M_CUSTOMRES));
	xres = new BSpinner("", B_TRANSLATE("Width:"), new BMessage(M_XRES));
	yres = new BSpinner("", B_TRANSLATE("Height:"), new BMessage(M_YRES));

	enablecropping = new BCheckBox("", B_TRANSLATE("Enable video cropping"), new BMessage(M_ENABLECROPPING));
	enablecropping->SetValue(B_CONTROL_OFF);
	topcrop = new BSpinner("", B_TRANSLATE("Top:"), new BMessage(M_TOPCROP));
	bottomcrop = new BSpinner("", B_TRANSLATE("Bottom:"), new BMessage(M_BOTTOMCROP));
    leftcrop = new BSpinner("", B_TRANSLATE("Left:"), new BMessage(M_LEFTCROP));
	rightcrop = new BSpinner("", B_TRANSLATE("Right:"), new BMessage(M_RIGHTCROP));

	enableaudio = new BCheckBox("", B_TRANSLATE("Enable audio encoding"), new BMessage(M_ENABLEAUDIO));
	enableaudio->SetValue(B_CONTROL_ON);
	ab = new BSpinner("", B_TRANSLATE("Bitrate (Kbit/s):"), new BMessage(M_AB));
	ar = new BSpinner("", B_TRANSLATE("Sampling rate (Hz):"), new BMessage(M_AR));
	ac = new BSpinner("", B_TRANSLATE("Audio channels:"), new BMessage(M_AC));

	bframes = new BSpinner("", B_TRANSLATE("'B' frames:"), nullptr);
	gop = new BSpinner("", B_TRANSLATE("GOP size:"), nullptr);
	highquality = new BCheckBox("",B_TRANSLATE("Use high quality settings"), new BMessage(M_HIGHQUALITY));
	fourmotion = new BCheckBox("", B_TRANSLATE("Use four motion vector"), new BMessage(M_FOURMOTION));
	deinterlace = new BCheckBox("", B_TRANSLATE("Deinterlace pictures"), new BMessage(M_DEINTERLACE));
	calcpsnr = new BCheckBox("", B_TRANSLATE("Calculate PSNR of compressed frames"), new BMessage(M_CALCPSNR));

	fixedquant = new BSpinner("", B_TRANSLATE("Use fixed video quantiser scale:"), nullptr);
	minquant = new BSpinner("", B_TRANSLATE("Min video quantiser scale:"), nullptr);
	maxquant = new BSpinner("", B_TRANSLATE("Max video quantiser scale:"), nullptr);
	quantdifference = new BSpinner("", B_TRANSLATE("Max difference between quantiser scale:"), nullptr);
	quantblur = new BSpinner("", B_TRANSLATE("Video quantiser scale blur:"), nullptr);
	quantcompression = new BSpinner("", B_TRANSLATE("Video quantiser scale compression:"), nullptr);

	outputtext = new BTextView("");
	outputtext->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	outputtext->MakeEditable(false);

	encodebutton = new BButton(B_TRANSLATE("Start"), new BMessage(M_ENCODE));
	encodebutton->SetEnabled(false);
	encode = new BTextControl("", "", nullptr);

	fSourceFilePanel = new BFilePanel(B_OPEN_PANEL,
									new BMessenger(this),
									NULL,
									B_FILE_NODE,
									false,
									new BMessage(M_SOURCEFILE_REF));

	fOutputFilePanel = new BFilePanel(B_SAVE_PANEL,
									new BMessenger(this),
									NULL,
									B_FILE_NODE,
									false,
									new BMessage(M_OUTPUTFILE_REF));

	fStatusBar = new BStatusBar("");
	fStatusBar->SetText(B_TRANSLATE("Waiting to start encoding" B_UTF8_ELLIPSIS));

	// set the min and max values for the spin controls
	vbitrate->SetMinValue(64);
	vbitrate->SetMaxValue(50000);
	framerate->SetMinValue(1);
	framerate->SetMaxValue(60);
	xres->SetMinValue(160);
	xres->SetMaxValue(7680);
	yres->SetMinValue(120);
	yres->SetMaxValue(4320);
	ab->SetMinValue(16);
	ab->SetMaxValue(500);
	ar->SetMaxValue(192000);

	// set the initial values
	vbitrate->SetValue(1000);
	framerate->SetValue(30);
	xres->SetValue(1280);
	yres->SetValue(720);
	ab->SetValue(128);
	ar->SetValue(44100);
	ac->SetValue(2);

	// set minimum size for the spinners
	set_spinner_minsize(vbitrate);
	set_spinner_minsize(framerate);
	set_spinner_minsize(xres);
	set_spinner_minsize(yres);
	set_spinner_minsize(ab);
	set_spinner_minsize(ar);
	set_spinner_minsize(ac);
	set_spinner_minsize(topcrop);
	set_spinner_minsize(bottomcrop);
	set_spinner_minsize(leftcrop);
	set_spinner_minsize(rightcrop);

	// set the default status for the conditional spinners
	benablecropping = false;
	benableaudio = true;
	bcustomres = false;
	benablevideo = false; // changing this breaks UI enabled/disabled behaviour
	xres->SetEnabled(false);
	yres->SetEnabled(false);

	// set the initial command line
	BuildLine();

	// create tabs and boxes
	BBox *fileoptionsbox = new BBox("");
	fileoptionsbox->SetLabel(B_TRANSLATE("File options"));
	BGroupLayout *fileoptionslayout = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
					B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGrid(B_USE_DEFAULT_SPACING)
			.Add(sourcefilebutton, 0, 0)
			.Add(sourcefile, 1, 0)
			.Add(outputfilebutton, 0, 1)
			.Add(outputfile, 1, 1)
			.SetColumnWeight(0, 0)
			.SetColumnWeight(1, 1)
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(outputfileformat)
			.AddStrut(B_USE_SMALL_SPACING)
			.Add(outputvideoformat)
			.AddStrut(B_USE_SMALL_SPACING)
			.Add(outputaudioformat)
		.End();
	fileoptionsbox->AddChild(fileoptionslayout->View());

	BBox *encodebox = new BBox("");
	encodebox->SetLabel(B_TRANSLATE("Encode"));
	BGroupLayout *encodelayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
					B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(encodebutton)
			.Add(encode)
		.End();
	encodebox->AddChild(encodelayout->View());

	BBox *videobox = new BBox("");
	videobox->SetLabel(B_TRANSLATE("Video"));
	BGroupLayout *videolayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
					B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(enablevideo)
		.AddGrid(B_USE_SMALL_SPACING,B_USE_SMALL_SPACING)
			.Add(vbitrate->CreateLabelLayoutItem(),0,0)
			.Add(vbitrate->CreateTextViewLayoutItem(),1,0)
			.Add(framerate->CreateLabelLayoutItem(),0,1)
			.Add(framerate->CreateTextViewLayoutItem(),1,1)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(customres)
		.AddGrid(B_USE_SMALL_SPACING,B_USE_SMALL_SPACING)
			.Add(xres->CreateLabelLayoutItem(),0,0)
			.Add(xres->CreateTextViewLayoutItem(),1,0)
			.Add(yres->CreateLabelLayoutItem(),0,1)
			.Add(yres->CreateTextViewLayoutItem(),1,1)
		.End();
	videobox->AddChild(videolayout->View());

	BBox *croppingoptionsbox = new BBox("");
	croppingoptionsbox->SetLabel(B_TRANSLATE("Cropping options"));
	BGroupLayout *croppingoptionslayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
					B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(enablecropping)
		.AddGrid(B_USE_SMALL_SPACING,B_USE_SMALL_SPACING)
			.Add(topcrop->CreateLabelLayoutItem(),0,0)
			.Add(topcrop->CreateTextViewLayoutItem(),1,0)
			.Add(bottomcrop->CreateLabelLayoutItem(),0,1)
			.Add(bottomcrop->CreateTextViewLayoutItem(),1,1)
			.Add(leftcrop->CreateLabelLayoutItem(),0,2)
			.Add(leftcrop->CreateTextViewLayoutItem(),1,2)
			.Add(rightcrop->CreateLabelLayoutItem(),0,3)
			.Add(rightcrop->CreateTextViewLayoutItem(),1,3)
		.End()
		.AddGlue();
	croppingoptionsbox->AddChild(croppingoptionslayout->View());

	BBox *audiobox = new BBox("");
	audiobox->SetLabel(B_TRANSLATE("Audio"));
	BGroupLayout *audiolayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
					B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(enableaudio)
		.AddGrid(B_USE_SMALL_SPACING,B_USE_SMALL_SPACING)
			.Add(ab->CreateLabelLayoutItem(),0,0)
			.Add(ab->CreateTextViewLayoutItem(),1,0)
			.Add(ar->CreateLabelLayoutItem(),0,1)
			.Add(ar->CreateTextViewLayoutItem(),1,1)
			.Add(ac->CreateLabelLayoutItem(),0,2)
			.Add(ac->CreateTextViewLayoutItem(),1,2)
		.End()
		.AddGlue();
	audiobox->AddChild(audiolayout->View());

	BView *mainoptionsview = new BView("",B_SUPPORTS_LAYOUT);
	BView *advancedoptionsview = new BView("",B_SUPPORTS_LAYOUT);
	BView *outputview = new BScrollView("", outputtext, B_SUPPORTS_LAYOUT, true, true);

	BLayoutBuilder::Group<>(mainoptionsview, B_HORIZONTAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
					B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(videobox)
		.Add(croppingoptionsbox)
		.Add(audiobox)
		.Layout();

	BLayoutBuilder::Group<>(advancedoptionsview, B_HORIZONTAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
					B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGroup(B_VERTICAL)
			.AddGrid(B_USE_SMALL_SPACING,B_USE_SMALL_SPACING)
				.Add(bframes->CreateLabelLayoutItem(),0,0)
				.Add(bframes->CreateTextViewLayoutItem(),1,0)
				.Add(gop->CreateLabelLayoutItem(),0,1)
				.Add(gop->CreateTextViewLayoutItem(),1,1)
			.End()
			.Add(new BSeparatorView(B_HORIZONTAL))
			.Add(highquality)
			.Add(fourmotion)
			.Add(deinterlace)
			.Add(calcpsnr)
		.End()
		.Add(new BSeparatorView(B_VERTICAL))
		.AddGroup(B_VERTICAL)
		.AddGrid(B_USE_SMALL_SPACING,B_USE_SMALL_SPACING)
			.Add(fixedquant->CreateLabelLayoutItem(),0,0)
			.Add(fixedquant->CreateTextViewLayoutItem(),1,0)
			.Add(minquant->CreateLabelLayoutItem(),0,1)
			.Add(minquant->CreateTextViewLayoutItem(),1,1)
			.Add(maxquant->CreateLabelLayoutItem(),0,2)
			.Add(maxquant->CreateTextViewLayoutItem(),1,2)
			.Add(quantdifference->CreateLabelLayoutItem(),0,3)
			.Add(quantdifference->CreateTextViewLayoutItem(),1,3)
			.Add(quantblur->CreateLabelLayoutItem(),0,4)
			.Add(quantblur->CreateTextViewLayoutItem(),1,4)
			.Add(quantcompression->CreateLabelLayoutItem(),0,5)
			.Add(quantcompression->CreateTextViewLayoutItem(),1,5)
		.End()
		.AddGlue()
		.End();

	tabview = new BTabView("");
	BTab *mainoptionstab = new BTab();
	BTab *advancedoptionstab = new BTab();
	BTab *outputtab = new BTab();

	tabview->AddTab(mainoptionsview, mainoptionstab);
	//tabview->AddTab(advancedoptionsview, advancedoptionstab); //don´t remove, will be needed later
	tabview->AddTab(outputview, outputtab);
	mainoptionstab->SetLabel(B_TRANSLATE("Main options"));
	advancedoptionstab->SetLabel(B_TRANSLATE("Advanced options"));
	outputtab->SetLabel(B_TRANSLATE("Output"));

	//menu bar layout
	BLayoutBuilder::Menu<>(fTopMenuBar)
		.AddMenu(B_TRANSLATE("App"))
			.AddItem(B_TRANSLATE("About"), B_ABOUT_REQUESTED)
			.AddSeparator()
			.AddItem(B_TRANSLATE("Quit"), B_QUIT_REQUESTED, 'Q')
		.End()
	.End();

	//main layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(-2,0,-2,0)
		.Add(fTopMenuBar)
		.Add(fileoptionsbox)
		.Add(tabview)
		.Add(encodebox)
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_DEFAULT_SPACING,0,B_USE_DEFAULT_SPACING,0)
			.Add(fStatusBar)
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

	//initialize command launcher
	fCommandLauncher = new CommandLauncher(new BMessenger(this));
}

//quitting
bool ffguiwin::QuitRequested()
{
	fCommandLauncher->PostMessage(B_QUIT_REQUESTED);
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

//message received
void ffguiwin::MessageReceived(BMessage *message)
{

	switch(message->what)
	{
		case B_ABOUT_REQUESTED:
		{
			be_app->PostMessage(B_ABOUT_REQUESTED);
		}
		case M_SOURCEFILE:
		case M_OUTPUTFILE:
		{
			BuildLine();
			set_encodebutton_state();
			break;
		}
		case M_OUTPUTFILEFORMAT:
		{
			BString outputfilename(outputfile->Text());
			outputfilename=outputfilename.Trim();
			if (!outputfilename.IsEmpty())
			{
				set_outputfile_extension();
			}
			BuildLine();
			break;
		}
		case M_OUTPUTVIDEOFORMAT:
		case M_OUTPUTAUDIOFORMAT:
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
			// turn all the video spin buttons off
			if (benablevideo == true)
			{
				vbitrate->SetEnabled(true);
				framerate->SetEnabled(true);
				customres->SetEnabled(true);

				if (bcustomres == true)
				{
					xres->SetEnabled(true);
					yres->SetEnabled(true);
				}

				enablecropping->SetEnabled(true);
				if (benablecropping == true)
				{
					topcrop->SetEnabled(true);
					bottomcrop->SetEnabled(true);
					leftcrop->SetEnabled(true);
					rightcrop->SetEnabled(true);
				}
				benablevideo = false;
				BuildLine();
			}
			else
			{
				vbitrate->SetEnabled(false);
				framerate->SetEnabled(false);
				customres->SetEnabled(false);
				xres->SetEnabled(false);
				yres->SetEnabled(false);
				enablecropping->SetEnabled(false);
				topcrop->SetEnabled(false);
				bottomcrop->SetEnabled(false);
				leftcrop->SetEnabled(false);
				rightcrop->SetEnabled(false);
				benablevideo = true;
				BuildLine();
			}
			break;
		}

		case M_CUSTOMRES:
		{
			if (bcustomres == false)
			{
				xres->SetEnabled(true);
				yres->SetEnabled(true);
				bcustomres = true;
				BuildLine();
			}
			else
			{
				xres->SetEnabled(false);
				yres->SetEnabled(false);
				bcustomres = false;
				BuildLine();
			}
			break;
		}

		case M_ENABLECROPPING:
		{
			//turn the cropping spinbuttons on or off, set a bool
			if (benablecropping == false)
			{
				topcrop->SetEnabled(true);
				bottomcrop->SetEnabled(true);
				leftcrop->SetEnabled(true);
				rightcrop->SetEnabled(true);
				benablecropping = true;
				BuildLine();
			}
			else
			{
				topcrop->SetEnabled(false);
				bottomcrop->SetEnabled(false);
				leftcrop->SetEnabled(false);
				rightcrop->SetEnabled(false);
				benablecropping = false;
				BuildLine();
			}
			break;
		}

		case M_ENABLEAUDIO:
		{
			//turn the audio spinbuttons on or off, set a bool
			if (benableaudio == true)
			{
				ab->SetEnabled(false);
				ac->SetEnabled(false);
				ar->SetEnabled(false);
				benableaudio = false;
				BuildLine();
			}
			else
			{
				ab->SetEnabled(true);
				ac->SetEnabled(true);
				ar->SetEnabled(true);
				benableaudio = true;
				BuildLine();
			}
			break;
		}
		case M_SOURCE:
		{
			fSourceFilePanel->Show();
			break;
		}
		case M_OUTPUT:
		{
			fOutputFilePanel->Show();
			break;
		}
		case M_SOURCEFILE_REF:
		{
			entry_ref ref;
			message->FindRef("refs", &ref);
			BEntry file_entry(&ref, true);
			BPath file_path(&file_entry);
			sourcefile->SetText(file_path.Path());
			outputfile->SetText(file_path.Path());
			set_outputfile_extension();
			BuildLine();
			set_encodebutton_state();
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

			outputfile->SetText(filename);
			BuildLine();
			set_encodebutton_state();
			break;
		}
		case M_ENCODE:
		{
			outputtext->SelectAll();
			outputtext->Clear();
			commandline.SetTo(encode->Text());
			commandline.Append(" -y");

			BString files_string;
			files_string << sourcefile->Text() << " -> " << outputfile->Text();
			fStatusBar->SetText(files_string.String());

			BMessage start_encode_message(M_RUN_COMMAND);
			start_encode_message.AddString("cmdline", commandline);
			fCommandLauncher->PostMessage(&start_encode_message);
			encode_duration = 0;
			encode_time = 0;
			duration_detected = false;
			break;
		}
		case M_PROGRESS:
		{
			BString progress_data;
			message->FindString("data", &progress_data);
			outputtext->Insert(progress_data.String());
			outputtext->ScrollTo(0.0, 1000000.0);

			//calculate progress percentage
			if (duration_detected) 	//the duration appears in the data all the time but
			{						//we need it only once

				int32 time_startpos = progress_data.FindFirst("time=");
				if (time_startpos > -1)
				{
					time_startpos+=5;
					int32 time_endpos = progress_data.FindFirst(".", time_startpos);
					BString time_string;
					progress_data.CopyInto(time_string, time_startpos, time_endpos-time_startpos);
					encode_time = get_seconds(time_string);

					int32 encode_percentage;
					if (encode_duration > 0)
					{
						encode_percentage = (encode_time * 100) / encode_duration;
					}
					else
					{
						encode_percentage = 0;
					}

					BMessage progress_update_message(B_UPDATE_STATUS_BAR);
					progress_update_message.AddFloat("delta", encode_percentage - fStatusBar->CurrentValue());
					BString percentage_string;
					percentage_string << encode_percentage << "%";
					progress_update_message.AddString("trailing_text", percentage_string.String());
					PostMessage(&progress_update_message, fStatusBar);
				}

			}
			else
			{
				int32 duration_startpos = progress_data.FindFirst("Duration:");
				if (duration_startpos > -1)
				{
					duration_startpos+=9;
					int32 duration_endpos = progress_data.FindFirst(".", duration_startpos);
					BString duration_string;
					progress_data.CopyInto(duration_string, duration_startpos, duration_endpos-duration_startpos);
					encode_duration = get_seconds(duration_string);
					duration_detected = true;
				}
			}

			break;
		}
		case M_COMMAND_FINISHED:
		{
			status_t exit_code;
			message->FindInt32("exitcode", &exit_code);
			BString finished_message;
			const char *play_button_label;

			if(exit_code == 0)
			{

				finished_message = B_TRANSLATE("Encoding finished successfully.");
				play_button_label = B_TRANSLATE("Play now");
			}
			else
			{
				finished_message << B_TRANSLATE("Encoding failed.");
				play_button_label = nullptr;
			}

			BAlert *finished_alert = new BAlert("", finished_message, B_TRANSLATE("OK"), play_button_label);
			int32 button = finished_alert->Go();
			if (button == 1)
			{
				play_video(outputfile->Text());
			}

			fStatusBar->Reset();
			fStatusBar->SetText(B_TRANSLATE("Waiting to start encoding" B_UTF8_ELLIPSIS));
			break;
		}
		case B_SIMPLE_DATA:
		{
			BPoint drop_point;
			message->FindPoint("_drop_point_", &drop_point);
			BRect sourcefile_rect = sourcefile->Bounds();
			sourcefile->ConvertToScreen(&sourcefile_rect);

			if(sourcefile_rect.Contains(drop_point))
			{
				entry_ref sourcefile_ref;
				message->FindRef("refs", &sourcefile_ref);
				BEntry sourcefile_entry(&sourcefile_ref, true);
				BPath sourcefile_path(&sourcefile_entry);
				sourcefile->SetText(sourcefile_path.Path());
				outputfile->SetText(sourcefile_path.Path());
				set_outputfile_extension();
				BuildLine();
				set_encodebutton_state();
			}

			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void ffguiwin::set_encodebutton_state()
{
	bool encodebutton_enabled;

	BString source_filename(sourcefile->Text());
	BString output_filename(outputfile->Text());
	source_filename.Trim();
	output_filename.Trim();


	if (!(source_filename.IsEmpty()) && !(output_filename.IsEmpty()))
	{
		encodebutton_enabled = true;
	}
	else
	{
		encodebutton_enabled = false;
	}

	encodebutton->SetEnabled(encodebutton_enabled);
}


int32
ffguiwin::get_seconds(BString& time_string)
{
	int32 hours = 0;
	int32 minutes = 0;
	int32 seconds = 0;
	BStringList time_list;

	time_string.Trim().Split(":", true, time_list);
	hours = std::atoi(time_list.StringAt(0).String());
	minutes = std::atoi(time_list.StringAt(1).String());
	seconds = std::atoi(time_list.StringAt(2).String());

	seconds+=minutes*60;
	seconds+=hours*3600;

	return seconds;
}


void
ffguiwin::set_outputfile_extension()
{
	BString output_filename(sourcefile->Text());
	int32 begin_ext = output_filename.FindLast(".");
	if (begin_ext != B_ERROR) //cut away extension if it already exists
	{
		++begin_ext;
		output_filename.RemoveChars(begin_ext, output_filename.Length()-begin_ext);
	}
	else
	{
		output_filename.Append(".");
	}

	output_filename.Append(outputfileformatpopup->FindMarked()->Label());
	outputfile->SetText(output_filename);
}


void
ffguiwin::set_spinner_minsize(BSpinner *spinner)
{
	BSize textview_prefsize = spinner->TextView()->PreferredSize();
	textview_prefsize.width+=20;
	textview_prefsize.height=B_SIZE_UNSET;
	spinner->CreateTextViewLayoutItem()->SetExplicitMinSize(textview_prefsize);
}

void
ffguiwin::play_video(const char* filepath)
{
	BEntry video_entry(filepath);
	entry_ref video_ref;
	video_entry.GetRef(&video_ref);
	be_roster->Launch(&video_ref);

}
