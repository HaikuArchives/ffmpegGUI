/*
 * Copyright 2003-2022, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	ffgui-window.cpp , 1/06/03
	Zach Dykstra
	Humdinger, humdingerb@gmail.com, 2022-2023
	Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022
*/

#include "ffgui-window.h"
#include "ffgui-application.h"
#include "ffgui-spinner.h"
#include "messages.h"
#include "commandlauncher.h"
#include "DurationToString.h"

#include <Alert.h>
#include <Box.h>
#include <Catalog.h>
#include <Entry.h>
#include <File.h>
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
#include <NodeInfo.h>
#include <Notification.h>
#include <PopUpMenu.h>
#include <Spinner.h>
#include <String.h>
#include <StringList.h>
#include <StringView.h>
#include <FilePanel.h>
#include <TabView.h>
#include <StringList.h>
#include <StatusBar.h>
#include <MenuBar.h>

#include <cstdlib>
#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Window"

static const char* kIdleText = B_TRANSLATE_MARK("Waiting to start encoding" B_UTF8_ELLIPSIS);


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
		commandline << " -b:a " << std::atoi(abpopup->FindMarked()->Label()) << "k";
		commandline << " -ar " << std::atoi(arpopup->FindMarked()->Label());
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
	// Invoker for the Alerts to use to send their messages to the timer
	fAlertInvoker.SetMessage(new BMessage(M_STOP_ALERT_BUTTON));
	fAlertInvoker.SetTarget(this);

	//initialize GUI elements
	fTopMenuBar = new BMenuBar("topmenubar");

	sourcefilebutton = new BButton(B_TRANSLATE("Source file"), new BMessage(M_SOURCE));
	sourcefilebutton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	sourcefile = new BTextControl("", "", NULL);
	sourcefile->SetModificationMessage(new BMessage(M_SOURCEFILE));

	outputfilebutton = new BButton(B_TRANSLATE("Output file"), new BMessage(M_OUTPUT));
	outputfilebutton->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));
	outputfile = new BTextControl("", "", NULL);
	outputfile->SetModificationMessage(new BMessage(M_OUTPUTFILE));

	mediainfo = new BStringView("mediainfo", B_TRANSLATE("Select a source file."));
	mediainfo->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	BFont font(be_plain_font);
	font.SetSize(ceilf(font.Size() * 0.9));
	mediainfo->SetFont(&font, B_FONT_SIZE);

	sourceplaybutton = new BButton("⯈", new BMessage(M_PLAY_SOURCE));
	outputplaybutton = new BButton("⯈", new BMessage(M_PLAY_OUTPUT));
	float height;
	sourcefile->GetPreferredSize(NULL, &height);
	BSize size(height, height);
	sourceplaybutton->SetExplicitSize(size);
	outputplaybutton->SetExplicitSize(size);
	sourceplaybutton->SetEnabled(false);
	outputplaybutton->SetEnabled(false);

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
	vbitrate = new ffguispinner("", B_TRANSLATE("Bitrate (Kbit/s):"), new BMessage(M_VBITRATE));
	framerate = new ffguispinner("", B_TRANSLATE("Framerate (fps):"), new BMessage(M_FRAMERATE));
	customres = new BCheckBox("", B_TRANSLATE("Use custom resolution"), new BMessage(M_CUSTOMRES));
	xres = new ffguispinner("", B_TRANSLATE("Width:"), new BMessage(M_XRES));
	yres = new ffguispinner("", B_TRANSLATE("Height:"), new BMessage(M_YRES));

	enablecropping = new BCheckBox("", B_TRANSLATE("Enable video cropping"), new BMessage(M_ENABLECROPPING));
	enablecropping->SetValue(B_CONTROL_OFF);
	topcrop = new ffguispinner("", B_TRANSLATE("Top:"), new BMessage(M_TOPCROP));
	bottomcrop = new ffguispinner("", B_TRANSLATE("Bottom:"), new BMessage(M_BOTTOMCROP));
    leftcrop = new ffguispinner("", B_TRANSLATE("Left:"), new BMessage(M_LEFTCROP));
	rightcrop = new ffguispinner("", B_TRANSLATE("Right:"), new BMessage(M_RIGHTCROP));

	enableaudio = new BCheckBox("", B_TRANSLATE("Enable audio encoding"), new BMessage(M_ENABLEAUDIO));
	enableaudio->SetValue(B_CONTROL_ON);

	abpopup = new BPopUpMenu("");
	abpopup->AddItem(new BMenuItem("96", new BMessage(M_AB)));
	abpopup->AddItem(new BMenuItem("128", new BMessage(M_AB)));
	abpopup->AddItem(new BMenuItem("160", new BMessage(M_AB)));
	abpopup->AddItem(new BMenuItem("196", new BMessage(M_AB)));
	abpopup->AddItem(new BMenuItem("320", new BMessage(M_AB)));
	abpopup->AddItem(new BMenuItem("1411", new BMessage(M_AB)));
	abpopup->ItemAt(1)->SetMarked(true);
	ab = new BMenuField(B_TRANSLATE("Bitrate (Kbit/s):"), abpopup);
	arpopup = new BPopUpMenu("");
	arpopup->AddItem(new BMenuItem("22050", new BMessage(M_AR)));
	arpopup->AddItem(new BMenuItem("44100", new BMessage(M_AR)));
	arpopup->AddItem(new BMenuItem("48000", new BMessage(M_AR)));
	arpopup->AddItem(new BMenuItem("96000", new BMessage(M_AR)));
	arpopup->AddItem(new BMenuItem("192000", new BMessage(M_AR)));
	arpopup->ItemAt(1)->SetMarked(true);
	ar = new BMenuField(B_TRANSLATE("Sampling rate (Hz):"), arpopup);
	ac = new ffguispinner("", B_TRANSLATE("Audio channels:"), new BMessage(M_AC));

	bframes = new ffguispinner("", B_TRANSLATE("'B' frames:"), nullptr);
	gop = new ffguispinner("", B_TRANSLATE("GOP size:"), nullptr);
	highquality = new BCheckBox("",B_TRANSLATE("Use high quality settings"), new BMessage(M_HIGHQUALITY));
	fourmotion = new BCheckBox("", B_TRANSLATE("Use four motion vector"), new BMessage(M_FOURMOTION));
	deinterlace = new BCheckBox("", B_TRANSLATE("Deinterlace pictures"), new BMessage(M_DEINTERLACE));
	calcpsnr = new BCheckBox("", B_TRANSLATE("Calculate PSNR of compressed frames"), new BMessage(M_CALCPSNR));

	fixedquant = new ffguispinner("", B_TRANSLATE("Use fixed video quantiser scale:"), nullptr);
	minquant = new ffguispinner("", B_TRANSLATE("Min video quantiser scale:"), nullptr);
	maxquant = new ffguispinner("", B_TRANSLATE("Max video quantiser scale:"), nullptr);
	quantdifference = new ffguispinner("", B_TRANSLATE("Max difference between quantiser scale:"), nullptr);
	quantblur = new ffguispinner("", B_TRANSLATE("Video quantiser scale blur:"), nullptr);
	quantcompression = new ffguispinner("", B_TRANSLATE("Video quantiser scale compression:"), nullptr);

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

	fPlayCheck = new BCheckBox("play_finished", B_TRANSLATE("Play when finished"), NULL);
	fPlayCheck->SetValue(B_CONTROL_OFF);

	fStatusBar = new BStatusBar("");
	fStatusBar->SetText(kIdleText);

	// set the min and max values for the spin controls
	vbitrate->SetMinValue(64);
	vbitrate->SetMaxValue(50000);
	framerate->SetMinValue(1);
	framerate->SetMaxValue(60);
	xres->SetMinValue(160);
	xres->SetMaxValue(7680);
	yres->SetMinValue(120);
	yres->SetMaxValue(4320);

	// set the initial values
	vbitrate->SetValue(1000);
	framerate->SetValue(30);
	xres->SetValue(1280);
	yres->SetValue(720);
	ac->SetValue(2);

	// set minimum size for the spinners
	set_spinner_minsize(vbitrate);
	set_spinner_minsize(framerate);
	set_spinner_minsize(xres);
	set_spinner_minsize(yres);
	set_spinner_minsize(ac);
	set_spinner_minsize(topcrop);
	set_spinner_minsize(bottomcrop);
	set_spinner_minsize(leftcrop);
	set_spinner_minsize(rightcrop);

	// set step values for the spinners
	vbitrate->SetStep(100);

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
		.AddGrid(B_USE_SMALL_SPACING, 0.0)
			.Add(sourcefilebutton, 0, 0)
			.Add(sourcefile, 1, 0)
			.Add(sourceplaybutton, 2, 0)
			.Add(mediainfo, 1, 1, 2, 1)
			.Add(BSpaceLayoutItem::CreateVerticalStrut(B_USE_SMALL_SPACING), 1, 2)
			.Add(outputfilebutton, 0, 3)
			.Add(outputfile, 1, 3)
			.Add(outputplaybutton, 2, 3)
			.SetColumnWeight(0, 0)
			.SetColumnWeight(1, 1)
			.SetColumnWeight(2, 0)
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
			.Add(ab->CreateMenuBarLayoutItem(),1,0)
			.Add(ar->CreateLabelLayoutItem(),0,1)
			.Add(ar->CreateMenuBarLayoutItem(),1,1)
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
			.AddGroup(B_VERTICAL)
				.AddGlue()
				.Add(fPlayCheck)
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
			break;
		}
		case M_SOURCEFILE:
		{
			// Dropped files and from the file dialog end up setting off
			// outputfile's ModificationMessage. Only call get_media_info once:
			get_media_info();
		} // intentional fall-though
		case M_OUTPUTFILE:
		{
			BuildLine();
			set_encodebutton_state();
			set_playbuttons_state();
			break;
		}
		case M_OUTPUTFILEFORMAT:
		{
			BString outputfilename(outputfile->Text());
			outputfilename=outputfilename.Trim();
			if (!outputfilename.IsEmpty())
			{
				set_outputfile_extension();
				set_playbuttons_state();
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
			BPath path = outputfile->Text();
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
			sourcefile->SetText(file_path.Path());
			outputfile->SetText(file_path.Path());
			set_outputfile_extension();
			BuildLine();
			set_encodebutton_state();
			set_playbuttons_state();
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
			set_outputfile_extension();
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
			encode_duration = 0;
			parse_media_output();
			update_media_info();
			break;
		}
		case M_ENCODE:
		{
			encode_starttime = real_time_clock();
			encodebutton->SetLabel(B_TRANSLATE("Stop"));
			encodebutton->SetMessage(new BMessage(M_STOP_ENCODING));

			outputtext->SelectAll();
			outputtext->Clear();
			commandline.SetTo(encode->Text());
			commandline.Append(" -y");

			BString files_string;
			files_string << sourcefile->Text() << " -> " << outputfile->Text();
			fStatusBar->SetText(files_string.String());

			BMessage start_encode_message(M_ENCODE_COMMAND);
			start_encode_message.AddString("cmdline", commandline);
			fCommandLauncher->PostMessage(&start_encode_message);
			encode_time = 0;
			break;
		}
		case M_STOP_ENCODING:
		{
			time_t now = (time_t)real_time_clock();
			// Only show if encoding has been running for more than 30s
			if (now - encode_starttime > 30) {
				fStopAlert = new BAlert("abort", B_TRANSLATE(
					"Are you sure, that you want to abort the encoding?\n"),
					B_TRANSLATE("Cancel"), B_TRANSLATE("Stop encoding"));
				fStopAlert->SetShortcut(0, B_ESCAPE);
				fStopAlert->Go(&fAlertInvoker);
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
			}
			break;
		}
		case M_ENCODE_PROGRESS:
		{
			BString progress_data;
			message->FindString("data", &progress_data);
			outputtext->Insert(progress_data.String());
			outputtext->ScrollTo(0.0, 1000000.0);

			//calculate progress percentage
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

			break;
		}
		case M_ENCODE_FINISHED:
		{
			encodebutton->SetLabel(B_TRANSLATE("Encode"));
			encodebutton->SetMessage(new BMessage(M_ENCODE));

			fStatusBar->Reset();
			fStatusBar->SetText(kIdleText);

			status_t exit_code;
			message->FindInt32("exitcode", &exit_code);

			if (exit_code == ABORTED)
				break;

			BNotification encodeFinished(B_INFORMATION_NOTIFICATION);
			encodeFinished.SetGroup(B_TRANSLATE_SYSTEM_NAME("ffmpeg GUI"));
			BString title(B_TRANSLATE("Encoding"));

			if (exit_code == SUCCESS) {
				encodeFinished.SetContent(B_TRANSLATE("Encoding finished successfully!"));

				if (fPlayCheck->Value() == B_CONTROL_ON)
					play_video(outputfile->Text());
				else {
					BPath path = outputfile->Text();

					if (path.InitCheck() == B_OK) {
						title = path.Leaf();

						entry_ref ref;
						get_ref_for_path(path.Path(), &ref);
						set_filetype(&ref);
						encodeFinished.SetOnClickFile(&ref);
					}
				}
				set_playbuttons_state();
			}
			else
				encodeFinished.SetContent(B_TRANSLATE("Encoding failed."));

			encodeFinished.SetTitle(title);
			encodeFinished.Send();

			if (fStopAlert != NULL) {
				fStopAlert->Lock();
				fStopAlert->TextView()->SetText(B_TRANSLATE(
					"Too late! Encoding has already finished.\n"));
				BButton* button = fStopAlert->ButtonAt(1);
				button->SetLabel(B_TRANSLATE_COMMENT("Duh!",
					"Button label of stop-encoding-alert if you're too late..."));
				fStopAlert->Unlock();
			}
			break;
		}
		case M_PLAY_SOURCE:
		{
			play_video(sourcefile->Text());
			break;
		}
		case M_PLAY_OUTPUT:
		{
			play_video(outputfile->Text());
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

			BRect sourcefile_rect = sourcefile->Bounds();
			sourcefile->ConvertToScreen(&sourcefile_rect);
			BRect outputfile_rect = outputfile->Bounds();
			outputfile->ConvertToScreen(&outputfile_rect);

			// add padding around the text controls for a larger target
			float padding = (outputfile_rect.top - sourcefile_rect.bottom) / 2;
			sourcefile_rect.InsetBy(-padding, -padding);
			outputfile_rect.InsetBy(-padding, -padding);

			if (sourcefile_rect.Contains(drop_point)) {
				sourcefile->SetText(file_path.Path());
				outputfile->SetText(file_path.Path());
			} else if (outputfile_rect.Contains(drop_point))
				outputfile->SetText(file_path.Path());
			else
				break;

			set_outputfile_extension();
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void ffguiwin::get_media_info()
{
	// Reset media info and video/audio tags
	fMediainfo = fVideoCodec = fAudioCodec = fVideoWidth = fVideoHeight = fVideoFramerate
		= fDuration = fVideoBitrate = fAudioBitrate = fAudioSamplerate = fAudioChannelLayout
		= "";

	BString command;
	command << "ffprobe -v error -show_entries format=duration,bit_rate:"
		"stream=codec_name,width,height,r_frame_rate,sample_rate,channel_layout "
		"-of default=noprint_wrappers=1 -select_streams v:0 "
		<< "\"" << sourcefile->Text() << "\" ; ";
	command << "ffprobe -v error -show_entries format=duration:"
		"stream=codec_name,sample_rate,channel_layout,bit_rate "
		"-of default=noprint_wrappers=1 -select_streams a:0 "
		<< "\"" << sourcefile->Text() << "\"";

	BMessage get_info_message(M_INFO_COMMAND);
	get_info_message.AddString("cmdline", command);
	fCommandLauncher->PostMessage(&get_info_message);
}


void ffguiwin::parse_media_output()
{
	BStringList list;
	fMediainfo.ReplaceAll("\n", "=");
	fMediainfo.Split("=", true, list);

	if (list.HasString("width")) {	// video stream is always first
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
		fAudioChannelLayout = list.StringAt(list.IndexOf("channel_layout") + 1);

		if (fDuration == "") // if audio-only (not filled by video ffprobe above)
			fDuration = list.StringAt(list.IndexOf("duration") + 1);

		int32 index = list.IndexOf("bit_rate");
		fAudioBitrate = list.StringAt(index + 1);
	}
}


void ffguiwin::remove_over_precision(BString& float_string)
{
	// Remove trailing "0" and "."
	while (true) {
		if (float_string.EndsWith("0")) {
			float_string.Truncate(float_string.CountChars() - 1);
			if (float_string.EndsWith(".")) {
				float_string.Truncate(float_string.CountChars() - 1);
				break;
			}
		}
		else break;
	}
}


void ffguiwin::update_media_info()
{
	if (fVideoFramerate != "" && fVideoFramerate != "N/A") {
		// Convert fractional representation (e.g. 50/1) to floating number
		BStringList calclist;
		bool status = fVideoFramerate.Split("/", true, calclist);
		if (calclist.StringAt(1) != "0") {
			float framerate = atof(calclist.StringAt(0)) / atof(calclist.StringAt(1));
			fVideoFramerate.SetToFormat("%.3f", framerate);
			remove_over_precision(fVideoFramerate);
		}
	}
	if (fVideoBitrate != "" && fVideoFramerate != "N/A") {
		// Convert bits/s to kBit/s
		int32 vbitrate = int32(ceil(atof(fVideoBitrate) / 1024));
		fVideoBitrate.SetToFormat("%" B_PRId32, vbitrate);
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
		encode_duration = atoi(fDuration); // Also used to calculate progress bar
		// Convert seconds to HH:MM:SS
		char durationText[64];
		duration_to_string(encode_duration, durationText, sizeof(durationText));
		fDuration = durationText;
	}
	BString text;
	text << "📺: "  ;
	if (fVideoCodec == "")
		text << B_TRANSLATE("No video track");
	else {
		text << fVideoCodec << ", " << fVideoWidth << "x" << fVideoHeight << ", ";
		text << fVideoFramerate << " " << B_TRANSLATE("fps") << ", ";
		text << fVideoBitrate << " " << "Kbit/s";
	}
	text << "    🔈: " ;
	if (fAudioCodec == "")
		text << B_TRANSLATE("No audio track");
	else {
		text << fAudioCodec << ", " << fAudioSamplerate << " " << "kHz, ";
		text << fAudioChannelLayout << ", " << fAudioBitrate << " " <<  "Kbits/s";
	}
	text << "    🕛: "  << fDuration;

	mediainfo->SetText(text.String());
}


void ffguiwin::set_playbuttons_state()
{
	bool valid = file_exists(sourcefile->Text());
	sourceplaybutton->SetEnabled(valid);

	valid = file_exists(outputfile->Text());
	outputplaybutton->SetEnabled(valid);
}


bool ffguiwin::file_exists(const char* filepath)
{
	BEntry entry(filepath);
	bool status = entry.Exists();

	return status;
}


void ffguiwin::set_filetype(entry_ref* ref)
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
	BString output_filename(outputfile->Text());
	if (output_filename == "")
		output_filename = sourcefile->Text();

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
