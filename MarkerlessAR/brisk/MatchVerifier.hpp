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

#ifndef MATCHVERIFIER_HPP
#define MATCHVERIFIER_HPP

#include <opencv2/features2d/features2d.hpp>

#define SQR(a) ((a)*(a))

template< typename KeypointT, template <typename KeypointT> class GroundTruthT >
struct MatchVerifier
{
	// Expects a path to ground truth data in GroundTruthT-style and a tolerance
	// value in pixels

	MatchVerifier(){}

	MatchVerifier(const char* file, double tol)
	{
		tol_ = tol;
		gt_.readHomography(file);
	}

	void cal_homography(const std::vector< KeypointT > match_left, const std::vector< KeypointT > match_right){

		gt_.cal_homographyLS(match_left, match_right);
	}

	// Returns if the specified points are a match
	bool isMatch(const KeypointT& left, const KeypointT& right, cv::Mat* homo = NULL)
	{
		bool res;
		if(homo != NULL)
		{	
			KeypointT src;
			cv::Mat invhomo = homo->inv();
			gt_.transform(left, src, invhomo);

			KeypointT r;
			gt_.transform(src, r);

			// L1 tolerance
			res = (SQR(r.pt.x - right.pt.x) <= tol_*tol_) && 
				(SQR(r.pt.y - right.pt.y) <= tol_*tol_);

		}
		else{
			KeypointT r;
			gt_.transform(left, r);

			// L1 tolerance
			res = (SQR(r.pt.x - right.pt.x) <= tol_*tol_) && 
				(SQR(r.pt.y - right.pt.y) <= tol_*tol_);
		}

		return res;
	}

	// Computes the fraction of correct matches
	float getRecognitionRate(const std::vector< KeypointT > match_left, 
		const std::vector< KeypointT > match_right )
	{
		assert(match_left.size() == match_right.size());

		int ok = 0;
		for (int i=0; i<(int)match_left.size(); ++i)
			ok += (int)isMatch(match_left[i], match_right[i]);

		return float(ok)/match_left.size();
	}

	float getRecognitionRate(	const vector<KeypointT>& keypoints1, 
								const vector<KeypointT>& keypoints2,
								const vector<vector<cv::DMatch> >& matches1to2, 
								cv::Mat* homo = NULL)
	{
		int ok = 0;
		int matchCnt = 0;
		// draw matches
		for( size_t i = 0; i < matches1to2.size(); i++ )
		{
			for( size_t j = 0; j < matches1to2[i].size(); j++ )
			{
				int i1 = matches1to2[i][j].queryIdx;
				int i2 = matches1to2[i][j].trainIdx;

				const KeypointT &kp2 = keypoints2[i1], &kp1 = keypoints1[i2];
				ok += (int)isMatch(kp1, kp2, homo);
				matchCnt++;
				break;
			}
		}
		return float(ok)/matchCnt;
	}

	inline const GroundTruthT< KeypointT >& getGroundTruth() const
	{
		return const_cast< const GroundTruthT< KeypointT >& >(gt_);
	}

private:
	GroundTruthT< KeypointT > gt_;
	double tol_;

};

#endif