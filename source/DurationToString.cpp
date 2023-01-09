/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */


#include "DurationToString.h"

#include <stdio.h>


void
duration_to_string(int32 seconds, char* string, size_t stringSize)
{
	bool negative = seconds < 0;
	if (negative)
		seconds = -seconds;

	int32 hours = seconds / 3600;
	seconds -= hours * 3600;
	int32 minutes = seconds / 60;
	seconds = seconds % 60;

	snprintf(string, stringSize, "%s%" B_PRId32 ":%02" B_PRId32 ":%02"
			B_PRId32, negative ? "-" : "", hours, minutes, seconds);
}
