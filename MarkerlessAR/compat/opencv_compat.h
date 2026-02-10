// opencv_compat.h — Compatibility wrappers for OpenCV C API functions
// removed in OpenCV 4.x
#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/core/core_c.h>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgcodecs/legacy/constants_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include "cvkalman_stub.h"

// --- CV_RGB returns cv::Scalar in OpenCV 4, but C API functions expect CvScalar ---
// Override to return CvScalar for backward compatibility with cvCircle, cvLine, etc.
#ifdef CV_RGB
#undef CV_RGB
#endif
#define CV_RGB(r, g, b) cvScalar((b), (g), (r), 0)

// --- Old error-handling macro aliases (removed in OpenCV 4) ---
// OpenCV 4 still has __CV_BEGIN__ / __CV_END__ but the short aliases were dropped.
#ifndef __BEGIN__
#define __BEGIN__  __CV_BEGIN__
#endif
#ifndef __END__
#define __END__    __CV_END__
#endif

// --- cvFindHomography (removed in OpenCV 4) ---
inline int cvFindHomography(const CvMat* srcPoints, const CvMat* dstPoints, CvMat* homography,
                            int method = 0, double ransacReprojThreshold = 3, CvMat* mask = 0)
{
    cv::Mat src = cv::cvarrToMat(srcPoints);
    cv::Mat dst = cv::cvarrToMat(dstPoints);
    cv::Mat H = cv::findHomography(src, dst, method, ransacReprojThreshold);
    if (H.empty()) return 0;
    cv::Mat homoDst = cv::cvarrToMat(homography);
    H.convertTo(homoDst, homoDst.type());
    return 1;
}

// --- cvRodrigues2 (removed in OpenCV 4) ---
inline void cvRodrigues2(const CvMat* src, CvMat* dst, CvMat* jacobian = 0)
{
    cv::Mat srcMat = cv::cvarrToMat(src);
    cv::Mat dstOut;
    if (jacobian) {
        cv::Mat jacOut;
        cv::Rodrigues(srcMat, dstOut, jacOut);
        cv::Mat jacDst = cv::cvarrToMat(jacobian);
        jacOut.convertTo(jacDst, jacDst.type());
    } else {
        cv::Rodrigues(srcMat, dstOut);
    }
    cv::Mat dstWrap = cv::cvarrToMat(dst);
    dstOut.convertTo(dstWrap, dstWrap.type());
}

// --- cvConvertPointsHomogenious (removed in OpenCV 4, was also misspelled) ---
inline void cvConvertPointsHomogenious(const CvMat* src, CvMat* dst)
{
    cv::Mat srcMat = cv::cvarrToMat(src);
    cv::Mat dstMat = cv::cvarrToMat(dst);
    // convertPointsHomogeneous: add or remove homogeneous coordinate
    if (srcMat.channels() > dstMat.channels()) {
        cv::convertPointsFromHomogeneous(srcMat.reshape(srcMat.channels(), srcMat.rows * srcMat.cols),
                                         dstMat);
    } else {
        cv::convertPointsToHomogeneous(srcMat.reshape(srcMat.channels(), srcMat.rows * srcMat.cols),
                                       dstMat);
    }
}

// --- cvProjectPoints2 (removed in OpenCV 4) ---
inline void cvProjectPoints2(const CvMat* objectPoints, const CvMat* r_vec, const CvMat* t_vec,
                             const CvMat* A, const CvMat* distCoeffs, CvMat* imagePoints,
                             CvMat* dpdr = 0, CvMat* dpdt = 0,
                             CvMat* dpdf = 0, CvMat* dpdc = 0, CvMat* dpddist = 0)
{
    cv::Mat objPts = cv::cvarrToMat(objectPoints);
    cv::Mat rvec = cv::cvarrToMat(r_vec);
    cv::Mat tvec = cv::cvarrToMat(t_vec);
    cv::Mat cameraMatrix = cv::cvarrToMat(A);
    cv::Mat distCoeffsMat = distCoeffs ? cv::cvarrToMat(distCoeffs) : cv::Mat();
    cv::Mat imgPts;
    cv::Mat jacobian;

    // Determine if we need the full jacobian
    bool needJacobian = (dpdr || dpdt || dpdf || dpdc || dpddist);
    cv::projectPoints(objPts, rvec, tvec, cameraMatrix, distCoeffsMat,
                      imgPts, needJacobian ? jacobian : cv::noArray());

    // Copy projected points back
    cv::Mat imgDst = cv::cvarrToMat(imagePoints);
    imgPts.reshape(imgDst.channels(), imgDst.rows).convertTo(imgDst, imgDst.type());

    // Extract jacobian sub-blocks if requested
    // Full jacobian from projectPoints is Nx(3+3+2+2+4/5/8/12/14) = Nx10..14
    if (needJacobian && !jacobian.empty()) {
        int col = 0;
        if (dpdr) {
            cv::Mat dst = cv::cvarrToMat(dpdr);
            jacobian.colRange(col, col + 3).convertTo(dst, dst.type());
        }
        col += 3;
        if (dpdt) {
            cv::Mat dst = cv::cvarrToMat(dpdt);
            jacobian.colRange(col, col + 3).convertTo(dst, dst.type());
        }
        col += 3;
        if (dpdf) {
            cv::Mat dst = cv::cvarrToMat(dpdf);
            jacobian.colRange(col, col + 2).convertTo(dst, dst.type());
        }
        col += 2;
        if (dpdc) {
            cv::Mat dst = cv::cvarrToMat(dpdc);
            jacobian.colRange(col, col + 2).convertTo(dst, dst.type());
        }
        col += 2;
        if (dpddist && distCoeffs) {
            cv::Mat dst = cv::cvarrToMat(dpddist);
            int ncols = std::min(jacobian.cols - col, dst.cols);
            jacobian.colRange(col, col + ncols).convertTo(dst, dst.type());
        }
    }
}

// --- cvFitEllipse (old form removed in OpenCV 4) ---
// Old API: cvFitEllipse(CvPoint2D32f* points, int count, CvBox2D* box)
// New API: cvFitEllipse2 returns CvBox2D (exists in imgproc_c.h)
inline void cvFitEllipse(const CvPoint2D32f* points, int count, CvBox2D* box)
{
    CvMat pointMat = cvMat(1, count, CV_32FC2, const_cast<CvPoint2D32f*>(points));
    *box = cvFitEllipse2(&pointMat);
}

// --- cvSaveImage (removed in OpenCV 4) ---
inline int cvSaveImage(const char* filename, const CvArr* image, const int* params = 0)
{
    cv::Mat mat = cv::cvarrToMat(image);
    return cv::imwrite(filename, mat) ? 1 : 0;
}

// --- cvRandInit (removed in OpenCV 4) ---
// Stub: initializes a CvRandState (also stubbed in cvkalman_stub.h)
inline void cvRandInit(CvRandState* state, double param1, double param2, int seed, int distType = 0)
{
    if (state) {
        state->state = (uint64)seed;
        state->disttype = distType;
        state->param[0] = cvScalar(param1, param1, param1, param1);
        state->param[1] = cvScalar(param2, param2, param2, param2);
    }
}

// --- cvCopyImage (removed in OpenCV 4) ---
// Alias for cvCopy
inline void cvCopyImage(const CvArr* src, CvArr* dst)
{
    cvCopy(src, dst, 0);
}

// --- cvConvertImage (removed in OpenCV 4) ---
// Old API: cvConvertImage(src, dst, flags) — did color swap / flip
#ifndef CV_CVTIMG_SWAP_RB
#define CV_CVTIMG_SWAP_RB 2
#endif
#ifndef CV_CVTIMG_FLIP
#define CV_CVTIMG_FLIP 1
#endif
inline void cvConvertImage(const CvArr* src, CvArr* dst, int flags = 0)
{
    cv::Mat srcMat = cv::cvarrToMat(src);
    cv::Mat dstMat = cv::cvarrToMat(dst);
    if (srcMat.data != dstMat.data)
        srcMat.copyTo(dstMat);
    if (flags & CV_CVTIMG_SWAP_RB) {
        if (dstMat.channels() >= 3)
            cv::cvtColor(dstMat, dstMat, cv::COLOR_BGR2RGB);
    }
    if (flags & CV_CVTIMG_FLIP) {
        cv::flip(dstMat, dstMat, 0);
    }
}

// --- cvLoadImage (removed in OpenCV 4) ---
// Returns a newly allocated IplImage* (caller must cvReleaseImage)
inline IplImage* cvLoadImage(const char* filename, int iscolor = 1)
{
    cv::Mat img = cv::imread(filename, iscolor);
    if (img.empty()) return 0;
    IplImage* ipl = cvCreateImage(cvSize(img.cols, img.rows),
                                  IPL_DEPTH_8U, img.channels());
    cv::Mat dst = cv::cvarrToMat(ipl);
    img.copyTo(dst);
    return ipl;
}

// --- cvFindChessboardCorners (removed in OpenCV 4) ---
inline int cvFindChessboardCorners(const void* image, CvSize pattern_size,
                                   CvPoint2D32f* corners, int* corner_count = 0,
                                   int flags = 0x01 | 0x02 | 0x04)
{
    cv::Mat img = cv::cvarrToMat(image);
    std::vector<cv::Point2f> pts;
    bool found = cv::findChessboardCorners(img, cv::Size(pattern_size.width, pattern_size.height), pts, flags);
    if (corner_count) *corner_count = (int)pts.size();
    for (size_t i = 0; i < pts.size(); i++) {
        corners[i].x = pts[i].x;
        corners[i].y = pts[i].y;
    }
    return found ? 1 : 0;
}

// --- cvDrawChessboardCorners (removed in OpenCV 4) ---
inline void cvDrawChessboardCorners(CvArr* image, CvSize pattern_size,
                                    CvPoint2D32f* corners, int count, int pattern_was_found)
{
    cv::Mat img = cv::cvarrToMat(image);
    std::vector<cv::Point2f> pts(count);
    for (int i = 0; i < count; i++) {
        pts[i] = cv::Point2f(corners[i].x, corners[i].y);
    }
    cv::drawChessboardCorners(img, cv::Size(pattern_size.width, pattern_size.height),
                              pts, pattern_was_found != 0);
}
