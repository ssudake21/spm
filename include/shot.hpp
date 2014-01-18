#ifndef __SHOT_H_INCLUDED__
#define __SHOT_H_INCLUDED__
#include<opencv2/imgproc/imgproc.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/core/core.hpp>
#include<iostream>
#include <cmath>
#include "detectshot.hpp"
#include "shotArray.hpp"

using namespace std;
using namespace cv;

extern void onMouseClick(int event, int x, int y, int flags, void* userdata);


class Shot
{
	private:
	Point startpoint, p1;
	double angleError;
	public:
	Shot();
	~Shot();
	void clear();
	void setShotStartP(Point position);
	void getUserInput(Mat &frame);
	void drawSuggested(Mat &frame);
	double angleErr(Mat &frame, ShotArray *shot_array);
	void velocityError();
	void rateShot();
	int shotType();
        double getSuggDist();
};

#endif
