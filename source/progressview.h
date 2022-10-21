#ifndef PROGRESSVIEW_H
#define PROGRESSVIEW_H

#include <View.h>


class ProgressView : public BView {
public:
	ProgressView();
	void Draw(BRect update_rect);
	void SetProgress(int32 progress_percentage);


private:
	int32 fProgressPercentage;

};

#endif