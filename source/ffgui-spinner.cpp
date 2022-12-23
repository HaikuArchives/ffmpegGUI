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
