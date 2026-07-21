#ifndef _CAPTURE_H_
#define _CAPTURE_H_

// IN ORDER TO USE POINTGREY CAMERAS, UNCOMMENT THE BELOW LINE.
//#define POINTGREY_CAPTURE

#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/videoio.hpp>

#include <string>
#include <vector>

#ifdef POINTGREY_CAPTURE
#include "FlyCap.h"
#endif

#define CAPTURE_NONE        0
#define CAPTURE_OPENCV      1
#define CAPTURE_POINTGREY   2
#define CAPTURE_SYNTHETIC   3

class SyntheticMarkerSource;

#define CAPTURE_FLIP        true
#define CAPTURE_DONT_FLIP   false

class Capture
{
public:
    Capture(void);
    ~Capture(void);

    bool Initialize( bool flip = CAPTURE_DONT_FLIP, int index = -1, char * filename = 0 );
    bool InitializeSynthetic( const char * markerFilename );
    bool InitializeSynthetic( const std::vector<std::string>& markerFilenames );
    void Terminate();

    bool        CaptureFrame();
    IplImage *  QueryFrame();

#ifdef __APPLE__
    // Hot-swap the AVFoundation device. Index follows the same enumeration
    // as PickCameraIndex / AVCap_Open. Safe to call from the main thread
    // while the matching/tracking workers are running — the new session
    // simply takes over feeding _matFrame.
    bool        SwitchCamera(int newIndex);
#endif

    int64       QueryTickCount();

private:

    bool        _fInitialized;

    IplImage *  _pFrame;
    cv::VideoCapture _vcap;
    cv::Mat     _matFrame;
    cv::Mat     _matFrameResized;  // 640x480 view used when camera delivers a different size
    IplImage    _iplHeader;        // IplImage header wrapping _matFrame{,Resized}
    SyntheticMarkerSource * _pSynthetic;

    int         _CaptureMethod;
#ifdef POINTGREY_CAPTURE
    CFlyCap *   _pCaptureFly;
#endif

    int64       _nTickCount;

    bool        _fFlipVertical;
};

#endif // _CAPTURE_H_
