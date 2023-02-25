/*
 * Copyright 2023, All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Humdinger, humdingerb@gmail.com, 2023
*/
#ifndef JOBWINDOW_H
#define JOBWINDOW_H

#include <Button.h>
#include <Window.h>

#include "commandlauncher.h"
#include "JobList.h"

// Start/Abort button status
enum {
	START = 0,
	ABORT
};


class JobWindow : public BWindow {
public:
					JobWindow(BRect frame, BMessage* settings);
					~JobWindow();

	virtual bool 	QuitRequested();
	virtual void 	MessageReceived(BMessage* message);

			void	AddJob(const char* filename, const char* duration, const char* commandline,
						int32 statusID = 0);
			bool	IsJobRunning();

	BMessage*		GetColumnState();
			void	SetColumnState(BMessage* archive);

private:
	status_t		LoadJobs(BMessage& jobs);
	status_t		SaveJobs();

	int32			CountFinished();
	bool			IsUniqueJob(const char* commandline);
	int32			IndexOfSameFilename(const char* filename);
	JobRow*			GetNextJob();
	void			UpdateButtonStates();
	void			SetStartButtonLabel(int32 state);

	CommandLauncher*	fJobCommandLauncher;
	JobList*		fJobList;
	JobRow*			fCurrentJob;

	bool			fJobRunning;

	BButton*		fStartAbortButton;
	BButton*		fRemoveButton;
	BButton*		fLogButton;
	BButton*		fClearButton;
	BButton*		fUpButton;
	BButton*		fDownButton;
};


#endif // JOBWINDOW_H
