/*
 * Copyright 2022-2023. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2023
*/


#ifndef CROPVIEW_H
#define CROPVIEW_H

#include <View.h>


class BBitmap;
class BFile;
class BString;


class CropView : public BView {
public:
	CropView();
	status_t LoadImage(const BString& filename);
	void Draw(BRect updateRect);
	void LayoutChanged();
	void SetLeftCrop(int32 leftcrop);
	void SetRightCrop(int32 rightcrop);
	void SetTopCrop(int32 topcrop);
	void SetBottomCrop(int32 bottomcrop);

private:
	void _SetDrawingRect();
	void _SetMarkerRect();

	BBitmap *fBitmap;
	bool	fImageLoaded;
	BSize	fImageSize;
	float	fResizeFactor;
	int32 	fLeftCrop;
	int32 	fRightCrop;
	int32 	fTopCrop;
	int32 	fBottomCrop;
	BRect fDrawingRect;
	BRect fMarkerRect;

};

#endif
