#pragma	once

#include <opencv2/opencv.hpp>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <stdio.h>
#include <stdarg.h>

using namespace cv;

void fpsCalculation(void);
void cvShowManyImages(char* title, int nArgs, ...);
void cvShowImg(const char *name, IplImage	*image, int wait, int width, int height);
void showImage(const char *name, Mat	&image, int wait, int width, int height);
void showManyImages(char* title, int nArgs, ...);