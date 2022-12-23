#ifndef FFGUI_SPINNER_H
#define FFGUI_SPINNER_H

#include "Spinner.h"

class ffguispinner : public BSpinner {
public:
	ffguispinner(const char* name, const char* label, BMessage* message);
	void Increment();
	void Decrement();
	void SetStep(int32 step);

private:
	int32 fStep;

};

#endif
