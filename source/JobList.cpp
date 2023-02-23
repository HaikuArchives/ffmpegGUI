/*
 * Copyright 2023, All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Humdinger, humdingerb@gmail.com, 2023
*/


#include "JobList.h"
#include "Utilities.h"

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "JobList"


// Job list view
JobList::JobList()
	:
	BColumnListView("Joblist", 0, B_FANCY_BORDER, false)
{
}


// Job list row
JobRow::JobRow(const char* jobname, const char* duration, const char* commandline, int32 statusID)
	:
	BRow(),
	fJobName(jobname),
	fDuration(duration),
	fCommandLine(commandline),
	fStatusID(statusID)
{
	SetField(new BStringField(fJobName.String()), kJobNameIndex);
	SetField(new BStringField(fDuration.String()), kDurationIndex);
	SetStatus(statusID);
	fDurationSecs = string_to_seconds(fDuration);
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
