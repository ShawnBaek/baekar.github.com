#pragma once
#define _STDINT
#include <windows.h>
#include <process.h>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <opencv2/gpu/gpu.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <iostream>
#include <list>

#include "stdint.h"
#include "brisk/brisk.h"
//#include "projection.h"
#include "brisk/Matcher.h"

#include "brisk/MatchVerifier.hpp"
#include "brisk/GroundTruth.hpp"

#include "HandyAR/HandyAR.h"

#include <gl/glut.h>
#include "SungwookUtility.hpp"
#include "SungwookFeature.hpp"
#include "SungwookAR.hpp"
#include "calibration/Camera.h"
//#include "KatoPoseEstimation/KatoPoseEstimator.h"
#include "wonjo.h"
#define	Image_Set_Matching

class GLOBAL
{
public:
	//OpenGL 관련된 변수
	static bool OpenGLinit;
	static CKatoPoseEstimator poseEstimator;
	static CvSize	size;
	static double	projectionMatrix[3][4];
	static double	distortionFactor[4];
	static IplImage img_binary;
	static CvCapture		*capture_C;
	static char	parameterFile[100];
	static CCamera	camera;
	static bool	loaded;

	static Results threadResults1;
	static Results threadResults2;


	static double threadmModelView1[16];
	static double threadmModelView2[16];


	static double	mModelView[16];
	static double	mProjection[16];


	// 영상 입력 받을 Capture형 선언
	static VideoCapture	capture;
	static VideoCapture	capture1;
	static VideoCapture	capture2;

	static char ch;
	static const int MAX_CORNERS;

	//CvFont	font;
	static vector<KeyPoint>	kpQuery;
	static Mat					queryDescriptors;

	static Ptr<FeatureDetector> detector;
	static Ptr<DescriptorExtractor> descriptorExtractor;


	//============================================================================//
	static vector<Point2f> g_mpts_1;
	static vector<Point2f> g_mpts_2;	
	//============================================================================//
	static Mat							img_rgbdatabase1;
	static Mat							img_graydatabase1;
	static Mat							img_centerdatabase1;

	static vector<KeyPoint>			kp_database1;
	static Mat							desc_database1;
	static vector<Point2f>				obj_corners1;
	static vector<Point2f>				obj_center_corners1;

	static vector<Point2f>				dst_matching_corners1;
	static vector<Point2f>				dst_tracking_corners1;



	static Mat							img_rgbdatabase2;
	static Mat							img_graydatabase2;
	static Mat							img_centerdatabase2;

	static vector<KeyPoint>			kp_database2;
	static Mat							desc_database2;
	static vector<Point2f>				obj_corners2;
	static vector<Point2f>				obj_center_corners2;

	static vector<Point2f>				dst_matching_corners2;
	static vector<Point2f>				dst_tracking_corners2;


	static Ptr<DescriptorMatcher> descriptorMatcher1;
	static Ptr<DescriptorMatcher> descriptorMatcher2;


	static Mat mInput, mQuery, mResult, gDetectionResult1, mResult2;
	static bool tracking1, tracking2;
	static bool bIsTracking[12];
	static bool bIsTrackingInit[12];
	static bool g_bisRunTrackingThread;

	static bool bThreadDetection1;
	static bool bThreadDetection2;

	static bool bInitTracking;
	static bool bThreadTracking1;
	static bool bThreadTracking2;

	// Thread 처리를 위한 HANDLE
	static HANDLE hMatchingThread[2];
	static HANDLE hTrackingThread[2];
	static HANDLE hDrawThread;
	static HANDLE hMutex;

	static unsigned uMatchingThreadID[2];
	static unsigned uTrackingThreadID[2];
	static unsigned uDrawThreadID;


	static bool computeHomography;
	static bool hamming;


	static CRITICAL_SECTION   hCriticalSection[2];
	static ofstream mslNFT_Log;

	static strFilename Filename[20];



	static void display();
	static void tick_func();
	static void init();
	static void mainLoop();
	static void doTrack(void *imgPtr);
	static LPCSTR lpszClass;
	static int InitializeEngineMain();
	static int ReleaseEngineMain();
	static unsigned __stdcall ThreadBRISKMatching(void *param);
	static unsigned __stdcall ThreadTracking(void *param);
	static unsigned __stdcall ThreadDraw(void *param);
};

