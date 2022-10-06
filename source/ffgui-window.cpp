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
#include <TabView.h>
#include <View.h>
#include <Box.h>


void ffguiwin::BuildLine() // ask all the views what they hold, reset the command string
{
	char buff[64];
	commandline = new BString("ffmpeg -i ");
	const char *contents (sourcefile->Text()); //append the input file name
	commandline->Append(contents);

	sprintf(buff," -f %s",outputfileformat->MenuItem()->Label()); // grab and set the file format
	commandline->Append(buff);
	
	if (benablevideo == false) // is video enabled, add options
	{
		sprintf(buff," -vcodec %s",outputvideoformat->MenuItem()->Label()); // grab and set the video encoder
		commandline->Append(buff);
		sprintf(buff," -b:v %d",(int)vbitrate->Value());
		commandline->Append(buff);
		sprintf(buff," -r %d",(int)framerate->Value());
		commandline->Append(buff);
		if (bcustomres == true)
		{
			sprintf(buff," -s %dx%d",(int)xres->Value(),(int)yres->Value());
			commandline->Append(buff);
		}
		
		// cropping options -- no point in cropping if we aren't encoding video...
		if (benablecropping == true)
		{
			// below should be rewritten to use only one crop, but I can't be bothered
			// to do so as ffmpeg supports multiple filters stacked on each other
			if ((int)topcrop->Value() != 0)
			{
				sprintf(buff," -vf crop=w=in_w:h=in_h:x=0:y=%d",(int)topcrop->Value());
				commandline->Append(buff);
			}
			if ((int)bottomcrop->Value() != 0)
			{
				sprintf(buff," -vf crop=w=in_w:h=in_h-%d:x=0:y=0",(int)bottomcrop->Value());
				commandline->Append(buff);	
			}
			if ((int)leftcrop->Value() != 0)
			{
				sprintf(buff," -vf crop=w=in_w:h=in_h:x=%d:y=0",(int)leftcrop->Value());
				commandline->Append(buff);
			}
			if ((int)rightcrop->Value() != 0)
			{
				sprintf(buff," -vf crop=w=in_w-%d:h=in_h:x=0:y=0",(int)rightcrop->Value());
				commandline->Append(buff);
			}
		}
	}
	else //nope, add the no video encoding flag
	{
		commandline->Append(" -vn");
	}
	
	if (benableaudio == true) // audio encoding enabled, grab the values
	{
		sprintf(buff," -acodec %s",outputaudioformat->MenuItem()->Label());
		commandline->Append(buff);
		sprintf(buff," -b:a %d",(int)ab->Value());
		commandline->Append(buff);
		sprintf(buff," -ar %d",(int)ar->Value());
		commandline->Append(buff);
		sprintf(buff," -ac %d",(int)ac->Value());
		commandline->Append(buff);
	}
	else
	{
		commandline->Append(" -an");
	}
	commandline->Append(" ");
	commandline->Append(outputfile->Text());
	commandline->Append(" &");
	printf(commandline->String());
	printf("\n");
	encode->SetText(commandline->String());
}

// new window object
ffguiwin::ffguiwin(BRect r, char *name, window_type type, ulong mode)
	: BWindow(r,name,type,mode)
{

	//initialize GUI elements
	sourcefilebutton = new BButton("Source file", new BMessage(M_SOURCE));
	sourcefile = new BTextControl("", "", nullptr);
	outputfilebutton = new BButton("Output file", new BMessage(M_OUTPUT));
	outputfile = new BTextControl("", "", nullptr);
	deletesource = new BCheckBox("", "Delete source when finished", nullptr);
	
	outputfileformatpopup = new BPopUpMenu("");
	outputfileformatpopup->AddItem(new BMenuItem("avi", nullptr));
	outputfileformatpopup->AddItem(new BMenuItem("vcd", nullptr));
	outputfileformatpopup->AddItem(new BMenuItem("mp4", nullptr));
	outputfileformatpopup->AddItem(new BMenuItem("mpeg", nullptr));
	outputfileformatpopup->AddItem(new BMenuItem("mkv", nullptr));
	outputfileformatpopup->AddItem(new BMenuItem("webm", nullptr));
	outputfileformat = new BMenuField("Output File Format", outputfileformatpopup);
	
	outputvideoformatpopup = new BPopUpMenu("");
	outputvideoformatpopup->AddItem(new BMenuItem("mpeg4", nullptr));
	outputvideoformatpopup->AddItem(new BMenuItem("vp7", nullptr));
	outputvideoformatpopup->AddItem(new BMenuItem("vp8", nullptr));
	outputvideoformatpopup->AddItem(new BMenuItem("vp9", nullptr));
	outputvideoformatpopup->AddItem(new BMenuItem("wmv1", nullptr));	
	outputvideoformat = new BMenuField("Output Video Format", outputvideoformatpopup);

	outputaudioformatpopup = new BPopUpMenu("");
	outputaudioformatpopup->AddItem(new BMenuItem("ac3", nullptr));
	outputaudioformatpopup->AddItem(new BMenuItem("aac", nullptr));
	outputaudioformatpopup->AddItem(new BMenuItem("opus", nullptr));
	outputaudioformatpopup->AddItem(new BMenuItem("vorbis", nullptr));
	outputaudioformat = new BMenuField("Output Audio Format", outputaudioformatpopup);
	
	enablevideo = new BCheckBox("", "Enable Video Encoding", nullptr);
	enablevideo->SetValue(B_CONTROL_ON);
	vbitrate = new BSpinner("", "Bitrate (Kbit/s)", nullptr);
	framerate = new BSpinner("", "Framerate (fps)", nullptr);
	customres = new BCheckBox("", "Use Custom Resolution", nullptr);
	xres = new BSpinner("", "Width", nullptr);
	yres = new BSpinner("", "Height", nullptr);
	
	enablecropping = new BCheckBox("", "Enable Video Cropping", nullptr);
	enablecropping->SetValue(B_CONTROL_ON);
	topcrop = new BSpinner("", "Top Crop Size", nullptr);
	bottomcrop = new BSpinner("", "Bottom Crop Size", nullptr);
    leftcrop = new BSpinner("", "Left Crop Size", nullptr);
	rightcrop = new BSpinner("", "Right Crop Size", nullptr);
	
	enableaudio = new BCheckBox("", "Enable Audio Encoding", nullptr);
	enableaudio->SetValue(B_CONTROL_ON);
	ab = new BSpinner("", "Bitrate (Kbit/s)", nullptr);
	ar = new BSpinner("", "Sampling Rate (Hz)", nullptr);
	ac = new BSpinner("", "Audio Channels", nullptr);	
	
	bframes = new BSpinner("", "'B' Frames", nullptr);
	gop = new BSpinner("", "GOP Size", nullptr);
	highquality = new BCheckBox("","Use High Quality Settings", nullptr);
	fourmotion = new BCheckBox("", "Use four motion vector", nullptr);
	deinterlace = new BCheckBox("", "Deinterlace Pictures", nullptr);
	calcpsnr = new BCheckBox("", "Calculate PSNR of Compressed Frames", nullptr);
	
	fixedquant = new BSpinner("", "Use Fixed Video Quantiser Scale", nullptr);
	minquant = new BSpinner("", "Min Video Quantiser Scale", nullptr);
	maxquant = new BSpinner("", "Max Video Quantiser Scale", nullptr);
	quantdifference = new BSpinner("", "Max Difference Between Quantiser Scale", nullptr);
	quantblur = new BSpinner("", "Video Quantiser Scale Blur", nullptr);
	quantcompression = new BSpinner("", "Video Quantiser Scale Compression", nullptr);	
	
	abouttext = new BTextView("");
	encodebutton = new BButton("Encode", new BMessage(M_ENCODE));
	encode = new BTextControl("", "", nullptr);
	
	// create tabs and boxes	
	BBox *fileoptionsbox = new BBox("");
	fileoptionsbox->SetLabel("File Options");
	BGroupLayout *fileoptionslayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(5,5,5,5)
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
		.SetInsets(5,5,5,5)
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
		.Add(vbitrate)
		.Add(framerate)
		.Add(customres)
		.Add(xres)
		.Add(yres);	
	videobox->AddChild(videolayout->View());
	
	BBox *croppingoptionsbox = new BBox("");
	croppingoptionsbox->SetLabel("Cropping Options");
	BGroupLayout *croppingoptionslayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(5,5,5,5)
		.Add(enablecropping)
		.Add(topcrop)
		.Add(bottomcrop)
		.Add(leftcrop)
		.Add(rightcrop);
	croppingoptionsbox->AddChild(croppingoptionslayout->View());
		
	BBox *audiobox = new BBox("");
	audiobox->SetLabel("Audio");
	BGroupLayout *audiolayout = BLayoutBuilder::Group<>(B_VERTICAL)
		.SetInsets(5,5,5,5)
		.Add(enableaudio)
		.Add(ab)
		.Add(ar)
		.Add(ac);
	audiobox->AddChild(audiolayout->View());
	
	
	BView *mainoptionsview = new BView("",B_SUPPORTS_LAYOUT);
	BView *advancedoptionsview = new BView("",B_SUPPORTS_LAYOUT);
	BView *outputview = new BView("",B_SUPPORTS_LAYOUT);
	BView *aboutview = new BView("",B_SUPPORTS_LAYOUT);
	
	BLayoutBuilder::Group<>(mainoptionsview, B_HORIZONTAL)
		.SetInsets(0)
		.Add(videobox)
		.Add(croppingoptionsbox)
		.Add(audiobox)
		.Layout();
	
	BLayoutBuilder::Group<>(advancedoptionsview, B_HORIZONTAL)
		.SetInsets(0)
		.AddGroup(B_VERTICAL)
			.Add(bframes)
			.Add(gop)
			.Add(highquality)
			.Add(fourmotion)
			.Add(deinterlace)
			.Add(calcpsnr)
		.End()
		.AddGroup(B_VERTICAL)
			.Add(fixedquant)
			.Add(minquant)
			.Add(maxquant)
			.Add(quantdifference)
			.Add(quantblur)
			.Add(quantcompression)
		.Layout();

	BLayoutBuilder::Group<>(outputview, B_HORIZONTAL)
		.SetInsets(0)
		.Layout();

	BLayoutBuilder::Group<>(aboutview, B_HORIZONTAL)
		.SetInsets(0)
		.Add(abouttext)
		.Layout();		
	
	BTabView *tabview = new BTabView("");
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
		.SetInsets(3)
		.Add(fileoptionsbox)
		.Add(tabview)	
		.Add(encodebox)
	.Layout();	
	
	
	/*
	topview=new VGroup //top vgroup
			(
				new VGroup
				(
					new MBorder
					(
						M_LABELED_BORDER,10,"File Options",
						new VGroup
						(
							new VGroup
							(
								new HGroup
								(
									
								),
								new HGroup
								(
									
								),
								
							),
							new HGroup
							(
								
							),0

						)
					),0
				),
				// tab
				new TabGroup
				(
					"Main Options",
				new HGroup // lower hgroup 
				(
					new VGroup // video vgroup
					(
						new MBorder
						(
							M_LABELED_BORDER,10,"Video",
							new VGroup
							(
								
							)
						),0
					),	
								
					new VGroup //cropping vgroup 
					(
						new MBorder
						(
							M_LABELED_BORDER,10,"Cropping Options",
							new VGroup
							(
								
							)
						),0
					),
					
					new VGroup // audio vgroup
					(
						new MBorder
						(
							M_LABELED_BORDER,10,"Audio",
							new VGroup
							(
								
							)
						),0
					),0	
				),
				"Advanced Options",
				new VGroup
				(
					new MBorder
					(	
						M_LABELED_BORDER,10,"Advanced Options",
						new HGroup
						(	
							new VGroup
							(
								

							),
							new MSplitter(),
							new VGroup
							(
								
								new Space(minimax(2,2,2,2)),0
							),0
						)
					),0
				),
				"Output",
				new VGroup
				(
					new MBorder
					(
						M_LABELED_BORDER,10,"Output",
						new VGroup
						(
							new Space(),0
						)
					),0
				),
				"About",
				new VGroup
				(
					new MBorder
					(
						M_LABELED_BORDER,10,"About",
						new VGroup
						(
							
						)
					),0
				),0
				),
				new VGroup // bottom encode hgroup
				(
					new MBorder
					(
						M_LABELED_BORDER,10,"Encode",
						new VGroup
						(
							new HGroup
							(
								
							),0
						)
					),0
				),0
			);						
	
	// tell LibLayout to line things up
	DivideSame(sourcefile,outputfile,0);
	DivideSame(outputfileformat,outputaudioformat,outputvideoformat,0);
	DivideSame(vbitrate,framerate,0);
	DivideSame(xres,yres,0);
	DivideSame(ab,ar,ac,0);
	DivideSame(topcrop,bottomcrop,leftcrop,rightcrop,0);
	DivideSame(fixedquant,minquant,maxquant,quantdifference,quantblur,quantcompression,0);
	DivideSame(highquality,fourmotion,deinterlace,calcpsnr,bframes,gop,0);
	*/
	
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
	// add the view
	//AddChild(dynamic_cast<BView*>(topview));
	// set the initial command line
	BuildLine();

}

//quitting
bool ffguiwin::QuitRequested()
{
	PostMessage(B_QUIT_REQUESTED);
	printf("have a nice day\n");
	exit(0);
}

//message received
void ffguiwin::MessageReceived(BMessage *message)
{

	message->PrintToStream();
	switch(message->what)
	{
		case M_NOMSG:
		{
			BuildLine();
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
*/		case '!pop':
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
		case '!spn':
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
		case '!chk':
		{
			BView *view (NULL);
			if( message->FindPointer("source", reinterpret_cast<void **>(&view)) == B_OK)
			{
				const char *name (view->Name());
				printf(name);
				printf("\n");
				// actions based on which checkbox was pressed
				if (strcmp(name,"enablevideo") == 0) // turn all the video spin buttons off
				{
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
				}
			
				if (strcmp(name,"customres") == 0)
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
				}					
			
				if (strcmp(name,"enablecropping") == 0 ) //turn the cropping spinbuttons on or off, set a bool
				{
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
				}
					
				if (strcmp(name,"enableaudio") == 0) //turn the audio spinbuttons on or off, set a bool
				{		
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
				}
			}	
			break;
		}
		case M_SOURCE:
		{
			printf("M_SOURCE: Source button pressed\n");
			break;
		}
		case M_OUTPUT:
		{
			printf("M_OUTPUT: Output button pressed\n");
			break;
		}
		case M_ENCODE:
		{
			printf("M_ENCODE: Encode button pressed\n");
			char run[4096];
			sprintf(run,"Terminal %s",commandline->String());
			printf("Running command: %s\n", commandline->String());
			system(run);
			break;
		}
		default:

			printf("recieved by window:\n");
			message->PrintToStream();
			printf("\n");
			printf("------------\n");
			BWindow::MessageReceived(message);
			break;
	}
}
