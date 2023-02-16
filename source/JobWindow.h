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


class JobWindow : public BWindow {
public:
					JobWindow(BRect rect, BMessenger* mainwindow);
					~JobWindow();

	virtual bool 	QuitRequested();
	virtual void 	MessageReceived(BMessage* message);

			void	AddJob(const char* jobname, const char* duration, const char* commandline,
						int32 statusID = 0);
			bool	IsJobRunning();

private:
	status_t		LoadJobs(BMessage& jobs);
	status_t		SaveJobs();

	bool			IsUnique(const char* commandline);
	JobRow*			GetNextJob();
	void			UpdateButtonStates();

	BMessenger*		fMainWindow;
	CommandLauncher*	fJobCommandLauncher;
	JobList*		fJobList;
	JobRow*			fCurrentJob;

	bool			fJobRunning;

	BButton*		fStartAbortButton;
	BButton*		fRemoveButton;
	BButton*		fClearButton;
	BButton*		fUpButton;
	BButton*		fDownButton;
};


#endif // JOBWINDOW_H
