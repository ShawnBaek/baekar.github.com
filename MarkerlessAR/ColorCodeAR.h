#pragma once

//#include <stdbool.h>
//#include "Decoder/czdecoder.h"
//#include "Decoder/bbd.h"
#include "PoseEstimator/KatoPoseEstimator.h"
#include "PoseEstimator/Camera.h"

//==========================================================================================
//======			ColorCode ½ÇÇà 
bool mslGetMarkerLocation(	Point2D	points[4], Results	*result);



class ColorCodeAR
{
private:
	const	static	int             xsize=640, ysize=480;

	czLPDecoderArea	colorCode;
	//lpCZCODE	result;
	CZCODE	result;

	double	mModelView[16];
	double	mProjection[16];

public:
	ColorCodeAR(void);
	~ColorCodeAR(void);

	void init();
	void final();
	void doTrack(void	*imgPtr);
	double *getModelViewMatrix();
	double *getProjectionMatrix();

	CCamera	camera;
	CKatoPoseEstimator	poseEstimator;
	Results	r;
	unsigned long	ID;

};
