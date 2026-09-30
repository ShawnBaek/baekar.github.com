#include <opencv2/opencv.hpp>
#include "brisk/brisk.h"
#include "brisk/Matcher.h"
#include "brisk/MatchVerifier.hpp"
#include "brisk/GroundTruth.hpp"

class Detector
{
	
	public :Detector();

			~Detector();
			

	private : Ptr<FeatureDetector> detector;


};
