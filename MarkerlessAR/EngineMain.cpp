#define _STDINT

// Platform headers
#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include "compat/win32_stub.h"
#endif

#ifdef __APPLE__
#include "compat/macos_camera_auth.h"
#endif

#include <fstream>

#include <opencv2/opencv.hpp>
// <opencv2/gpu/gpu.hpp> removed — OpenCV CUDA module not available on macOS
#include <opencv2/core/core.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio/videoio_c.h>
#include <opencv2/videoio/legacy/constants_c.h>
#include <opencv2/core/core_c.h>
#include <opencv2/imgproc/imgproc_c.h>

#include <iostream>
#include <list>

// stdint.h — system-provided on macOS; MSVC polyfill only needed for old VS
#ifdef _MSC_VER
#include "msc_stdint.h"
#else
#include <cstdint>
#endif
#include "brisk/brisk.h"
//#include "projection.h"
#include "brisk/Matcher.h"

#include "brisk/MatchVerifier.hpp"
#include "brisk/GroundTruth.hpp"

#include "HandyAR/HandyAR.h"

// OpenGL/GLUT headers — macOS paths
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "SungwookUtility.hpp"
#include "SungwookFeature.hpp"
#include "SungwookAR.hpp"
#include "calibration/Camera.h"
#include "KatoPoseEstimation/KatoPoseEstimator.h"
#include "wonjo.h"
//#include <boost/lexical_cast.hpp>

// GLFW for macOS windowing
#ifndef _WIN32
#include <unistd.h>
#include "legacy_engine.h"
// Pointer state handed in by the application each frame (was read from GLFW here).
static legacy_engine::Pointer g_pointer;
static bool g_mouseWasDown = false;
static double g_mousePrevX = 0, g_mousePrevY = 0;
#endif

#ifndef _WIN32
#include <dirent.h>
#include <algorithm>
#ifdef __APPLE__
#include "compat/macos_window_capture.h"
#include "compat/macos_camera_menu.h"
#endif
#include "Contents.hpp"

static Contents  g_contents;
#ifdef __APPLE__
static WCStream* g_winStream  = nullptr;
static unsigned int g_winTexture = 0;
static const int    kWinTexW    = 512;
static const int    kWinTexH    = 512;
static std::vector<uint8_t> g_winFrameBuf;
static bool         g_winPlaneSpawned = false;
#endif
#endif

using namespace cv;




//OpenGL 관련된 변수
bool OpenGLinit=false;
CKatoPoseEstimator poseEstimator;
CvSize	size;
double	projectionMatrix[3][4];
double	distortionFactor[4];
IplImage img_binary;
CvCapture		*capture_C	= NULL;
char	parameterFile[100];
CCamera	camera;
bool	loaded;

Results threadResults1;
Results threadResults2;


double threadmModelView1[16];
double threadmModelView2[16];


double	mModelView[16];
double	mProjection[16];


// 영상 입력 받을 Capture형 선언
// On macOS, only one VideoCapture can access a camera at a time (AVFoundation).
// gCapture owns the camera; threads read from a shared frame buffer instead.
#ifdef _WIN32
VideoCapture	capture(0);
VideoCapture	capture1(0);
VideoCapture	capture2(0);
#else
// Shared frame buffer: mainLoop publishes the application's frames here and
// the matching/tracking threads read from it.
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
// The shared frame buffer and the matching/tracking threads moved to
// legacy_marker_tracker.cpp, which the application drives through
// IMarkerTracker.
// Dummy VideoCapture objects (never opened) to keep Windows code path compilable
VideoCapture	capture;
VideoCapture	capture1;
VideoCapture	capture2;

// Current frame handed in by the application's IFrameSource
// (legacy_engine::Start / RenderFrame). g_inputLive is false for the
// "camera unavailable" placeholder source.
static cv::Mat  g_inputFrame;
static uint64_t g_inputSequence = 0;
static int64    g_inputTick = 0;
static bool     g_inputLive = false;

// Camera fallback: when camera is unavailable, use a dummy frame
static bool g_cameraAvailable = false;
static cv::Mat g_dummyFrameMat;
static IplImage g_dummyIpl;
// Tracking results handed in by the application for the current frame.
static legacy_engine::TrackingInput g_tracking;

#ifndef _WIN32
static void DrawMultiMarkerDetections()
{
	// Outlines for the multi-marker tracker; the 2012 single-marker pipeline
	// draws its own rectangle with the 3D overlay below.
	if (g_tracking.drivesPose)
		return;

	static const float colors[][3] = {
		{0.10f, 0.95f, 1.00f},
		{1.00f, 0.25f, 0.75f},
		{1.00f, 0.90f, 0.15f},
		{0.25f, 1.00f, 0.35f},
		{1.00f, 0.50f, 0.10f},
		{0.55f, 0.45f, 1.00f}
	};

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 640, 480, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT | GL_CURRENT_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glLineWidth(4.0f);

	for (const legacy_engine::TrackedMarker& marker : g_tracking.markers) {
		if (!marker.found)
			continue;

		const float* color = colors[marker.index %
			(sizeof(colors) / sizeof(colors[0]))];
		glColor3f(color[0], color[1], color[2]);
		glBegin(GL_LINE_LOOP);
		for (const cv::Point2f& corner : marker.outline)
			glVertex2f(corner.x, corner.y);
		glEnd();

		const std::string status = marker.tracking
			? std::to_string(marker.trackedPoints) + " tracked"
			: std::to_string(marker.inliers) + " inliers";
		const std::string label = std::to_string(marker.index + 1) +
			"  " + marker.name + "  " + status;
		const float labelX = marker.outline[0].x;
		const float labelY = std::max(14.0f, marker.outline[0].y - 8.0f);
		glRasterPos2f(labelX, labelY);
		for (char character : label)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, character);
	}

	glPopAttrib();
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}
#endif
#endif

char ch=0;
const int MAX_CORNERS=200;

//#define	Calculate_Time
#ifdef	Calculate_Time
	#include <time.h>
	 //Number of the frames processed per second in the application
	extern int fps;
	void fpsCalculation(void);
#endif
	
#define	Image_Set_Matching

//CvFont	font;
vector<KeyPoint>	kpQuery;
Mat					queryDescriptors;

Ptr<FeatureDetector> detector;
Ptr<DescriptorExtractor> descriptorExtractor;
	

//============================================================================//
vector<Point2f> g_mpts_1;
vector<Point2f> g_mpts_2;	
//============================================================================//
Mat							img_rgbdatabase1;
Mat							img_graydatabase1;
Mat							img_centerdatabase1;

vector<KeyPoint>			kp_database1;
Mat							desc_database1;
vector<Point2f>				obj_corners1(4);
vector<Point2f>				obj_center_corners1(4);

vector<Point2f>				dst_matching_corners1(4);
vector<Point2f>				dst_tracking_corners1(4);


	
Mat							img_rgbdatabase2;
Mat							img_graydatabase2;
Mat							img_centerdatabase2;

vector<KeyPoint>			kp_database2;
Mat							desc_database2;
vector<Point2f>				obj_corners2(4);
vector<Point2f>				obj_center_corners2(4);

vector<Point2f>				dst_matching_corners2(4);
vector<Point2f>				dst_tracking_corners2(4);


Ptr<DescriptorMatcher> descriptorMatcher1;
Ptr<DescriptorMatcher> descriptorMatcher2;


Mat mInput, mQuery, mResult, gDetectionResult1, mResult2;
bool tracking1, tracking2;
bool bIsTracking[12];
bool bIsTrackingInit[12];
bool g_bisRunTrackingThread;

bool bThreadDetection1=false;
bool bThreadDetection2=false;

bool bInitTracking=true;
bool bThreadTracking1=false;
bool bThreadTracking2=false;

// Thread 처리를 위한 HANDLE
HANDLE hMatchingThread[2];
HANDLE hTrackingThread[2];
HANDLE hDrawThread;
HANDLE hMutex;

unsigned uMatchingThreadID[2];
unsigned uTrackingThreadID[2];
unsigned uDrawThreadID;


bool computeHomography = true;
bool hamming = true;

	
CRITICAL_SECTION   hCriticalSection[2];
ofstream mslNFT_Log("mslNFT_Log.txt");

strFilename Filename[20] =
{
	//파일명을 iu2에서 yejin으로 수정함
    "image/yejin.jpg",
	"image/yejin.jpg",
};


//////////////////////////////HandyAR Display()////////////////////////////////////////////

// void display()
// {
//     gFingertipPoseEstimation.TickCountBegin();//(7) Rendering
// 
//     // display info
//     IplImage * image = gFingertipPoseEstimation.OnDisplay();
// 
// 	//120325 cwj
//     // check if there have been any openGL problems
// //     GLenum errCode = glGetError();
// //     if( errCode != GL_NO_ERROR )
// //     {
// //         const GLubyte *errString = gluErrorString( errCode );
// //         fprintf( stderr, "OpenGL error: %s\n", errString );
// //     }
//     // clear the buffers of the last frame
// //     glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
// // 
// //     // set the drawing region of the window
// //     glViewport( 0, 0, image->width, image->height );
// 
// 
// 	//120325 cwj
//     // set viewing frustrum to match camera FOV (ref: ARTag's 3d_augmentations.cpp)
//     
// 	//gFingertipPoseEstimation.SetOpenGLFrustrum();	//->replace next;
// 	D3DXMATRIXA16 matProj;
// 	gFingertipPoseEstimation.D3DXMakeProjectionMatrix(&matProj);
// 	wonjo_dx::SetProjectionMatrix(&matProj);
// 	
// 
// 
//     // set modelview matrix of the camera
//     //gFingertipPoseEstimation.SetOpenGLModelView();
//     D3DXMATRIXA16 matView;
// 	gFingertipPoseEstimation.D3DXMakeProjectionMatrix(&matView);
// 	wonjo_dx::SetModelViewMatrix(&matView);
// 	//~120325 cwj
// 
// /*    glMatrixMode(GL_MODELVIEW);
// 	glLoadIdentity();
//     glMultMatrixf( gFingertipPoseEstimation.QueryModelViewMat() );/**/
// 
// 
//     // render virtual objects
// //     GLfloat colorRed[3] = { 1.0, 0.0, 0.0 };
// // 	GLfloat colorGreen[3] = { 0.0, 1.0, 0.0 };
// // 	GLfloat colorBlue[3] = { 0.0, 0.0, 1.0 };
// //     GLfloat colorYellow[3] = { 1.0, 1.0, 0.0 };
// //     GLfloat colorDarkYellow[3] = { 0.6, 0.6, 0.0 };
// //     GLfloat colorWhite[3] = { 1.0, 1.0, 1.0 };
// //     GLfloat colorGray[3] = { 0.5, 0.5, 0.5 };
// //~120325 cwj
//     
//     if ( gFingertipPoseEstimation.QueryValidPose() )
//     {
// 		std::cout << " Valid Fingertip!! " <<std::endl;
// 
// 		//3차원 화살표 그리기.
// 		wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("Arrow3Axis.X"), 0.1f);
// //120325 cwj
// // 	    glPushMatrix();
// //        
// //         // Axis
// //         if ( gnModel == MODEL_COORDINATE_AXES )
// //         {
// //             glDisable(GL_LIGHTING);
// //             glLineWidth( 3 );
// // 		    glBegin(GL_LINES);
// // 			    glMaterialfv(GL_FRONT, GL_AMBIENT, colorRed);
// //                 glColor3fv( colorRed );
// // 			    glVertex3f(0, 0, 0);
// // 			    glVertex3f(100, 0, 0);
// // 			    glMaterialfv(GL_FRONT, GL_AMBIENT, colorGreen);
// //                 glColor3fv( colorGreen );
// // 			    glVertex3f(0, 0, 0);
// // 			    glVertex3f(0, 100, 0);
// // 			    glMaterialfv(GL_FRONT, GL_AMBIENT, colorBlue);
// //                 glColor3fv( colorBlue );
// // 			    glVertex3f(0, 0, 0);
// // 			    glVertex3f(0, 0, 100);
// // 		    glEnd();
// //             glPushMatrix();
// //                 glColor3fv( colorRed );
// //                 glTranslatef( 100, 0, 0 );
// //                 glRotatef( 90, 0, 1, 0 );
// //                 glutSolidCone( 5, 10, 16, 16 );
// //             glPopMatrix();
// //             glPushMatrix();
// //                 glColor3fv( colorGreen );
// //                 glTranslatef( 0, 100, 0 );
// //                 glRotatef( -90, 1, 0, 0 );
// //                 glutSolidCone( 5, 10, 16, 16 );
// //             glPopMatrix();
// //             glPushMatrix();
// //                 glColor3fv( colorBlue );
// //                 glTranslatef( 0, 0, 100 );
// //                 glRotatef( 90, 0, 0, 1 );
// //                 glutSolidCone( 5, 10, 16, 16 );
// //             glPopMatrix();
// //             glEnable(GL_LIGHTING);/**/
// //         }
// // 
// //             /*
// // 		    glPushMatrix();
// // 			    glTranslatef(0,0,30);
// // 			    glRotatef(90, 1, 0, 0);
// // 			    glutSolidTeapot(50);
// // 		    glPopMatrix();/**/
// // 	    glPopMatrix();
// //~120325 cwj
//     }/**/
// 
// //120325 cwj
// 	//glutSwapBuffers();
// //~120325 cwj
// 
//     
// 	// Record the output video
// //120325 cwj
// //     if ( fRecord )
// //     {
// // 		//120325 cwj
// // //        glReadPixels( 0, 0, image->width, image->height, GL_RGB, GL_UNSIGNED_BYTE, image->imageData );
// // 		//~120325 cwj
// //         cvConvertImage( image, image, CV_CVTIMG_SWAP_RB );
// //         cvWriteFrame( gpRecordOutput, image );
// //     }
// //     if ( fScreenshot )
// //     {
// // 		//120325 cwj
// // //        glReadPixels( 0, 0, image->width, image->height, GL_RGB, GL_UNSIGNED_BYTE, image->imageData );
// // 		//~120325 cwj
// //         cvConvertImage( image, image, CV_CVTIMG_SWAP_RB );
// //         cvSaveImage( "screenshot_render.png", image );
// //         fScreenshot = false;
// //     }
// //~120325 cwj
//     gFingertipPoseEstimation.TickCountEnd();//(7) Rendering
//     gFingertipPoseEstimation.TickCountNewLine();
// }



// void tick_func() {
//     
//     // capture
//     IplImage * frame = 0;
// 
//     gCapture.CaptureFrame();
//     frame = gCapture.QueryFrame();
//     if ( !frame )
//     {
//         return;
//     }
//     gFingertipPoseEstimation.OnCapture( frame, gCapture.QueryTickCount() );
// 
//     // record
//     if ( fRecord )
//     {
//         cvWriteFrame( gpRecord, frame );
//     }
// 
//     // process fingertip pose estimation
// 
// 	//핑거에 대한 프로세싱을 시작하는 부분
//     gFingertipPoseEstimation.OnProcess();
// 
// 	//120325 cwj
//     // draw objects
//     //glutPostRedisplay();
//     //~120325 cwj
// }


void init()
{
    // turn on z-buffering, so we get proper occlusion
    glEnable( GL_DEPTH_TEST );

    // properly scale normal vectors
    glEnable( GL_NORMALIZE );

    // turn on default lighting
    glEnable( GL_LIGHTING );

    // light 0
    GLfloat light_position[] = { 100.0, 500, 200, 1.0 };
    GLfloat white_light[] = { 1.0, 1.0, 1.0, 0.8 };
    GLfloat lmodel_ambient[] = { 0.9, 0.9, 0.9, 0.5 };

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_SMOOTH);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
    glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

    glEnable(GL_LIGHT0);

    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_TEXTURE_2D);
    glCullFace(GL_BACK);
    glFrontFace(GL_CW);

    // Init Earth Texture
    glGenTextures( 1, &gnEarthTexID );
    glBindTexture( GL_TEXTURE_2D, gnEarthTexID );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, 0 );
    cv::Mat tempMat = cv::imread( EARTH_TEXTURE_FILENAME, cv::IMREAD_COLOR );
    if (!tempMat.empty()) {
        cv::cvtColor( tempMat, tempMat, cv::COLOR_BGR2RGB );
        IplImage tempImageHdr = cvIplImage(tempMat);
        IplImage * tempImage = &tempImageHdr;
        cvFlip( tempImage );
        glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, tempImage->width, tempImage->height, GL_RGB, GL_UNSIGNED_BYTE, tempImage->imageData );
    } else {
        fprintf(stderr, "Warning: could not load texture '%s'\n", EARTH_TEXTURE_FILENAME);
    }
    // tempImage is stack-allocated wrapper, no release needed

    // Init bunny
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glVertexPointer(3,GL_FLOAT,0,bunny);
    glNormalPointer(GL_FLOAT,0,normals);

    // Callback functions
    //glutDisplayFunc( display );
    //	glutReshapeFunc( reshape );
    //키보드 기능 주석처리
	//glutKeyboardFunc( keyboard_func );
    //	glutKeyboardUpFunc( keyboard_up_func );
    
	//마우스 클릭 기능 주석처리
	//glutMouseFunc( mouse_click_func );
    //	glutMotionFunc( mouse_move_func );
    //	glutSpecialFunc( special_func );
    //	glutSpecialUpFunc ( special_up_func );
    //	glutTimerFunc(33, timer_func, 1);
    //glutIdleFunc(tick_func);

}


/////////////////////////////////////////////////////////////////////////////////////////////


namespace wonjo_dx
{
	static void drawDX()
	{
		//if(bThreadDetection1==false) return;
		
//		double viewbuff[16];
//		double projbuff[16];
		
		D3DXMATRIXA16 matProj;
		D3DXMATRIXA16 matView;


		camera.D3DXMakeProjectionMatrix(&matProj);
		wonjo_dx::SetProjectionMatrix(&matProj);

		//camera.D3DXMakeViewMatrix();

		camera.D3DXMakeProjectionMatrix(&matView);
		wonjo_dx::SetModelViewMatrix(&matView);
/*

		memcpy(viewbuff,getModelViewMatrix(0),sizeof(viewbuff));
//		memcpy(projbuff,getProjectionMatrix(),sizeof(projbuff));
//		SetProjectionMatrix(projbuff);
		SetProjectionMatrix(getProjectionMatrix());
		SetModelViewMatrix(viewbuff);
// 		std::cout << "viewbuff is : ";
// 		for(int i = 0 ; i < 16 ; ++i)
// 			std::cout << viewbuff[i] << " ";
// 		std::cout << std::endl;
*/


		AAR3DDrawMesh("IEFrame",NULL, 35.0f );
	}
}

IplImage		*img_input;

int InitializeEngineMain();  // forward declaration


static void mainLoop(void)
{
#ifdef _WIN32
	if( GetKeyState(VK_LBUTTON) & 0x8000 )
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(NULL, &pt);
		wonjo_dx::Picking(pt);
	}
#else
	{
		const double mx = g_pointer.x;
		const double my = g_pointer.y;
		const bool mouseDown = g_pointer.leftDown;

		if (mouseDown && !g_mouseWasDown) {
			// Mouse-down edge: pick the AR content under the cursor.
			POINT pt; pt.x = (LONG)mx; pt.y = (LONG)my;
			D3DXVECTOR3 ro, rd;
			wonjo_dx::PickingRay(pt, &ro, &rd);
			int hit = g_contents.pick(ro, rd);
			if (hit >= 0) {
				g_contents.select(hit);
				fprintf(stderr, "BaekAR: picked AR content #%d\n", hit);
			} else {
				g_contents.deselectAll();
				// Preserve the original z=0 picking trace for the no-hit case.
				wonjo_dx::Picking(pt);
			}
		} else if (mouseDown && g_mouseWasDown) {
			// Drag: translate the selected item.
			g_contents.dragSelected(mx - g_mousePrevX, my - g_mousePrevY);
		}
		g_mousePrevX = mx;
		g_mousePrevY = my;
		g_mouseWasDown = mouseDown;
	}
#endif
	
	IplImage		img_output;
	//경민 여기서 img_input에 값이 들어오나 확인좀
	
	
	//여기서 사각형을 그려주는 것으로 수정

	IplImage * image = gFingertipPoseEstimation.OnDisplay();


#ifdef _TESTMODE
	//카메라값 추출
// 	 	std::cout << "cameraResultXY [0] = " 
// 	 		<< camera.featuresResult.vertex[0].x << "/"
// 	 		<< camera.featuresResult.vertex[0].y << "\t[1] = "
// 	 		<< camera.featuresResult.vertex[1].x << "/"
// 	 		<< camera.featuresResult.vertex[1].y << "\t[2] = "
// 	 		<< camera.featuresResult.vertex[2].x << "/"
// 	 		<< camera.featuresResult.vertex[2].y << "\t[3] = "
// 	 		<< camera.featuresResult.vertex[3].x << "/"
// 	 		<< camera.featuresResult.vertex[3].y << "\t[4] = "
// 	 		<< std::endl;
	
	D3DXVECTOR2 v2[4];

	//320, 240 이 중점임을 고려
// 	v2[0].x = 420.0f;		v2[1].x = 630.0f;
// 	v2[0].y = 210.0f;		v2[1].y = 210.0f;
// 
// 
// 	v2[3].x = 420.0f;		v2[2].x = 630.0f;
// 	v2[3].y = 470.0f;		v2[2].y = 470.0f;


	static float factorx = 0.03f;
	static float factory = 0.05f;
	factorx*=1.03f;
//	factory*=1.03f;


// 	v2[0].x = 275.0f-factorx;		v2[1].x = 365.0f+factorx;
// 	v2[0].y = 165.0f-factory;		v2[1].y = 165.0f-factory;
// 
// 
// 	v2[3].x = 275.0f-factorx;		v2[2].x = 365.0f+factorx;
// 	v2[3].y = 315.0f+factory;		v2[2].y = 315.0f+factory;

	v2[0].x = 275.0f+factorx;		v2[1].x = 365.0f+factorx;
	v2[0].y = 165.0f;		v2[1].y = 165.0f;


	v2[3].x = 275.0f+factorx;		v2[2].x = 365.0f+factorx;
	v2[3].y = 315.0f;		v2[2].y = 315.0f;

// 	v2[0].x = 275.0f;		v2[1].x = 365.0f;
// 	v2[0].y = 165.0f;		v2[1].y = 165.0f;
// 
// 
// 	v2[3].x = 275.0f;		v2[2].x = 365.0f;
// 	v2[3].y = 315.0f;		v2[2].y = 315.0f;

	
	
	for(int i = 0 ; i < 4 ; ++i)
	{
		camera.featuresResult.vertex[i].x = v2[i].x;
		camera.featuresResult.vertex[i].y = v2[i].y;
		dst_matching_corners1[i].x = v2[i].x;
		dst_matching_corners1[i].y = v2[i].y;
		dst_tracking_corners1[i].x = v2[i].x;
		dst_tracking_corners1[i].y = v2[i].y;
	}

	bThreadDetection1 = true;
	bThreadTracking1 = true;

#endif




	cvFlip(image);
	// Don't draw the rectangle into the image here (the camera-preview
	// quad does an H+V texture flip that puts cvLine output in the wrong
	// place on screen). The rectangle is drawn later in OpenGL screen-
	// space ortho, sharing the same coordinate convention as the AR
	// overlay teapot — they always agree.
	
	
	cvFlip(image, NULL, -1);



	
	/*
	if(bThreadDetection1==true||bThreadTracking1==true){
		
		camera.featurePoseEstimation();
		
	}*/



	D3DXMATRIXA16 matProj;
	D3DXMATRIXA16 matView;
	if(image)
	{
		wonjo_dx::BeginRender();
		wonjo_dx::AAR3DDrawCameraPreview(image->imageData,640,480);
		DrawMultiMarkerDetections();

		// Projection and view come from the application's IPoseEstimator
		// (the 2012 CCamera, now in legacy_pose.cpp).
		for (int i = 0; i < 16; ++i) matProj[i] = g_tracking.projection[i];
		wonjo_dx::SetProjectionMatrix(&matProj);

		if(g_tracking.drivesPose && g_tracking.poseValid && !g_tracking.markers.empty())
		{
			const std::array<cv::Point2f, 4>& markerCorners = g_tracking.markers.front().outline;
			for (int i = 0; i < 16; ++i) matView[i] = g_tracking.view[i];
			wonjo_dx::SetModelViewMatrix(&matView);

#ifndef _WIN32
			static int matLogTick = 0;
			if (++matLogTick % 30 == 0) {
				fprintf(stderr, "matProj:\n  %g %g %g %g\n  %g %g %g %g\n  %g %g %g %g\n  %g %g %g %g\n",
					matProj.m[0][0], matProj.m[0][1], matProj.m[0][2], matProj.m[0][3],
					matProj.m[1][0], matProj.m[1][1], matProj.m[1][2], matProj.m[1][3],
					matProj.m[2][0], matProj.m[2][1], matProj.m[2][2], matProj.m[2][3],
					matProj.m[3][0], matProj.m[3][1], matProj.m[3][2], matProj.m[3][3]);
				fprintf(stderr, "matView:\n  %g %g %g %g\n  %g %g %g %g\n  %g %g %g %g\n  %g %g %g %g\n",
					matView.m[0][0], matView.m[0][1], matView.m[0][2], matView.m[0][3],
					matView.m[1][0], matView.m[1][1], matView.m[1][2], matView.m[1][3],
					matView.m[2][0], matView.m[2][1], matView.m[2][2], matView.m[2][3],
					matView.m[3][0], matView.m[3][1], matView.m[3][2], matView.m[3][3]);
				fflush(stderr);
			}

			// AR overlay anchored to the matching corners (screen-space).
			// The marker pose matrices are D3D LH and don't translate cleanly
			// to OpenGL's RH convention, so the world-space DrawMesh path
			// projects the geometry off-screen. As a working alternative
			// that still tracks the marker correctly: render a 3D mesh in an
			// ortho overlay, positioned at the centroid of dst_matching_corners1
			// and sized by the marker's diagonal in screen pixels. The mesh
			// follows the marker as you move it because the screen-space
			// corners do.
			{
				float cx = 0, cy = 0;
				for (int k = 0; k < 4; ++k) {
					cx += markerCorners[k].x;
					cy += markerCorners[k].y;
				}
				cx *= 0.25f;
				cy *= 0.25f;
				float dx = markerCorners[0].x - markerCorners[2].x;
				float dy = markerCorners[0].y - markerCorners[2].y;
				float diag = sqrtf(dx*dx + dy*dy);
				float meshScale = diag * 0.20f;  // teapot ~ 40% of marker diagonal

				if (diag > 20.0f && diag < 1500.0f) {
					glMatrixMode(GL_PROJECTION);
					glPushMatrix();
					glLoadIdentity();
					glOrtho(0, 640, 480, 0, -1000, 1000);
					glMatrixMode(GL_MODELVIEW);
					glPushMatrix();
					glLoadIdentity();

					// Yellow tracking rectangle — same coord system as the
					// teapot, so they always agree on where the marker is.
					glDisable(GL_DEPTH_TEST);
					glDisable(GL_TEXTURE_2D);
					glDisable(GL_LIGHTING);
					glColor3f(1.0f, 1.0f, 0.0f);
					glLineWidth(4.0f);
					glBegin(GL_LINE_LOOP);
						for (int k = 0; k < 4; ++k)
							glVertex2f(markerCorners[k].x,
							           markerCorners[k].y);
					glEnd();
					glLineWidth(1.0f);

					// Real .X mesh — Arrow3Axis.X loaded by Assimp in PR #14.
					// Anchored at the marker centroid like the teapot was.
					glTranslatef(cx, cy, 0);
					// Spin slowly around screen-space Y so the user sees it's 3D.
					static float spinDeg = 0;
					spinDeg += 1.2f;
					glRotatef(spinDeg, 0, 1, 0);
					// Mesh native extents are ~1 unit; scale to marker size.
					// Negative Y to align with screen-down convention.
					glScalef(meshScale, -meshScale, meshScale);

					glEnable(GL_DEPTH_TEST);
					glEnable(GL_LIGHTING);
					glEnable(GL_LIGHT0);
					GLfloat lp[4] = { 0.5f, 0.5f, 1.0f, 0.0f };
					GLfloat ld[4] = { 1, 1, 1, 1 };
					GLfloat la[4] = { 0.3f, 0.3f, 0.3f, 1 };
					glLightfv(GL_LIGHT0, GL_POSITION, lp);
					glLightfv(GL_LIGHT0, GL_DIFFUSE,  ld);
					glLightfv(GL_LIGHT0, GL_AMBIENT,  la);
					glEnable(GL_COLOR_MATERIAL);
					glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

					// Identity world matrix → DrawMesh draws at the current
					// modelview position (where we just translated/scaled).
					D3DXMATRIXA16 ident;
					for (int r = 0; r < 4; ++r)
						for (int c = 0; c < 4; ++c)
							ident.m[r][c] = (r == c) ? 1.0f : 0.0f;
					wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("Arrow3Axis.X"), &ident);

					glDisable(GL_LIGHTING);

					glPopMatrix();
					glMatrixMode(GL_PROJECTION);
					glPopMatrix();
					glMatrixMode(GL_MODELVIEW);
				}
			}
#endif

// 			std::cout << "marker projection is ";
// 			for(int i = 0 ; i < 16 ; ++i)
// 			{
// 				std::cout << matProj[i] << " ";
// 			}
// 			std::cout << std::endl;
// 			std::cout << "marker view is ";
// 			for(int i = 0 ; i < 16 ; ++i)
// 			{
// 				std::cout << matView[i] << " ";
// 			}
// 			std::cout << std::endl;
			
			//wonjo_dx::AAR3DDrawMesh("IEFrame",NULL,35.0f);
#ifdef _WIN32
			// AAR3DTexturing captures a target Win32 window's pixels via
			// FindWindow/GetDC/PrintWindow as an AR texture — Windows-only.
			wonjo_dx::AAR3DTexturing text("IEFrame",NULL);
#endif

			
// 			if(camera.m_vecTrans)	//m_vecTrans : (= T)
// 			{
// 				D3DXMATRIXA16 matAddrRot;	//z축방향이 뒤집어지는 보정.
// 				D3DXMatrixIdentity(&matAddrRot);
// 				D3DXMatrixRotationAxis(&matAddrRot,&D3DXVECTOR3(0,1,0),Deg2Rad(180));
// 				D3DXMATRIXA16 matRot;
// 				D3DXMatrixIdentity(&matRot);
// 				matRot[0] = camera.m_Rotation3x3->data.db[0];
// 				matRot[1] = camera.m_Rotation3x3->data.db[1];
// 				matRot[2] = camera.m_Rotation3x3->data.db[2];
// 				matRot[4] = camera.m_Rotation3x3->data.db[3];
// 				matRot[5] = camera.m_Rotation3x3->data.db[4];
// 				matRot[6] = camera.m_Rotation3x3->data.db[5];
// 				matRot[8] = camera.m_Rotation3x3->data.db[6];
// 				matRot[9] = camera.m_Rotation3x3->data.db[7];
// 				matRot[10] = camera.m_Rotation3x3->data.db[8];
// 				D3DXMatrixInverse(&matRot,NULL,&matRot);		//R^-1
// 
// 				D3DXMATRIXA16 matWorld;
// 				D3DXMATRIXA16* pMatw = &matWorld;
// 
// 				//Draw Ipad
// 				D3DXMatrixIdentity(pMatw);
// 				//D3DXMatrixMultiply(pMatw,pMatw,wonjo_dx::MakeScaleMatrix(0.2,0.2,0.2));								//스케일:ipad
// 				D3DXMatrixMultiply(pMatw,pMatw,wonjo_dx::MakeScaleMatrix(35,35,35));									//스케일:plane
// 				D3DXMatrixMultiply(pMatw,pMatw,&matAddrRot);															//z뒤집기
// 				D3DXMatrixMultiply(pMatw,pMatw,wonjo_dx::MakeTranslationMatrix(/*translation vector xyz*/
// 				-camera.m_vecTrans->data.db[0], -camera.m_vecTrans->data.db[1], -camera.m_vecTrans->data.db[2]));		//이동
// 				D3DXMatrixMultiply(pMatw,pMatw,&matRot);																//회전
// 				//wonjo_dx::Matrix_LH_RH_Swap(pMatw,pMatw);
// 				//wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("ipad_hi.X"),pMatw);										//드로우
// 				//wonjo_dx::DrawPlane(pMatw);
// 				
// 
// 				//Draw Arrows
// 				D3DXMatrixIdentity(pMatw);
// 				D3DXMatrixMultiply(pMatw,pMatw,wonjo_dx::MakeScaleMatrix(10,10,10));									//스케일
// 				D3DXMatrixMultiply(pMatw,pMatw,&matAddrRot);															//z뒤집기
// 				D3DXMatrixMultiply(pMatw,pMatw,wonjo_dx::MakeTranslationMatrix(/*translation vector xyz*/
// 				-camera.m_vecTrans->data.db[0], -camera.m_vecTrans->data.db[1], -camera.m_vecTrans->data.db[2]));		//이동
// 				D3DXMatrixMultiply(pMatw,pMatw,&matRot);																//회전
// 				//wonjo_dx::Matrix_LH_RH_Swap(pMatw,pMatw);
// 				wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("Arrow3Axis.X"),pMatw);									//드로우
// 
// 				wonjo_dx::DrawPlane(wonjo_dx::MakeScaleMatrix(35,35,35));
// 			}
// 			
			
 			wonjo_dx::DrawPlane(wonjo_dx::MakeScaleMatrix(35,35,35));
 			wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("Arrow3Axis.X"),wonjo_dx::MakeScaleMatrix(10,10,10));			//드로우
//

#ifdef __APPLE__
			// Window-as-AR-texture (thesis novelty): pull the latest frame from
			// ScreenCaptureKit, upload to a GL texture, and render via Contents.
			// The textured plane lives in marker-local space, so it locks to
			// the tracked image and the user can pick/drag it.
			if (g_winStream) {
				if (WCStream_LatestFrame(g_winStream, g_winFrameBuf.data(), kWinTexW, kWinTexH)) {
					wonjo_dx::UploadTexture(&g_winTexture, g_winFrameBuf.data(), kWinTexW, kWinTexH);
				}
				if (g_winTexture != 0 && !g_winPlaneSpawned) {
					g_contents.addWindowPlane(g_winTexture, /*halfSize=*/50.0f);
					g_winPlaneSpawned = true;
				}
				g_contents.render();
			}
#endif

		}//end of processing marker
	
		
	}



	//120325 cwj handy AR 추가
	//HandyAR에 대한 프로시져
	
	//////////////////////////////////////////////////////////////////////////
	//손 끝점 찾아서 그림.
	// capture
	IplImage * frame = 0;

#ifdef _WIN32
	gCapture.CaptureFrame();
	frame = gCapture.QueryFrame();
	if ( !frame )
	{
		return;
	}
	gFingertipPoseEstimation.OnCapture( frame, gCapture.QueryTickCount() );
#else
	// The frame comes from the application's IFrameSource (legacy_engine::RenderFrame).
	IplImage frameHeader = cvIplImage(g_inputFrame);
	frame = &frameHeader;
	gFingertipPoseEstimation.OnCapture( frame, g_inputTick );

	// Marker tracking is fed by the application (IMarkerTracker::submit).
#endif

	
	// process fingertip pose estimation
	if(wonjo_dx::bDrawHand)
	{
		//핑거에 대한 프로세싱을 시작하는 부분
		gFingertipPoseEstimation.OnProcess();
		//////////////////////////////////////////////////////////////////////////
	
		//display();
	
		gFingertipPoseEstimation.TickCountBegin();//(7) Rendering
		// display info
		//IplImage * image = gFingertipPoseEstimation.OnDisplay();

	
  		gFingertipPoseEstimation.D3DXMakeProjectionMatrix(&matProj);
  		wonjo_dx::SetProjectionMatrix(&matProj);
	// 	
	
 		gFingertipPoseEstimation.D3DXMakeViewMatrix(&matView);
 		wonjo_dx::SetModelViewMatrix(&matView);
	// 	
		//D3DXMatrixPerspectiveFovLH(&matProj,Deg2Rad(73.0f),640/480,1.0f,100000.0f);
	
// 		std::cout << "marker projection is " << std::endl;
// 		for(int i = 0 ; i < 4 ; ++i)
// 		{
// 			for(int j = 0 ; j < 4 ; ++j)
// 			{
// 				std::cout << matProj[i*4+j] << "\t";
// 			}
// 			std::cout<<std::endl;
// 		}
//		std::cout << std::endl;
//		
	//	D3DXMatrixLookAtLH(&matView,&D3DXVECTOR3(0,20000,20000),&D3DXVECTOR3(0,0,0),&D3DXVECTOR3(0,1,0));
#ifdef _WIN32
		wonjo_dx::GetDevice()->SetTransform(D3DTS_PROJECTION,&matProj);
		wonjo_dx::GetDevice()->SetTransform(D3DTS_VIEW,&matView);
#endif	
		//wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("Arrow3Axis.X"), 1000000000.0f);
		//wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("hand000.X"), 10000000);
		wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("Arrow3Axis.X"), wonjo_dx::MakeScaleMatrix(10.0f,10.0f,10.0f));

		
		//native hand
		//wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("hand001.X"), wonjo_dx::MakeScaleMatrix(5.0f,5.0f,5.0f));
		
		//z축으로 조금 내림...
		D3DXMATRIXA16 matWorld = *wonjo_dx::MakeScaleMatrix(5.0f,5.0f,5.0f) * (*wonjo_dx::MakeTranslationMatrix(0,0,200));	 
		wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("hand001.X"), &matWorld);

		//wonjo_dx::AAR3DDrawMesh("IEFrame",NULL,35.0f);

#ifndef _WIN32
		// Thesis interaction model: 5-finger gesture = point/click. Use the
		// index finger's smoothed 2D position (in 320x240 HandyAR space)
		// as a virtual cursor; on entering the gesture, run a pick against
		// g_contents; while held, drag the selected item.
		static bool         g_fingerActive = false;
		static CvPoint2D32f g_fingerPrev   = {0, 0};
#endif
		if ( gFingertipPoseEstimation.QueryValidPose() )
		{
			std::cout << " Valid Fingertip!! " <<std::endl;
#ifndef _WIN32
			CvPoint2D32f tip = gFingertipPoseEstimation.QueryFingertip2D(1); // index finger
			float curX = tip.x * 2.0f;  // 320x240 -> 640x480 GLFW window
			float curY = tip.y * 2.0f;
			if (!g_fingerActive) {
				POINT pt; pt.x = (LONG)curX; pt.y = (LONG)curY;
				D3DXVECTOR3 ro, rd;
				wonjo_dx::PickingRay(pt, &ro, &rd);
				int hit = g_contents.pick(ro, rd);
				if (hit >= 0) {
					g_contents.select(hit);
					fprintf(stderr, "BaekAR: fingertip picked AR content #%d at (%g,%g)\n",
					        hit, curX, curY);
				}
			} else {
				g_contents.dragSelected(curX - g_fingerPrev.x, curY - g_fingerPrev.y);
			}
			g_fingerPrev.x = curX;
			g_fingerPrev.y = curY;
			g_fingerActive = true;
#endif
		}
#ifndef _WIN32
		else {
			g_fingerActive = false;  // gesture ended — release drag state
		}
#endif
		gFingertipPoseEstimation.TickCountEnd();//(7) Rendering
		gFingertipPoseEstimation.TickCountNewLine();
	}


	//~120325 cwj
	
	
	wonjo_dx::Flip();


	


}

//BSW 이제 안쓰는 함수
double* getModelViewMatrix(int index) {
//	std::cout << "modelview matrix is : ";
	for(int i=0; i<3; ++i) {
		for(int j=0; j<4; ++j) {
			if(bThreadDetection1==true)
			{
//				std::cout << threadResults1.Tcm[i][j] << " ";
				threadmModelView1[j*4 + i] = threadResults1.Tcm[i][j];
			}
			if(bThreadDetection2==true)
				threadmModelView2[j*4 + i] = threadResults2.Tcm[i][j];
			
		}
	}
//	std::cout << std::endl;
	if(bThreadDetection1==true){
		threadmModelView1[0*4+3] = threadmModelView1[1*4+3] = threadmModelView1[2*4+3] = 0.0;
		threadmModelView1[3*4+3] = 1.0;
		//return threadmModelView1;
	}
	if(bThreadDetection2==true){
		threadmModelView2[0*4+3] = threadmModelView2[1*4+3] = threadmModelView2[2*4+3] = 0.0;
		threadmModelView2[3*4+3] = 1.0;
		//return threadmModelView2;
	}
	

	if(index==0)
		return threadmModelView1;
	else if(index==1)
		return threadmModelView2;
	
	//return mModelView;
	
}
//const GLdouble	gl_cpara[] = {	2.190473345f, .0f, .0f, 0.0f,
//								0.0f, -3.025392424f, .0f, .0f,
//								-0.0109375f, -0.010416667f, 1.02020202f, 1.0f,
//								.0f, 0.0f, -101.010101f, .0f};
double* getProjectionMatrix() {

	// Windows 버전 Projection Matrix
	mProjection[0] = 2.190473345f;
	mProjection[1] = 0.0;
	mProjection[2] = 0.0;
	mProjection[3] = 0.0;
	mProjection[4] = 0.0;
	mProjection[5] = -3.025392424f;
	mProjection[6] = 0.0;
	mProjection[7] = 0.0;
	mProjection[8] = -0.0109375f;
	mProjection[9] = -0.010416667f;
	mProjection[10] = 1.02020202f;
	mProjection[11] = 1.0;
	mProjection[12] = 0.0;
	mProjection[13] = 0.0;
	mProjection[14] = -101.010101f;
	mProjection[15] = 0.0;
	
	return mProjection;
}


void doTrack(void	*imgPtr){
		
	//detection이 성공했을 경우
	if(bThreadDetection1==true||bThreadTracking1==true){
		//	poseEstimator.calculateTransformationMatrix(&camera, &threadResults1);

		//bsw 새롭게 추가한 함수
		camera.featurePoseEstimation();

	}
	if(bThreadDetection2==true)
		poseEstimator.calculateTransformationMatrix(&camera, &threadResults2);
	
}

#ifdef _WIN32
LRESULT CALLBACK WndProc(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam);
#endif
LPCSTR lpszClass = "BaekAR Application : State of Art Argumented Reality Browser";
//int main(int argc, char* argv[]) 
//INT APIENTRY WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nShowCmd )
int InitializeEngineMain()
{	
	
	
	/////////////////////////////Init HandyAR Engine//////////////////////////
	 // load fingertip coordinates
    // (if the file does not exist, this call will just fail.)
    // (then the user may measure fingertip coordinates and save them.)
    if ( gFingertipPoseEstimation.LoadFingertipCoordinates( gszFingertipFilename ) == false )
    {
        printf("fingertip coordinate '%s' file was not loaded.\n", gszFingertipFilename );
    }
    else
    {
        printf("fingertip coordinate '%s' file was loaded.\n", gszFingertipFilename );
    }

    // Frame input comes from the application's IFrameSource (camera,
    // synthetic, replay or placeholder); see legacy_engine::Start.
    IplImage firstFrameHeader = cvIplImage(g_inputFrame);
    IplImage * frame = &firstFrameHeader;
    g_cameraAvailable = g_inputLive;
    if ( !g_cameraAvailable )
    {
        fprintf( stderr, "Camera unavailable — running with dummy frame.\n" );
        fflush(stderr);
    }

    // Create Glut Window
	//120325 cwj
	//glutInitWindowSize( frame->width, frame->height );
    //glutCreateWindow( "BaekAR 3DoF Hand" );
	//~120325 cwj

    // Init OpenGL and Glut
	//120325 cwj
	//init();
	//~120325 cwj

    // Init Fingertip Pose Estimation
    fprintf(stderr, "DBG: About to init FingertipPoseEstimation...\n"); fflush(stderr);
    if ( gFingertipPoseEstimation.Initialize( frame, "calibration/calibration.txt" ) == false )
    {
#ifndef _WIN32
        fprintf(stderr, "Warning: FingertipPoseEstimation init failed (continuing anyway)\n");
        fflush(stderr);
#else
        return -1;
#endif
    }
    fprintf(stderr, "DBG: FingertipPoseEstimation init done\n"); fflush(stderr);

	//120325 cwj
    // initialize video writer
    //gpRecord = cvCreateVideoWriter( RECORD_FILENAME, -1, 15, cvGetSize(frame) );
    //gpRecordOutput = cvCreateVideoWriter( RECORD_FILENAME_OUTPUT, -1, 15, cvGetSize(frame) );


    // Start main Glut loop
    //glutMainLoop();
	//~120325 cwj



	//////////////////////////////////////////////////////////////////////////

	threadResults1.dir=0;
	hMutex = CreateMutex(NULL, FALSE, NULL);
	
	bIsTracking[0]=false;
	bIsTracking[1]=false;

	bIsTrackingInit[0]=true;
	bIsTrackingInit[1]=true;

	g_bisRunTrackingThread=false;

	InitializeCriticalSection(&hCriticalSection[0]);

	#ifdef	Calculate_Time
		double	time_matching;
	#endif
	
	
	// The BRISK marker database and the matching/tracking threads moved to
	// legacy_marker_tracker.cpp (IMarkerTracker); the marker pose moved to
	// legacy_pose.cpp (IPoseEstimator).
	fprintf(stderr, "DBG: InitializeEngineMain complete!\n"); fflush(stderr);
	return 0;
}

int ReleaseEngineMain()
{
	
		

	/////////////////////////////////////////////////////////////////////////////////

	
	//시간측정하는 코드...예제
	double	time_start  = cv::getTickCount();
	double	time_detect_and_describing = (cv::getTickCount() - time_start ) / cv::getTickFrequency() ;
	
	time_start = cv::getTickCount();	
	
	double time_matching = (cv::getTickCount() - time_start ) / cv::getTickFrequency();
	
	
	//cout << "Detection and Description : " << time_detect_and_describing << " ms\t";
	//cout << "Matching : " << time_matching << " ms\t";
	//cout << "TOTAL : " << time_total_consuming << " ms\t";
	cout << "FPS : " << time_matching << " frames" << endl;
	
	
	//CloseHandle( hDrawThread);
	CloseHandle( hMatchingThread[0] );	
	CloseHandle( hMatchingThread[1] );
	
	//CloseHandle( hTrackingThread[0] );

	//mslNFT_Log.close();
	return 0;
}

// ThreadDraw, ThreadBRISKMatching and ThreadTracking moved to
// legacy_marker_tracker.cpp (the draw thread was never started).

#ifdef _WIN32
INT APIENTRY WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nShowCmd )
{
	//console
	AllocConsole();
	freopen("CONOUT$","wt",stdout);

	//////////////////////////////////////////////////////////////////////////
	//initialize directX
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	//g_hInst=hInstance;

	WndClass.cbClsExtra=0;
	WndClass.cbWndExtra=0;
	WndClass.hbrBackground=(HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor=LoadCursor(NULL,IDC_ARROW);
	WndClass.hIcon=LoadIcon(NULL,IDI_APPLICATION);
	WndClass.hInstance=hInstance;
	WndClass.lpfnWndProc=(WNDPROC)WndProc;
	WndClass.lpszClassName=lpszClass;
	WndClass.lpszMenuName=NULL;
	WndClass.style=CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	hWnd=CreateWindow(lpszClass,lpszClass,WS_POPUPWINDOW,
		CW_USEDEFAULT,CW_USEDEFAULT,640,480,
		NULL,(HMENU)NULL,hInstance,NULL);
	ShowWindow(hWnd,nShowCmd);


	//초기화 (EN: Initialization)
	if(FAILED(wonjo_dx::AAR3DInitD3D(hWnd)))
	{
		MessageBox(NULL,"DirectX Device Failed.\nthe application will be terminated.","BaekAR",MB_OK);
		return 0;	//실패시 윈도우 끝내버림. (EN: Terminate window on failure)
	}
	//~initialize directX
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////


	InitializeEngineMain();

	//direct X code mainloop
	PeekMessage( &Message, NULL, 0U, 0U, PM_REMOVE );
	while(true) {	//main loop start

		//윈도우 핸들링이 들어올 때 처리 (EN: Handle window messages)
		if( PeekMessage( &Message, NULL, 0U, 0U, PM_REMOVE ) )
		{
			TranslateMessage( &Message );
			DispatchMessage( &Message );
			continue;
		}
		//메시지가 들어오지 않았을 때 처리 (main loop) (EN: Process main loop when no messages)
		else mainLoop();
	}	//end of main loop
	//~direct X code

	ReleaseEngineMain();
	return 0;
}
#else
// Entry points for the application layer (legacy_engine.h). main() lives in
// apps/baekar/main.cpp; frames, marker tracking and the marker pose come
// from the application's ports.

static int    g_argc = 0;
static char** g_argv = nullptr;

namespace legacy_engine {

bool Prepare(const Options& options, int argc, char** argv)
{
	g_argc = argc;
	g_argv = argv;
	wonjo_dx::bDrawHand = options.handTracking ? TRUE : FALSE;

	// Window-as-AR-texture (thesis novelty): ScreenCaptureKit streams the
	// chosen window's pixels into g_winFrameBuf.
	if (options.windowCapture && options.cameraInput) {
#ifdef __APPLE__
		const uint32_t winId = WCPicker_PickWindowID();
		if (winId != 0) {
			g_winStream = WCStream_Open(winId, kWinTexW, kWinTexH);
			if (g_winStream) {
				g_winFrameBuf.assign(kWinTexW * kWinTexH * 4, 0);
				fprintf(stderr, "BaekAR: window stream ready — plane will spawn on first marker pose.\n");
			}
		}
#else
		fprintf(stderr, "BaekAR: --window-capture needs macOS ScreenCaptureKit; ignored.\n");
#endif
	}
	fflush(stderr);
	return true;
}

bool InitializeRenderer()
{
	// GLUT supplies glutSolidCone and bitmap fonts used by init() and overlays.
	glutInit(&g_argc, g_argv);

	// Initialize D3D stub (allocates static gpDevice so GetDevice() is non-null)
	wonjo_dx::AAR3DInitD3D(nullptr);
	init();
	return true;
}

static void SetInputFrame(const FrameInput& input)
{
	g_inputFrame = input.bgr;
	g_inputSequence = input.sequence;
	g_inputTick = input.tickCount;
	g_inputLive = input.live;
}

bool Start(const FrameInput& firstFrame)
{
	// Runs on the GL thread: FingertipPoseEstimation::Initialize calls
	// glGenTextures, and macOS GL calls only work on the context's thread.
	SetInputFrame(firstFrame);
	fprintf(stderr, "BaekAR: Starting engine initialization...\n");
	const int rc = InitializeEngineMain();
	if (rc != 0) {
		fprintf(stderr, "BaekAR: Engine initialization failed (%d).\n", rc);
		return false;
	}
	fprintf(stderr, "BaekAR: Engine initialized successfully.\n");
	return true;
}

void RenderFrame(const FrameInput& frame, const TrackingInput& tracking, const Pointer& pointer)
{
	SetInputFrame(frame);
	g_tracking = tracking;
	g_pointer = pointer;
	mainLoop();
}

void Shutdown()
{
	ReleaseEngineMain();
#ifdef __APPLE__
	if (g_winStream) {
		WCStream_Close(g_winStream);
		g_winStream = nullptr;
	}
#endif
}

}  // namespace legacy_engine
#endif






#ifdef _WIN32
//Message Loop
LRESULT CALLBACK WndProc(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	switch(iMessage) {
	case WM_CREATE:
		return 0;
	case WM_PAINT:
		hdc=BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		wonjo_dx::AAR3DReleaseSystem();
		ExitProcess(0);
		PostQuitMessage(0);
		return 0;
	case WM_LBUTTONDOWN:
		//std::cout << "click" << std::endl;
		{
// 			POINT pt;
// 			GetCursorPos(&pt);
// 			ScreenToClient(hWnd, &pt);
// 			wonjo_dx::Picking(pt);
			/*
			D3DXVECTOR3 triangles[6];
			triangles[0]=D3DXVECTOR3(-30,-30,0);
			triangles[1]=D3DXVECTOR3(-30,30,0);
			triangles[2]=D3DXVECTOR3(30,-30,0);
			triangles[3]=D3DXVECTOR3(30,-30,0);
			triangles[4]=D3DXVECTOR3(-30,30,0);
			triangles[5]=D3DXVECTOR3(30,30,0);
			d3d::PickWithTriangle(pt,triangles,6);
			*/
		}
		return 0;
	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_ESCAPE:
			wonjo_dx::AAR3DReleaseSystem();
			ExitProcess(0);
			PostQuitMessage(0);
			return 0;
		}
	}
	return(DefWindowProc(hWnd,iMessage,wParam,lParam));
}
#endif // _WIN32
