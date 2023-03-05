/*
 * Copyright 2023, All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Humdinger, humdingerb@gmail.com, 2023
*/


#include "JobWindow.h"
#include "Messages.h"

#include <Alert.h>
#include <Catalog.h>
#include <ColumnTypes.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Notification.h>
#include <Path.h>
#include <Roster.h>
#include <StringFormat.h>

#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "JobWindow"


JobWindow::JobWindow(BRect frame, BMessage* settings, BMessenger* target)
	:
	BWindow(frame, B_TRANSLATE("Job manager"), B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fJobRunning(false),
	fSingleJob(WAITING),
	fJobNumber(1),
	fMainWindow(target)
{
	fJobList = new JobList();
	fJobList->SetSelectionMode(B_SINGLE_SELECTION_LIST);
	fJobList->SetSelectionMessage(new BMessage(M_JOB_SELECTED));
	fJobList->SetInvocationMessage(new BMessage(M_JOB_INVOKED));

	float colWidth = be_plain_font->StringWidth("XXX") + 15;
	BIntegerColumn* numberCol = new BIntegerColumn(B_TRANSLATE_COMMENT(
		"N°", "'Number' abbreviation"), colWidth, colWidth / 1.5, colWidth * 2);
	fJobList->AddColumn(numberCol, kJobNumberIndex);

	colWidth = be_plain_font->StringWidth("somereasonablyquitelongoutputfilename.avi");
	BStringColumn* nameCol = new BStringColumn(B_TRANSLATE("Job name"), colWidth,
		colWidth / 4, colWidth * 4, B_TRUNCATE_MIDDLE);
	fJobList->AddColumn(nameCol, kJobNameIndex);

	colWidth = be_plain_font->StringWidth("🕛: 00:00:00") + 20;
	BStringColumn* timeCol = new BStringColumn(B_TRANSLATE("Duration"), colWidth,
		colWidth / 4, colWidth * 2, B_TRUNCATE_BEGINNING);
	fJobList->AddColumn(timeCol, kDurationIndex);

	colWidth = be_plain_font->StringWidth(B_TRANSLATE("Running: 100%")) + 40;
	BStringColumn* statusCol = new BStringColumn(B_TRANSLATE("Status"), colWidth,
		colWidth / 4, colWidth * 2, B_TRUNCATE_END);
	fJobList->AddColumn(statusCol, kStatusIndex);

	fStartAbortButton = new BButton(B_TRANSLATE("Start jobs"), new BMessage(M_JOB_START));
	fStartAbortButton->MakeDefault(true);
	fRemoveButton = new BButton(B_TRANSLATE("Remove"), new BMessage(M_JOB_REMOVE));
	fLogButton = new BButton(B_TRANSLATE("Error log"), new BMessage(M_JOB_LOG));
	fClearButton = new BButton(B_TRANSLATE("Clear finished"), new BMessage(M_CLEAR_LIST));
	fUpButton = new BButton("⏶", new BMessage(M_LIST_UP));
	fDownButton = new BButton("⏷", new BMessage(M_LIST_DOWN));

	// Set button sizes
	BSize size = fStartAbortButton->PreferredSize();
	size.width = std::max(size.width, fRemoveButton->PreferredSize().width);
	size.width = std::max(size.width, fLogButton->PreferredSize().width);
	size.width = std::max(size.width, fClearButton->PreferredSize().width);

	fStartAbortButton->SetExplicitSize(size);
	fRemoveButton->SetExplicitSize(size);
	fLogButton->SetExplicitSize(size);
	fClearButton->SetExplicitSize(size);

	float width = be_plain_font->StringWidth("XXX");
	size = BSize(width, width);
	fUpButton->SetExplicitSize(size);
	fDownButton->SetExplicitSize(size);

	BLayoutBuilder::Group<>(this, B_HORIZONTAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_WINDOW_SPACING)
		.Add(fJobList)
		.AddGroup(B_VERTICAL)
			.Add(fStartAbortButton)
			.Add(fRemoveButton)
			.Add(fLogButton)
			.Add(fClearButton)
			.AddGlue()
			.AddGroup(B_HORIZONTAL)
				.Add(fUpButton)
				.AddGlue()
			.End()
			.AddGroup(B_HORIZONTAL)
				.Add(fDownButton)
				.AddGlue()
			.End()
		.End();

	// initialize job command launcher
	fJobCommandLauncher = new CommandLauncher(new BMessenger(this));

	BMessage jobs;
	_LoadJobs(jobs);

	const char* filename;
	const char* duration;
	const char* command;
	int32 i = 0;
	while ((jobs.FindString("filename", i, &filename) == B_OK)
			&& (jobs.FindString("duration", i, &duration) == B_OK)
			&& ((jobs.FindString("command", i, &command) == B_OK))) {
		AddJob(filename, duration, command);
		i++;
	}

	if (fJobList->CountRows() != 0)
		fJobList->AddToSelection(fJobList->RowAt(0));

	_UpdateButtonStates();

	// apply window settings
	if (settings->FindRect("job_window", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}
	MoveOnScreen();

	// apply column settings
	BMessage columnSettings;
	if (settings->FindMessage("column settings", &columnSettings) == B_OK) {
		columnSettings.RemoveName("sortID"); // forget previous sorting column
		fJobList->LoadState(&columnSettings);
	}
}


JobWindow::~JobWindow()
{
	// clear finished or errored jobs before saving
	for (int32 i = fJobList->CountRows() - 1; i >= 0; i--) {
		JobRow* row = dynamic_cast<JobRow*>(fJobList->RowAt(i));
		int32 status = row->GetStatus();
		if ((status == ERROR) or (status == FINISHED))
			fJobList->RemoveRow(row);
	}

	_SaveJobs();
}


bool
JobWindow::QuitRequested()
{
	if (!IsHidden())
		Hide();

	return false;
}


void
JobWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_JOB_SELECTED:
		{
			_UpdateButtonStates();
			break;
		}
		case M_JOB_INVOKED:
		{
			JobRow* currentRow = dynamic_cast<JobRow*>(fJobList->CurrentSelection());
			if (currentRow == NULL)
				break;

			int32 status = currentRow->GetStatus();
			if (status == FINISHED) {
				_PlayVideo(currentRow->GetFilename());
				break;
			} else if (status == ERROR) {
				_ShowLog(currentRow);
				break;
			} else if (status == RUNNING)
				break;

			// Else we're WAITING: fall through and to a single job
		}
		case M_SINGLE_JOB:
			fSingleJob = RUNNING;
			// intentional fall through
		case M_JOB_START:
		{
			fCurrentJob = _GetNextJob();
			if (fCurrentJob == NULL) {
				SetTitle(B_TRANSLATE("Job manager"));
				fJobRunning = false;
				_UpdateButtonStates();

				int32 count = (fSingleJob == FINISHED)? 1 : fJobList->CountRows();
				BString text;
				static BStringFormat format(B_TRANSLATE("{0, plural,"
					"one{Encoding job finished.}"
					"other{Encoding jobs finished.}}"));
				format.Format(text, count);
				BNotification encodeFinished(B_INFORMATION_NOTIFICATION);
				encodeFinished.SetGroup(B_TRANSLATE_SYSTEM_NAME("ffmpeg GUI"));
				encodeFinished.SetTitle(B_TRANSLATE("Job manager"));
				encodeFinished.SetContent(text);
				encodeFinished.Send();

				break;
			}

			fJobRunning = true;
			fCurrentJob->SetStatus(RUNNING);
			_UpdateButtonStates();

			BMessage startMsg(M_ENCODE_COMMAND);
			startMsg.AddString("cmdline", fCurrentJob->GetCommandLine());
			fJobCommandLauncher->PostMessage(&startMsg);
			break;
		}
		case M_JOB_ABORT:
		{
			BMessage stop_encode_message(M_STOP_COMMAND);
			fJobCommandLauncher->PostMessage(&stop_encode_message);

			fJobRunning = false;
			SetTitle(B_TRANSLATE("Job manager"));
			break;
		}
		case M_JOB_REMOVE:
		{
			BRow* row = fJobList->CurrentSelection();
			int32 rowIndex = fJobList->IndexOf(row);
			fJobList->RemoveRow(row);

			int32 count = fJobList->CountRows();
			_SendJobCount(count);
			// Did we remove the first or last row?
			fJobList->AddToSelection(
				fJobList->RowAt((rowIndex > count - 1) ? count - 1 : rowIndex));

			_UpdateButtonStates();
			break;
		}
		case M_JOB_LOG:
		{
			JobRow* currentRow = dynamic_cast<JobRow*>(fJobList->CurrentSelection());
			_ShowLog(currentRow);
			break;
		}
		case M_CLEAR_LIST:
		{
			for (int32 i = fJobList->CountRows() - 1; i >= 0; i--) {
				JobRow* row = dynamic_cast<JobRow*>(fJobList->RowAt(i));
				int32 status = row->GetStatus();
				if (status == FINISHED)
					fJobList->RemoveRow(row);
			}
			_SendJobCount(fJobList->CountRows());
			_UpdateButtonStates();
			break;
		}
		case M_LIST_UP:
		{
			BRow* row = fJobList->CurrentSelection();
			int32 rowIndex = fJobList->IndexOf(row);
			if (rowIndex < 1)
				break;

			fJobList->SwapRows(rowIndex, rowIndex - 1);
			fJobList->AddToSelection(fJobList->RowAt(rowIndex - 1));
			_UpdateButtonStates();
			break;
		}
		case M_LIST_DOWN:
		{
			BRow* row = fJobList->CurrentSelection();
			int32 rowIndex = fJobList->IndexOf(row);
			int32 last = fJobList->CountRows() - 1;
			if ((rowIndex == last) || (rowIndex < 0))
				break;

			fJobList->SwapRows(rowIndex, rowIndex + 1);
			fJobList->AddToSelection(fJobList->RowAt(rowIndex + 1));
			_UpdateButtonStates();
			break;
		}
		case M_ENCODE_PROGRESS:
		{
			BString progress_data;
			message->FindString("data", &progress_data);
			fCurrentJob->AddToLog(progress_data);

			int32 seconds;
			message->FindInt32("time", &seconds);

			// calculate progress percentage
			if (seconds > -1) {
				int32 duration = fCurrentJob->GetDurationSeconds();
				int32 encode_percentage;

				if (duration > 0)
					encode_percentage = (seconds * 100) / duration;
				else
					encode_percentage = 0;

				BString title(B_TRANSLATE("Job"));
				if (fSingleJob == RUNNING)
					title << " " << fCurrentJob->GetJobNumber() << ": ";
				else
					title << " (" << _CountFinished() << "/" << fJobList->CountRows() << "): ";
				title << encode_percentage << "%";
				SetTitle(title);

				title = B_TRANSLATE("Running:");
				title << " " << encode_percentage << "%";
				fCurrentJob->SetStatus(title);
			}
			break;
		}
		case M_ENCODE_FINISHED:
		{
			status_t exit_code;
			message->FindInt32("exitcode", &exit_code);

			if (exit_code == ABORTED) {
				fSingleJob = WAITING;
				fCurrentJob->SetStatus(WAITING);
				_UpdateButtonStates();
				break;
			}
			if (exit_code == SUCCESS)
				fCurrentJob->SetStatus(FINISHED);
			else
				fCurrentJob->SetStatus(ERROR);

			if (fSingleJob == RUNNING)
				fSingleJob = FINISHED; // means single job finished

			BMessenger(this).SendMessage(M_JOB_START);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


status_t
JobWindow::_LoadJobs(BMessage& jobs)
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	status = path.Append("ffmpegGUI");
	if (status != B_OK)
		return status;

	status = path.Append("jobs");
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK)
		return status;

	return jobs.Unflatten(&file);
}


status_t
JobWindow::_SaveJobs()
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	status = path.Append("ffmpegGUI");
	if (status != B_OK)
		return status;

	status = create_directory(path.Path(), 0777);
	if (status != B_OK)
		return status;

	status = path.Append("jobs");
	if (status != B_OK)
		return status;

	// remove "jobs" file if there are none.
	if (fJobList->CountRows() == 0) {
		BEntry entry(path.Path());
		status = entry.Remove();
		return status;
	}

	BFile file;
	status = file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	BMessage jobs('jobs');

	for (int32 i = 0; i < fJobList->CountRows(); i++) {
		JobRow* row = dynamic_cast<JobRow*>(fJobList->RowAt(i));
		int32 state = row->GetStatus();
		if (state != FINISHED) {
			jobs.AddString("filename", row->GetFilename());
			jobs.AddString("duration", row->GetDuration());
			jobs.AddString("command", row->GetCommandLine());
		}
	}

	if (status == B_OK)
		status = jobs.Flatten(&file);

	return status;
}


void
JobWindow::_PlayVideo(const char* filepath)
{
	BEntry video_entry(filepath);
	entry_ref video_ref;
	video_entry.GetRef(&video_ref);
	be_roster->Launch(&video_ref);
}


void
JobWindow::_ShowLog(JobRow* row)
{
	BString text;
	text << row->GetJobName() << ":\n\n" << row->GetLog();
	BAlert* alert = new BAlert("log", text, B_TRANSLATE("OK"));
	alert->SetShortcut(0, B_ESCAPE);
	alert->Go();
}


void
JobWindow::AddJob(const char* filename, const char* duration, const char* commandline,
				int32 statusID)
{
	if (!_IsUniqueJob(commandline))
		return;

	int32 index = _IndexOfSameFilename(filename);
	if (index == -1) {
		JobRow* row = new JobRow(fJobNumber++, filename, duration, commandline, WAITING);
		fJobList->AddRow(row);
		_SendJobCount(fJobList->CountRows());
		_UpdateButtonStates();
		return;
	}

	BString text(B_TRANSLATE("There's already a job with that same output file:\n"
		"%filename%\n\n"
		"Better choose another output file, or remove the existing job.\n"));
	text.ReplaceFirst("%filename%", filename);
	BAlert* alert = new BAlert("fileexists", text,
		B_TRANSLATE("Cancel"), B_TRANSLATE("Show job manager"));
	alert->SetShortcut(0, B_ESCAPE);

	int32 choice = alert->Go();
	switch (choice) {
		case 0:
			return;
		case 1:
		{
			fJobList->AddToSelection(fJobList->RowAt(index));
			if (IsHidden())
				Show();
		}
	}
}


bool
JobWindow::IsJobRunning()
{
	return fJobRunning;
}


BMessage*
JobWindow::GetColumnState()
{
	BMessage columnSettings;
	fJobList->SaveState(&columnSettings);

	return (new BMessage(columnSettings));
}


void
JobWindow::SetColumnState(BMessage* archive)
{
	BMessage columnSettings;
	if (archive->FindMessage("column settings", &columnSettings) == B_OK)
		fJobList->LoadState(&columnSettings);
}


void
JobWindow::_SendJobCount(int32 count)
{
	BMessage message(M_JOB_COUNT);
	message.AddInt32("jobcount", count);
	fMainWindow->SendMessage(&message);
}


int32
JobWindow::_CountFinished()
{
	int32 count = 1;

	for (int32 i = 0; i < fJobList->CountRows(); i++) {
		JobRow* row = dynamic_cast<JobRow*>(fJobList->RowAt(i));
		int32 status = row->GetStatus();
		if (status == FINISHED)
			count++;
	}
	return count;
}

bool
JobWindow::_IsUniqueJob(const char* commandline)
{
	for (int32 i = 0; i < fJobList->CountRows(); i++) {
		JobRow* row = dynamic_cast<JobRow*>(fJobList->RowAt(i));
		BString command = row->GetCommandLine();
		if (command == commandline)
			return false;
	}
	return true;
}


int32
JobWindow::_IndexOfSameFilename(const char* filename)
{
	for (int32 i = 0; i < fJobList->CountRows(); i++) {
		JobRow* row = dynamic_cast<JobRow*>(fJobList->RowAt(i));
		BString name = row->GetFilename();
		if (name == filename)
			return i;
	}
	return -1;
}


JobRow*
JobWindow::_GetNextJob()
{
	if (fSingleJob == RUNNING) {
		JobRow* currentRow = dynamic_cast<JobRow*>(fJobList->CurrentSelection());
		return currentRow;
	}

	// single job finished
	if (fSingleJob == FINISHED)
		return NULL;

	for (int32 i = 0; i < fJobList->CountRows(); i++) {
		JobRow* row = dynamic_cast<JobRow*>(fJobList->RowAt(i));
		int32 status = row->GetStatus();
		if (status == WAITING)
			return row;
	}
	return NULL;
}


void
JobWindow::_UpdateButtonStates()
{
	int32 count = fJobList->CountRows();
	_SetStartButtonLabel((fJobRunning == true) ? ABORT : START);

	// Empty list
	if (count == 0) {
		fStartAbortButton->SetEnabled(false);
		fRemoveButton->SetEnabled(false);
		fLogButton->SetEnabled(false);
		fClearButton->SetEnabled(false);
		fUpButton->SetEnabled(false);
		fDownButton->SetEnabled(false);
		return;
	}

	bool finishOrError = false;
	bool waitOrRun = false;
	for (int32 i = 0; i < count; i++) {
		JobRow* row = dynamic_cast<JobRow*>(fJobList->RowAt(i));
		int32 status = row->GetStatus();
		if ((status == FINISHED) or (status == ERROR))
			finishOrError = true;
		if ((status == WAITING) or (status == RUNNING))
			waitOrRun = true;
	}
	fClearButton->SetEnabled(finishOrError);
	fStartAbortButton->SetEnabled(waitOrRun);

	JobRow* currentRow = dynamic_cast<JobRow*>(fJobList->CurrentSelection());
	// Nothing selected
	if (currentRow == NULL) {
		fRemoveButton->SetEnabled(false);
		fLogButton->SetEnabled(false);
		fUpButton->SetEnabled(false);
		fDownButton->SetEnabled(false);
		return;
	}

	int32 status = currentRow->GetStatus();
	fLogButton->SetEnabled((status == ERROR) ? true :false);
	fRemoveButton->SetEnabled((status == RUNNING) ? false :true);

	// Move up/down button logic
	int32 rowIndex = fJobList->IndexOf(fJobList->CurrentSelection());
	if ((rowIndex == 0) or (count == 1))
		fUpButton->SetEnabled(false);
	else
		fUpButton->SetEnabled(true);

	if ((rowIndex == count - 1) or (count == 1))
		fDownButton->SetEnabled(false);
	else
		fDownButton->SetEnabled(true);
}


void
JobWindow::_SetStartButtonLabel(int32 state)
{
	int32 count = (fSingleJob == RUNNING)? 1 : fJobList->CountRows();
	BString text;

	if (state == START) {
		static BStringFormat format(B_TRANSLATE("{0, plural,"
			"one{Start job}"
			"other{Start jobs}}"));
		format.Format(text, count);
		fStartAbortButton->SetMessage(new BMessage(M_JOB_START));
	} else {
		static BStringFormat format(B_TRANSLATE("{0, plural,"
			"one{Abort job}"
			"other{Abort jobs}}"));
		format.Format(text, count);
		fStartAbortButton->SetMessage(new BMessage(M_JOB_ABORT));
	}
	fStartAbortButton->SetLabel(text);
}
