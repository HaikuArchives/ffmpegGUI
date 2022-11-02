/*
 * Copyright 2003, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	ffgui-window.cpp , 1/06/03
	Zach Dykstra
*/

#include <stdio.h>
#include "ffgui-window.h"
#include "ffgui-application.h"
#include "messages.h"
#include <MenuItem.h>
#include <LayoutBuilder.h>
#include <View.h>
#include <Box.h>
#include <SeparatorView.h>
#include <Entry.h>
#include <Path.h>
#include <Alert.h>
#include <ScrollView.h>

#include <cstdlib>
#include <iostream>


void ffguiwin::BuildLine() // ask all the views what they hold, reset the command string
{
	BString commandline("ffmpeg -i ");
	commandline << "\"" << sourcefile->Text() << "\"";  //append the input file name
	commandline << " -f " << outputfileformat->MenuItem()->Label(); // grab and set the file format

	if (benablevideo == false) // is video enabled, add options
	{
		commandline << " -vcodec " << outputvideoformat->MenuItem()->Label(); // grab and set the video encoder
		commandline << " -b:v " << vbitrate->Value();
		commandline << " -r " << framerate->Value();
		if (bcustomres == true)
		{
			commandline << "-s " << xres->Value() << "x" << yres->Value();
		}

		// cropping options -- no point in cropping if we aren't encoding video...
		if (benablecropping == true)
		{
			// below should be rewritten to use only one crop, but I can't be bothered
			// to do so as ffmpeg supports multiple filters stacked on each other
			if (topcrop->Value() != 0)
			{
				commandline << " -vf crop=w=in_w:h=in_h:x=0:y=" << topcrop->Value();
			}
			if (bottomcrop->Value() != 0)
			{
				commandline << " -vf crop=w=in_w:h=in_h-" << bottomcrop->Value() << ":x=0:y=0";
			}
			if (leftcrop->Value() != 0)
			{
				commandline << " -vf crop=w=in_w:h=in_h:x=" << leftcrop->Value() << ":y=0";
			}
			if (rightcrop->Value() != 0)
			{
				commandline << " -vf crop=w=in_w-" << rightcrop->Value() << ":h=in_h:x=0:y=0";
			}
		}
	}
	else //nope, add the no video encoding flag
	{
		commandline << " -vn";
	}

	if (benableaudio == true) // audio encoding enabled, grab the values
	{
		commandline << " -acodec " << outputaudioformat->MenuItem()->Label();
		commandline << " -b:a " << ab->Value();
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
ffguiwin::ffguiwin(BRect r, char *name, window_type type, ulong mode)
	: BWindow(r,name,type,mode)
{
	sourcefile_specified = false;
	outputfile_specified = false;

	//initialize GUI elements
	sourcefilebutton = new BButton("Source file", new BMessage(M_SOURCE));
	sourcefile = new BTextControl("", "", new BMessage(M_SOURCEFILE));
	outputfilebutton = new BButton("Output file", new BMessage(M_OUTPUT));
	outputfile = new BTextControl("", "", new BMessage(M_OUTPUTFILE));
	deletesource = new BCheckBox("", "Delete source when finished", new BMessage(M_DELETESOURCE));

	outputfileformatpopup = new BPopUpMenu("");
	outputfileformatpopup->AddItem(new BMenuItem("avi", new BMessage(M_OUTPUTFILEFORMAT)));
	outputfileformatpopup->AddItem(new BMenuItem("vcd", new BMessage(M_OUTPUTFILEFORMAT)));
	outputfileformatpopup->AddItem(new BMenuItem("mp4", new BMessage(M_OUTPUTFILEFORMAT)));
	outputfileformatpopup->AddItem(new BMenuItem("mpeg", new BMessage(M_OUTPUTFILEFORMAT)));
	outputfileformatpopup->AddItem(new BMenuItem("mkv", new BMessage(M_OUTPUTFILEFORMAT)));
	outputfileformatpopup->AddItem(new BMenuItem("webm", new BMessage(M_OUTPUTFILEFORMAT)));
	outputfileformatpopup->ItemAt(0)->SetMarked(true);
	outputfileformat = new BMenuField("Output File Format", outputfileformatpopup);

	outputvideoformatpopup = new BPopUpMenu("");
	outputvideoformatpopup->AddItem(new BMenuItem("mpeg4", new BMessage(M_OUTPUTVIDEOFORMAT)));
	outputvideoformatpopup->AddItem(new BMenuItem("vp7", new BMessage(M_OUTPUTVIDEOFORMAT)));
	outputvideoformatpopup->AddItem(new BMenuItem("vp8", new BMessage(M_OUTPUTVIDEOFORMAT)));
	outputvideoformatpopup->AddItem(new BMenuItem("vp9", new BMessage(M_OUTPUTVIDEOFORMAT)));
	outputvideoformatpopup->AddItem(new BMenuItem("wmv1", new BMessage(M_OUTPUTVIDEOFORMAT)));
	outputvideoformatpopup->ItemAt(0)->SetMarked(true);
	outputvideoformat = new BMenuField("Output Video Format", outputvideoformatpopup);

	outputaudioformatpopup = new BPopUpMenu("");
	outputaudioformatpopup->AddItem(new BMenuItem("ac3", new BMessage(M_OUTPUTAUDIOFORMAT)));
	outputaudioformatpopup->AddItem(new BMenuItem("aac", new BMessage(M_OUTPUTAUDIOFORMAT)));
	outputaudioformatpopup->AddItem(new BMenuItem("opus", new BMessage(M_OUTPUTAUDIOFORMAT)));
	outputaudioformatpopup->AddItem(new BMenuItem("vorbis", new BMessage(M_OUTPUTAUDIOFORMAT)));
	outputaudioformatpopup->ItemAt(0)->SetMarked(true);
	outputaudioformat = new BMenuField("Output Audio Format", outputaudioformatpopup);

	enablevideo = new BCheckBox("", "Enable Video Encoding", new BMessage(M_ENABLEVIDEO));
	enablevideo->SetValue(B_CONTROL_ON);
	vbitrate = new BSpinner("", "Bitrate (Kbit/s)", new BMessage(M_VBITRATE));
	framerate = new BSpinner("", "Framerate (fps)", new BMessage(M_FRAMERATE));
	customres = new BCheckBox("", "Use Custom Resolution", new BMessage(M_CUSTOMRES));
	xres = new BSpinner("", "Width", new BMessage(M_XRES));
	yres = new BSpinner("", "Height", new BMessage(M_YRES));

	enablecropping = new BCheckBox("", "Enable Video Cropping", new BMessage(M_ENABLECROPPING));
	enablecropping->SetValue(B_CONTROL_ON);
	topcrop = new BSpinner("", "Top Crop Size", new BMessage(M_TOPCROP));
	bottomcrop = new BSpinner("", "Bottom Crop Size", new BMessage(M_BOTTOMCROP));
    leftcrop = new BSpinner("", "Left Crop Size", new BMessage(M_LEFTCROP));
	rightcrop = new BSpinner("", "Right Crop Size", new BMessage(M_RIGHTCROP));

	enableaudio = new BCheckBox("", "Enable Audio Encoding", new BMessage(M_ENABLEAUDIO));
	enableaudio->SetValue(B_CONTROL_ON);
	ab = new BSpinner("", "Bitrate (Kbit/s)", new BMessage(M_AB));
	ar = new BSpinner("", "Sampling Rate (Hz)", new BMessage(M_AR));
	ac = new BSpinner("", "Audio Channels", new BMessage(M_AC));

	bframes = new BSpinner("", "'B' Frames", nullptr);
	gop = new BSpinner("", "GOP Size", nullptr);
	highquality = new BCheckBox("","Use High Quality Settings", new BMessage(M_HIGHQUALITY));
	fourmotion = new BCheckBox("", "Use four motion vector", new BMessage(M_FOURMOTION));
	deinterlace = new BCheckBox("", "Deinterlace Pictures", new BMessage(M_DEINTERLACE));
	calcpsnr = new BCheckBox("", "Calculate PSNR of Compressed Frames", new BMessage(M_CALCPSNR));

	fixedquant = new BSpinner("", "Use Fixed Video Quantiser Scale", nullptr);
	minquant = new BSpinner("", "Min Video Quantiser Scale", nullptr);
	maxquant = new BSpinner("", "Max Video Quantiser Scale", nullptr);
	quantdifference = new BSpinner("", "Max Difference Between Quantiser Scale", nullptr);
	quantblur = new BSpinner("", "Video Quantiser Scale Blur", nullptr);
	quantcompression = new BSpinner("", "Video Quantiser Scale Compression", nullptr);

	abouttext = new BTextView("");
	abouttext->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	outputtext = new BTextView("");
	outputtext->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	outputtext->MakeEditable(false);

	encodebutton = new BButton("Encode", new BMessage(M_ENCODE));
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
	fStatusBar->SetExplicitMinSize(BSize(B_SIZE_UNSET, 50));

// set the names for each control, so they can be figured out in MessageReceived
	vbitrate->SetName("vbitrate");
	framerate->SetName("framerate");
	xres->SetName("xres");
	yres->SetName("yres");
	topcrop->SetName("topcrop");
	bottomcrop->SetName("bottomcrop");
	leftcrop->SetName("leftcrop");
	rightcrop->SetName("rightcrop");
	ab->SetName("ab");
	ar->SetName("ar");
	ac->SetName("ac");
	enablevideo->SetName("enablevideo");
	enableaudio->SetName("enableaudio");
	enablecropping->SetName("enablecropping");
	deletesource->SetName("deletesource");
	customres->SetName("customres");

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

	// set the default status for the conditional spinners
	benablecropping = true;
	benableaudio = true;
	bcustomres = false;
	benablevideo = false; // changing this breaks UI enabled/disabled behaviour
	xres->SetEnabled(false);
	yres->SetEnabled(false);

	// set the about view text
	abouttext->MakeEditable(false);
	abouttext->SetText("  ffmpeg gui v1.0\n\n"
					   "  Thanks to mmu_man, Jeremy, DeadYak, Marco, etc...\n\n"
					   "  md@geekport.com\n\n"
					   "  made more or less usable by reds <reds@sakamoto.pl> - have fun! ");

	// set the initial command line
	BuildLine();


	// create tabs and boxes
	BBox *fileoptionsbox = new BBox("");
	fileoptionsbox->SetLabel("File Options");
	BGroupLayout *fileoptionslayout = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(3,3,3,3)
		.AddGroup(B_HORIZONTAL)
			.Add(sourcefilebutton)
			.Add(sourcefile)
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(outputfilebutton)
			.Add(outputfile)
		.End()
		.Add(deletesource)
		.AddGroup(B_HORIZONTAL)
			.Add(outputfileformat)
			.Add(outputvideoformat)
			.Add(outputaudioformat)
		.End();
	fileoptionsbox->AddChild(fileoptionslayout->View());

	BBox *encodebox = new BBox("");
	encodebox->SetLabel("Encode");
	BGroupLayout *encodelayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(0,0,0,0)
		.AddGroup(B_HORIZONTAL)
			.Add(encodebutton)
			.Add(encode)
		.End();
	encodebox->AddChild(encodelayout->View());

	BBox *videobox = new BBox("");
	videobox->SetLabel("Video");
	BGroupLayout *videolayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(5,5,5,5)
		.Add(enablevideo)
		.AddGrid(B_USE_SMALL_SPACING,B_USE_SMALL_SPACING)
			.Add(vbitrate->CreateLabelLayoutItem(),0,0)
			.Add(vbitrate->CreateTextViewLayoutItem(),1,0)
			.Add(framerate->CreateLabelLayoutItem(),0,1)
			.Add(framerate->CreateTextViewLayoutItem(),1,1)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(customres)
		.Add(xres)
		.Add(yres);
	videobox->AddChild(videolayout->View());

	BBox *croppingoptionsbox = new BBox("");
	croppingoptionsbox->SetLabel("Cropping Options");
	BGroupLayout *croppingoptionslayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(5,5,5,5)
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
	audiobox->SetLabel("Audio");
	BGroupLayout *audiolayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(5,5,5,5)
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
	BView *aboutview = new BView("",B_SUPPORTS_LAYOUT);

	BLayoutBuilder::Group<>(mainoptionsview, B_HORIZONTAL)
		.SetInsets(5)
		.Add(videobox)
		.Add(croppingoptionsbox)
		.Add(audiobox)
		.Layout();

	BLayoutBuilder::Group<>(advancedoptionsview, B_HORIZONTAL)
		.SetInsets(5)
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

	BLayoutBuilder::Group<>(aboutview, B_HORIZONTAL)
		.SetInsets(0)
		.Add(abouttext);

	tabview = new BTabView("");
	BTab *mainoptionstab = new BTab();
	BTab *advancedoptionstab = new BTab();
	BTab *outputtab = new BTab();
	BTab *abouttab = new BTab();

	tabview->AddTab(mainoptionsview, mainoptionstab);
	tabview->AddTab(advancedoptionsview, advancedoptionstab);
	tabview->AddTab(outputview, outputtab);
	tabview->AddTab(aboutview, abouttab);
	mainoptionstab->SetLabel("Main Options");
	advancedoptionstab->SetLabel("Advanced Options");
	outputtab->SetLabel("Output");
	abouttab->SetLabel("About");

	//main layout
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(5)
		.Add(fileoptionsbox)
		.Add(tabview)
		.Add(encodebox)
		.Add(fStatusBar)
	.Layout();

	ResizeToPreferred();
	MoveOnScreen();

	fStatusBar->Hide();

	//initialize command launcher
	fCommandLauncher = new CommandLauncher(new BMessenger(this));

}

//quitting
bool ffguiwin::QuitRequested()
{
	fCommandLauncher->PostMessage(B_QUIT_REQUESTED);
	PostMessage(B_QUIT_REQUESTED);
	printf("have a nice day\n");
	exit(0);
}

//message received
void ffguiwin::MessageReceived(BMessage *message)
{

	//message->PrintToStream();
	switch(message->what)
	{
		case M_NOMSG:
		{
			BuildLine();
			break;
		}
		case M_SOURCEFILE:
		{
			BuildLine();
			sourcefile_specified = !(BString(sourcefile->Text()).Trim().IsEmpty());
			set_encodebutton_state();
			break;
		}
		case M_OUTPUTFILE:
		{
			BuildLine();
			outputfile_specified = !(BString(outputfile->Text()).Trim().IsEmpty());
			set_encodebutton_state();
			break;
		}

/*		case '_PBL':
		{
			BuildLine();
			break;
		}
		case '_UKU':
		{
			BuildLine();
			break;
		}
		case '_UKD':
		{
			BuildLine();
			break;
		}
*/		case M_OUTPUTFILEFORMAT:
		case M_OUTPUTVIDEOFORMAT:
		case M_OUTPUTAUDIOFORMAT:
		{
			BView *view (NULL);
			printf("!pop found\n");
			if (message->FindPointer("source", reinterpret_cast<void **>(&view)) == B_OK)
			{
				printf("found view pointer\n");
			}
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
			BView *view (NULL);
			if (message->FindPointer("source", reinterpret_cast<void **>(&view)) == B_OK)
			{
				const char *name (view->Name());
				printf(name);
				printf("\n");
			}
			break;
		}

		case M_ENABLEVIDEO:
		{
			// turn all the video spin buttons off
			if (benablevideo == true)
			{
				vbitrate->SetEnabled(true);
				vbitrate->ChildAt(0)->Invalidate();
				framerate->SetEnabled(true);
				framerate->ChildAt(0)->Invalidate();
				customres->SetEnabled(true);

				if (bcustomres == true)
				{
					xres->SetEnabled(true);
					xres->ChildAt(0)->Invalidate();
					yres->SetEnabled(true);
					yres->ChildAt(0)->Invalidate();
				}

				enablecropping->SetEnabled(true);
				if (benablecropping == true)
				{
					topcrop->SetEnabled(true);
					topcrop->ChildAt(0)->Invalidate();
					bottomcrop->SetEnabled(true);
					bottomcrop->ChildAt(0)->Invalidate();
					leftcrop->SetEnabled(true);
					leftcrop->ChildAt(0)->Invalidate();
					rightcrop->SetEnabled(true);
					rightcrop->ChildAt(0)->Invalidate();
				}
				benablevideo = false;
				BuildLine();
			}
			else
			{
				vbitrate->SetEnabled(false);
				vbitrate->ChildAt(0)->Invalidate();
				framerate->SetEnabled(false);
				framerate->ChildAt(0)->Invalidate();
				customres->SetEnabled(false);
				xres->SetEnabled(false);
				xres->ChildAt(0)->Invalidate();
				yres->SetEnabled(false);
				yres->ChildAt(0)->Invalidate();
				enablecropping->SetEnabled(false);
				topcrop->SetEnabled(false);
				topcrop->ChildAt(0)->Invalidate();
				bottomcrop->SetEnabled(false);
				bottomcrop->ChildAt(0)->Invalidate();
				leftcrop->SetEnabled(false);
				leftcrop->ChildAt(0)->Invalidate();
				rightcrop->SetEnabled(false);
				rightcrop->ChildAt(0)->Invalidate();
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
				xres->ChildAt(0)->Invalidate();
				yres->SetEnabled(true);
				yres->ChildAt(0)->Invalidate();
				bcustomres = true;
				BuildLine();
			}
			else
			{
				xres->SetEnabled(false);
				xres->ChildAt(0)->Invalidate();
				yres->SetEnabled(false);
				yres->ChildAt(0)->Invalidate();
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
				topcrop->ChildAt(0)->Invalidate();
				bottomcrop->SetEnabled(true);
				bottomcrop->ChildAt(0)->Invalidate();
				leftcrop->SetEnabled(true);
				leftcrop->ChildAt(0)->Invalidate();
				rightcrop->SetEnabled(true);
				rightcrop->ChildAt(0)->Invalidate();
				benablecropping = true;
				BuildLine();
			}
			else
			{
				topcrop->SetEnabled(false);
				topcrop->ChildAt(0)->Invalidate();
				bottomcrop->SetEnabled(false);
				bottomcrop->ChildAt(0)->Invalidate();
				leftcrop->SetEnabled(false);
				leftcrop->ChildAt(0)->Invalidate();
				rightcrop->SetEnabled(false);
				rightcrop->ChildAt(0)->Invalidate();
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
				ab->ChildAt(0)->Invalidate();
				ac->SetEnabled(false);
				ac->ChildAt(0)->Invalidate();
				ar->SetEnabled(false);
				ar->ChildAt(0)->Invalidate();
				benableaudio = false;
				BuildLine();
			}
			else
			{
				ab->SetEnabled(true);
				ab->ChildAt(0)->Invalidate();
				ac->SetEnabled(true);
				ac->ChildAt(0)->Invalidate();
				ar->SetEnabled(true);
				ar->ChildAt(0)->Invalidate();
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
			BuildLine();
			sourcefile_specified = true;
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
			outputfile_specified = true;
			set_encodebutton_state();
			break;
		}
		case M_ENCODE:
		{
			outputtext->SelectAll();
			outputtext->Clear();
			//tabview->Select(2);
			commandline.SetTo(encode->Text());
			commandline.Append(" -y");

			BString files_string;
			files_string << sourcefile->Text() << " -> " << outputfile->Text();
			fStatusBar->SetText(files_string.String());
			fStatusBar->Show();


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
					int32 encode_percentage = (encode_time * 100) / encode_duration;
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


			//outputtext->ScrollToOffset(outputtext->TextLength());
			break;
		}
		case M_COMMAND_FINISHED:
		{
			status_t exit_code;
			message->FindInt32("exitcode", &exit_code);
			BString finished_message;
			if(exit_code == 0)
			{
				finished_message = "The video was converted successfully";
			}
			else
			{
				finished_message << "Converting the video failed";
			}

			BAlert *finished_alert = new BAlert("", finished_message, "OK");
			finished_alert->Go();

			fStatusBar->Reset();
			fStatusBar->Hide();
			break;
		}
		default:
			/*
			printf("recieved by window:\n");
			message->PrintToStream();
			printf("\n");
			printf("------------\n");
			*/
			BWindow::MessageReceived(message);
			break;
	}
}


void ffguiwin::set_encodebutton_state()
{
	bool encodebutton_enabled;

	if (sourcefile_specified && outputfile_specified)
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