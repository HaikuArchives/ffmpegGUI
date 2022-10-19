#ifndef COMMANDLAUNCHER_H
#define COMMANDLAUNCHER_H


#include <Looper.h>
#include <Messenger.h>
#include <String.h>

class CommandLauncher : public BLooper{
public:
	CommandLauncher(BMessenger *target_messenger);
	void MessageReceived(BMessage *message);

private:
	void run_command();

	BString 	fCmdline;
	BMessenger	*fTargetMessenger;
	bool 		fBusy;
};

#endif