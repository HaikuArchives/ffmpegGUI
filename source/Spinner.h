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

	// Allow not doing an Invoke() on SetValue() until Haiku's BSpinner class is fixed
	virtual status_t	Invoke(BMessage* message = NULL);

	void 	SetWithoutInvoke(int32 value);
	void 	Increment();
	void 	Decrement();
	void 	SetStep(int32 step);

private:
	bool	fInvoke;
	int32 	fStep;
};


class DecSpinner : public BDecimalSpinner {
public:
						DecSpinner(const char* name, const char* label, BMessage* message);

	// Allow not doing an Invoke() on SetValue() until Haiku's BSpinner class is fixed
	virtual status_t	Invoke(BMessage* message = NULL);

	void 	SetWithoutInvoke(int32 value);
	void 	SetWithoutInvoke(double value);
	void 	Increment();
	void 	Decrement();

private:
	bool	fInvoke;
};

#endif // SPINNER_H
