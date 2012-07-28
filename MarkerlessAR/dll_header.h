#ifdef	_WIN32
	//#include <cv.h>
	//#include <cxcore.h>
	//#include <highgui.h>
#include <opencv2/opencv.hpp>
#else
	#include <OpenCV/OpenCV.h>
#endif

#define	PRECISION_64

#if defined(PRECISION_32)
	typedef	CvPoint2D32f	CvPoint2D;
	typedef	CvPoint3D32f	CvPoint3D;
	typedef	float			ARTKFloat;
#define		CV_FLOAT_SCALAR	CV_32FC1
#define		cvPoint3D		cvPoint3D32f
#elif defined(PRECISION_64)
	typedef	CvPoint2D64f	CvPoint2D;
	typedef	CvPoint3D64f	CvPoint3D;
	typedef	double			ARTKFloat;
#define		CV_FLOAT_SCALAR	CV_64FC1
#define		cvPoint3D		cvPoint3D64f
#else
	typedef	CvPoint			CvPoint2D;
	typedef	CvPoint3D64d	CvPoint3D;
	typedef	double			ARTKFloat;
#endif