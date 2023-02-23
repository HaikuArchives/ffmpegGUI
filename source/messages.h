/*
 * Copyright 2003-2023, Zach Dykstra. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Zach Dykstra,2003
 * Humdinger, humdingerb@gmail.com, 2022-2023
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022-2023
*/


#ifndef MESSAGES_H
#define MESSAGES_H


// Buttons
enum {
	M_SOURCE = 1100,
	M_OUTPUT,
	M_ENCODE,
	M_STOP_ENCODING,
	M_PLAY_SOURCE,
	M_PLAY_OUTPUT
};
// Spinners
enum {
	M_VBITRATE = 1200,
	M_FRAMERATE,
	M_XRES,
	M_YRES,
	M_TOPCROP,
	M_BOTTOMCROP,
	M_LEFTCROP,
	M_RIGHTCROP,
	M_AUDIOBITRATE,
	M_SAMPLERATE,
	M_CHANNELS
};
// CheckBoxes
enum {
	 M_ENABLEVIDEO = 1300,
	 M_CUSTOMRES,
	 M_ENABLECROPPING,
	 M_ENABLEAUDIO,
	 M_HIGHQUALITY,
	 M_FOURMOTION,
	 M_DEINTERLACE,
	 M_CALCPSNR,
};
// Popup Menus
enum {
	 M_OUTPUTFILEFORMAT = 1400,
	 M_OUTPUTVIDEOFORMAT,
	 M_OUTPUTAUDIOFORMAT
};
// Text Controls
enum {
	 M_SOURCEFILE = 1500,
	 M_OUTPUTFILE
};
// File Panels
enum {
	 M_SOURCEFILE_REF = 1600,
	 M_OUTPUTFILE_REF
};
// Command Launcher
enum {
	 M_ENCODE_COMMAND = 1700,
	 M_ENCODE_PROGRESS,
	 M_ENCODE_FINISHED,
	 M_INFO_COMMAND,
	 M_INFO_OUTPUT,
	 M_INFO_FINISHED,
	 M_STOP_COMMAND,
};
// Misc
enum {
	 M_STOP_ALERT_BUTTON = 1800,
	 M_QUIT_ALERT_BUTTON
};
// Menus
enum {
	 M_COPY_COMMAND = 1900,
 	 M_ADD_JOB,
	 M_JOB_MANAGER,
	 M_DEFAULTS
};
// Job window
enum {
	 M_JOB_SELECTED = 2000,
	 M_JOB_START,
	 M_JOB_ABORT,
	 M_JOB_LOG,
	 M_JOB_REMOVE,
	 M_CLEAR_LIST,
	 M_LIST_UP,
	 M_LIST_DOWN,
};

#endif // MESSAGES_H
