/*
Copyright 2010 Computer Vision Lab,
Ecole Polytechnique Federale de Lausanne (EPFL), Switzerland.
All rights reserved.

Author: Michael Calonder (http://cvlab.epfl.ch/~calonder)
Version: 1.0

This file is part of the BRIEF DEMO software.

BRIEF DEMO is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

BRIEF DEMO is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
BRIEF DEMO; if not, write to the Free Software Foundation, Inc., 51 Franklin
Street, Fifth Floor, Boston, MA 02110-1301, USA
*/

#pragma once
#include <opencv2/features2d/features2d.hpp>
#include <vector>
using std::vector;

template<typename T>
T* readMat(const std::string& name, T* &mat, int m, int n)
{
	std::ifstream ifs(name.c_str(), std::ios_base::in);
	if (!ifs.is_open()) printf("read_mat: cannot read %s\n", name);

	if (mat == NULL) mat = new T[m*n];
	for (int i=0; i<m*n; i++) {
		if (ifs.eof()) printf("Attempt to read beyond EOF in %s\n", name);
		ifs >> mat[i];
	}

	return mat;
}

template<typename T>
void writeMat(const std::string& name, T* mat, int m, int n, std::string fmt="", bool append=false)
{
	if (fmt.empty()) fmt = numToStr_trait< T >::getFormat();

	std::ofstream ofs(name.c_str(), (append ? std::ios::app : std::ios::out));
	if (!ofs.good()) {
		printf("Cannot open %s for output\n", name);
		return;
	}

	char buf[1024];
	std::string tmp;
	for (int i=0; i<m; i++) {
		tmp = "";
		for (int j=0; j<n; j++) {
			sprintf(buf, fmt.c_str(), mat[i*n+j]);
			tmp += std::string(buf) + " ";
		}
		ofs << tmp.substr(0, tmp.length()-1) << std::endl;
	}

	ofs.close();
}

template< typename KeypointT >
static void findHomography(const vector< KeypointT >& left, const vector< KeypointT >& right, double* mat)
{
	if(mat == NULL)
		mat = new double[9];

	CvMat* matLeft = cvCreateMat(left.size(), 2, CV_32FC1);
	CvMat* matRight = cvCreateMat(left.size(), 2, CV_32FC1);
	CvMat* matHomo = cvCreateMat(3, 3, CV_32FC1);

	for(int i=0; i<left.size(); i++){
		cvmSet(matLeft, i, 0, left[i].x);	cvmSet(matLeft, i, 1, left[i].y);
		cvmSet(matRight, i, 0, right[i].x);	cvmSet(matRight, i, 1, right[i].y);
	}

	//get homography with least square method
	cvFindHomography(matLeft, matRight, matHomo);

	//copy and print
	for(int y=0; y<3; y++){
		for(int x=0; x<3; x++){
			mat[y*3+x] = cvmGet(matHomo, y, x);
			printf("%f ", mat[y*3+x] );
		}
		printf("\n");
	}

	cvReleaseMat(&matLeft);
	cvReleaseMat(&matRight);
}

template< typename KeypointT >
class GroundTruth
{
public:
	// Assumes standard naming conventions for image and homography files
	GroundTruth(){
		homographies_ = NULL;
	}
	GroundTruth(const char* filehomo)
	{
		homographies_ = NULL;
		readHomography(filehomo);
	}

	~GroundTruth(){
		if(homographies_ != NULL)
			delete[] homographies_;
	}

	void cal_homographyLS(const std::vector< KeypointT > match_left, const std::vector< KeypointT > match_right){

		findHomography(match_left, match_right, homographies_ );
	}

	void readHomography(const char* file){

		std::string url = file;

		if(homographies_ == NULL)
			homographies_ = new double[9];

		readMat(url, homographies_, 3, 3);
	}

	// Returns the corresponding image point in the right image (with index ix)
	void transform(const KeypointT& left, KeypointT& right)
	{
		// Project point from left to right image
		double x[3] = {left.pt.x, left.pt.y, 1};
		double y[3];

		memset(y, 0, 3*sizeof(y[0]));

		// 3x3 matrix-vector multiply
		double *H = homographies_;
		y[0] = H[0]*x[0] + H[1]*x[1] + H[2]*x[2];
		y[1] = H[3]*x[0] + H[4]*x[1] + H[5]*x[2];
		y[2] = H[6]*x[0] + H[7]*x[1] + H[8]*x[2];
		right.pt.x = cvRound(y[0]/y[2]);
		right.pt.y = cvRound(y[1]/y[2]);
	}

	void transform(const KeypointT& left, KeypointT& right, cv::Mat homo)
	{
		// Project point from left to right image
		double x[3] = {left.pt.x, left.pt.y, 1};
		double y[3];

		memset(y, 0, 3*sizeof(y[0]));

		// 3x3 matrix-vector multiply
		y[0] = homo.at<double>(0, 0)*x[0] + homo.at<double>(0, 1)*x[1] + homo.at<double>(0, 2)*x[2];
		y[1] = homo.at<double>(1, 0)*x[0] + homo.at<double>(1, 1)*x[1] + homo.at<double>(1, 2)*x[2];
		y[2] = homo.at<double>(2, 0)*x[0] + homo.at<double>(2, 1)*x[1] + homo.at<double>(2, 2)*x[2];
		right.pt.x = cvRound(y[0]/y[2]);
		right.pt.y = cvRound(y[1]/y[2]);
	}

private:
	double* homographies_;     // One homography for each dataset
};
