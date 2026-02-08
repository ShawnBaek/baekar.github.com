#include "stdafx.h"
#include "FlyCap.h"

//=============================================================================
// PGR Includes
//=============================================================================
#include <PGRError.h>

CFlyCap::CFlyCap(void)
{
    _FlyCaptureContext = NULL;
    _pFrame = NULL;
}

CFlyCap::~CFlyCap(void)
{
    Terminate();
}

bool CFlyCap::Initialize( int index )
{
    if ( _FlyCaptureContext != NULL )
        return true;

    FlyCaptureError error =  ::flycaptureCreateContext( &_FlyCaptureContext );
    if ( error != FLYCAPTURE_OK )
    {
        return false;
    }

    //
    // Initialize using PGR GUI Dialog
    //
    FlyCaptureInfo  arInfo[ 32 ];
    unsigned int    uiCamerasOnBus = 32;

    _nCameraBusIndex = -1;
    //
    // Show the camera selection dialog.
    //
    unsigned long   ulSerialNumber = 0;
    int             iDialogStatus;

    //
    // Reenumerate the bus, just incase it changed during the dialog.
    //

    error = ::flycaptureBusEnumerateCameras( arInfo, &uiCamerasOnBus );
    if ( error != FLYCAPTURE_OK ) return false;

    //
    // Determine bus index for camera.  (should do bug 1376 instead.)
    //
    if ( index >= 0 )
    {
        for( unsigned i = 0; i < uiCamerasOnBus; i++ )
        {
            if( arInfo[ i ].SerialNumber == ulSerialNumber )
            {
                _nCameraBusIndex = i;
            }
        }
    }
    else if ( uiCamerasOnBus >= 0 )
    {
        _nCameraBusIndex = 0;
    }

    //
    // initialize
    error = ::flycaptureInitialize( 
                    _FlyCaptureContext, _nCameraBusIndex );
    if ( error != FLYCAPTURE_OK ) return false;


    //
    // Get camera info
    //
    error = ::flycaptureGetCameraInfo( _FlyCaptureContext, &_CameraInfo );
    if ( error != FLYCAPTURE_OK )
    {
        return false;
    }

    //
    // Initialize Capture
    //
    error = ::flycaptureInitialize( _FlyCaptureContext, 0 );
    if ( error != FLYCAPTURE_OK )
    {
        return false;
    }

    //
    // Start Capture
    //
    error = ::flycaptureStart(
        _FlyCaptureContext,
        FLYCAPTURE_VIDEOMODE_ANY,
        FLYCAPTURE_FRAMERATE_30 );
    while ( error == FLYCAPTURE_NOT_STARTED )
    {
        ::Sleep( 100 );
        error = ::flycaptureStart(
            _FlyCaptureContext,
            FLYCAPTURE_VIDEOMODE_ANY,
            FLYCAPTURE_FRAMERATE_30 );
    }
    if ( error != FLYCAPTURE_OK )
    {
        return false;
    }

    //
    // Allocate Images
    //
    memset( &_ImageRaw, 0x0, sizeof( _ImageRaw ) );
    memset( &_ImageProcessed, 0x0, sizeof( _ImageProcessed ) );
//    _ImageProcessed.pData = new unsigned char[ 640 * 480 * 3 ];
//    memset( _ImageProcessed.pData, 0x0, 640 * 480 * 3 );
    _pFrame = cvCreateImage( cvSize( 640, 480 ), 8, 3 );
    _pFrame->origin = 1;

    // Combining the two image structures with their image data buffer
    _ImageProcessed.pData = (unsigned char *)(_pFrame->imageData);

    return true;
}

void CFlyCap::Terminate()
{
    FlyCaptureError error = ::flycaptureStop( _FlyCaptureContext );
    if ( error != FLYCAPTURE_OK )
    {
        // error occurred while stopping
    }

    ::flycaptureDestroyContext( _FlyCaptureContext );
    _FlyCaptureContext = NULL;

/*    if ( _ImageProcessed.pData != NULL )
    {
        delete [] _ImageProcessed.pData;
        _ImageProcessed.pData = NULL;
    }
*/
    cvReleaseImage( &_pFrame );
    _pFrame = NULL;
}

IplImage * CFlyCap::Capture()
{
    FlyCaptureImagePlus imageP = { 0 };
    FlyCaptureError error;
    
    //
    // Capture a Frame
    //
    error = ::flycaptureGrabImage2(
        _FlyCaptureContext,
        &imageP.image );
    if ( error != FLYCAPTURE_OK )
    {
        return NULL;
    }

    //
    // Convert the Format to BGR
    //
    _ImageProcessed.pixelFormat = FLYCAPTURE_BGR;

    error = ::flycaptureConvertImage(
        _FlyCaptureContext,
        &imageP.image,
        &_ImageProcessed );
    if ( error != FLYCAPTURE_OK )
    {
        return NULL;
    }

    return _pFrame;
}
