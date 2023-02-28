/*
 * Copyright 2022-2023. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022
 * Humdinger, humdingerb@gmail.com, 2022-2023
*/


#ifndef COMMANDLAUNCHER_H
#define COMMANDLAUNCHER_H


#include <Looper.h>
#include <Messenger.h>
#include <String.h>


enum {
	ENCODING = 0,
	INFO
};

enum {
	SUCCESS = 0,
	FAILED,
	ABORTED
};

class CommandLauncher : public BLooper {
public:
					CommandLauncher(BMessenger* target_messenger);
	void 			MessageReceived(BMessage* message);

private:
	static status_t	_Command(void* self);
	void 			RunCommand();
	int32			GetCurrentTime(const char* buffer);

	BString 		fCommandline;
	BMessage* 		fOutputMessage;
	BMessage* 		fFinishMessage;
	BMessenger* 	fTargetMessenger;
	bool			fBusy;
	int32			fCommandFlag;
	thread_id 		fThread;
	status_t 		fErrorCode;
};

#endif // COMMANDLAUNCHER_H
