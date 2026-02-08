#pragma	once

#include "SungwookAR.hpp"


/*Function///////////////////////////////////////////////////////////////
/*Function///////////////////////////////////////////////////////////////

Name:       makeDescriptors			for query

Purpose:    주어진 이미지에서 keypoints를 찾고 각 keypoints들의 descriptor를 만듦

Parameters: Mat		&img_input:					keypoints를 찾고 descriptor를 생성할 이미지 
			vector<KeyPoint>	&keypoints:		검출된 keypoints들의 목록
			Mat		&descriptors:				검출된 keypoints들에 대응되는 descriptor들
							
Language:   C++

///////////////////////////////////////////////////////////////////////*/
void	makeDescriptors(const Mat &img_input, vector<KeyPoint> &keypoints, Mat &descriptors);

/*Function///////////////////////////////////////////////////////////////

Name:       makeDescriptors			for DB

Purpose:    주어진 이미지에서 keypoints를 찾고 각 keypoints들의 descriptor를 만듦

Parameters: Mat		&img_input:					keypoints를 찾고 descriptor를 생성할 이미지 
			vector<KeyPoint>	&keypoints:		검출된 keypoints들의 목록
			Mat		&descriptors:				검출된 keypoints들에 대응되는 descriptor들
							
Language:   C++

///////////////////////////////////////////////////////////////////////*/
void	makeDescriptors(const	vector<Mat>	&img_input, vector<vector<KeyPoint>> &keypoints, 
	vector<Mat> &descriptors);	

//Copy (x,y) location of descriptor matches found from KeyPoint data structures into Point2f vectors
#ifdef BRISK_FeatureDescriptor
void matches2points(const vector<vector<DMatch> >& matches, const vector<KeyPoint>& kpts_train,
                    const vector<KeyPoint>& kpts_query, vector<Point2f>& pts_train,
                    vector<Point2f>& pts_query);

#elif(BRIEF_FeatureDescriptor)
void matches2points(const vector<DMatch>& matches, const int imageIdx, const vector<vector<KeyPoint>>& kpts_train, 
	const vector<KeyPoint>& kpts_query, vector<Point2f>&	pts_train, vector<Point2f>&	pts_query);
#endif

// Returns whether H is a nice homography matrix or not
bool niceHomography(const CvMat * H);

Mat swMoveImage(Mat originalImage, Mat *tranformedImage, double idxScale);
void swMoveCorners(Mat originalImage, 	vector<Point2f>	src, vector<Point2f>	&dst,
	unsigned int	idxRotation,	unsigned int	idxScale);

void mslTransformKeypoint(	vector<KeyPoint>	src, vector<KeyPoint>	&dst,
	unsigned int	idxRotation,	unsigned int	idxScale) ;


void mslRotateKeypoint(vector<KeyPoint>	src, vector<KeyPoint>	&dst,
	unsigned int	idxRotation, unsigned int idxScale);

void mslOriginKeypoint(vector<KeyPoint>	src, vector<KeyPoint>	&dst,
	unsigned int	idxRotation, unsigned int idxScale);

//void mslTransformImage(Mat originalImage, Mat *tranformedImage, double rotation, double scale){
Mat mslTransformImage(Mat originalImage, Mat *tranformedImage, int idxRotation, int idxScale);

//실제 사용하지 않는 함수다
void mslTransformPoint(	const vector<Point>	src, vector<Point2f>	&dst,
	 int	idxRotation,	 int	idxScale) ;