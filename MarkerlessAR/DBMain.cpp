#include "stdafx.h"

#include <opencv2/core/core.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <math.h>

#include <fstream>
#include <list>

#include "stdint.h"
#include "brisk/brisk.h"

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <atlstr.h>

using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ostream;
using namespace cv;
using namespace std;

#include "SungwookFeature.hpp"



//typedef	cv::Point_<int>	Quadangle[4];
#define PI 3.14159

Mat mslTransformImage(Mat originalImage, Mat *tranformedImage, int idxRotation, int idxScale);
void mslTransformKeypoint(	vector<KeyPoint>	src, vector<KeyPoint>	&dst, unsigned int	idxRotation,	unsigned int	idxScale);
void  mslRotateKeypoint(	vector<KeyPoint>	src, vector<KeyPoint>	&dst,	unsigned int	idxRotation,	unsigned int	idxScale); 
void  mslOriginKeypoint(	vector<KeyPoint>	src, vector<KeyPoint>	&dst,	unsigned int	idxRotation,	unsigned int	idxScale); 

int idx_counter=0;
strFilename Filename[20] = 
{
    "../image/iu2.jpg",
	"../image/iu1.jpg",
	
};


int main()
{
	//  Database FileOpen:Mode=WRITE
	//FileStorage fs(XML_FILE_NAME2, FileStorage::WRITE);
	FileStorage fs;
	char FeatureID[50];    

	char DescriptorID[50];
	char QuadangleID[50];   



	Mat	desc_transformed;

	Ptr<FeatureDetector>	featureDetector;
	const int DESIRED_FTRS = 100;					// DB 한장에서 뽑는 Feature 개수

	// Create feature extractor
#if	defined(FAST_FeatureDetector)
	featureDetector = new GridAdaptedFeatureDetector(new FastFeatureDetector(30, true), DESIRED_FTRS, 4, 4);
	//featureDetector = new FastFeatureDetector(30, true);
#elif defined(AGAST_FeatureDetector)
	featureDetector = new BriskFeatureDetector(60,2);
#elif defined(SURF_FeatureDetector)
	featureDetector = new GridAdaptedFeatureDetector(new SurfFeatureDetector(500, 3, 4), DESIRED_FTRS, 4, 4);
#elif defined(SIFT_FeatureDetector)
	featureDetector = FeatureDetector::create("SIFT");
#elif defined(GFTT_FeatureDetector)
	featureDetector = FeatureDetector::create("GFTT");
#elif defined(MSER_FeatureDetector)
	featureDetector = FeatureDetector::create("MSER");
#elif defined(HARRIS_FeatureDetector)
	featureDetector = FeatureDetector::create("HARRIS");
#endif	


	Ptr<DescriptorExtractor>	descriptorExtractor;

#if defined(BRIEF_FeatureDescriptor)
	descriptorExtractor = new BriefDescriptorExtractor(32);				// 16, 32, or 64
#elif defined(BRISK_FeatureDescriptor)
	descriptorExtractor = new BriskDescriptorExtractor();
#elif	defined(SURF_FeatureDescriptor)
	descriptorExtractor = new SurfDescriptorExtractor();
#endif

	vector<KeyPoint>					kp_original;	
	vector<KeyPoint>					kp_original_transformed;
	vector<KeyPoint>					kp_transform;	
	vector<KeyPoint>					kp_transformOrigin;	
	vector<vector<vector<KeyPoint>>>	kp_database;

	vector<vector<Mat>>					desc_database;
	vector<int>							kp_size;

	Mat	img_original_db;
	Mat	img_original_copy;
	Mat	desc_original;	
	for(int counting=0; counting<8; counting++){
		idx_counter=0;


		switch(counting){
		case 0:	fs.open(XML_FILE_NAME1, FileStorage::WRITE);
				break;
		case 1:	fs.open(XML_FILE_NAME2, FileStorage::WRITE);
				break;
				

		}
		
			

		Mat		img_original	=	imread(Filename[counting]._strFilename.c_str() , CV_LOAD_IMAGE_GRAYSCALE);	
		
		cvtColor(img_original, img_original_copy, CV_GRAY2BGR);

		if(img_original.empty()) 
		{ 
			fprintf(stderr, "Can not load %d image %s\n", counting, Filename[counting]._strFilename.c_str()); 
			return -1; 
		} 

		featureDetector->detect(img_original, kp_original);
		descriptorExtractor->compute(img_original, kp_original, desc_transformed);

		vector<KeyPoint>	kp_canonical;
		vector<KeyPoint>	kp_canonical_transformed;
		
		kp_canonical.push_back(KeyPoint(0, 0, 1));
		kp_canonical.push_back(KeyPoint(img_original.cols, 0, 1));
		kp_canonical.push_back(KeyPoint(img_original.cols, img_original.rows, 1));
		kp_canonical.push_back(KeyPoint(0, img_original.rows, 1));
		
		

		//Quadrangle	quad_canonical;
		//quad_canonical[0] = Point2f(0,0);
		//quad_canonical[1] = Point2f(img_original.cols,0);
		//quad_canonical[2] = Point2f(img_original.cols,img_original.rows);
		//quad_canonical[3] = Point2f(0,img_original.rows);
		//Quadrangle	quad_transformed;


		//write Database
		{
				for(int idx_scale = 0; idx_scale < NUMBER_OF_SCALE_STEPS; ++idx_scale) {
					if(idx_scale == 4)
						cout << "FOUND";

					//Scale이 1.0인 오리지널 영상에 대한 정보를 저장.
					for(int i=0; i<NUMBER_OF_SCALE_STEPS; i++){
						if(scaleFactor[i]==1.0){
							img_original_db=mslTransformImage(img_original, &img_original_db, 0, i);
							break;
						}

					}

					//From 0 to Number of rotation
					for(int idx_rotation = 0; idx_rotation < NUMBER_OF_ROTATION_STEPS; ++idx_rotation, idx_counter++) {
						
						Mat	img_transformed;
						//Mat	desc_original;		

						//영상을 변환하여 img_transformed에 저장.
						img_transformed = mslTransformImage(img_original, &img_transformed, idx_rotation, idx_scale);

						//변환된 영상에 대하여 Feature Detect를 수행
						if(idx_rotation==0)
							featureDetector->detect(img_transformed, kp_original);

						featureDetector->detect(img_transformed, kp_transform);


						descriptorExtractor->compute(img_transformed, kp_transform, desc_original);

						//mslRotateKeypoint(kp_original, kp_original_transformed, idx_rotation, idx_scale);

						//변형된 이미지에 대한 Keypoint들을 원 상태의 이미지에 대한 Keypoint로 복구시키는 함수
						//mslOriginKeypoint(kp_transform, kp_transformOrigin, idx_rotation, idx_scale);

						
				


						/*
						if(idx_scale==0 && idx_rotation==0){
							for(int i=0; i<NUMBER_OF_SCALE_STEPS; i++){
								if(scaleFactor[i]==1.0){
									mslTransformKeypoint(kp_canonical, kp_canonical_transformed, 0, i);
									break;
								}
							}

							//Write Quadangle
							sprintf(QuadangleID, "%s%d", "QuadangleIndex", counting);
							Quadrangle	quad;
							quad[0] = kp_canonical_transformed[0].pt;
							quad[1] = kp_canonical_transformed[1].pt;
							quad[2] = kp_canonical_transformed[2].pt;
							quad[3] = kp_canonical_transformed[3].pt;
							fs<<QuadangleID<<"[";
							fs<<quad[0]<<quad[1]<<quad[2]<<quad[3];	
							fs<<"]";


						}else{
							mslTransformKeypoint(kp_canonical, kp_canonical_transformed, idx_rotation, idx_scale);


						}
				*/
						
						//CvPoint2D32f center;
						//center.x=img_transformed.rows/2.0;
						//center.y=img_transformed.cols/2.0;
						//QuadAngle에 대한 Rotation 및 Scale 변환 계산
						//mslTransformPoint(img_original, qr_DstPoints, center, idx_rotation, idx_scale);
						

						

						


						//DATABASE WRITE BY SUNGWOOK 2011.06.01
						{				
							//Write KeyPoints
							sprintf(FeatureID, "%s%d", "FeatureIndex", idx_counter );
							//write(fs, FeatureID, kp_original);
							
							//회전된 Keypoint들을 원본영상에 매핑시킨 것으로 수정함 by 성욱 11.11.22
							write(fs, FeatureID, kp_transform);
							
							
							//Write Descriptors
							sprintf(DescriptorID, "%s%d", "DescriptorIndex", idx_counter);
							fs<<DescriptorID<<"[";
							fs<<desc_original;	
							fs<<"]";

												

						}						
						//Quadangle 계산 및 저장하는 부분 by 성욱 11.11.22

						{
							mslTransformKeypoint(kp_canonical, kp_canonical_transformed, idx_rotation, idx_scale);

							//Write Quadangle
							sprintf(QuadangleID, "%s%d", "QuadangleIndex", idx_counter);
							Quadrangle	quad;
							quad[0] = kp_canonical_transformed[0].pt;
							quad[1] = kp_canonical_transformed[1].pt;
							quad[2] = kp_canonical_transformed[2].pt;
							quad[3] = kp_canonical_transformed[3].pt;
							fs<<QuadangleID<<"[";
							fs<<quad[0]<<quad[1]<<quad[2]<<quad[3];	
							fs<<"]";

						}


						//// 그림 그려서 확인하자
						Mat	img_canvas;
						cvtColor(img_transformed, img_canvas, CV_GRAY2BGR);
						
						for(int i=0; i<kp_transform.size(); ++i) {
							cv::circle(img_canvas, kp_transform[i].pt, 3, CV_RGB(0, 255, 0), -1 );
							//printf("(%lf, %lf)\n", kp_canonical_transformed[i].pt.x, kp_canonical_transformed[i].pt.y);
						}     
						for(int i=0; i<4; ++i){
							cv::circle(img_canvas, kp_canonical_transformed[i].pt, 3, CV_RGB(255, 255, 255), -1 );
						}

																	      
						printf("\n");
						namedWindow("test");
						imshow("test", img_canvas);

						

						waitKey(0);
				

						cout<<idx_scale<<": "<<idx_rotation<<" Write Complete"<<endl;


					}

				}   
		
			//fs.~FileStorage();
		}

	}
	cout<<"Complete";
	fs.release();
	//return 0;
}