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
	FAILED ,
	ABORTED
};

class CommandLauncher : public BLooper{
public:
	CommandLauncher(BMessenger *target_messenger);
	void MessageReceived(BMessage *message);

private:
	static status_t		_ffmpeg_command(void* self);
	void 				run_command();

	BString 	fCmdline;
	BMessage	*fOutputMessage;
	BMessage	*fFinishMessage;
	BMessenger	*fTargetMessenger;
	bool 		fBusy;
	int32		fCommandFlag;
	thread_id	fThread;
	status_t	fErrorCode;
};

#endif
