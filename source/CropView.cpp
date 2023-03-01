/*
 * Copyright 2022-2023. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Andi Machovec (BlueSky), andi.machovec@gmail.com, 2023
*/


#include "CropView.h"

#include <File.h>
#include <Bitmap.h>
#include <TranslationUtils.h>
#include <TranslatorFormats.h>


CropView::CropView()
	:
	BView("", B_SUPPORTS_LAYOUT|B_WILL_DRAW)
{
	fImageLoaded = false;
	fTopCrop = 0;
	fBottomCrop = 0;
	fLeftCrop = 0;
	fRightCrop = 0;
}


status_t
CropView::LoadImage(const BString& filename)
{
	fBitmap = BTranslationUtils::GetBitmap(filename.String());

	if (fBitmap != nullptr)
	{
		if (fBitmap->IsValid())
		{
			fImageLoaded = true;
			fImageSize = fBitmap->Bounds().Size();

			_SetDrawingRect();
			_SetMarkerRect();

			return B_OK;
		}
	}

	return B_ERROR;
}


void
CropView::Draw(BRect updateRect)
{
	if (fImageLoaded)
	{
		DrawBitmap(fBitmap, fBitmap->Bounds(), fDrawingRect);

		if ((fTopCrop+fBottomCrop+fLeftCrop+fRightCrop) > 0) // only draw crop marker when
		{													 // at least on cropping value is set
			SetHighColor(255,0,0);
			StrokeRect(fCropMarkerRect, B_SOLID_HIGH);
		}

		Invalidate();
	}
}


void
CropView::LayoutChanged()
{
	if (fImageLoaded)
	{
		_SetDrawingRect();
		_SetMarkerRect();
	}
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
CropView::_SetDrawingRect()
{
	fDrawingRect = Bounds();
	fResizeFactor = fDrawingRect.Height() / fImageSize.Height();
	float drawing_width = fImageSize.Width() * fResizeFactor;
	float resize_delta;
	if (drawing_width > fDrawingRect.Width())
	{
		drawing_width = fDrawingRect.Width();
		fResizeFactor = drawing_width / fImageSize.Width();
		float drawing_height = fImageSize.Height() * fResizeFactor;
		resize_delta = (fDrawingRect.Height() - drawing_height) / 2;
		fDrawingRect.top += resize_delta;
		fDrawingRect.bottom -= resize_delta;
	}
	else
	{
		resize_delta = (fDrawingRect.Width() - drawing_width) / 2;
		fDrawingRect.left += resize_delta;
		fDrawingRect.right -= resize_delta;
	}
}


void
CropView::_SetMarkerRect()
{
	if (fImageLoaded)
	{
		fCropMarkerRect = fDrawingRect;
		fCropMarkerRect.top += fTopCrop * fResizeFactor;
		fCropMarkerRect.bottom -= fBottomCrop * fResizeFactor;
		fCropMarkerRect.left += fLeftCrop * fResizeFactor;
		fCropMarkerRect.right -= fRightCrop * fResizeFactor;
	}
}
