#include "Capture.h"
#include "../compat/synthetic_marker_source.h"
#include <opencv2/imgproc.hpp>
#ifndef _WIN32
#include <thread>
#include <atomic>
#include <chrono>
#endif

#ifdef __APPLE__
#include "../compat/macos_av_capture.h"
static AVCap* g_avcap = nullptr;
#endif

Capture::Capture(void)
{
    _fInitialized = false;

#ifdef POINTGREY_CAPTURE
    _pCaptureFly = 0;
#endif

    _pFrame = 0;
    _pSynthetic = 0;
    _CaptureMethod = CAPTURE_NONE;

    _fFlipVertical = false;
}

bool Capture::InitializeSynthetic( const char * markerFilename )
{
    if ( !markerFilename )
        return false;

    return InitializeSynthetic( std::vector<std::string>( 1, markerFilename ) );
}

bool Capture::InitializeSynthetic( const std::vector<std::string>& markerFilenames )
{
    if ( _fInitialized || markerFilenames.empty() )
        return false;

    _pSynthetic = new SyntheticMarkerSource();
    if ( !_pSynthetic->Initialize( markerFilenames, 640, 480 ) )
    {
        delete _pSynthetic;
        _pSynthetic = 0;
        return false;
    }

    _CaptureMethod = CAPTURE_SYNTHETIC;
    _fFlipVertical = false;
    _fInitialized = CaptureFrame();
    if ( !_fInitialized )
    {
        delete _pSynthetic;
        _pSynthetic = 0;
        _CaptureMethod = CAPTURE_NONE;
    }
    return _fInitialized;
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
        _vcap.open( filename );
        if ( _vcap.isOpened() )
        {
            _CaptureMethod = CAPTURE_OPENCV;
            goto Initialized;
        }
    }

    //
    // Try capture from camera using OpenCV (cv::VideoCapture)
    //
    {
        int camIndex = (index < 0) ? 0 : index;
        fprintf(stderr, "Capture: opening camera index %d...\n", camIndex);
        fflush(stderr);

#ifdef __APPLE__
        // Use native AVFoundation capture (modern AVCaptureDeviceDiscoverySession
        // — includes Continuity Camera, which OpenCV's deprecated AVFoundation
        // path doesn't enumerate). The index here matches PickCameraIndex's
        // numbering since both use the same discovery enumeration.
        g_avcap = AVCap_Open(camIndex, 640, 480);
        if (g_avcap) {
            int aw = 0, ah = 0;
            // Wait briefly for the first frame so actual size is known.
            for (int i = 0; i < 50 && (aw == 0 || ah == 0); ++i) {
                AVCap_GetActualSize(g_avcap, &aw, &ah);
                if (aw && ah) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            fprintf(stderr, "Capture: AVCap opened (native %dx%d)\n", aw, ah);
            fflush(stderr);
            _CaptureMethod = CAPTURE_OPENCV;  // reuse the existing branch
            goto Initialized;
        }
        fprintf(stderr, "Capture: AVCap_Open failed for index %d\n", camIndex);
        fflush(stderr);
#else
        _vcap.open( camIndex );

        if ( _vcap.isOpened() )
        {
            int aw = (int)_vcap.get(cv::CAP_PROP_FRAME_WIDTH);
            int ah = (int)_vcap.get(cv::CAP_PROP_FRAME_HEIGHT);
            fprintf(stderr, "Capture: camera index=%d opened (native %dx%d, backend=%s)\n",
                    camIndex, aw, ah, _vcap.getBackendName().c_str());
            fflush(stderr);
            _vcap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
            _vcap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
            _CaptureMethod = CAPTURE_OPENCV;
            goto Initialized;
        }
        fprintf(stderr, "Capture: failed to open camera %d\n", camIndex);
        fflush(stderr);
#endif
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

#ifdef __APPLE__
bool Capture::SwitchCamera(int newIndex)
{
    if (g_avcap) { AVCap_Close(g_avcap); g_avcap = nullptr; }
    g_avcap = AVCap_Open(newIndex, 640, 480);
    if (!g_avcap) {
        fprintf(stderr, "Capture::SwitchCamera: AVCap_Open(%d) failed\n", newIndex);
        return false;
    }
    int aw = 0, ah = 0;
    for (int i = 0; i < 50 && (aw == 0 || ah == 0); ++i) {
        AVCap_GetActualSize(g_avcap, &aw, &ah);
        if (aw && ah) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    fprintf(stderr, "Capture::SwitchCamera: now using index %d (native %dx%d)\n",
            newIndex, aw, ah);
    return true;
}
#endif

void Capture::Terminate()
{
#ifdef __APPLE__
    if (g_avcap) { AVCap_Close(g_avcap); g_avcap = nullptr; }
#endif
    if ( _vcap.isOpened() )
    {
        _vcap.release();
    }

    if ( _pSynthetic )
    {
        delete _pSynthetic;
        _pSynthetic = 0;
    }

#ifdef POINTGREY_CAPTURE
    if ( _pCaptureFly )
    {
        delete _pCaptureFly;
        _pCaptureFly = 0;
    }
#endif

    _pFrame = 0;
    _fInitialized = false;
    _CaptureMethod = CAPTURE_NONE;
}

bool Capture::CaptureFrame()
{
    switch ( _CaptureMethod )
    {
    case CAPTURE_SYNTHETIC:
        if ( !_pSynthetic || !_pSynthetic->NextFrame( _matFrame ) )
            return false;
        _iplHeader = cvIplImage( _matFrame );
        _pFrame = &_iplHeader;
        break;

    case CAPTURE_OPENCV:
#ifdef __APPLE__
        if (g_avcap) {
            int aw = 0, ah = 0;
            AVCap_GetActualSize(g_avcap, &aw, &ah);
            if (aw == 0 || ah == 0) return false;
            // Allocate native-size buffer + copy from AVF.
            _matFrame.create(ah, aw, CV_8UC3);
            if (!AVCap_GetLatestBGR(g_avcap, _matFrame.data, aw, ah)) {
                // No fresh frame yet — keep the previous IplImage view valid.
                if (_pFrame) return true;
                return false;
            }
            // Resize to 640x480 if the camera delivers a different size
            // (Continuity Camera, USB cams, etc.).
            if (_matFrame.cols != 640 || _matFrame.rows != 480) {
                cv::resize(_matFrame, _matFrameResized, cv::Size(640, 480), 0, 0, cv::INTER_AREA);
                _iplHeader = cvIplImage(_matFrameResized);
            } else {
                _iplHeader = cvIplImage(_matFrame);
            }
            _pFrame = &_iplHeader;
            break;
        }
        // fall through to cv::VideoCapture path if AVCap unavailable
#endif
        if ( _vcap.isOpened() )
        {
            _vcap >> _matFrame;
            if ( _matFrame.empty() )
            {
                fprintf(stderr, "Capture: frame is empty (camera may need warmup)\n");
                return false;
            }
            if ( _matFrame.cols != 640 || _matFrame.rows != 480 )
            {
                cv::resize( _matFrame, _matFrameResized, cv::Size(640, 480), 0, 0, cv::INTER_AREA );
                _iplHeader = cvIplImage(_matFrameResized);
            }
            else
            {
                _iplHeader = cvIplImage(_matFrame);
            }
            _pFrame = &_iplHeader;
        }
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
