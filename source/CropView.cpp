/*
 * Copyright 2022-2023. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2023
*/


#include "CropView.h"

#include <Bitmap.h>
#include <String.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>


CropView::CropView()
	:
	BView("", B_SUPPORTS_LAYOUT | B_WILL_DRAW)
{
	fImageLoaded = false;
	fTopCrop = 0;
	fBottomCrop = 0;
	fLeftCrop = 0;
	fRightCrop = 0;
	fEnabled = false;
}


void
CropView::Draw(BRect updateRect)
{
	if (fImageLoaded)
	{
		SetDrawingMode(B_OP_COPY);
		DrawBitmap(fCurrentImage, fCurrentImage->Bounds(), fDrawingRect);

		// only draw crop marker when at least on cropping value is set
		if ((fTopCrop + fBottomCrop + fLeftCrop + fRightCrop) > 0) {
			SetHighColor(0,0,0);
			SetLowColor(255,255,255);
			StrokeRect(fMarkerRect, B_MIXED_COLORS);
		}

		// grey out the view if it is not enabled
		if (!fEnabled) {
			SetLowColor(0,0,0);
			SetDrawingMode(B_OP_BLEND);
			FillRect(fDrawingRect, B_SOLID_LOW);
		}
	}
}


void
CropView::LayoutChanged()
{
	if (fImageLoaded) {
		_SetDrawingRect();
		_SetMarkerRect();
	}
}


status_t
CropView::LoadImage(const char* path)
{
	fCurrentImage = BTranslationUtils::GetBitmap(path);

	if (fCurrentImage != nullptr) {
		if (fCurrentImage->IsValid()) {
			fImageLoaded = true;
			fImageSize = fCurrentImage->Bounds().Size();

			_SetDrawingRect();
			_SetMarkerRect();
			Invalidate();
			return B_OK;
		}
	}

	return B_ERROR;
}


void
CropView::SetLeftCrop(int32 leftcrop)
{
	fLeftCrop = leftcrop;
	_SetMarkerRect();
}


void
CropView::SetRightCrop(int32 rightcrop)
{
	fRightCrop = rightcrop;
	_SetMarkerRect();
}


void
CropView::SetTopCrop(int32 topcrop)
{
	fTopCrop = topcrop;
	_SetMarkerRect();
}


void
CropView::SetBottomCrop(int32 bottomcrop)
{
	fBottomCrop = bottomcrop;
	_SetMarkerRect();
}


void
CropView::SetEnabled(bool enabled)
{
	fEnabled = enabled;
	Invalidate();
}

void
CropView::_SetDrawingRect()
{
	fDrawingRect = Bounds();
	fResizeFactor = fDrawingRect.Height() / fImageSize.Height();
	float drawing_width = fImageSize.Width() * fResizeFactor;
	float resize_delta;

	if (drawing_width > fDrawingRect.Width()) {
		drawing_width = fDrawingRect.Width();
		fResizeFactor = drawing_width / fImageSize.Width();
		float drawing_height = fImageSize.Height() * fResizeFactor;
		resize_delta = (fDrawingRect.Height() - drawing_height) / 2;
		fDrawingRect.top += resize_delta;
		fDrawingRect.bottom -= resize_delta;
	} else {
		resize_delta = (fDrawingRect.Width() - drawing_width) / 2;
		fDrawingRect.left += resize_delta;
		fDrawingRect.right -= resize_delta;
	}
}


void
CropView::_SetMarkerRect()
{
	if (fImageLoaded) {
		fMarkerRect = fDrawingRect;
		fMarkerRect.top += fTopCrop * fResizeFactor;
		fMarkerRect.bottom -= fBottomCrop * fResizeFactor;
		fMarkerRect.left += fLeftCrop * fResizeFactor;
		fMarkerRect.right -= fRightCrop * fResizeFactor;
	}

	Invalidate();
}
