#include "../global.h"
#include "Capture.h"
#include "FingertipPoseEstimation.h"
#include "../3dobjects/bunny/inc/bunny.h"

IplImage * image;
IplImage * gray;
IplImage * handRegion;

CvFont font;

Capture             gCapture;
FingertipPoseEstimation gFingertipPoseEstimation;

//HandRegion          gHandRegion;
//FingerTip           gFingerTip;
//FingertipTracker    gFingertipTracker;
//HandTracker         gHandTracker;
//PoseEstimation      gPoseEstimation;

bool fHandRegion = false;
bool fDistTrans = false;
bool fFingertipDetected = false;

IplImage * cameraCenter;

bool fRecord = false;
bool fScreenshot = true;

CvVideoWriter * gpRecord = 0;
CvVideoWriter * gpRecordOutput = 0;
#define RECORD_FILENAME         "record.avi"
#define RECORD_FILENAME_OUTPUT  "output.avi"

bool fInputVideoFile = false;
char gszInputVideoFilename[255];
//bool fFlipFrame = CAPTURE_DONT_FLIP;
//CAPTURE_FLIP
bool fFlipFrame = CAPTURE_FLIP;

// Earth Texture
GLuint  gnEarthTexID;
#define EARTH_TEXTURE_FILENAME  "../3dobjects/earthTexture.jpg"

// Render Model
#define MODEL_BUNNY 1
#define MODEL_EARTH 2
#define MODEL_ARTAG 3
#define MODEL_COORDINATE_AXES   4
#ifdef ARTAG
int gnModel = MODEL_ARTAG;
#else
int gnModel = MODEL_COORDINATE_AXES;
#endif

#ifdef PROCESS_320x240
#define FINGERTIP_COORDINATE_FILENAME   "../calibration/fingertip_320x240.dat"
#else
#define FINGERTIP_COORDINATE_FILENAME   "../calibration/fingertip_640x480.dat"
#endif
char gszFingertipFilename[255] = FINGERTIP_COORDINATE_FILENAME;

