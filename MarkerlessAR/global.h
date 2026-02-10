#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "wonjo.h"
// OpenCV headers updated for OpenCV 4.x
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
// OpenCV C API backward compatibility (legacy types: IplImage, CvMat, CvPoint, etc.)
#include <opencv2/core/core_c.h>
#include <opencv2/core/types_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>
// CvKalman stub (removed from OpenCV 4)
#include "compat/cvkalman_stub.h"
// cvFindHomography / cvRodrigues2 wrappers (removed from OpenCV 4)
#include "compat/opencv_compat.h"
// OpenGL/GLUT headers — macOS paths
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <stdio.h>
#include <time.h>
#include "calibration/cvFindExtrinsicCameraParams3.h"

// Running Options
#define PROCESS_320x240
#define OPENGL_LOOP
//#define ARTAG

// For Experiments
#define TRANSLATE_COORDINATE
#define ELLIPSE_FITTING

// For Capture headers
//#define POINTGREY_CAPTURE

#endif // _GLOBAL_H_
