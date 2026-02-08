/*
    BRISK - Binary Robust Invariant Scalable Keypoints
    Reference implementation of
    [1] Stefan Leutenegger,Margarita Chli and Roland Siegwart, BRISK:
    	Binary Robust Invariant Scalable Keypoints, in Proceedings of
    	the IEEE International Conference on Computer Vision (ICCV2011).

    Copyright (C) 2011  The Autonomous Systems Lab, ETH Zurich,
    Stefan Leutenegger, Simon Lynen and Margarita Chli.

    This file is part of BRISK.

    BRISK is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    BRISK is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with BRISK.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "SungwookAR.hpp"
#include "SungwookFeature.hpp"
#include "stdint.h"
#include "brisk/brisk.h"





/*Function///////////////////////////////////////////////////////////////
/*Function///////////////////////////////////////////////////////////////

Name:       makeDescriptors			for query

Purpose:    주어진 이미지에서 keypoints를 찾고 각 keypoints들의 descriptor를 만듦

Parameters: Mat		&img_input:					keypoints를 찾고 descriptor를 생성할 이미지 
			vector<KeyPoint>	&keypoints:		검출된 keypoints들의 목록
			Mat		&descriptors:				검출된 keypoints들에 대응되는 descriptor들
							
Language:   C++

///////////////////////////////////////////////////////////////////////*/
void	makeDescriptors(	const Mat			&img_input,									// 입력 변수
							vector<KeyPoint>	&keypoints,		Mat		&descriptors)		// 출력 변수
{


	//=======================================================================
	//=======================================================================
	//===		1. Feature Extraction
	Ptr<FeatureDetector>	featureDetector;
	const int DESIRED_FTRS = 100;		// query 영상에서 뽑히는 Feature 의 개수

	// Create feature extractor
#if	defined(FAST_FeatureDetector)
	featureDetector = new GridAdaptedFeatureDetector(new FastFeatureDetector(30, true), DESIRED_FTRS, 4, 4);
	//featureDetector = new FastFeatureDetector(30, true);
#elif defined(AGAST_FeatureDetector)
	featureDetector = new BriskFeatureDetector(60,2);
#elif defined(SURF_FeatureDetector)
	featureDetector = new SurfFeatureDetector(500, 3, 4);
#elif defined(SIFT_FeatureDetector)
	featureDetector = FeatureDetector::create("SIFT");
#elif defined(GFTT_FeatureDetector)
	featureDetector = FeatureDetector::create("GFTT");
#elif defined(MSER_FeatureDetector)
	featureDetector = FeatureDetector::create("MSER");
#elif defined(HARRIS_FeatureDetector)
	featureDetector = FeatureDetector::create("HARRIS");
#endif
	if(	featureDetector.empty()	)
		cerr	<<	"Can not create feature detector of given type." << endl;
	keypoints.clear();
	featureDetector->detect(img_input, keypoints);
	//===		end of 1. Feature Extraction
	//=======================================================================




	//=======================================================================
	//=======================================================================
	//===		2. Feature Description
	Ptr<DescriptorExtractor>	descriptorExtractor;
#if defined(BRIEF_FeatureDescriptor)
	descriptorExtractor = new BriefDescriptorExtractor(32);				// 16, 32, or 64
#elif defined(BRISK_FeatureDescriptor)
	descriptorExtractor = new BriskDescriptorExtractor();
#elif	defined(SURF_FeatureDescriptor)
	descriptorExtractor = new SurfDescriptorExtractor();
#endif

	if(	descriptorExtractor.empty()	)
		cerr	<<	"Can not create descriptor extractor of given type." << endl;
#ifdef	Time_Count
	double	time = GetTickCount();
	double	sum = 0.0;
	for(int i=0; i<100; ++i) {
		descriptorExtractor->compute(img_input, keypoints, descriptors);
		sum += GetTickCount() - time;
		time = GetTickCount();
	}
	printf("Descriptor creation took %f ms.\n", sum/100);
#endif

	descriptorExtractor->compute(img_input, keypoints, descriptors);

	//===		end of 2. Feature Description
	//=======================================================================
}



/*Function///////////////////////////////////////////////////////////////

Name:       makeDescriptors			for DB

Purpose:    주어진 이미지에서 keypoints를 찾고 각 keypoints들의 descriptor를 만듦

Parameters: Mat		&img_input:					keypoints를 찾고 descriptor를 생성할 이미지 
			vector<KeyPoint>	&keypoints:		검출된 keypoints들의 목록
			Mat		&descriptors:				검출된 keypoints들에 대응되는 descriptor들
							
Language:   C++

///////////////////////////////////////////////////////////////////////*/
void	makeDescriptors(	const	vector<Mat>					&img_input,									// 입력 변수
							vector<vector<KeyPoint>>			&keypoints,		vector<Mat>		&descriptors)		// 출력 변수
{


	//=======================================================================
	//=======================================================================
		//===		1. Feature Extraction
	Ptr<FeatureDetector>	featureDetector;
	const int DESIRED_FTRS = 50;

	// Create feature extractor
#if	defined(FAST_FeatureDetector)
	//featureDetector = new GridAdaptedFeatureDetector(new FastFeatureDetector(30, true), DESIRED_FTRS, 4, 4);
	featureDetector = new DynamicAdaptedFeatureDetector(new FastAdjuster(20, true), 10, 20, 5);
#elif defined(AGAST_FeatureDetector)
	featureDetector = new BriskFeatureDetector(60,2);
	//featureDetector = new FastFeatureDetector(30, true);
#elif defined(SURF_FeatureDetector)
	featureDetector = new SurfFeatureDetector(500, 3, 4);
#elif defined(SIFT_FeatureDetector)
	featureDetector = FeatureDetector::create("SIFT");
#elif defined(GFTT_FeatureDetector)
	featureDetector = FeatureDetector::create("GFTT");
#elif defined(MSER_FeatureDetector)
	featureDetector = FeatureDetector::create("MSER");
#elif defined(HARRIS_FeatureDetector)
	featureDetector = FeatureDetector::create("HARRIS");
#endif
	if(	featureDetector.empty()	)
		cerr	<<	"Can not create feature detector of given type." << endl;
		
	keypoints.clear();
	featureDetector->detect(img_input, keypoints);
	//===		end of 1. Feature Extraction
	//=======================================================================




	//=======================================================================
	//=======================================================================
	//===		2. Feature Description
	Ptr<DescriptorExtractor>	descriptorExtractor;
#if defined(BRIEF_FeatureDescriptor)
	descriptorExtractor = new BriefDescriptorExtractor(16);				// 16, 32, or 64
#elif defined(BRISK_FeatureDescriptor)
	descriptorExtractor = new BriskDescriptorExtractor();
#elif	defined(SURF_FeatureDescriptor)
	descriptorExtractor = new SurfDescriptorExtractor(4, 2, true);
#endif
	if(	descriptorExtractor.empty()	)
		cerr	<<	"Can not create descriptor extractor of given type." << endl;

	descriptorExtractor->compute(img_input, keypoints, descriptors);
	//===		end of 2. Feature Description
	//=======================================================================
}


//Copy (x,y) location of descriptor matches found from KeyPoint data structures into Point2f vectors
#ifdef BRISK_FeatureDescriptor
void matches2points(const vector<vector<DMatch> >& matches, const vector<KeyPoint>& kpts_train,
                    const vector<KeyPoint>& kpts_query, vector<Point2f>& pts_train,
                    vector<Point2f>& pts_query)
{
  pts_train.clear();
  pts_query.clear();

  for (size_t k = 0; k < matches.size(); k++)
  {
    for (size_t i = 0; i < matches[k].size(); i++)
    {
        const DMatch& match = matches[k][i];
        pts_query.push_back(kpts_query[match.queryIdx].pt);
        pts_train.push_back(kpts_train[match.trainIdx].pt);
    }
  }

}
#elif(BRIEF_FeatureDescriptor)
void matches2points(const vector<DMatch>& matches, const int imageIdx,
	const vector<vector<KeyPoint>>&			kpts_train,		const vector<KeyPoint>& kpts_query, 
	vector<Point2f>&						pts_train,		vector<Point2f>&		pts_query)
{
	pts_train.clear();
	pts_query.clear();
	pts_train.reserve(matches.size());
	pts_query.reserve(matches.size());

	for (size_t i = 0; i < matches.size(); i++)
	{
		const DMatch& match = matches[i];
		pts_train.push_back(kpts_train	[ imageIdx ]
		[match.trainIdx].pt);
		pts_query.push_back(kpts_query[match.queryIdx].pt);
	}
}
#endif

void	convertKP2Point2f(vector<KeyPoint> &kp_reference, vector<Point2f> &points) 
{
	points.clear();
	points.reserve(kp_reference.size());
	vector<KeyPoint>::iterator	iter;
	
	for(iter = kp_reference.begin(); iter != kp_reference.end(); ++iter)
	{
		points.push_back( Point2f( (*iter).pt.x, (*iter).pt.y) );
	}
}

// Returns whether H is a nice homography matrix or not
bool niceHomography(const CvMat * H)
{
	const double det = cvmGet(H, 0, 0) * cvmGet(H, 1, 1) - cvmGet(H, 1, 0) * cvmGet(H, 0, 1);
	if (det < 0)
		return false;

	const double N1 = sqrt(cvmGet(H, 0, 0) * cvmGet(H, 0, 0) + cvmGet(H, 1, 0) * cvmGet(H, 1, 0));
	if (N1 > 4 || N1 < 0.1)
		return false;

	const double N2 = sqrt(cvmGet(H, 0, 1) * cvmGet(H, 0, 1) + cvmGet(H, 1, 1) * cvmGet(H, 1, 1));
	if (N2 > 4 || N2 < 0.1)
		return false;

	const double N3 = sqrt(cvmGet(H, 2, 0) * cvmGet(H, 2, 0) + cvmGet(H, 2, 1) * cvmGet(H, 2, 1));
	if (N3 > 0.002)
		return false;

	return true;
}



//Move Original Image to Center Image
Mat swMoveImage(Mat originalImage, Mat *tranformedImage, double idxScale){
	
	IplImage *src = &IplImage(originalImage);
	double	angle  = 0.0;
		
	double x = src->width*idxScale/2.0;
	double y = src->height*idxScale/2.0;
	x*=x; y*=y;
	double r = sqrt(x+y);
	
	if(idxScale<=1.0){
		x = src->width*1.0/2.0;
		y = src->height*1.0/2.0;
		x*=x; y*=y;
		r = sqrt(x+y);
	}


	//변환된 이미지가 저장될 공간 생성
	IplImage *src2	= cvCreateImage( cvSize(r*2, r*2), IPL_DEPTH_8U, 1);
	IplImage *dst	= cvCreateImage( cvSize(r*2, r*2), IPL_DEPTH_8U, 1);

	cvZero(src2);
	cvZero(dst);
	
	//원본 이미지크기만큼 관심영역을 지정
	cvSetImageROI(src2, cvRect(src2->width/2-src->width/2, src2->height/2-src->height/2, src->width, src->height));

	cvCopy(src, src2);
	cvResetImageROI(src2);

	//회전중심설정  
	CvPoint2D32f center = cvPoint2D32f( src2->width/2.0, src2->height/2.0);
	CvMat *rot_mat		= cvCreateMat( 2, 3, CV_32FC1);

	// 메트릭스 변환  
	cv2DRotationMatrix( center, angle, idxScale, rot_mat); 
	// 선형보간
	cvWarpAffine( src2, dst, rot_mat, CV_INTER_LINEAR/*+CV_WARP_FILL_OUTLIERS*/); 
		
	Mat	ret(dst);

	cvReleaseImage( &src2 );
	cvReleaseMat( &rot_mat);

	return ret;
}

//Move Original Corners to Center Corners
void swMoveCorners(Mat originalImage, 	vector<Point2f>	src, vector<Point2f>	&dst,
	unsigned int	idxRotation,	unsigned int	idxScale) {

		IplImage *imgsrc = &IplImage(originalImage);

		double	rotationAngle = 0.0;
		double	scale = 0.5;

		//double	scale = scaleFactor[idxScale];
		//double	rotationAngle = 360.0 * idxRotation/NUMBER_OF_ROTATION_STEPS;

		//회전중심설정
		double x = imgsrc->width*scale/2.0;
		double y = imgsrc->height*scale/2.0;
		x*=x; y*=y;
		double r = sqrt(x+y);

		if(scale<=1.0){
			x = imgsrc->width*1.0/2.0;
			y = imgsrc->height*1.0/2.0;
			x*=x; y*=y;
			r = sqrt(x+y);
		}

		//Mat	transformationMatrix = 	getRotationMatrix2D(Point2f(IMAGE_WIDTH/2.0, IMAGE_HEIGHT/2.0), rotationAngle, scaleFactor[idxScale]);
		Mat	transformationMatrix = 	getRotationMatrix2D(Point2f(imgsrc->width/2.0, imgsrc->height/2.0)
			, rotationAngle, scale);

		//	destination의 크기 재설정
		dst.clear();
		dst.resize(src.size());
		
		//	Iteration
		for(int i=0; i<src.size(); ++i) {
			float	x_ =	transformationMatrix.at<double>(0, 0) * src[i].x + 
				transformationMatrix.at<double>(0, 1) * src[i].y +
				transformationMatrix.at<double>(0, 2) + (r-imgsrc->width/2.0);
			float	y_ =	transformationMatrix.at<double>(1, 0) * src[i].x + 
				transformationMatrix.at<double>(1, 1) * src[i].y +
				transformationMatrix.at<double>(1, 2) + (r-imgsrc->height/2.0);

			dst[i] = src[i];
			dst[i].x=x_;
			dst[i].y=y_;
			//dst[i].pt = Point2f(x_, y_);
		}
		
}


void mslTransformKeypoint(	vector<KeyPoint>	src, vector<KeyPoint>	&dst,
	unsigned int	idxRotation,	unsigned int	idxScale) {

		double	rotationAngle = ROT_ANGLE_INCREMENT * idxRotation;
		double	scale = scaleFactor[idxScale];

		//double	scale = scaleFactor[idxScale];
		//double	rotationAngle = 360.0 * idxRotation/NUMBER_OF_ROTATION_STEPS;

		//회전중심설정
		double x = sz_origin.width*scale/2.0;
		double y = sz_origin.height*scale/2.0;
		x*=x; y*=y;
		double r = sqrt(x+y);

		if(scale<=1.0){
			x = sz_origin.width*1.0/2.0;
			y = sz_origin.height*1.0/2.0;
			x*=x; y*=y;
			r = sqrt(x+y);
		}

		//Mat	transformationMatrix = 	getRotationMatrix2D(Point2f(IMAGE_WIDTH/2.0, IMAGE_HEIGHT/2.0), rotationAngle, scaleFactor[idxScale]);
		Mat	transformationMatrix = 	getRotationMatrix2D(Point2f(sz_origin.width/2.0, sz_origin.height/2.0)
			, rotationAngle, scale);

		//	destination의 크기 재설정
		dst.clear();
		dst.resize(src.size());

		//	Iteration
		for(int i=0; i<src.size(); ++i) {
			float	x_ =	transformationMatrix.at<double>(0, 0) * src[i].pt.x + 
				transformationMatrix.at<double>(0, 1) * src[i].pt.y +
				transformationMatrix.at<double>(0, 2) + (r-sz_origin.width/2.0);
			float	y_ =	transformationMatrix.at<double>(1, 0) * src[i].pt.x + 
				transformationMatrix.at<double>(1, 1) * src[i].pt.y +
				transformationMatrix.at<double>(1, 2) + (r-sz_origin.height/2.0);

			dst[i] = src[i];
			dst[i].pt = Point2f(x_, y_);
		}
}

void mslOriginKeypoint(vector<KeyPoint>	src, vector<KeyPoint>	&dst,
	unsigned int	idxRotation, unsigned int idxScale){

		
		double	rotationAngle = 360-ROT_ANGLE_INCREMENT * idxRotation;
		double	scale = scaleFactor[idxScale];

		//회전중심설정
		double x = sz_origin.width*scale/2.0;
		double y = sz_origin.height*scale/2.0;
		x*=x; y*=y;
		double r = sqrt(x+y);

		if(scale<=1.0){
			x = sz_origin.width*1.0/2.0;
			y = sz_origin.height*1.0/2.0;
			x*=x; y*=y;
			r = sqrt(x+y);
		}
	
		scale = 1.0/scaleFactor[idxScale];
		Mat	transformationMatrix = 	getRotationMatrix2D(Point2f(r, r)
			, rotationAngle, scale);


		//	destination의 크기 재설정
		dst.clear();
		dst.resize(src.size());

		//	Iteration
		for(int i=0; i<src.size(); ++i) {
			


			float	x_ =	transformationMatrix.at<double>(0, 0) * (src[i].pt.x) + 
				transformationMatrix.at<double>(0, 1) * (src[i].pt.y) +
				transformationMatrix.at<double>(0, 2) ;
			float	y_ =	transformationMatrix.at<double>(1, 0) * (src[i].pt.x) + 
				transformationMatrix.at<double>(1, 1) * (src[i].pt.y) +
				transformationMatrix.at<double>(1, 2);

			
			dst[i] = src[i];
			dst[i].pt = Point2f(x_, y_);
		}



}



void mslRotateKeypoint(vector<KeyPoint>	src, vector<KeyPoint>	&dst,
	unsigned int	idxRotation,	unsigned int	idxScale) {

		double	rotationAngle = ROT_ANGLE_INCREMENT * idxRotation;
		double	scale = scaleFactor[idxScale];


		//double	scale = scaleFactor[idxScale];
		//double	rotationAngle = 360.0 * idxRotation/NUMBER_OF_ROTATION_STEPS;

	
		//회전중심설정
		double x = sz_origin.width*scale/2.0;
		double y = sz_origin.height*scale/2.0;
		x*=x; y*=y;
		double r = sqrt(x+y);

		if(scale<=1.0){
			x = sz_origin.width*1.0/2.0;
			y = sz_origin.height*1.0/2.0;
			x*=x; y*=y;
			r = sqrt(x+y);
		}
		
		//Mat	transformationMatrix = 	getRotationMatrix2D(Point2f(IMAGE_WIDTH/2.0, IMAGE_HEIGHT/2.0), rotationAngle, scaleFactor[idxScale]);

		/*
		Mat	transformationMatrix = 	getRotationMatrix2D(Point2f(sz_origin.width/2.0, sz_origin.height/2.0)
			, rotationAngle, 1.0);*/

		Mat	transformationMatrix = 	getRotationMatrix2D(Point2f(r, r)
			, rotationAngle, 1.0);


		//	destination의 크기 재설정
		dst.clear();
		dst.resize(src.size());

		//	Iteration
		for(int i=0; i<src.size(); ++i) {
			


			float	x_ =	transformationMatrix.at<double>(0, 0) * (src[i].pt.x) + 
				transformationMatrix.at<double>(0, 1) * (src[i].pt.y) +
				transformationMatrix.at<double>(0, 2) ;
			float	y_ =	transformationMatrix.at<double>(1, 0) * (src[i].pt.x) + 
				transformationMatrix.at<double>(1, 1) * (src[i].pt.y) +
				transformationMatrix.at<double>(1, 2);

			/*
			float	x_ =	transformationMatrix.at<double>(0, 0) * src[i].pt.x + 
				transformationMatrix.at<double>(0, 1) * src[i].pt.y +
				transformationMatrix.at<double>(0, 2) - (r-sz_origin.width/2.0);
			float	y_ =	transformationMatrix.at<double>(1, 0) * src[i].pt.x + 
				transformationMatrix.at<double>(1, 1) * src[i].pt.y +
				transformationMatrix.at<double>(1, 2) - (r-sz_origin.height/2.0);*/



			dst[i] = src[i];
			dst[i].pt = Point2f(x_, y_);
		}



}

//void mslTransformImage(Mat originalImage, Mat *tranformedImage, double rotation, double scale){
Mat mslTransformImage(Mat originalImage, Mat *tranformedImage, int idxRotation, int idxScale){

	//	const double angle  = 45.0; //회전각도  
	//	const double scale  = 2.0;  //크기

	IplImage *src = &IplImage(originalImage);

	//double	angle = 360.0 * idxRotation/NUM_OF_ROTATION;
	double	angle  = ROT_ANGLE_INCREMENT * idxRotation;

	//반지름 구하기
	double	scale = scaleFactor[idxScale];

	sz_origin.width = src->width;
	sz_origin.height = src->height;


	double x = src->width*scale/2.0;
	double y = src->height*scale/2.0;
	x*=x; y*=y;
	double r = sqrt(x+y);


	if(scale<=1.0){
		x = src->width*1.0/2.0;
		y = src->height*1.0/2.0;
		x*=x; y*=y;
		r = sqrt(x+y);
	}


	//변환된 이미지가 저장될 공간 생성
	IplImage *src2	= cvCreateImage( cvSize(r*2, r*2), IPL_DEPTH_8U, 1);
	IplImage *dst	= cvCreateImage( cvSize(r*2, r*2), IPL_DEPTH_8U, 1);

	cvZero(src2);
	cvZero(dst);


	//원본 이미지크기만큼 관심영역을 지정
	cvSetImageROI(src2, cvRect(src2->width/2-src->width/2, src2->height/2-src->height/2, src->width, src->height));

	cvCopy(src, src2);
	cvResetImageROI(src2);

	//회전중심설정  
	CvPoint2D32f center = cvPoint2D32f( src2->width/2.0, src2->height/2.0);
	CvMat *rot_mat		= cvCreateMat( 2, 3, CV_32FC1);

	// 메트릭스 변환  
	cv2DRotationMatrix( center, angle, scale, rot_mat); 
	// 선형보간
	cvWarpAffine( src2, dst, rot_mat, CV_INTER_LINEAR/*+CV_WARP_FILL_OUTLIERS*/); 

	// 저장
	//char	FILE_NAME[] = "cola";
	//char	save_file_name[100];
	//sprintf(save_file_name, "%s_s%d_r%d.png", FILE_NAME, idxScale, idxRotation);
	//cvSaveImage(save_file_name, dst);

	//	HIGH GUI 
	//cvNamedWindow("Affine_Transform", 1);
	//cvShowImage("Affine_Transform", dst);
	//cvWaitKey(10);
	//cvDestroyWindow("Affine_Transform");

	//CvMat *mat=cvCreateMat(dst->height, dst->width, CV_32FC1);
	//cvConvert(dst, mat);
	Mat	ret(dst);

	cvReleaseImage( &src2 );
	//cvReleaseImage( &dst );
	cvReleaseMat( &rot_mat);

	return ret;
}
