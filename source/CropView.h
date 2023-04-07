/*
 * Copyright 2022-2023. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2023
*/


#ifndef CROPVIEW_H
#define CROPVIEW_H

#include <StringList.h>
#include <View.h>

class BBitmap;
class BString;


class CropView : public BView {
public:
				CropView();

	void		Draw(BRect updateRect);
	void 		LayoutChanged();

	status_t	LoadImage(const char* path);

	void		SetLeftCrop(int32 leftcrop);
	void		SetRightCrop(int32 rightcrop);
	void		SetTopCrop(int32 topcrop);
	void		SetBottomCrop(int32 bottomcrop);
	void		SetEnabled(bool enabled);


private:
	void		_SetDrawingRect();
	void 		_SetMarkerRect();

	BBitmap*	fCurrentImage;
	bool		fImageLoaded;
	BSize		fImageSize;
	BRect 		fDrawingRect;
	BRect 		fMarkerRect;
	float		fResizeFactor;

	int32 		fLeftCrop;
	int32 		fRightCrop;
	int32 		fTopCrop;
	int32 		fBottomCrop;
	bool 		fEnabled;
};

#endif
