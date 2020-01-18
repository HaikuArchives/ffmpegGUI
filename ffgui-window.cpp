/*
	ffgui-window.cpp , 1/06/03
	Zach Dykstra
*/

#include <stdio.h>
#include "ffgui-window.h"
#include "ffgui-application.h"
#include "messages.h"
#include "MenuItem.h"

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
	: MWindow(r,name,type,mode)
{
//create the view for the main window gui controls
	
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
									new MButton("Source file",new BMessage(M_SOURCE),NULL,minimax(-1,-1,-1,-1)),
									sourcefile=new MTextControl("",""),0
								),
								new HGroup
								(
									new MButton("Output file",new BMessage(M_OUTPUT),NULL,minimax(-1,-1,-1,-1)),
									outputfile=new MTextControl("",""),0
								),
								deletesource=new MCheckBox("Delete source when finished"),0
							),
							new HGroup
							(
								outputfileformat=new MPopup("Output File Format","avi","vcd","mp4","mpeg","mkv","webm",0), // todo: add more formats
								outputvideoformat=new MPopup("Output Video Format","mpeg4","vp7","vp8","vp9","wmv1",0),
								outputaudioformat=new MPopup ("Output Audio Format","ac3","aac","opus","vorbis",0),0
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
								enablevideo=new MCheckBox("Enable Video Encoding",0,true),
								vbitrate = new SpinButton("Bitrate (Kbit/s)",SPIN_INTEGER),
								framerate=new SpinButton("Framerate (fps)",SPIN_INTEGER),
								new MSplitter(),
								customres=new MCheckBox("Use Custom Resolution",0,false),
								xres=new SpinButton("Width",SPIN_INTEGER),
								yres=new SpinButton("Height",SPIN_INTEGER),0
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
								enablecropping=new MCheckBox("Enable Video Cropping",0,true),
								topcrop=new SpinButton("Top Crop Size",SPIN_INTEGER),
								bottomcrop=new SpinButton("Bottom Crop Size",SPIN_INTEGER),
								leftcrop=new SpinButton("Left Crop Size",SPIN_INTEGER),
								rightcrop=new SpinButton("Right Crop Size",SPIN_INTEGER),0
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
								enableaudio=new MCheckBox("Enable Audio Encoding",0,true),
								ab=new SpinButton("Bitrate (Kbit/s)",SPIN_INTEGER),
								ar=new SpinButton("Sampling Rate (Hz)",SPIN_INTEGER),
								ac=new SpinButton("Audio Channels",SPIN_INTEGER),0
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
								bframes=new SpinButton("'B' Frames",SPIN_INTEGER),
								gop=new SpinButton("GOP Size",SPIN_INTEGER),
								new Space(minimax(2,2,2,2)),
								new MSplitter(),
								highquality=new MCheckBox("Use High Quality Settings",0,false),
								fourmotion=new MCheckBox("Use four motion vector",0,false),
								deinterlace=new MCheckBox("Deinterlace Pictures",0,false),
								calcpsnr=new MCheckBox("Calculate PSNR of Compressed Frames",0,false),0

							),
							new MSplitter(),
							new VGroup
							(
								fixedquant=new SpinButton("Use Fixed Video Quantiser Scale",SPIN_INTEGER),
								minquant=new SpinButton("Min Video Quantiser Scale",SPIN_INTEGER),
								maxquant=new SpinButton("Max Video Quantiser Scale",SPIN_INTEGER),
								quantdifference=new SpinButton("Max Difference Between Quantiser Scale",SPIN_INTEGER),
								quantblur=new SpinButton("Video Quantiser Scale Blur",SPIN_INTEGER),
								quantcompression=new SpinButton("Video Quantiser Scale Compression",SPIN_INTEGER),
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
							abouttext=new MTextView(),0
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
								new MButton("Encode",new BMessage(M_ENCODE),NULL,minimax(-1,-1,-1,-1)),
								encode=new MTextControl("",""),0
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
	vbitrate->SetMinimum(64);
	vbitrate->SetMaximum(50000);
	framerate->SetMinimum(1);
	framerate->SetMaximum(60);
	xres->SetMinimum(160);
	xres->SetMaximum(7680);
	yres->SetMinimum(120);
	yres->SetMaximum(4320);
	ab->SetMinimum(16);
	ab->SetMaximum(500);
	ar->SetMaximum(192000);
	
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
	AddChild(dynamic_cast<BView*>(topview));
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
			MWindow::MessageReceived(message);
			break;
	}
}
