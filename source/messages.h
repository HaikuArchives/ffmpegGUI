#ifndef MESSAGES_H
#define MESSAGES_H

/*
 * Copyright 2003, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	ffgui-window.cpp , 1/06/03
	Zach Dykstra
	Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022
*/

// Buttons
const uint32 M_NOMSG = 0x00000000;
const uint32 M_SOURCE = 0x1100;
const uint32 M_OUTPUT = 0x1101;
const uint32 M_ENCODE = 0x1102;

// Spinners
const uint32 M_VBITRATE = 0x1200;
const uint32 M_FRAMERATE = 0x1201;
const uint32 M_XRES = 0x1202;
const uint32 M_YRES = 0x1203;
const uint32 M_TOPCROP = 0x1204;
const uint32 M_BOTTOMCROP = 0x1205;
const uint32 M_LEFTCROP = 0x1206;
const uint32 M_RIGHTCROP = 0x1207;
const uint32 M_AB = 0x1208;
const uint32 M_AR = 0x1209;
const uint32 M_AC = 0x1210;

// CheckBoxes
const uint32 M_ENABLEVIDEO = 0x1301;
const uint32 M_CUSTOMRES = 0x1302;
const uint32 M_ENABLECROPPING = 0x1303;
const uint32 M_ENABLEAUDIO = 0x1304;
const uint32 M_HIGHQUALITY = 0x1305;
const uint32 M_FOURMOTION = 0x1306;
const uint32 M_DEINTERLACE = 0x1307;
const uint32 M_CALCPSNR = 0x1308;

// Popup Menus
const uint32 M_OUTPUTFILEFORMAT = 0x1400;
const uint32 M_OUTPUTVIDEOFORMAT = 0x1401;
const uint32 M_OUTPUTAUDIOFORMAT = 0x1402;

//Text Controls
const uint32 M_SOURCEFILE = 0x1500;
const uint32 M_OUTPUTFILE = 0x1501;

//File Panels
const uint32 M_SOURCEFILE_REF = 0x1600;
const uint32 M_OUTPUTFILE_REF = 0x1601;

//Command Launcher
const uint32 M_RUN_COMMAND = 0x1700;
const uint32 M_PROGRESS = 0x1701;
const uint32 M_COMMAND_FINISHED = 0x1702;

#endif