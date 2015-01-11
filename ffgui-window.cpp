/*
	ffgui-window.cpp , 1/06/03
	Zach Dykstra
*/
/* sample command line, for reference
ffmpeg -i /boot/home/Desktop/VTS_01_[1-3].VOB -cropleft 12 -cropright 4 -croptop 64 -cropbottom 64 -f avi -vcodec mpeg4 -b 1000 -g 132 -bf 2 -acodec mp3 -ab 128 -ar 44100 outfile.MPEG4.avi   
*/

#include <stdio.h>
#include "ffgui-window.h"
#include "ffgui-application.h"
#include "messages.h"
#include "MenuItem.h"
//char buff[30];
//sprintf(buff, "%d", mydouble);

void ffguiwin::BuildLine() // ask all the views what they hold, reset the command string
{
	char buff[30];
	commandline = new BString("ffmpeg -i ");
	const char *contents (sourcefile->Text()); //append the input file name
	commandline->Append(contents);

	sprintf(buff," -f %s",outputfileformat->MenuItem()->Label()); // grab and set the file format
	commandline->Append(buff);
	
	if (benablevideo == false) // is video enabled, add options
	{
		sprintf(buff," -vcodec %s",outputvideoformat->MenuItem()->Label()); // grab and set the video encoder
		commandline->Append(buff);
		sprintf(buff," -b %d",(int)vbitrate->Value());
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
			if ((int)topcrop->Value() != 0)// crop value other than 0?
			{
				sprintf(buff," -croptop %d",(int)topcrop->Value());
				commandline->Append(buff);
			}
			if ((int)bottomcrop->Value() != 0)
			{
				sprintf(buff," -cropbottom %d",(int)bottomcrop->Value());
				commandline->Append(buff);	
			}
			if ((int)leftcrop->Value() != 0)
			{
				sprintf(buff," -cropleft %d",(int)leftcrop->Value());
				commandline->Append(buff);
			}
			if ((int)rightcrop->Value() != 0)
			{
				sprintf(buff," -cropright %d",(int)rightcrop->Value());
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
		sprintf(buff," -ab %d",(int)ab->Value());
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
								outputfileformat=new MPopup("Output File Format","avi","vcd","mpeg",0),
								outputvideoformat=new MPopup("Output Video Format","mpeg4","msmpeg4v1","wmv1",0),
								outputaudioformat=new MPopup ("Output Audio Format","mp3","ac3",0),0
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
								vbitrate = new SpinButton("Bitrate (kbit/s)",SPIN_INTEGER),
								framerate=new SpinButton("Framerate (hz)",SPIN_INTEGER),
								new MSplitter(),
								customres=new MCheckBox("Use Custom Resolution",0,false),
								xres=new SpinButton("X Resolution",SPIN_INTEGER),
								yres=new SpinButton("Y Resolution",SPIN_INTEGER),0
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
								ab=new SpinButton("Bitrate (kbit/s)",SPIN_INTEGER),
								ar=new SpinButton("Sampling Rate (hz)",SPIN_INTEGER),
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
	double d; // snicker
	d = 50; vbitrate->SetMinimum(d);
	d = 2000; vbitrate->SetMaximum(d);
	d = 5; framerate->SetMinimum(d);
	d = 30; framerate->SetMaximum(d);
	d = 160; xres->SetMinimum(d);
	d = 720; xres->SetMaximum(d);
	d = 120; yres->SetMinimum(d);
	d = 480; yres->SetMaximum(d);
	d = 24; ab->SetMinimum(d);
	d = 320; ab->SetMaximum(d);
	d = 48000; ar->SetMaximum(d);
	
	// set the initial values 
	d = 800; vbitrate->SetValue(d);
	d = 25; framerate->SetValue(d);
	d = 640; xres->SetValue(d);
	d = 480; yres->SetValue(d);
	d = 128; ab->SetValue(d);
	d = 44100; ar->SetValue(d);
	d = 2; ac->SetValue(d);
	
	// set the default status for the conditional spinners
	benablecropping = true;
	benableaudio = true;
	bcustomres = false;
	benablevideo = false;
	xres->SetEnabled(false);
	yres->SetEnabled(false);
	
	// set the about view text
	abouttext->MakeEditable(false);
	abouttext->SetText("  ffmpeg gui v1.0\n\n" "  Thanks to mmu_man, Jeremy, DeadYak, Marco, etc...\n\n" 
					   "  md@geekport.com");
	// add the view
	AddChild(dynamic_cast<BView*>(topview));
	// set the initial command line
	BuildLine();

}

//quitting
bool ffguiwin::QuitRequested()
{
	PostMessage(B_QUIT_REQUESTED);
	return TRUE;
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
			char run[1200];
			sprintf(run,"/boot/beos/apps/Terminal /boot/home/config/bin/%s",commandline->String());
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
