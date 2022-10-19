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
	fTargetMessenger(target_messenger)
{
	fBusy = false;
	Run();
}


void
CommandLauncher::MessageReceived(BMessage *message)
{
	switch(message->what)
	{
		case M_RUN_COMMAND:
		{
			if(!fBusy)
			{
				message->FindString("cmdline", &fCmdline);
				fBusy = true;
				run_command();
			}
		}

		default:
			BLooper::MessageReceived(message);
	}

}


void
CommandLauncher::run_command()
{

	//redirect stdout and stderr
	int stderr_pipe[2];
	int original_stderr = dup(STDERR_FILENO);

	if (pipe(stderr_pipe) == 0)
	{
		dup2(stderr_pipe[1], STDERR_FILENO);
		close(stderr_pipe[1]);
	}

	int pipe_flags = fcntl(stderr_pipe[0], F_GETFD);
	pipe_flags |= FD_CLOEXEC;
	fcntl(stderr_pipe[0], F_SETFD, pipe_flags);

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

	dup2 (original_stderr, STDERR_FILENO);

	//read stderr output and send to target
	if (error_code >= 0)
	{
		while (true)
		{
			char buffer[4096];
			ssize_t amount_read = read(stderr_pipe[0], buffer, sizeof(buffer));

			if (amount_read <= 0)
			{
				break;
			}
			std::cout << "data received" << std::endl;

			BMessage progress_message(M_PROGRESS);
			progress_message.AddString("data", buffer);
			fTargetMessenger->SendMessage(&progress_message);
		}
	}

	//clean up
	close(stderr_pipe[0]);
	status_t proc_exit_code = 0;
	wait_for_thread(proc_id, &proc_exit_code);

	fBusy = false;

}