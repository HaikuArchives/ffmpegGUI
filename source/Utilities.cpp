/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 *
 * Humdinger, humdingerb@gmail.com, 2022
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022-2023
*/


#include "Utilities.h"

#include <StringList.h>

#include <cstdlib>
#include <stdio.h>


void
remove_over_precision(BString& float_string)
{
	// Remove trailing "0" and "."
	while (true) {
		if (float_string.EndsWith("0")) {
			float_string.Truncate(float_string.CountChars() - 1);
			if (float_string.EndsWith(".")) {
				float_string.Truncate(float_string.CountChars() - 1);
				break;
			}
		} else
			break;
	}
}


void
seconds_to_string(int32 seconds, char* string, size_t stringSize)
{
	bool negative = seconds < 0;
	if (negative)
		seconds = -seconds;

	int32 hours = seconds / 3600;
	seconds -= hours * 3600;
	int32 minutes = seconds / 60;
	seconds = seconds % 60;

	snprintf(string, stringSize, "%s%" B_PRId32 ":%02" B_PRId32 ":%02" B_PRId32,
		negative ? "-" : "", hours, minutes, seconds);
}


int32
string_to_seconds(BString& time_string)
{
	int32 hours = 0;
	int32 minutes = 0;
	int32 seconds = 0;
	BStringList time_list;

	time_string.Trim().Split(":", true, time_list);
	hours = std::atoi(time_list.StringAt(0).String());
	minutes = std::atoi(time_list.StringAt(1).String());
	seconds = std::atoi(time_list.StringAt(2).String());

	seconds += minutes * 60;
	seconds += hours * 3600;

	return seconds;
}
