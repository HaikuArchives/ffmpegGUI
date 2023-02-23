/*
 * Copyright 2022-2023. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022
 * Humdinger, humdingerb@gmail.com, 2022-2023
*/


#include "commandlauncher.h"
#include "messages.h"
#include "Utilities.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <iostream>
#include <unistd.h>


CommandLauncher::CommandLauncher(BMessenger* target_messenger)
	:
	BLooper("commandlauncher"),
	fTargetMessenger(target_messenger),
	fOutputMessage(NULL),
	fFinishMessage(NULL)
{
	fBusy = false;
	fErrorCode = 0;
	Run();
}


void
CommandLauncher::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_STOP_COMMAND:
		{
			if (fBusy) {
				fErrorCode = ABORTED;
				kill_thread(fThread);
			}
			break;
		}
		case M_ENCODE_COMMAND:
		{
			if (!fBusy) {
				fOutputMessage = new BMessage(M_ENCODE_PROGRESS);
				fFinishMessage = new BMessage(M_ENCODE_FINISHED);
				message->FindString("cmdline", &fCommandline);
				fBusy = true;
				fErrorCode = 0;
				fCommandFlag = ENCODING;

				thread_id thread
					= spawn_thread(_ffmpeg_command, "ffmpeg command", B_LOW_PRIORITY, this);
				if (thread >= B_OK)
					resume_thread(thread);
			}
			break;
		}
		case M_INFO_COMMAND:
		{
			if (!fBusy) {
				fOutputMessage = new BMessage(M_INFO_OUTPUT);
				fFinishMessage = new BMessage(M_INFO_FINISHED);
				message->FindString("cmdline", &fCommandline);
				fBusy = true;
				fErrorCode = 0;
				fCommandFlag = INFO;

				thread_id thread
					= spawn_thread(_ffmpeg_command, "ffprobe command", B_LOW_PRIORITY, this);
				if (thread >= B_OK)
					resume_thread(thread);
			}
			break;
		}
		default:
			BLooper::MessageReceived(message);
	}
}


status_t
CommandLauncher::_ffmpeg_command(void* _self)
{
	CommandLauncher* self = (CommandLauncher*) _self;
	self->RunCommand();
	return B_OK;
}


void
CommandLauncher::RunCommand()
{
	// redirect stderr + stout
	int stderr_pipe[2];
	int stdout_pipe[2];
	int original_stderr = dup(STDERR_FILENO);
	int original_stdout = dup(STDOUT_FILENO);

	if (pipe(stderr_pipe) == 0) {
		dup2(stderr_pipe[1], STDERR_FILENO);
		close(stderr_pipe[1]);
	}
	if (pipe(stdout_pipe) == 0) {
		dup2(stdout_pipe[1], STDOUT_FILENO);
		close(stdout_pipe[1]);
	}

	int pipe_flags = fcntl(stderr_pipe[0], F_GETFD);
	pipe_flags |= FD_CLOEXEC;
	fcntl(stderr_pipe[0], F_SETFD, pipe_flags);

	pipe_flags = fcntl(stdout_pipe[0], F_GETFD);
	pipe_flags |= FD_CLOEXEC;
	fcntl(stdout_pipe[0], F_SETFD, pipe_flags);

	// create thread for ffmpeg
	const char* arguments[4];
	arguments[0] = "/bin/sh";
	arguments[1] = "-c";
	arguments[2] = fCommandline.String();
	arguments[3] = nullptr;

	fThread = load_image(3, arguments, const_cast<const char**>(environ));
	status_t error_code = fThread;

	if (error_code >= 0) {
		setpgid(fThread, fThread);
		error_code = resume_thread(fThread);
	}

	dup2(original_stderr, STDERR_FILENO);
	dup2(original_stdout, STDOUT_FILENO);

	// read stderr output and send to target
	if (error_code >= 0) {
		char buffer[4096];
		while (true) {
			ssize_t amount_read;
			if (fCommandFlag == ENCODING)
				amount_read = read(stderr_pipe[0], buffer, sizeof(buffer));
			else
				amount_read = read(stdout_pipe[0], buffer, sizeof(buffer));

			if (amount_read <= 0)
				break;

			// Make sure the buffer is null terminated
			buffer[amount_read] = 0;

			int32 seconds = GetCurrentTime(buffer);
			fOutputMessage->AddInt32("time", seconds);
			fOutputMessage->AddString("data", buffer);
			fTargetMessenger->SendMessage(fOutputMessage);
			fOutputMessage->RemoveName("time");
			fOutputMessage->RemoveName("data");

			// check if output contains error messages
			BString output_string(buffer);
			if (output_string.FindFirst("Error while decoding stream") != B_ERROR) {
				fErrorCode = FAILED;
				kill_thread(fThread);
				break;
			}
		}
	}

	// clean up
	dup2(original_stderr, STDERR_FILENO);
	close(stderr_pipe[0]);
	dup2(original_stdout, STDOUT_FILENO);
	close(stdout_pipe[0]);

	status_t proc_exit_code = 0;
	wait_for_thread(fThread, &proc_exit_code);

	// inform target that the command has finished
	if (fErrorCode != SUCCESS)
		proc_exit_code = fErrorCode;

	fFinishMessage->AddInt32("exitcode", proc_exit_code);
	fTargetMessenger->SendMessage(fFinishMessage);
	fBusy = false;

	delete fOutputMessage;
	delete fFinishMessage;
}


int32
CommandLauncher::GetCurrentTime(const char* buffer)
{
	BString output(buffer);
	int32 time_startpos = output.FindFirst("time=");
	if (time_startpos == -1)
		return -1;

	time_startpos += 5;
	int32 time_endpos = output.FindFirst(".", time_startpos);
	BString time_string;
	output.CopyInto(time_string, time_startpos, time_endpos - time_startpos);

	return (string_to_seconds(time_string));
}
