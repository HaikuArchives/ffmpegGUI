/*
 * Copyright 2022-2023. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2023
*/


#ifndef CROPVIEW_H
#define CROPVIEW_H

#include <View.h>
#include <StringList.h>

class BBitmap;
class BString;


class CropView : public BView {
public:
	CropView();
	void SetFilenames(const BStringList& filenames);
	status_t SetCurrentImage(int32 index);
	void Draw(BRect updateRect);
	void LayoutChanged();
	void SetLeftCrop(int32 leftcrop);
	void SetRightCrop(int32 rightcrop);
	void SetTopCrop(int32 topcrop);
	void SetBottomCrop(int32 bottomcrop);
	void SetEnabled(bool enabled);


private:
	void _SetDrawingRect();
	void _SetMarkerRect();
	status_t _LoadImage(const BString& filename);

	BBitmap*	fCurrentImage;
	BStringList	fImageFilenames;
	int32		fImageIndex;
	bool		fImageLoaded;
	BSize		fImageSize;
	float		fResizeFactor;
	int32 		fLeftCrop;
	int32 		fRightCrop;
	int32 		fTopCrop;
	int32 		fBottomCrop;
	bool 		fEnabled;
	BRect 		fDrawingRect;
	BRect 		fMarkerRect;

};

#endif
