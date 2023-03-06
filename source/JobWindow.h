/*
 * Copyright 2023, All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Humdinger, humdingerb@gmail.com, 2023
*/
#ifndef JOBWINDOW_H
#define JOBWINDOW_H

#include <Button.h>
#include <PopUpMenu.h>
#include <Window.h>

#include "CommandLauncher.h"
#include "JobList.h"

// Start/Abort button status
enum {
	START = 0,
	ABORT
};


class ContextMenu : public BPopUpMenu {
public:
					ContextMenu(const char* name, BMessenger target);
	virtual 		~ContextMenu();

private:
	BMessenger		fTarget;
};


class JobWindow : public BWindow {
public:
					JobWindow(BRect frame, BMessage* settings, BMessenger* target);
					~JobWindow();

	virtual bool 	QuitRequested();
	virtual void 	MessageReceived(BMessage* message);

			void	AddJob(const char* filename, const char* duration, const char* commandline,
						int32 statusID = 0);
			bool	IsJobRunning();

	BMessage*		GetColumnState();
			void	SetColumnState(BMessage* archive);

private:
	void			_ShowPopUpMenu(BPoint where);
	bool			fShowingPopUpMenu;

	status_t		_LoadJobs(BMessage& jobs);
	status_t		_SaveJobs();

	void			_PlayVideo(const char* filepath);
	void			_ShowLog(JobRow* row);

	void			_SendJobCount(int32);
	int32			_CountFinished();
	bool			_IsUniqueJob(const char* commandline);
	int32			_IndexOfSameFilename(const char* filename);
	JobRow*			_GetNextJob();
	void			_UpdateStates();
	void			_SetStartAbortLabel(int32 state);

private:
	CommandLauncher*	fJobCommandLauncher;
	BMessenger*		fMainWindow;
	JobList*		fJobList;
	JobRow*			fCurrentJob;
	int32			fJobNumber;

	bool			fJobRunning;
	int32			fSingleJob;

	BMenuItem*		fStartAbortMenu;
	BMenuItem*		fStartAbortSingleMenu;
	BMenuItem*		fClearMenu;
	BMenuItem*		fPlayMenu;
	BMenuItem*		fLogMenu;
	BMenuItem*		fRemoveMenu;
	BMenuItem*		fRemoveAllMenu;


	BButton*		fStartAbortButton;
	BButton*		fRemoveButton;
	BButton*		fLogButton;
	BButton*		fClearButton;
	BButton*		fUpButton;
	BButton*		fDownButton;
};


#endif // JOBWINDOW_H
