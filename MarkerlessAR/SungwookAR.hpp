#pragma	once

#include <iostream>
#include <string>
#include <time.h>
#include <vector>
#include <atlstr.h>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/gpu/gpu.hpp>


using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ostream;

using namespace cv;
using namespace std;
using namespace cv::gpu;

const static unsigned int IMAGE_WIDTH = 640;
const static unsigned int IMAGE_HEIGHT = 480;

const double NUM_OF_ROTATION = 18.0;
const unsigned int	NUM_OF_SCALE = 3;

static double	scaleFactor[]	= {0.5, 1.0, 2.0};


static	Size	sz_origin;  

const	Size2i	query_image_size(640, 480);
const	Size2i	train_image_size(300, 216);


struct Quadrangle
{
	cv::Point_<float>& operator[](size_t i) { return data[i]; }
	cv::Point_<float> operator[](size_t i) const { return data[i]; }
	cv::Point_<float> data[4];
	size_t	size() { return 4; }
};

struct strFilename
{
    string _strFilename;
};




struct strTracking
{
	int idxcounter;
	Quadrangle corner;
	vector<Point2f> keypoint;
	vector<Point2f> keypoint_train;
	Mat matQuery;

};

struct strTracking_
{
	int idxcounter;
	bool init;

};
/*
typedef struct 
{
	int					ID;				// Detector::readID()에서 계산							/  Detector::doDetecting()에서 저장
	int					dir;				// Detector::readID()에서 계산							/  Detector::doDetecting()에서 저장
	bool				cf;				// Detector::readID()에서 계산							/  Detector::doDetecting()에서 저장

	CvPoint		vertex[4];	// Detector::MSL_arGetLine2()에서 계산		/  Detector::MSL_arGetLine2()에서 저장
	CvPoint		center;		// Detector::GetLocationInfo()에서 계산		/  Detector::GetLocationInfo()에서 저장
	double				area;			// Detector::doDetecting()에서 계산				/  Detector::doDetecting()에서 저장
	double				line[4][3];	// Detector::MSL_arGetLine2()에서 계산		/  Detector::MSL_arGetLine2()에서 저장
	double				Tcm[3][4];	// 
} Results;
*/
//const	char			XML_FILE_NAME[] = "database_restricted.yml";

// FeatureIndex : e_sh_r0 <- Enum_ScaleHalf_Rotation0, e_sn_r0 <- Enum_ScaleNormal_Rotation0, e_sd_r0 <- Enum_ScaleDouble_Rotation0
enum FeatureIndex{e_begin=0,e_sh_r0=0, e_sh_r20, e_sh_r40, e_sh_r60, e_sh_r80, e_sh_r100, e_sh_r120, e_sh_r140, e_sh_r160, e_sh_r180, e_sh_r200, e_sh_r220, e_sh_r240, e_sh_r260, e_sh_r280, e_sh_r300, e_sh_r320, e_sh_r340, e_sh_r360,  
	e_sn_r0, e_sn_r20, e_sn_r40, e_sn_r60, e_sn_r80, e_sn_r100, e_sn_r120, e_sn_r140, e_sn_r160, e_sn_r180, e_sn_r200, e_sn_r220, e_sn_r240, e_sn_r260, e_sn_r280, e_sn_r300, e_sn_r320, e_sn_r340, e_sn_r360, 
	e_sd_r0, e_sd_r20, e_sd_r40, e_sd_r60, e_sd_r80, e_sd_r100, e_sd_r120, e_sd_r140, e_sd_r160, e_sd_r180, e_sd_r200, e_sd_r220, e_sd_r240, e_sd_r260, e_sd_r280, e_sd_r300, e_sd_r320, e_sd_r340, e_sd_r360
	,e_end 	
};

/*
Feature Detector Algorithms
*/

//#define	FAST_FeatureDetector
#define AGAST_FeatureDetector
//#define	SIFT_FeatureDetector
//#define	SURF_FeatureDetector
//#define	HARRIS_FeatureDetector
//#define	GFTT_FeatureDetector
//#define	MSER_FeatureDetector
//#define	STAR_FeatureDetector

//#define	SIFT_FeatureDescriptor
//#define	SURF_FeatureDescriptor
//#define	BRIEF_FeatureDescriptor
#define BRISK_FeatureDescriptor
//#define	OpponentSURF_FeatureDescriptor


// threshold value for matches.
static const int max_match_distance = 32; 

//#define Calculate_Time
#ifndef	MSL_Image_Set_Matching
#define	MSL_Image_Set_Matching

#define	PI	3.141591
// Minimum  scale ratio of  the program.  It indicates  that templates
// having scales  [0.5, 1]  of the original  template will be  used to
// generate new templates. Scales are determined in a logarithm base.

static const float SMALLEST_SCALE_CHANGE = 0.2; //<-- 0.5 * 0.2 = 1.0 이니깐... 그 차이값을 의미하는 듯

// Number of different scales used to generate the templates.
//영향을 주는 부분
static const int NUMBER_OF_SCALE_STEPS = 3;
//static const int NUMBER_OF_SCALE_STEPS = 3;

// Number  of   different  rotation   angles  used  to   generate  the
// templates. 18 indicates that  [0 20 ... 340] degree-rotated samples
// will be stored in the 2-D array for each scale.
static const int NUMBER_OF_ROTATION_STEPS = 20;
//static const int NUMBER_OF_ROTATION_STEPS = 1;

//[Image Set DB] 에서 DB 의 크기
static const int NUMBER_OF_IMAGE_SET = NUMBER_OF_SCALE_STEPS * NUMBER_OF_ROTATION_STEPS;

#include <math.h>
#include <string>

static const float ROT_ANGLE_INCREMENT = 360.0 / NUMBER_OF_ROTATION_STEPS;
static const float k = exp(log(SMALLEST_SCALE_CHANGE) / (NUMBER_OF_SCALE_STEPS - 1));
 
//[Multi Tracking] 에서 각기 다른 DB 영상의 종류
static const int NUMBER_OF_TRAIN_IMAGES = 2;


#endif
unsigned __stdcall ThreadBRISKMatching(void *param);
unsigned __stdcall ThreadMatching(void *param);
unsigned __stdcall ThreadTracking(void *param);

unsigned __stdcall ThreadDraw(void *param);

double *getModelViewMatrix(int index);
double *getProjectionMatrix();
void doTrack(void	*imgPtr);