#ifndef _CAPTURE_H_
#define _CAPTURE_H_

// IN ORDER TO USE POINTGREY CAMERAS, UNCOMMENT THE BELOW LINE.
//#define POINTGREY_CAPTURE

#include "cv.h"
#include "highgui.h"

#ifdef POINTGREY_CAPTURE
#include "FlyCap.h"
#endif

#define CAPTURE_NONE        0
#define CAPTURE_OPENCV      1
#define CAPTURE_POINTGREY   2

#define CAPTURE_FLIP        true
#define CAPTURE_DONT_FLIP   false

class Capture
{
public:
    Capture(void);
    ~Capture(void);

    bool Initialize( bool flip = CAPTURE_DONT_FLIP, int index = -1, char * filename = 0 );
    void Terminate();

    bool        CaptureFrame();
    IplImage *  QueryFrame();

    int64       QueryTickCount();

private:

    bool        _fInitialized;

    IplImage *  _pFrame;

    int         _CaptureMethod;
    CvCapture * _pCaptureOpenCV;
#ifdef POINTGREY_CAPTURE
    CFlyCap *   _pCaptureFly;
#endif

    int64       _nTickCount;

    bool        _fFlipVertical;
};

#endif // _CAPTURE_H_
