/*
 * Copyright 2022-2023. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2022
 * Humdinger, humdingerb@gmail.com, 2022-2023
*/


#ifndef SPINNER_H
#define SPINNER_H

#include "DecimalSpinner.h"
#include "Spinner.h"


class Spinner : public BSpinner {
public:
			Spinner(const char* name, const char* label, BMessage* message);
	void 	Increment();
	void 	Decrement();
	void 	SetStep(int32 step);

private:
	int32 	fStep;
};


class DecSpinner : public BDecimalSpinner {
public:
			DecSpinner(const char* name, const char* label, BMessage* message);
	void 	Increment();
	void 	Decrement();
};

#endif // SPINNER_H
