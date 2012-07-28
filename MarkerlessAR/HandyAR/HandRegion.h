#ifndef _HAND_REGION_H_
#define _HAND_REGION_H_

#include "cv.h"
#include "highgui.h"
#include "MixGaussian.h"
#include "ColorHistogram.h"

#define ALPHA   0.9

class HandRegion
{
public:
    HandRegion(void);
    ~HandRegion(void);

    bool LoadSkinColorProbTable();
    bool LoadHandMask( char * filename );
    bool InitHandMask( IplImage * srcImage );

    IplImage * GetHandRegion( IplImage * srcImage, IplImage * srcGrayImage, bool fScreenshot = false );

    IplImage * QueryHandRegion() { return _pImage; }
    IplImage * QueryHandMask() { return _pImageHandMask; }

    void LearnColor( IplImage * srcImage, IplImage * maskImage );
    void LearnColorCrCb( bool fLearn = true );

    void DrawContour( IplImage * dstImage, CvPoint start, IplImage * distImage );

private:

    MixGaussian     _SkinColor;
    MixGaussian     _NonSkinColor;

    ColorHistogram  _ColorHistogram;

    IplImage *      _pImage;            // Hand Region Image ( binary )
    IplImage *      _pImageGradient;    // Gradient
    IplImage *      _pImageHandMask;    // Hand Mask Image
    IplImage *      _pImageYCrCb;       // Y-Cr-Cb Image

    bool            _fLearned;

    int             _histPixel[16][16][16];
    int             _histPixelCrCb[16][16][16];
    int             _nTotalPixel;

};

#endif // _HAND_REGION_H_
