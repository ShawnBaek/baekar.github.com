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
#include <Windows.h>
#include <opencv2/opencv.hpp>
#include <fstream>
#include <iostream>
#include <list>

#include "stdint.h"
#include "brisk.h"
//#include "projection.h"
#include "Matcher.h"

#include "MatchVerifier.hpp"
#include "GroundTruth.hpp"



using namespace cv;

static const int T_SIFT=0, T_SURF=1, T_BRIEF=2, T_BRISK=3;
static const int TYPE_RS=0, TYPE_U=1, TYPE_S=2, TYPE_US=3;

#define Pixel(type,dataStart,step,size,x,y,channel) *((type*)(dataStart+step*(y)+(x)*size+channel))

//standard configuration for the case of no file given
const int n=12;
const float r=2.5; // found 8-9-11, r=3.6, exponent 1.5

void patchWarp(const Mat& imgSrc, std::vector<Mat> cont_imgDst, KeyPoint key, int patchSize, int numView);

void readDescriptors(string file1, string file2, 
					 std::vector<KeyPoint> keypoints1,
					 std::vector<KeyPoint> keypoints2)
{
	// try to read descriptor files
	std::string desc1 = std::string(file1);
	std::string desc2 = std::string(file2);
	std::ifstream descf1(desc1.c_str());
	
	if(!descf1.good()){
		std::cout<<"Descriptor file not found at " << desc1<<std::endl;
		return;
	}
	std::ifstream descf2(desc2.c_str());
	if(!descf2.good()){
		std::cout<<"Descriptor file not found at " << desc2<<std::endl;
		return;
	}

	// fill the keypoints
	std::string str1;
	std::stringstream strstrm1;
	std::getline(descf1,str1);
	std::getline(descf1,str1);
	while(!descf1.eof()){
		std::getline(descf1,str1);
		float x,y,a;
		strstrm1.str(str1);
		strstrm1>>x;
		strstrm1>>y;
		strstrm1>>a;
		float r=sqrt(1.0/a);
		keypoints1.push_back(KeyPoint(x, y, 4.0*r));
	}
	std::string str2;
	std::stringstream strstrm2;
	std::getline(descf2,str2);
	std::getline(descf2,str2);
	while(!descf2.eof()){
		std::getline(descf2,str2);
		float x,y,a;
		strstrm2.str(str2);
		strstrm2>>x;
		strstrm2>>y;
		strstrm2>>a;
		float r=sqrt(1.0/a);
		keypoints2.push_back(KeyPoint(x, y, 4.0*r));
	}

	// clean up
	descf1.close();
	descf2.close();
}

void patchWarp(const Mat& imgSrc, std::vector<Mat> cont_imgDst, KeyPoint key, int numView)
{
	//! image에서 keypoint의 patch를 patchsize만큼 warping한다
	PatchGenerator generater;

	Mat matM(2, 3, CV_64F);
	double *M = (double*)matM.data;
	RNG& rng = theRNG();

	int halfSize = 20/2;
	//Rect roi(key.pt.x-halfSize, key.pt.y-halfSize, patchSize, patchSize);
	//Mat canvas(imgSrc, roi);

	Point2f center1(imgSrc.cols/2, imgSrc.rows/2);
	Point2f center2(halfSize, halfSize);

	for(int i=0; i<numView; i++){

		generater.generateRandomTransform(center1, center2, matM, rng);

		CV_Assert(matM.type() == CV_64F);
		Rect roi(INT_MAX, INT_MAX, INT_MIN, INT_MIN);

		Point2f ptSrc[4];
		ptSrc[0].x = key.pt.x - halfSize;
		ptSrc[0].y = key.pt.y - halfSize;
		ptSrc[1].x = key.pt.x + halfSize;
		ptSrc[1].y = key.pt.y - halfSize;
		ptSrc[2].x = key.pt.x + halfSize;
		ptSrc[2].y = key.pt.y + halfSize;
		ptSrc[3].x = key.pt.x - halfSize;
		ptSrc[3].y = key.pt.y + halfSize;

		printf("---------------------------------------\n");
		for(int i=0; i<6; i++){
			printf("%f\n", M[i]);
		}

		for(int k = 0; k < 4; k++ )
		{
			Point2f pt1;
			pt1.x = (float)(M[0]*ptSrc[k].x + M[1]*ptSrc[k].y + M[2]);
			pt1.y = (float)(M[3]*ptSrc[k].x + M[4]*ptSrc[k].y + M[5]);
			
			roi.x = std::min(roi.x, cvFloor(pt1.x));
			roi.y = std::min(roi.y, cvFloor(pt1.y));
			roi.width = std::max(roi.width, cvCeil(pt1.x));
			roi.height = std::max(roi.height, cvCeil(pt1.y));
		}

		roi.width -= roi.x + 1;
		roi.height -= roi.y + 1;

		Mat canvas_roi(cvSize(roi.width, roi.height), CV_8U);
		M[2] -= roi.x;
		M[5] -= roi.y;

		Size size = canvas_roi.size();
		rng.fill(canvas_roi, RNG::UNIFORM, Scalar::all(0), Scalar::all(256));
		warpAffine( imgSrc, canvas_roi, matM, size, INTER_LINEAR, BORDER_TRANSPARENT);

		cont_imgDst.push_back(canvas_roi);
	}

	printf("Num warped patches : %d\n", cont_imgDst.size());

	imshow("Image", imgSrc);
	imshow("Patch", cont_imgDst);
	waitKey();
}

void descriptorTest(int type_desc, int brisk_type, 
					string fname1, string fname2, string homofile){


	LARGE_INTEGER start, finish;
	LARGE_INTEGER ticksPerSec;
	double timeSec;
	if (!QueryPerformanceFrequency(&ticksPerSec)){ 
		// 고해상도 타이머를 지원하지 않음 
		return;
	} 

	// names of the two image files
	Mat imgRGB1;
	Mat imgRGB2;
	
	vector<Mat> cont_homo;

	imgRGB1 = imread(fname1, 1);
	imgRGB2 = imread(fname2, 1);

	//Mat dst;
	//tiltImage(imgGray1, dst, 1.0, 0.5, 1.0, 1.0);
	//calHomograpy(cont_homo, imgRGB1, 2.0, 2.0, 2.0, 2.0);
	//calHomograpy(cont_homo, imgRGB1, 1.0, 0.8, 1.2, 1.8);
	//tiltImage(imgRGB1, dst, cont_homo[0]);
	//dst.copyTo(imgRGB1);
	//imshow("src1", imgRGB1);
	//waitKey(0);

	//imshow("src1", imgRGB1);
	//imshow("src2", imgRGB2);
	//waitKey(0);

	// convert to grayscale
	Mat imgGray1;
	Mat imgGray2;
	cvtColor(imgRGB1,imgGray1, CV_BGR2GRAY);
	cvtColor(imgRGB2,imgGray2, CV_BGR2GRAY);

	// run FAST in first image
	std::vector<KeyPoint> keypoints1, keypoints2;
	int threshold;

	//affine_brisk estimation
	//perspective transform with openCV

	// create the detector:
	Ptr<FeatureDetector> detector;
	// extractor:
	bool hamming=true;
	Ptr<DescriptorExtractor> descriptorExtractor;

	float edgeThreshold, thresh;
	//choose the detector
	switch(type_desc){
		case T_SIFT:
			edgeThreshold = 10;
			thresh = 0.08 / SIFT::CommonParams::DEFAULT_NOCTAVE_LAYERS / 2.0;
			detector = new SiftFeatureDetector(thresh, edgeThreshold);
			descriptorExtractor = new SiftDescriptorExtractor();
			hamming=false;
			break;
		case T_SURF:
			threshold = 400;
			detector = new SurfFeatureDetector(threshold);
			descriptorExtractor = new SurfDescriptorExtractor();
			hamming=false;
			break;
		case T_BRIEF:
			threshold = 30;
			detector = new FastFeatureDetector(threshold,true);
			descriptorExtractor = new BriefDescriptorExtractor(64);
			break;
		case T_BRISK:
			detector = new BriskFeatureDetector(60,2);

			//choose BRISK type [AGAST]
			switch(brisk_type){
				case TYPE_RS:
					descriptorExtractor = new BriskDescriptorExtractor();
					break;
				case TYPE_U:
					descriptorExtractor = new BriskDescriptorExtractor(false);
					break;
				case TYPE_S:
					descriptorExtractor = new BriskDescriptorExtractor(false, false);
					break;
				case TYPE_US:
					descriptorExtractor = new BriskDescriptorExtractor(true, false);
					break;
			}
			break;
	}


	QueryPerformanceCounter(&start);
	
	// run the detector
	detector->detect(imgGray1,keypoints1);
	detector->detect(imgGray2,keypoints2);

	QueryPerformanceCounter(&finish);
	timeSec = (finish.QuadPart - start.QuadPart) / (double)ticksPerSec.QuadPart;
	printf("-- Test ---------------------------------------------\n");
	printf("Detection : %f ms\n", (float)timeSec*1000);
	printf("Detection : %f ms\n", (float)timeSec*1000 / (keypoints1.size()+keypoints2.size()));

	QueryPerformanceCounter(&start);

	//std::vector<Mat> cont_patchDesc;
	//patchWarp(imgGray1, cont_patchDesc, keypoints1[300], 20);
	
	// get the descriptors
	Mat descriptors1, descriptors2;
	descriptorExtractor->compute(imgGray1,keypoints1,descriptors1);
	descriptorExtractor->compute(imgGray2,keypoints2,descriptors2);

	QueryPerformanceCounter(&finish);	
	timeSec = (finish.QuadPart - start.QuadPart) / (double)ticksPerSec.QuadPart;
	printf("Description : %f ms\n", (float)timeSec*1000);
	printf("Description : %f ms\n", (float)timeSec*1000 / (keypoints1.size()+keypoints2.size()));

	QueryPerformanceCounter(&start);

	// matching
	std::vector<std::vector<DMatch> > matches;
	/*Ptr<DescriptorMatcher> descriptorMatcher;
	if(hamming)
		descriptorMatcher = new BruteForceMatcher<Hamming>(); 
	else
		descriptorMatcher = new BruteForceMatcher<L2<float> >();

	if(hamming)
		descriptorMatcher->radiusMatch(descriptors2,descriptors1,matches,100.0);
	else
		descriptorMatcher->radiusMatch(descriptors2,descriptors1,matches,0.21);*/

	BDHamming::commonRadiusMatch(descriptors2,descriptors1,matches,100.0);

	QueryPerformanceCounter(&finish);
	timeSec = (finish.QuadPart - start.QuadPart) / (double)ticksPerSec.QuadPart;
	printf("Match : %f ms\n", (float)timeSec*1000 / (keypoints1.size()+keypoints2.size()));

	Mat outimg;
	//check with homography ground truth
	MatchVerifier< KeyPoint, GroundTruth > verif(homofile.c_str(), 2);
	//float rr = verif.getRecognitionRate(keypoints1, keypoints2, matches, &cont_homo[0]);	
	float rr = verif.getRecognitionRate(keypoints1, keypoints2, matches, NULL);
	printf("Got %.2f%% of %i retrieved matches right\n", rr*100, (int)matches.size());
	
	// drawing
	drawMatches(imgRGB2, keypoints2, imgRGB1, keypoints1, matches,outimg,
		Scalar(0,255,0), Scalar(0,0,255),
		std::vector<std::vector<char> >(), DrawMatchesFlags::DRAW_RICH_KEYPOINTS );
	namedWindow("Matches");
	imshow("Matches", outimg);
	waitKey();
}



void descriptorCamTest(int type_desc, int brisk_type, 
					string fname1){

	VideoCapture	capture(0);
	LARGE_INTEGER start, finish;
	LARGE_INTEGER ticksPerSec;
	double timeSec;
	if (!QueryPerformanceFrequency(&ticksPerSec)){ 
		// 고해상도 타이머를 지원하지 않음 
		return;
	} 

	// names of the two image files
	Mat imgRGB1;
	Mat imgRGB2;
	
	vector<Mat> cont_homo;

	imgRGB1 = imread(fname1, 1);
	//imgRGB2 = imread(fname2, 1);

	//Mat dst;
	//tiltImage(imgGray1, dst, 1.0, 0.5, 1.0, 1.0);
	//calHomograpy(cont_homo, imgRGB1, 2.0, 2.0, 2.0, 2.0);
	//calHomograpy(cont_homo, imgRGB1, 1.0, 0.8, 1.2, 1.8);
	//tiltImage(imgRGB1, dst, cont_homo[0]);
	//dst.copyTo(imgRGB1);
	//imshow("src1", imgRGB1);
	//waitKey(0);

	//imshow("src1", imgRGB1);
	//imshow("src2", imgRGB2);
	//waitKey(0);

	// convert to grayscale
	Mat imgGray1;
	Mat imgGray2;
	cvtColor(imgRGB1,imgGray1, CV_BGR2GRAY);
	
	// run FAST in first image
	std::vector<KeyPoint> keypoints1, keypoints2;
	int threshold;

	//affine_brisk estimation
	//perspective transform with openCV

	// create the detector:
	Ptr<FeatureDetector> detector;
	// extractor:
	bool hamming=true;
	Ptr<DescriptorExtractor> descriptorExtractor;

	float edgeThreshold, thresh;
	//choose the detector
	switch(type_desc){
		case T_SIFT:
			edgeThreshold = 10;
			thresh = 0.08 / SIFT::CommonParams::DEFAULT_NOCTAVE_LAYERS / 2.0;
			detector = new SiftFeatureDetector(thresh, edgeThreshold);
			descriptorExtractor = new SiftDescriptorExtractor();
			hamming=false;
			break;
		case T_SURF:
			threshold = 400;
			detector = new SurfFeatureDetector(threshold);
			descriptorExtractor = new SurfDescriptorExtractor();
			hamming=false;
			break;
		case T_BRIEF:
			threshold = 30;
			detector = new FastFeatureDetector(threshold,true);
			descriptorExtractor = new BriefDescriptorExtractor(64);
			break;
		case T_BRISK:
			//detector = new BriskFeatureDetector(30,0);


			detector = new BriskFeatureDetector(60,2);

			//choose BRISK type [AGAST]
			switch(brisk_type){
				case TYPE_RS:
					descriptorExtractor = new BriskDescriptorExtractor();
					break;
				case TYPE_U:
					descriptorExtractor = new BriskDescriptorExtractor(false);
					break;
				case TYPE_S:
					descriptorExtractor = new BriskDescriptorExtractor(false, false);
					break;
				case TYPE_US:
					descriptorExtractor = new BriskDescriptorExtractor(true, false);
					break;
			}
			break;
	}


	QueryPerformanceCounter(&start);
	
	// run the detector
	detector->detect(imgGray1,keypoints1);
	
	QueryPerformanceCounter(&finish);
	timeSec = (finish.QuadPart - start.QuadPart) / (double)ticksPerSec.QuadPart;
	printf("-- Test ---------------------------------------------\n");
	printf("Detection : %f ms\n", (float)timeSec*1000);
	printf("Detection : %f ms\n", (float)timeSec*1000 / (keypoints1.size()+keypoints2.size()));

	QueryPerformanceCounter(&start);

	//std::vector<Mat> cont_patchDesc;
	//patchWarp(imgGray1, cont_patchDesc, keypoints1[300], 20);
	
	// get the descriptors
	Mat descriptors1, descriptors2;
	descriptorExtractor->compute(imgGray1,keypoints1,descriptors1);
	//descriptorExtractor->compute(imgGray2,keypoints2,descriptors2);

	QueryPerformanceCounter(&finish);	
	timeSec = (finish.QuadPart - start.QuadPart) / (double)ticksPerSec.QuadPart;
	printf("Description : %f ms\n", (float)timeSec*1000);
	printf("Description : %f ms\n", (float)timeSec*1000 / (keypoints1.size()+keypoints2.size()));

	
		// matching
	std::vector<std::vector<DMatch> > matches;
	/*Ptr<DescriptorMatcher> descriptorMatcher;
	if(hamming)
		descriptorMatcher = new BruteForceMatcher<Hamming>(); 
	else
		descriptorMatcher = new BruteForceMatcher<L2<float> >();

	if(hamming)
		descriptorMatcher->radiusMatch(descriptors2,descriptors1,matches,100.0);
	else
		descriptorMatcher->radiusMatch(descriptors2,descriptors1,matches,0.21);*/

	

	Mat outimg;
	//check with homography ground truth
	//MatchVerifier< KeyPoint, GroundTruth > verif(homofile.c_str(), 2);
	//float rr = verif.getRecognitionRate(keypoints1, keypoints2, matches, &cont_homo[0]);	
	//float rr = verif.getRecognitionRate(keypoints1, keypoints2, matches, NULL);
	//printf("Got %.2f%% of %i retrieved matches right\n", rr*100, (int)matches.size());


	namedWindow("Matches");

	

	while(1){

		capture >> imgRGB2;
		cvtColor(imgRGB2,imgGray2, CV_BGR2GRAY);


		QueryPerformanceCounter(&start);

		detector->detect(imgGray2,keypoints2);

		QueryPerformanceCounter(&finish);
		timeSec = (finish.QuadPart - start.QuadPart) / (double)ticksPerSec.QuadPart;
		printf("-- Test ---------------------------------------------\n");
		printf("Detection : %f ms\n", (float)timeSec*1000);
		printf("Detection : %f ms\n", (float)timeSec*1000 / (keypoints1.size()+keypoints2.size()));


		QueryPerformanceCounter(&start);

		descriptorExtractor->compute(imgGray2,keypoints2,descriptors2);
		QueryPerformanceCounter(&finish);	
		timeSec = (finish.QuadPart - start.QuadPart) / (double)ticksPerSec.QuadPart;
		printf("Description : %f ms\n", (float)timeSec*1000);
		printf("Description : %f ms\n", (float)timeSec*1000 / (keypoints1.size()+keypoints2.size()));


		QueryPerformanceCounter(&start);


		BDHamming::commonRadiusMatch(descriptors2,descriptors1,matches,100.0);

		QueryPerformanceCounter(&finish);
		timeSec = (finish.QuadPart - start.QuadPart) / (double)ticksPerSec.QuadPart;
		printf("Match : %f ms\n", (float)timeSec*1000 / (keypoints1.size()+keypoints2.size()));



		// drawing
		drawMatches(imgRGB2, keypoints2, imgRGB1, keypoints1, matches,outimg,
			Scalar(0,255,0), Scalar(0,0,255),
			std::vector<std::vector<char> >(), DrawMatchesFlags::DRAW_RICH_KEYPOINTS );

		imshow("Matches", outimg);
			waitKey(1);


			keypoints2.clear();
			matches.clear();

			
			
	}

	//QueryPerformanceCounter(&start);


	

	



}


int main(int argc, char ** argv) {

// 	descriptorTest(BRISK, TYPE_U, "graf/img1.bmp", "graf/img2.bmp", "graf/H1to2p");
// 	descriptorTest(BRISK, TYPE_U, "graf/img1.bmp", "graf/img3.bmp", "graf/H1to3p");
// 	descriptorTest(BRISK, TYPE_U, "graf/img1.bmp", "graf/img4.bmp", "graf/H1to4p");
// 	descriptorTest(BRISK, TYPE_U, "graf/img1.bmp", "graf/img5.bmp", "graf/H1to5p");
// 	descriptorTest(BRISK, TYPE_U, "graf/img1.bmp", "graf/img6.bmp", "graf/H1to6p");

	//descriptorTest(BRISK, TYPE_RS, "wall/img1.bmp", "wall/img2.bmp", "wall/H1to2p");
	//descriptorTest(BRISK, TYPE_RS, "100200.jpg", "100202.jpg", "wall/H1to1p");
// 	descriptorTest(BRISK, TYPE_RS, "wall/img1.bmp", "wall/img3.bmp", "wall/H1to3p");
// 	descriptorTest(BRISK, TYPE_RS, "wall/img1.bmp", "wall/img4.bmp", "wall/H1to4p");
// 	descriptorTest(BRISK, TYPE_RS, "wall/img1.bmp", "wall/img5.bmp", "wall/H1to5p");
	
	
	//descriptorTest(T_BRISK, TYPE_RS, "wall/img1.bmp", "wall/img6.bmp", "wall/H1to6p");


	descriptorCamTest(T_BRISK, TYPE_RS, "wall/img1.bmp");


// 	descriptorTest(SURF, TYPE_RS, "wall/img1.bmp", "wall/img2.bmp", "wall/H1to2p");
// 	descriptorTest(SURF, TYPE_RS, "wall/img1.bmp", "wall/img3.bmp", "wall/H1to3p");
// 	descriptorTest(SURF, TYPE_RS, "wall/img1.bmp", "wall/img4.bmp", "wall/H1to4p");
// 	descriptorTest(SURF, TYPE_RS, "wall/img1.bmp", "wall/img5.bmp", "wall/H1to5p");
// 	descriptorTest(SURF, TYPE_RS, "wall/img1.bmp", "wall/img6.bmp", "wall/H1to6p");

// 	descriptorTest(BRIEF, TYPE_S, "wall/img1.bmp", "wall/img2.bmp", "wall/H1to2p");
// 	descriptorTest(BRIEF, TYPE_S, "wall/img1.bmp", "wall/img3.bmp", "wall/H1to3p");
// 	descriptorTest(BRIEF, TYPE_S, "wall/img1.bmp", "wall/img4.bmp", "wall/H1to4p");
// 	descriptorTest(BRIEF, TYPE_S, "wall/img1.bmp", "wall/img5.bmp", "wall/H1to5p");
// 	descriptorTest(BRIEF, TYPE_S, "wall/img1.bmp", "wall/img6.bmp", "wall/H1to6p");
	
	/*if(argc==1){
		detector = new BriskFeatureDetector(60,4);
	}
	else{
		if(strncmp("FAST", argv[3], 4 )==0){
  			threshold = atoi(argv[3]+4);
			if(threshold==0)
				threshold = 30;
			detector = new FastFeatureDetector(threshold,true);
		}
		else if(strncmp("AGAST", argv[3], 5 )==0){
			threshold = atoi(argv[3]+5);
			if(threshold==0)
				threshold = 30;
			detector = new BriskFeatureDetector(threshold,0);
		}
		else if(strncmp("BRISK", argv[3], 5 )==0){
			threshold = atoi(argv[3]+5);
			if(threshold==0)
				threshold = 30;
			detector = new BriskFeatureDetector(threshold,4);
		}
		else if(strncmp("SURF", argv[3], 4 )==0){
			threshold = atoi(argv[3]+4);
			if(threshold==0)
				threshold = 400;
			detector = new SurfFeatureDetector(threshold);
		}
		else if(strncmp("SIFT", argv[3], 4 )==0){
			float thresh = 0.04 / SIFT::CommonParams::DEFAULT_NOCTAVE_LAYERS / 2.0;
			float edgeThreshold=atof(argv[3]+4);
			if(edgeThreshold==0)
				thresh = 10.0;
			detector = new SiftFeatureDetector(thresh,edgeThreshold);
		}
		else{
			detector = FeatureDetector::create( argv[3] );
		}
		if (detector.empty()){
			std::cout << "Detector " << argv[3] << " not recognized. Check spelling!" << std::endl;
			return 3;
		}
	}*/

	// now the extractor:
	/*bool hamming=true;
	Ptr<DescriptorExtractor> descriptorExtractor;
	// now the extractor:
	if(argc==1){
		descriptorExtractor = new BriskDescriptorExtractor();
	}
	else{
		if(std::string(argv[4])=="BRISK"){
			descriptorExtractor = new BriskDescriptorExtractor();
		}
		else if(std::string(argv[4])=="U-BRISK"){
			descriptorExtractor = new BriskDescriptorExtractor(false);
		}
		else if(std::string(argv[4])=="SU-BRISK"){
			descriptorExtractor = new BriskDescriptorExtractor(false,false);
		}
		else if(std::string(argv[4])=="S-BRISK"){
			descriptorExtractor = new BriskDescriptorExtractor(true,false);
		}
		else if(std::string(argv[4])=="BRIEF"){
			descriptorExtractor = new BriefDescriptorExtractor(64);
		}
		else if(std::string(argv[4])=="CALONDER"){
			descriptorExtractor = new CalonderDescriptorExtractor<float>("current.rtc");
			hamming=false;
		}
		else if(std::string(argv[4])=="SURF"){
			descriptorExtractor = new SurfDescriptorExtractor();
			hamming=false;
		}
		else if(std::string(argv[4])=="SIFT"){
			descriptorExtractor = new SiftDescriptorExtractor();
			hamming=false;
		}
		else{
			descriptorExtractor = DescriptorExtractor::create( argv[4] );
		}
		if (descriptorExtractor.empty()){
			hamming=false;
			std::cout << "Descriptor " << argv[4] << " not recognized. Check spelling!" << std::endl;
			return 4;
		}
	}*/

	return 0;
}
