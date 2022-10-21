#include "progressview.h"


ProgressView::ProgressView()
	:
	BView("", B_WILL_DRAW|B_SUPPORTS_LAYOUT)
{
	fProgressPercentage = 0;

}


void
ProgressView::Draw(BRect update_rect)
{

	if(fProgressPercentage > 0)
	{



	}

}


void
ProgressView::SetProgress(int32 progress_percentage)
{


}