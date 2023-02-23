/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */


#ifndef UTILITIES_H
#define UTILITIES_H


#include <String.h>
#include <SupportDefs.h>


void	remove_over_precision(BString& float_string);
void	seconds_to_string(int32 seconds, char* string, size_t stringSize);
int32	string_to_seconds(BString& time_string);

#endif // UTILITIES_H
