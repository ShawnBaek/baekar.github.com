#include "Capture.h"
#include <opencv2/imgproc.hpp>
#ifndef _WIN32
#include <thread>
#include <atomic>
#include <chrono>
#endif

Capture::Capture(void)
{
    _fInitialized = false;

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

#ifndef _WIN32
        // On macOS, VideoCapture::open can hang indefinitely if the camera
        // subsystem is stuck (macOS 26 beta bug, or daemon crash).
        // Use a timeout: try opening in a thread, wait up to 10 seconds.
        {
            std::atomic<bool> openDone{false};
            std::thread([&]() {
                _vcap.open( camIndex );
                openDone = true;
            }).detach();

            auto start = std::chrono::steady_clock::now();
            while (!openDone) {
                auto elapsed = std::chrono::steady_clock::now() - start;
                if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= 10) {
                    fprintf(stderr, "Capture: camera open timed out after 10s (camera may be unavailable)\n");
                    fflush(stderr);
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
#else
        _vcap.open( camIndex );
#endif

        if ( _vcap.isOpened() )
        {
            fprintf(stderr, "Capture: camera opened successfully, setting 640x480\n");
            fflush(stderr);
            _vcap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
            _vcap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
            _CaptureMethod = CAPTURE_OPENCV;
            goto Initialized;
        }
        fprintf(stderr, "Capture: failed to open camera %d\n", camIndex);
        fflush(stderr);
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
    if ( _vcap.isOpened() )
    {
        _vcap.release();
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
}

bool Capture::CaptureFrame()
{
    switch ( _CaptureMethod )
    {
    case CAPTURE_OPENCV:
        if ( _vcap.isOpened() )
        {
            _vcap >> _matFrame;
            if ( _matFrame.empty() )
            {
                fprintf(stderr, "Capture: frame is empty (camera may need warmup)\n");
                return false;
            }
            // CAP_PROP_FRAME_WIDTH/HEIGHT is advisory on macOS — Continuity
            // Camera (iPhone) ignores it and delivers e.g. 1920x1080. Force
            // the size HandyAR's cvPyrDown(frame -> 320x240) expects.
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
