/*
 * Copyright 2023, All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Humdinger, humdingerb@gmail.com, 2023
*/


#include "JobList.h"
#include "Utilities.h"

#include <Catalog.h>
#include <StringList.h>

#include <stdio.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "JobList"


// Job list view
JobList::JobList()
	:
	BColumnListView("Joblist", 0, B_FANCY_BORDER, false)
{
}


// Job list row
JobRow::JobRow(int32 jobnumber, const char* filename, const char* duration,
	const char* commandline, int32 statusID)
	:
	BRow(),
	fFilename(filename),
	fDuration(duration),
	fCommandLine(commandline),
	fStatusID(statusID)
{
	BStringList name;
	fFilename.Split("/", true, name);
	fJobName = name.Last();
	fDurationSecs = string_to_seconds(fDuration);
	BString symbolDuration(fDuration);
	symbolDuration.Prepend("🕛: " );

	SetField(new BIntegerField(jobnumber), kJobNumberIndex);
	SetField(new BStringField(fJobName.String()), kJobNameIndex);
	SetField(new BStringField(symbolDuration.String()), kDurationIndex);
	SetStatus(statusID);
}


void
JobRow::SetStatus(int32 statusID)
{
	switch (statusID) {
		case WAITING:
			fStatus = B_TRANSLATE("Waiting");
			break;
		case RUNNING:
			fStatus = B_TRANSLATE("Running");
			break;
		case FINISHED:
			fStatus = B_TRANSLATE("Finished");
			break;
		case ERROR:
			fStatus = B_TRANSLATE("Error");
			break;
		default:
			return;
	}
	SetField(new BStringField(fStatus.String()), kStatusIndex);
	fStatusID = statusID;
}


void
JobRow::SetStatus(BString status)
{
	SetField(new BStringField(status), kStatusIndex);
}


void JobRow::AddToLog(BString log)
{
	fLog << log;
}
