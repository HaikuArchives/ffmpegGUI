/*
 * Copyright 2023, All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Humdinger, humdingerb@gmail.com, 2023
*/
#ifndef JOBLIST_H
#define JOBLIST_H


#include <ColumnListView.h>
#include <ColumnTypes.h>

// Column indexes
const int32 kJobNumberIndex = 0;
const int32 kJobNameIndex = 1;
const int32 kDurationIndex = 2;
const int32 kStatusIndex = 3;

// Job status
enum {
	WAITING = 0,
	RUNNING,
	FINISHED,
	ERROR,
};

class JobList : public BColumnListView {
public:
					JobList();
};


class JobRow : public BRow {
public:
					JobRow(int32 jobnumber, const char* jobname, const char* duration,
						const char* commandline, int32 statusID);

	int32			GetJobNumber() { return fJobNumber; };
	const char*		GetFilename() { return fFilename.String(); };
	const char*		GetJobName() { return fJobName.String(); };
	const char*		GetDuration() { return fDuration.String(); };
	int32			GetDurationSeconds() { return fDurationSecs; };
	const char*		GetCommandLine() { return fCommandLine.String(); };
	int32			GetStatus() { return fStatusID; };
	const char*		GetLog() { return fLog.String(); };

	void			SetStatus(int32 statusID);
	void			SetStatus(BString status);
	void			AddToLog(BString log);

private:
	BString			fFilename;
	BString			fJobName;
	BString			fDuration;
	BString			fCommandLine;
	BString			fStatus;
	BString			fLog;
	int32			fJobNumber;
	int32			fDurationSecs;
	int32			fStatusID;
};


#endif // JOBLIST_H
