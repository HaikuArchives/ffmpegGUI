#include "commandlauncher.h"
#include "messages.h"

#include <cstdio>
#include <array>
#include <unistd.h>
#include <algorithm>
#include <iostream>

CommandLauncher::CommandLauncher(BMessenger *target_messenger)
	:
	BLooper("commandlauncher"),
	fTargetMessenger(target_messenger),
	fOutputMessage(NULL),
	fFinishMessage(NULL)
{
	fBusy = false;
	Run();
}


void
CommandLauncher::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
		case M_ENCODE_COMMAND:
		{
			if(!fBusy)
			{
				fOutputMessage = new BMessage(M_ENCODE_PROGRESS);
				fFinishMessage = new BMessage(M_ENCODE_FINISHED);
				message->FindString("cmdline", &fCmdline);
				fBusy = true;
				fCommandFlag = ENCODING;
				run_command();
			}
			break;
		}
		case M_INFO_COMMAND:
		{
			if(!fBusy)
			{
				fOutputMessage = new BMessage(M_INFO_OUTPUT);
				fFinishMessage = new BMessage(M_INFO_FINISHED);
				message->FindString("cmdline", &fCmdline);
				fBusy = true;
				fCommandFlag = INFO;
				run_command();
			}
			break;
		}
		default:
			BLooper::MessageReceived(message);
	}

}


void
CommandLauncher::run_command()
{

	//redirect stderr + stout
	int stderr_pipe[2];
	int stdout_pipe[2];
	int original_stderr = dup(STDERR_FILENO);
	int original_stdout = dup(STDOUT_FILENO);

	if (pipe(stderr_pipe) == 0)
	{
		dup2(stderr_pipe[1], STDERR_FILENO);
		close(stderr_pipe[1]);
	}
	if (pipe(stdout_pipe) == 0)
	{
		dup2(stdout_pipe[1], STDOUT_FILENO);
		close(stdout_pipe[1]);
	}

	int pipe_flags = fcntl(stderr_pipe[0], F_GETFD);
	pipe_flags |= FD_CLOEXEC;
	fcntl(stderr_pipe[0], F_SETFD, pipe_flags);

	pipe_flags = fcntl(stdout_pipe[0], F_GETFD);
	pipe_flags |= FD_CLOEXEC;
	fcntl(stdout_pipe[0], F_SETFD, pipe_flags);

	//create thread for ffmpeg
	const char *arguments[4];
	arguments[0] = "/bin/sh";
	arguments[1] = "-c";
	arguments[2] = fCmdline.String();
	arguments[3] = nullptr;

	thread_id proc_id = load_image(3, arguments, const_cast<const char **>(environ));
	status_t error_code = proc_id;

	if (error_code >= 0)
	{
		setpgid(proc_id, proc_id);
		error_code = resume_thread(proc_id);
	}

	dup2(original_stderr, STDERR_FILENO);
	dup2(original_stdout, STDOUT_FILENO);

	//read stderr output and send to target
	bool error_detected = false;
	if (error_code >= 0)
	{
		char buffer[4096];
		while (true)
		{
			ssize_t amount_read;
			if (fCommandFlag == ENCODING)
				amount_read = read(stderr_pipe[0], buffer, sizeof(buffer));
			else
				amount_read = read(stdout_pipe[0], buffer, sizeof(buffer));

			if (amount_read <= 0)
				break;

			// Make sure the buffer is null terminated
			buffer[amount_read] = 0;

			fOutputMessage->AddString("data", buffer);
			fTargetMessenger->SendMessage(fOutputMessage);
			fOutputMessage->RemoveName("data");

			//check if output contains error messages
			BString output_string(buffer);
			if(output_string.FindFirst("Error while decoding stream") != B_ERROR)
			{
				error_detected = true;
				kill_thread(proc_id);
				break;
			}
		}
	}

	//clean up
	dup2(original_stderr, STDERR_FILENO);
	close(stderr_pipe[0]);
	dup2(original_stdout, STDOUT_FILENO);
	close(stdout_pipe[0]);

	status_t proc_exit_code = 0;
	wait_for_thread(proc_id, &proc_exit_code);

	//inform target that the command has finished
	if(error_detected)
	{
		proc_exit_code = 911;
	}
	fFinishMessage->AddInt32("exitcode", proc_exit_code);
	fTargetMessenger->SendMessage(fFinishMessage);
	fBusy = false;

	delete fOutputMessage;
	delete fFinishMessage;
}
