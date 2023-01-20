#include "ffgui-spinner.h"

ffguispinner::ffguispinner(const char* name, const char* label, BMessage* message)
	:
	BSpinner(name, label, message)
{

	fStep = 1;

}


void
ffguispinner::Increment()
{

	SetValue(Value() + fStep);

}


void
ffguispinner::Decrement()
{

	SetValue(Value() - fStep);

}


void
ffguispinner::SetStep(int32 step)
{

	fStep = step;

}


// Spinner for floating point

ffguidecspinner::ffguidecspinner(const char* name, const char* label, BMessage* message)
	:
	BDecimalSpinner(name, label, message)
{
	SetPrecision(0);
}


void
ffguidecspinner::Increment()
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
ffguidecspinner::Decrement()
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
