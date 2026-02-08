#include "Capture.h"

Capture::Capture(void)
{
    _fInitialized = false;
    
    _pCaptureOpenCV = 0;
#ifdef POINTGREY_CAPTURE
    _pCaptureFly = 0;
#endif

    _pFrame = 0;

    _fFlipVertical = false;
}

Capture::~Capture(void)
{
    Terminate();
}

bool Capture::Initialize( bool flip, int index, char * filename )
{
    if ( _fInitialized )
    {
        goto Finished;
    }

    _fFlipVertical = flip;

    //
    // Try capture from file
    //
    if ( filename )
    {
        _pCaptureOpenCV = cvCreateFileCapture( filename );
        if ( _pCaptureOpenCV )
        {
            _CaptureMethod = CAPTURE_OPENCV;
            goto Initialized;
        }
    }

    //
    // Try capture from camera using OpenCV
    //
    _pCaptureOpenCV = cvCreateCameraCapture( index );
    if ( _pCaptureOpenCV )
    {
        _CaptureMethod = CAPTURE_OPENCV;
        goto Initialized;
    }

#ifdef POINTGREY_CAPTURE
    //
    // Try capture from camera using PointGrey
    //
    _pCaptureFly = new CFlyCap();
    if ( _pCaptureFly->Initialize( index ) )
    {
        _CaptureMethod = CAPTURE_POINTGREY;
        goto Initialized;
    }
#endif

    //
    // No more attempt, failure
    //
    _fInitialized = false;
    goto Finished;

Initialized:
    //
    // If successfully initialized, try first capture
    //
    _fInitialized = CaptureFrame();

Finished:
    return _fInitialized;
}

void Capture::Terminate()
{
    if ( _pCaptureOpenCV )
    {
        cvReleaseCapture( &_pCaptureOpenCV );
        _pCaptureOpenCV = 0;
    }

#ifdef POINTGREY_CAPTURE
    if ( _pCaptureFly )
    {
        delete _pCaptureFly;
        _pCaptureFly = 0;
    }
#endif

    _fInitialized = false;
}

bool Capture::CaptureFrame()
{
    switch ( _CaptureMethod )
    {
    case CAPTURE_OPENCV:
        if ( _pCaptureOpenCV )
            _pFrame = cvQueryFrame( _pCaptureOpenCV );
        else
            return false;
        break;
        
#ifdef POINTGREY_CAPTURE
    case CAPTURE_POINTGREY:
        _pFrame = _pCaptureFly->Capture();
        break;
#endif

    default:
        break;
    }

    _nTickCount = cvGetTickCount();

    if ( _pFrame == 0 )
    {
        return false;
    }

    if ( _fFlipVertical )
    {
        cvFlip( _pFrame, 0, 0 );
    }

    return true;
}

IplImage * Capture::QueryFrame()
{
    return _pFrame;
}

int64 Capture::QueryTickCount()
{
    return _nTickCount;
}
