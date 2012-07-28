#pragma	once

#include "SungwookAR.hpp"


/*Function///////////////////////////////////////////////////////////////
/*Function///////////////////////////////////////////////////////////////

Name:       makeDescriptors			for query

Purpose:    �־��� �̹������� keypoints�� ã�� �� keypoints���� descriptor�� ����

Parameters: Mat		&img_input:					keypoints�� ã�� descriptor�� ������ �̹��� 
			vector<KeyPoint>	&keypoints:		����� keypoints���� ���
			Mat		&descriptors:				����� keypoints�鿡 �����Ǵ� descriptor��
							
Language:   C++

///////////////////////////////////////////////////////////////////////*/
void	makeDescriptors(const Mat &img_input, vector<KeyPoint> &keypoints, Mat &descriptors);

/*Function///////////////////////////////////////////////////////////////

Name:       makeDescriptors			for DB

Purpose:    �־��� �̹������� keypoints�� ã�� �� keypoints���� descriptor�� ����

Parameters: Mat		&img_input:					keypoints�� ã�� descriptor�� ������ �̹��� 
			vector<KeyPoint>	&keypoints:		����� keypoints���� ���
			Mat		&descriptors:				����� keypoints�鿡 �����Ǵ� descriptor��
							
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

//���� ������� �ʴ� �Լ���
void mslTransformPoint(	const vector<Point>	src, vector<Point2f>	&dst,
	 int	idxRotation,	 int	idxScale) ;