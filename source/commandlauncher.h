#ifndef COMMANDLAUNCHER_H
#define COMMANDLAUNCHER_H


#include <Looper.h>
#include <Messenger.h>
#include <String.h>

enum {
	ENCODING = 0,
	INFO
};


class CommandLauncher : public BLooper{
public:
	CommandLauncher(BMessenger *target_messenger);
	void MessageReceived(BMessage *message);

private:
	void run_command();

	BString 	fCmdline;
	BMessage	*fOutputMessage;
	BMessage	*fFinishMessage;
	BMessenger	*fTargetMessenger;
	bool 		fBusy;
	int32		fCommandFlag;
};

#endif
