/*
 * Copyright 2022-2023. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022
 * Humdinger, humdingerb@gmail.com, 2022-2023
*/


#include "Spinner.h"


Spinner::Spinner(const char* name, const char* label, BMessage* message)
	:
	BSpinner(name, label, message),
	fInvoke(true)
{
	fStep = 1;
}


status_t
Spinner::Invoke(BMessage* message)
{
	if (fInvoke == false) {
		fInvoke = true;
		return B_OK;
	}
	return (BSpinner::Invoke(message));
}


void
Spinner::SetWithoutInvoke(int32 value)
{
	fInvoke = false;
	BSpinner::SetValue(value);
}


void
Spinner::Increment()
{
	SetValue(Value() + fStep);
}


void
Spinner::Decrement()
{
	SetValue(Value() - fStep);
}


void
Spinner::SetStep(int32 step)
{
	fStep = step;
}


// Spinner for floating point
DecSpinner::DecSpinner(const char* name, const char* label, BMessage* message)
	:
	BDecimalSpinner(name, label, message),
	fInvoke(true)
{
	SetPrecision(0);
}


status_t
DecSpinner::Invoke(BMessage* message)
{
	if (fInvoke == false) {
		fInvoke = true;
		return B_OK;
	}
	return (BDecimalSpinner::Invoke(message));
}


void
DecSpinner::SetWithoutInvoke(double value)
{
	fInvoke = false;
	BDecimalSpinner::SetValue(value);
}


void
DecSpinner::SetWithoutInvoke(int32 value)
{
	fInvoke = false;
	BDecimalSpinner::SetValue(value);
}


void
DecSpinner::SetFromTextWithoutInvoke()
{
	fInvoke = false;
	BDecimalSpinner::SetValueFromText();
}


void
DecSpinner::Increment()
{
	double value = Value();
	// show 'special' framerates as 24/1.001, 30/1.001, 60/1.001
	if (value >= 23 && value < 23.976) {
		SetPrecision(3);
		SetValue(23.976);
	} else if (value >= 29.0 && value < 29.97) {
		SetPrecision(2);
		SetValue(29.97);
	} else if (value >= 59.0 && value < 59.97) {
		SetPrecision(2);
		SetValue(59.97);
	} else {
		SetPrecision(0);
		SetValue(truncf(Value() + 1.0));
	}
}


void
DecSpinner::Decrement()
{
	double value = Value();
	// show 'special' framerates as 24/1.001, 30/1.001, 60/1.001
	if (value <= 24 && value > 23.976) {
		SetPrecision(3);
		SetValue(23.976);
	} else if (value <= 30.0 && value > 29.97) {
		SetPrecision(2);
		SetValue(29.97);
	} else if (value <= 59.0 && value > 59.97) {
		SetPrecision(2);
		SetValue(59.97);
	} else {
		SetPrecision(0);
		SetValue(ceilf(Value()) - 1.0);
	}
}
