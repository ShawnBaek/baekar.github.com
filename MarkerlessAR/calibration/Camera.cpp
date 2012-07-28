/** 
\file Camera.cpp
\brief 카메라의 입력 설정
*/

#include "Camera.h"
#include <stdio.h>

#ifdef	_WIN32
//#include <cv.h>
//#include <cxcore.h>
//#include <highgui.h>
#include <opencv2/opencv.hpp>
using namespace cv;
#else
#include <OpenCV/OpenCV.h>
#endif

CCamera::CCamera(void)
{
	size = cvSize(640, 480);	// 우선 사이즈 초기화 나중에 바꿔도 됨
}

CCamera::CCamera(bool	turnOn) {
	size = cvSize(640, 480);	// 우선 사이즈 초기화 나중에 바꿔도 됨
}


CCamera::~CCamera(void)
{
}

#define	removeComments()			do {fgets(line, 100, fp);} while( !strcmp(line, "\n") );

bool CCamera::load(const char* fileName)
{
	
	//====================================================================================================
	//===		수동 초기화

	// Distortion Factor read
	this->distortionFactor[0] = 318.5;
	this->distortionFactor[1] = 263.5;
	this->distortionFactor[2] = 26.2;
	this->distortionFactor[3] = 1.0127565206658486;

	// Projection Matrix read
	this->projectionMatrix[0][0] = 700.95147029922452;
	this->projectionMatrix[0][1] = 0.0;
	this->projectionMatrix[0][2] = 316.50;
	this->projectionMatrix[0][3] = 0.0;

	this->projectionMatrix[1][0] = 0.0;
	this->projectionMatrix[1][1] = 726.09418165353668;
	this->projectionMatrix[1][2] = 241.50;
	this->projectionMatrix[1][3] = 0.0;

	this->projectionMatrix[2][0] = 0.0;
	this->projectionMatrix[2][1] = 0.0;
	this->projectionMatrix[2][2] = 1.0;
	this->projectionMatrix[2][3] = 0.0;

	// Size read
	this->size.width = 640;
	this->size.height = 480;

	//===		수동 초기화 끝
	//====================================================================================================

	loaded = true;

	return true;
}

#define PD_LOOP 3

void CCamera::observ2Ideal(CvPoint	observed, CvPoint2D *ideal)
{
	ARTKFloat  z02, z0, p, q, z, px, py;
	register int     i;

	px = observed.x - this->distortionFactor[0];
	py = observed.y - this->distortionFactor[1];
	p = this->distortionFactor[2]/(ARTKFloat)100000000.0;
	z02 = px*px+ py*py;
	q = z0 = (ARTKFloat)sqrt(px*px+ py*py);

	for( i = 1; ; i++ ) {
		if( z0 != 0.0 ) {
			z = z0 - (((ARTKFloat)1.0 - p*z02)*z0 - q) / ((ARTKFloat)1.0 - (ARTKFloat)3.0*p*z02);
			px = px * z / z0;
			py = py * z / z0;
		}
		else {
			px = 0.0;
			py = 0.0;
			break;
		}
		if( i == PD_LOOP ) break;

		z02 = px*px+ py*py;
		z0 = (ARTKFloat)sqrt(px*px+ py*py);
	}

	ideal->x = px / this->distortionFactor[3] + this->distortionFactor[0];
	ideal->y = py / this->distortionFactor[3] + this->distortionFactor[1];
}

void CCamera::observ2Ideal(int ox, int oy, ARTKFloat *ix, ARTKFloat *iy)
{
	ARTKFloat  z02, z0, p, q, z, px, py;
	register int     i;

	px = ox - this->distortionFactor[0];
	py = oy - this->distortionFactor[1];
	p = this->distortionFactor[2]/(ARTKFloat)100000000.0;
	z02 = px*px+ py*py;
	q = z0 = (ARTKFloat)sqrt(px*px+ py*py);

	for( i = 1; ; i++ ) {
		if( z0 != 0.0 ) {
			z = z0 - (((ARTKFloat)1.0 - p*z02)*z0 - q) / ((ARTKFloat)1.0 - (ARTKFloat)3.0*p*z02);
			px = px * z / z0;
			py = py * z / z0;
		}
		else {
			px = 0.0;
			py = 0.0;
			break;
		}
		if( i == PD_LOOP ) break;

		z02 = px*px+ py*py;
		z0 = (ARTKFloat)sqrt(px*px+ py*py);
	}

	*ix = px / this->distortionFactor[3] + this->distortionFactor[0];
	*iy = py / this->distortionFactor[3] + this->distortionFactor[1];
}

void CCamera::ideal2Observ(CvPoint2D ideal, CvPoint *observed)
{
	ARTKFloat    x, y, d;

	x = (ideal.x - this->distortionFactor[0]) * this->distortionFactor[3];
	y = (ideal.y - this->distortionFactor[1]) * this->distortionFactor[3];
	if( x == 0.0 && y == 0.0 ) {
		observed->x = this->distortionFactor[0];
		observed->y = this->distortionFactor[1];
	}
	else {
		d = (ARTKFloat)1.0 - this->distortionFactor[2]/(ARTKFloat)100000000.0 * (x*x+y*y);
		observed->x = x * d + this->distortionFactor[0];
		observed->y = y * d + this->distortionFactor[1];
	}
}

void CCamera::ideal2Observ(CvPoint2D ideal, CvPoint2D *observed)
{
	ARTKFloat    x, y, d;

	x = (ideal.x - this->distortionFactor[0]) * this->distortionFactor[3];
	y = (ideal.y - this->distortionFactor[1]) * this->distortionFactor[3];
	if( x == 0.0 && y == 0.0 ) {
		observed->x = this->distortionFactor[0];
		observed->y = this->distortionFactor[1];
	}
	else {
		d = (ARTKFloat)1.0 - this->distortionFactor[2]/(ARTKFloat)100000000.0 * (x*x+y*y);
		observed->x = x * d + this->distortionFactor[0];
		observed->y = y * d + this->distortionFactor[1];
	}
}
