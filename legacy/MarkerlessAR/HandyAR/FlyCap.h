#ifndef _FLY_CAP_H_
#define _FLY_CAP_H_

//=============================================================================
// PGR Includes
//=============================================================================
#include <PGRFlyCapture.h>
#include <PGRFlyCapturePlus.h>

// OpenCV
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>

class CFlyCap
{
public:
    CFlyCap(void);
    ~CFlyCap(void);

    bool Initialize( int index = -1 );
    IplImage * Capture();
    void Terminate();

private:
    //
    // FlyCapture data
    //
    FlyCaptureContext   _FlyCaptureContext;
    FlyCaptureImage     _ImageRaw;
    FlyCaptureImage     _ImageProcessed;
    FlyCaptureInfoEx    _CameraInfo;
    int                 _nCameraBusIndex;

    //
    // IplImage data
    IplImage *          _pFrame;
};

#endif  // _FLY_CAP_H_
