/*
 * Copyright 2023, All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Humdinger, humdingerb@gmail.com, 2023
*/


#include "JobWindow.h"
#include "messages.h"

#include <Alert.h>
#include <Catalog.h>
#include <ColumnTypes.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Notification.h>
#include <Path.h>
#include <StringFormat.h>

#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "JobWindow"


JobWindow::JobWindow(BRect rect, BMessenger* mainwindow)
	:
	BWindow(rect, B_TRANSLATE("Job manager"), B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fJobRunning(false)
{

	fJobList = new JobList();
	fJobList->SetSelectionMode(B_SINGLE_SELECTION_LIST);
	fJobList->SetSelectionMessage(new BMessage(M_JOB_SELECTED));

	float colWidth = be_plain_font->StringWidth("/some/reasonably/long/path/to/the/file");
	BStringColumn* nameCol = new BStringColumn(B_TRANSLATE("Job name"), colWidth,
		colWidth / 4, colWidth * 4, B_TRUNCATE_MIDDLE);
	fJobList->AddColumn(nameCol, kJobNameIndex);

	colWidth = be_plain_font->StringWidth("🕛: 00:00:00") + 10;
	BStringColumn* timeCol = new BStringColumn(B_TRANSLATE("Duration"), colWidth,
		colWidth / 4, colWidth, B_TRUNCATE_BEGINNING);
	fJobList->AddColumn(timeCol, kDurationIndex);

	colWidth = be_plain_font->StringWidth(B_TRANSLATE("Finished")) + 10;
	BStringColumn* statusCol = new BStringColumn(B_TRANSLATE("Status"), colWidth,
		colWidth / 4, colWidth * 2, B_TRUNCATE_END);
	fJobList->AddColumn(statusCol, kStatusIndex);

	fStartAbortButton = new BButton(B_TRANSLATE("Start jobs"), new BMessage(M_JOB_START));
	fRemoveButton = new BButton(B_TRANSLATE("Remove"), new BMessage(M_JOB_REMOVE));
	fLogButton = new BButton(B_TRANSLATE("Show log"), new BMessage(M_JOB_LOG));
	fClearButton = new BButton(B_TRANSLATE("Clear finished"), new BMessage(M_CLEAR_LIST));
	fUpButton = new BButton("⏶", new BMessage(M_LIST_UP));
	fDownButton = new BButton("⏷", new BMessage(M_LIST_DOWN));

	float width = be_plain_font->StringWidth("XXX");
	BSize size(width, width);
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
	LoadJobs(jobs);

	const char* jobname;
	const char* duration;
	const char* command;
	int32 i = 0;
	while ((jobs.FindString("jobname", i, &jobname) == B_OK)
			&& (jobs.FindString("duration", i, &duration) == B_OK)
			&& ((jobs.FindString("command", i, &command) == B_OK))) {
		AddJob(jobname, duration, command);
		i++;
	}
	UpdateButtonStates();
}


JobWindow::~JobWindow()
{
	if (fJobList->CountRows() != 0);
		SaveJobs();
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
			UpdateButtonStates();
			break;
		}
		case M_JOB_START:
		{
			fCurrentJob = GetNextJob();
			if (fCurrentJob == NULL) {
				fJobRunning = false;
				UpdateButtonStates();

				int32 count = fJobList->CountRows();
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
			UpdateButtonStates();

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
			UpdateButtonStates();
			break;
		}
		case M_JOB_REMOVE:
		{
			BRow* row = fJobList->CurrentSelection();
			int32 rowIndex = fJobList->IndexOf(row);
			fJobList->RemoveRow(row);

			int32 count = fJobList->CountRows();
			// Did we remove the first or last row?
			fJobList->AddToSelection(
				fJobList->RowAt((rowIndex > count - 1) ? count - 1 : rowIndex));

			UpdateButtonStates();
			break;
		}
		case M_JOB_LOG:
		{
			BString text;
			JobRow* currentRow = dynamic_cast<JobRow*>(fJobList->CurrentSelection());
			text << currentRow->GetJobName() << ":\n" << currentRow->GetLog();
			BAlert* alert = new BAlert("log", text, B_TRANSLATE("OK"));
			alert->SetShortcut(0, B_ESCAPE);
			alert->Go();
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
			UpdateButtonStates();
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
			UpdateButtonStates();
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
			UpdateButtonStates();
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

				char percent[4];
				snprintf(percent, sizeof(percent), "%" B_PRId32, encode_percentage);

				BString title(B_TRANSLATE("Job: %percent%%"));
				title.ReplaceFirst("%percent%", percent);
				SetTitle(title);

				title = B_TRANSLATE("Running: %percent%%");
				title.ReplaceFirst("%percent%", percent);
				fCurrentJob->SetStatus(title);
			}
			break;
		}
		case M_ENCODE_FINISHED:
		{
			SetTitle(B_TRANSLATE("Job manager"));

			status_t exit_code;
			message->FindInt32("exitcode", &exit_code);

			if (exit_code == ABORTED) {
				fCurrentJob->SetStatus(WAITING);
				break;
			}
			if (exit_code == SUCCESS)
				fCurrentJob->SetStatus(FINISHED);
			else
				fCurrentJob->SetStatus(ERROR);

			BMessenger(this).SendMessage(M_JOB_START);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


status_t
JobWindow::LoadJobs(BMessage& jobs)
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
JobWindow::SaveJobs()
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

	BFile file;
	status = file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	BMessage jobs('jobs');

	for (int32 i = 0; i < fJobList->CountRows(); i++) {
		JobRow* row = dynamic_cast<JobRow*>(fJobList->RowAt(i));
		int32 state = row->GetStatus();
		if (state != FINISHED) {
			jobs.AddString("jobname", row->GetJobName());
			jobs.AddString("duration", row->GetDuration());
			jobs.AddString("command", row->GetCommandLine());
		}
	}

	if (status == B_OK)
		status = jobs.Flatten(&file);

	return status;
}


void
JobWindow::AddJob(const char* jobname, const char* duration, const char* commandline,
				int32 statusID)
{
	if (!IsUnique(commandline))
		return;

	JobRow* row = new JobRow(jobname, duration, commandline, WAITING);
	fJobList->AddRow(row);
	UpdateButtonStates();
}


bool
JobWindow::IsJobRunning()
{
	return fJobRunning;
}


bool
JobWindow::IsUnique(const char* commandline)
{
	for (int32 i = 0; i < fJobList->CountRows(); i++) {
		JobRow* row = dynamic_cast<JobRow*>(fJobList->RowAt(i));
		BString command = row->GetCommandLine();
		if (command == commandline)
			return false;
	}
	return true;
}


JobRow*
JobWindow::GetNextJob()
{
	for (int32 i = 0; i < fJobList->CountRows(); i++) {
		JobRow* row = dynamic_cast<JobRow*>(fJobList->RowAt(i));
		int32 status = row->GetStatus();
		if (status == WAITING)
			return row;
	}
	return NULL;
}


void
JobWindow::UpdateButtonStates()
{
	int32 count = fJobList->CountRows();
	SetStartButtonLabel((fJobRunning == true) ? ABORT : START);

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
	} else
		fRemoveButton->SetEnabled(true);

	int32 status = currentRow->GetStatus();
	if (status == ERROR)
		fLogButton->SetEnabled(true);

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
JobWindow::SetStartButtonLabel(int32 state)
{
	int32 count = fJobList->CountRows();
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
