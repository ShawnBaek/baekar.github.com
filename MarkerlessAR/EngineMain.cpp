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
#include <GLFW/glfw3.h>
static GLFWwindow* g_window = nullptr;
static bool g_mouseDown = false;
static bool g_mouseWasDown = false;
static double g_mousePrevX = 0, g_mousePrevY = 0;
#endif

#ifdef __APPLE__
#include <dirent.h>
#include <algorithm>
#include "compat/macos_window_capture.h"
#include "Contents.hpp"
static Contents  g_contents;
static WCStream* g_winStream  = nullptr;
static unsigned int g_winTexture = 0;
static const int    kWinTexW    = 512;
static const int    kWinTexH    = 512;
static std::vector<uint8_t> g_winFrameBuf;
static bool         g_winPlaneSpawned = false;
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
// Shared frame buffer: mainLoop captures from gCapture and stores the latest
// frame here.  Matching/tracking threads read from this shared buffer.
#include <mutex>
static std::mutex g_frameMutex;
static Mat g_sharedFrame;

static Mat getSharedFrame() {
	std::lock_guard<std::mutex> lock(g_frameMutex);
	return g_sharedFrame.clone();
}
static void setSharedFrame(const Mat& frame) {
	std::lock_guard<std::mutex> lock(g_frameMutex);
	frame.copyTo(g_sharedFrame);
}
// Dummy VideoCapture objects (never opened) to keep Windows code path compilable
VideoCapture	capture;
VideoCapture	capture1;
VideoCapture	capture2;

// Camera fallback: when camera is unavailable, use a dummy frame
static bool g_cameraAvailable = false;
static cv::Mat g_dummyFrameMat;
static IplImage g_dummyIpl;
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

#ifdef __APPLE__
// Lists feature-detectable photos in image/ and lets the user pick one as
// the marker image (the BRISK reference). Empty input or a bad index
// keeps the hardcoded default in Filename[0].
static std::string PickMarkerImage()
{
	const char* dirpath = "image";
	DIR* d = opendir(dirpath);
	if (!d) {
		fprintf(stderr, "MarkerPicker: cannot open '%s' (cwd-relative); keeping default\n", dirpath);
		return "";
	}
	std::vector<std::string> images;
	struct dirent* ent;
	while ((ent = readdir(d)) != nullptr) {
		std::string name = ent->d_name;
		if (name.size() < 5) continue;
		std::string lower = name;
		std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
		auto endsWith = [&](const char* ext) {
			size_t L = strlen(ext);
			return lower.size() >= L && lower.compare(lower.size()-L, L, ext) == 0;
		};
		if (endsWith(".jpg") || endsWith(".jpeg") || endsWith(".png"))
			images.push_back(name);
	}
	closedir(d);
	std::sort(images.begin(), images.end());

	if (images.empty()) {
		fprintf(stderr, "MarkerPicker: no images found in '%s'\n", dirpath);
		return "";
	}

	fprintf(stderr, "\n=== Pick a feature-detectable marker image ===\n");
	for (size_t i = 0; i < images.size(); ++i)
		fprintf(stderr, "  [%2zu] %s\n", i, images[i].c_str());
	fprintf(stderr, "Enter index (0-%zu), or anything else to keep default (yejin.jpg): ",
	        images.size()-1);
	fflush(stderr);

	char line[64] = {0};
	if (!fgets(line, sizeof(line), stdin)) return "";
	int idx = -1;
	if (sscanf(line, "%d", &idx) != 1 || idx < 0 || idx >= (int)images.size())
		return "";

	std::string path = std::string(dirpath) + "/" + images[idx];
	fprintf(stderr, "MarkerPicker: chose [%d] %s\n", idx, path.c_str());
	return path;
}
#endif

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

#ifndef _WIN32
#include <thread>
#include <atomic>
static bool g_engineInitialized = false;
static std::atomic<bool> g_engineInitStarted{false};
static std::atomic<bool> g_engineInitDone{false};
static std::atomic<int>  g_engineInitResult{-1};

static void engineInitThread() {
	g_engineInitResult = InitializeEngineMain();
	g_engineInitDone = true;
}
#endif

static void mainLoop(void)
{
#ifndef _WIN32
	// Run engine init synchronously on the main thread the first time mainLoop
	// fires. Capture::open has its own 10s timeout and the AVFoundation auth
	// request runs at startup, so the only way init blocks is a stuck camera
	// daemon — which the timeout handles. Init must run on the main thread
	// because FingertipPoseEstimation::Initialize calls glGenTextures, and on
	// macOS GL calls only work on the thread that owns the context.
	if (!g_engineInitialized) {
		fprintf(stderr, "BaekAR: Starting engine initialization...\n");
		fflush(stderr);
		int rc = InitializeEngineMain();
		if (rc != 0) {
			fprintf(stderr, "BaekAR: Engine initialization failed (%d). Check camera permissions.\n", rc);
			fprintf(stderr, "BaekAR: On macOS, grant camera access in:\n");
			fprintf(stderr, "        System Settings > Privacy & Security > Camera\n");
			fflush(stderr);
			if (g_window) glfwSetWindowShouldClose(g_window, GLFW_TRUE);
			return;
		}
		fprintf(stderr, "BaekAR: Engine initialized successfully.\n");
		fflush(stderr);
		g_engineInitialized = true;
		return; // skip first frame to let things settle
	}
#endif
#ifdef _WIN32
	if( GetKeyState(VK_LBUTTON) & 0x8000 )
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(NULL, &pt);
		wonjo_dx::Picking(pt);
	}
#else
	if (g_window) {
		double mx, my;
		glfwGetCursorPos(g_window, &mx, &my);

		if (g_mouseDown && !g_mouseWasDown) {
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
		} else if (g_mouseDown && g_mouseWasDown) {
			// Drag: translate the selected item.
			g_contents.dragSelected(mx - g_mousePrevX, my - g_mousePrevY);
		}
		g_mousePrevX = mx;
		g_mousePrevY = my;
		g_mouseWasDown = g_mouseDown;
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
	if(bThreadTracking1==true)
	{		
		cvLine(image, cvPoint((int)dst_tracking_corners1[0].x,(int)dst_tracking_corners1[0].y), cvPoint((int)dst_tracking_corners1[1].x,(int)dst_tracking_corners1[1].y), cvScalar( 0, 255, 255), 4);
		cvLine(image, cvPoint((int)dst_tracking_corners1[1].x,(int)dst_tracking_corners1[1].y), cvPoint((int)dst_tracking_corners1[2].x,(int)dst_tracking_corners1[2].y), cvScalar( 0, 255, 255), 4);
		cvLine(image, cvPoint((int)dst_tracking_corners1[2].x,(int)dst_tracking_corners1[2].y), cvPoint((int)dst_tracking_corners1[3].x,(int)dst_tracking_corners1[3].y), cvScalar( 0, 255, 255), 4);
		cvLine(image, cvPoint((int)dst_tracking_corners1[3].x,(int)dst_tracking_corners1[3].y), cvPoint((int)dst_tracking_corners1[0].x,(int)dst_tracking_corners1[0].y), cvScalar( 0, 255, 255), 4);
	}
	else if(bThreadDetection1==true)
	{
		cvLine(image, cvPoint((int)dst_matching_corners1[0].x,(int)dst_matching_corners1[0].y), cvPoint((int)dst_matching_corners1[1].x,(int)dst_matching_corners1[1].y), cvScalar( 255, 255, 255), 4);
		cvLine(image, cvPoint((int)dst_matching_corners1[1].x,(int)dst_matching_corners1[1].y), cvPoint((int)dst_matching_corners1[2].x,(int)dst_matching_corners1[2].y), cvScalar( 255, 255, 255), 4);
		cvLine(image, cvPoint((int)dst_matching_corners1[2].x,(int)dst_matching_corners1[2].y), cvPoint((int)dst_matching_corners1[3].x,(int)dst_matching_corners1[3].y), cvScalar( 255, 255, 255), 4);
		cvLine(image, cvPoint((int)dst_matching_corners1[3].x,(int)dst_matching_corners1[3].y), cvPoint((int)dst_matching_corners1[0].x,(int)dst_matching_corners1[0].y), cvScalar( 255, 255, 255), 4);
	}
	
	
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
		//D3DXMatrixPerspectiveFovLH(&matProj,Deg2Rad(73.0f),640/480,1.0f,100000.0f);
		camera.D3DXMakeProjectionMatrix(&matProj);
		wonjo_dx::SetProjectionMatrix(&matProj);

		if(bThreadDetection1 || bThreadTracking1)
		{
			camera.featurePoseEstimation();
			
			camera.D3DXMakeViewMatrix(&matView);
			//D3DXMatrixLookAtLH(&matView,&D3DXVECTOR3(0,0,-200.0f),&D3DXVECTOR3(0,0,0),&D3DXVECTOR3(0,1,0));
			wonjo_dx::SetModelViewMatrix(&matView);

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

#ifndef _WIN32
	if (g_cameraAvailable) {
#endif
		gCapture.CaptureFrame();
		frame = gCapture.QueryFrame();
#ifndef _WIN32
	}
	if ( !frame )
	{
		// Use dummy frame when camera is unavailable
		frame = &g_dummyIpl;
	}
#else
	if ( !frame )
	{
		return;
	}
#endif
	gFingertipPoseEstimation.OnCapture( frame, gCapture.QueryTickCount() );

#ifndef _WIN32
	// Publish camera frame to shared buffer for matching/tracking threads
	if (g_cameraAvailable) {
		cv::Mat frameMat = cv::cvarrToMat(frame, false);
		setSharedFrame(frameMat);
	}
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

#ifdef __APPLE__
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
#ifdef __APPLE__
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
#ifdef __APPLE__
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

    // initialize capture
    bool cameraOk = false;
    if ( fInputVideoFile )
    {
        // Capture From File
        if ( !gCapture.Initialize( fFlipFrame, -1, "record_debug.avi" ) )
        {
            fprintf( stderr, "capture initialization failed.\n" );
        }
        else
            cameraOk = true;
    } else
    {
        // Live Capture From Camera
        if ( !gCapture.Initialize( fFlipFrame ) )
        {
            fprintf( stderr, "capture initialization failed.\n" );
        }
        else
            cameraOk = true;
    }

    // Init Capture
    IplImage * frame = 0;
    if (cameraOk) {
        gCapture.CaptureFrame();
        frame = gCapture.QueryFrame();
    }
    if ( !frame )
    {
#ifndef _WIN32
        // Camera unavailable — create a dummy frame so the app can still run
        fprintf( stderr, "Camera unavailable — running with dummy frame.\n" );
        fflush(stderr);
        g_cameraAvailable = false;
        g_dummyFrameMat = cv::Mat::zeros(480, 640, CV_8UC3);
        cv::putText(g_dummyFrameMat, "Camera Unavailable", cv::Point(140, 220),
                    cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(0, 0, 255), 2);
        cv::putText(g_dummyFrameMat, "BaekAR Engine Running", cv::Point(130, 280),
                    cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2);
        g_dummyIpl = cvIplImage(g_dummyFrameMat);
        frame = &g_dummyIpl;
#else
        fprintf( stderr, "failed to capture a frame...\n" );
        return -1;
#endif
    }
#ifndef _WIN32
    else {
        g_cameraAvailable = true;
    }
#endif

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
	
	
	// Note: capture/capture1/capture2 are NOT opened on macOS — only one VideoCapture
	// can access camera 0 at a time with AVFoundation. gCapture owns the camera.
	// The matching/tracking threads will get empty frames and skip processing.

	fprintf(stderr, "DBG: Creating BRISK detector...\n"); fflush(stderr);
	// Use OpenCV's built-in BRISK (the bundled MarkerlessAR/brisk/ predates
	// OpenCV's Feature2D interface and throws "not implemented" on detect()).
	{
		cv::Ptr<cv::BRISK> brisk = cv::BRISK::create(60, 2);
		detector = brisk;
		descriptorExtractor = brisk;
	}
	
	//BFMatcher matcher(NORM_L2);
	if(hamming){
		//descriptorMatcher1 = new BFMatcher(NORM_L1);
		//descriptorMatcher2 = new BFMatcher(NORM_L1);
		
		
		//descriptorMatcher2 = new BruteForceMatcher<HammingSse>(); 

		
		descriptorMatcher1 = new BFMatcher(NORM_HAMMING);
		descriptorMatcher2 = new BFMatcher(NORM_HAMMING);
		
	}
	else{
		//descriptorMatcher1 = new BruteForceMatcher<L2<float> >();
		//descriptorMatcher1 = new BruteForceMatcher<L2<float> >();
	}

	//Create BRISK Database

	//Read from Filename Structure 
	//temporaly filename override
	/*
	FILE* overridefile;
	overridefile=fopen("markerfile.txt","rt");
	if(overridefile)
	{
		char ch[256];
		memset(ch,0,sizeof(ch));
		fgets(ch,255,overridefile);
		fclose(overridefile);
		img_rgbdatabase1=imread(ch, 1);
	}
	else
	{
		img_rgbdatabase1=imread(Filename[0]._strFilename.c_str(), 1);
	}
	*/
	fprintf(stderr, "DBG: Loading database images...\n"); fflush(stderr);
	img_rgbdatabase1=imread(Filename[0]._strFilename.c_str(), 1);
	fprintf(stderr, "DBG: img1 loaded: %dx%d\n", img_rgbdatabase1.cols, img_rgbdatabase1.rows); fflush(stderr);
	//2번은 오버라이드하지않는다.
	img_rgbdatabase2=imread(Filename[1]._strFilename.c_str(), 1);
	fprintf(stderr, "DBG: img2 loaded: %dx%d\n", img_rgbdatabase2.cols, img_rgbdatabase2.rows); fflush(stderr);

	//Convert rgb to gray
	cvtColor(img_rgbdatabase1,img_graydatabase1, CV_BGR2GRAY);

	//For NCC Patch Tracking Move img_graydatabase1 to img_centerdatabase1
	img_centerdatabase1=swMoveImage(img_graydatabase1, &img_centerdatabase1, 0.5);
				
	//Extract Image Coners 
	obj_corners1[0] = cvPoint(0,0); 
	obj_corners1[1] = cvPoint( img_rgbdatabase1.cols, 0 );
	obj_corners1[2] = cvPoint( img_rgbdatabase1.cols, img_rgbdatabase1.rows ); 
	obj_corners1[3] = cvPoint( 0, img_rgbdatabase1.rows );

	//Move obj_corners to center
	swMoveCorners(img_graydatabase1, obj_corners1, obj_center_corners1, 0.0, 1.0);
		
	fprintf(stderr, "DBG: Detecting keypoints...\n"); fflush(stderr);
	//Detect Keypoints from img_centerdatase using AGAST Feature Detector
	detector->detect(img_centerdatabase1,kp_database1);
	//detector->detect(img_centerdatabase2,kp_database2);
	

	//Create Description from img_centerdatabase1 using BRISK Feature Descriptor
	descriptorExtractor->compute(img_centerdatabase1,kp_database1,desc_database1);
	//descriptorExtractor->compute(img_centerdatabase2,kp_database2,desc_database2);
	
	
#ifdef _WIN32
	capture >> gDetectionResult1;
#else
	// On macOS, use gCapture's frame (capture is not opened separately)
	{
		IplImage* initFrame = gCapture.QueryFrame();
		if (initFrame) {
			gDetectionResult1 = cv::cvarrToMat(initFrame, true);
		}
	}
#endif
	if(gDetectionResult1.empty()) {
		gDetectionResult1 = Mat::zeros(480, 640, CV_8UC3);
	}
	
	int val1=1, val2=2, val3=3, val4=4, val5=5, val6=6,val7=7, val8=8, val9=9, val10=10, val11=11, val12=12;

	fprintf(stderr, "DBG: Starting threads...\n"); fflush(stderr);
	hMatchingThread[0] = (HANDLE)_beginthreadex( NULL, 0, &ThreadBRISKMatching, &val1, 0, &uMatchingThreadID[0] );
	//hMatchingThread[1] = (HANDLE)_beginthreadex( NULL, 0, &ThreadBRISKMatching, &val2, 0, &uMatchingThreadID[1] );
	hTrackingThread[0] = (HANDLE)_beginthreadex( NULL, 0, &ThreadTracking, &val1, 0, &uTrackingThreadID[0] );

	

	//hDrawThread=(HANDLE)_beginthreadex( NULL, 0, &ThreadDraw, 0, 0, &uDrawThreadID);
		
	// Camera resolution is now set inside gCapture.Initialize() via cv::VideoCapture
	// capture_C = cvCaptureFromCAM(0);  // removed — legacy C API, gCapture handles capture
	// cvSetCaptureProperty(capture_C , CV_CAP_PROP_FRAME_WIDTH, 640);
	// cvSetCaptureProperty(capture_C , CV_CAP_PROP_FRAME_HEIGHT, 480);


	//remove opengl
// 	glutInit(&argc, argv);
// 	init();
 	fprintf(stderr, "DBG: Loading camera calibration...\n"); fflush(stderr);
 	camera.load("calibration/calibration.txt");
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

unsigned int ThreadDraw(void *param)
{
	namedWindow("BaekAR", CV_WINDOW_AUTOSIZE|CV_GUI_NORMAL);
	while(true){


#ifdef _WIN32
		capture1>>mInput;
#else
		mInput = getSharedFrame();
#endif

		// Skip if no frame available yet
		if(mInput.empty()) {
#ifndef _WIN32
			struct timespec ts = {0, 50000000}; // 50ms
			nanosleep(&ts, NULL);
#endif
			continue;
		}

		//Detection or Tracking 체크

		if(bThreadTracking1==true)
		{
			
			line( mInput, dst_tracking_corners1[0], dst_tracking_corners1[1], cvScalar( 0, 255, 255), 4 );
			line( mInput, dst_tracking_corners1[1], dst_tracking_corners1[2], cvScalar( 0, 255, 255), 4 );
			line( mInput, dst_tracking_corners1[2], dst_tracking_corners1[3], cvScalar( 0, 255, 255), 4 );
			line( mInput, dst_tracking_corners1[3], dst_tracking_corners1[0], cvScalar( 0, 255, 255), 4 );	

		}else
		{

			line( mInput, dst_matching_corners1[0], dst_matching_corners1[1], cvScalar( 255, 255, 255), 4 );
			line( mInput, dst_matching_corners1[1], dst_matching_corners1[2], cvScalar( 255, 255, 255), 4 );
			line( mInput, dst_matching_corners1[2], dst_matching_corners1[3], cvScalar( 255, 255, 255), 4 );
			line( mInput, dst_matching_corners1[3], dst_matching_corners1[0], cvScalar( 255, 255, 255), 4 );	

		}

		/*
		if(bThreadDetection2==true)
		{
			for(int i=0; i<4; ++i) {
			line(mInput,
				dst_matching_corners2[i],		dst_matching_corners2[(i+1)%4],	Scalar(0, 255, 0),	3);
			}
		}*/
		
		imshow("BaekAR", mInput);
		
		cvWaitKey(1);

	

	}	
	destroyWindow("BaekAR");
	_endthreadex(0);
	return 0;

}



unsigned int ThreadBRISKMatching(void *param)
{
	//namedWindow("BaekAR", CV_WINDOW_AUTOSIZE|CV_GUI_NORMAL);

	int idxcount=*((int*)param);
	Mat outimg;
	
	bool mbistracking=false;

	bool mbisDetecting=false;

	vector<KeyPoint>	kp_camera_matching_thread;
	Mat					desc_camera_matching_thread;

	Mat matching_thread_rgbcamera, matching_thread_graycamera, matching_thread_result;
		
	vector<KeyPoint>			kp_database;
	Mat							desc_database;
	Mat							img_database;
	vector<Point2f>				obj_matching_corners(4);
	vector<Point2f>				dst_matching_corners(4);


	//namedWindow("Matches");
	if(idxcount==1){
			img_database=img_centerdatabase1;
 			kp_database=kp_database1;
			desc_database=desc_database1;
			obj_matching_corners=obj_center_corners1;
			mbisDetecting=bThreadDetection1;
			mbistracking=bThreadTracking1;

	}
	else if(idxcount==2){
			img_database=img_centerdatabase2;
			kp_database=kp_database2;
			desc_database=desc_database2;
			//mbistracking=bIsTracking[1];
			mbisDetecting=bThreadDetection2;
			mbistracking=bThreadTracking2;



	}

	while(true){
				
		if(idxcount==1){
#ifdef _WIN32
			capture1>>matching_thread_rgbcamera;
#else
			matching_thread_rgbcamera = getSharedFrame();
#endif
			mbistracking=bThreadTracking1;

		}
		else if(idxcount==2){
#ifdef _WIN32
			capture2>>matching_thread_rgbcamera;
#else
			matching_thread_rgbcamera = getSharedFrame();
#endif
			mbistracking=bThreadTracking2;
		}

		// Skip if no frame available yet
		if(matching_thread_rgbcamera.empty()) {
#ifndef _WIN32
			struct timespec ts = {0, 50000000}; // 50ms
			nanosleep(&ts, NULL);
#endif
			continue;
		}

		//Convert Camera Input with RGB to Camera Input with GRAY
		cvtColor(matching_thread_rgbcamera, matching_thread_graycamera, CV_BGR2GRAY);
		
//#ifdef _DEBUG
//		return 0;
//#endif
		detector->detect(matching_thread_graycamera,kp_camera_matching_thread);
		descriptorExtractor->compute(matching_thread_graycamera,kp_camera_matching_thread,desc_camera_matching_thread);
		
		matching_thread_result=matching_thread_rgbcamera;
		std::vector<std::vector<DMatch> > matches;
		
		//Matching /
		vector<DMatch> matches_popcount; 
		//double pop_time = match(kpts_1, kpts_2, matcher_popcount, desc_1, desc_2, matches_popcount);
		
		// Original used radiusMatch(..., 100.0) which accepts every train
		// descriptor within Hamming distance 100 — far too loose for OpenCV's
		// 64-byte BRISK, so RANSAC kept fitting confident-but-wrong homographies
		// (e.g. yejin.jpg's bounds locking onto terminal text).
		// knnMatch(k=2) + Lowe's ratio (best/second-best < 0.75) is the standard
		// robust filter and gives stable, marker-aligned poses.
		{
			std::vector<std::vector<DMatch>> raw;
			descriptorMatcher1->knnMatch(desc_camera_matching_thread, desc_database, raw, 2);
			matches.clear();
			matches.reserve(raw.size());
			for (size_t k = 0; k < raw.size(); ++k) {
				if (raw[k].size() == 2 &&
				    raw[k][0].distance < 0.75f * raw[k][1].distance) {
					matches.push_back({ raw[k][0] });
				} else if (raw[k].size() == 1) {
					matches.push_back({ raw[k][0] });
				}
			}
		}
		
			
		//Compute Homography
		vector<Point2f> mpts_1, mpts_2;		// train이 1 query가 2
		CvPoint dst_corners[4];
		matches2points(matches, kp_database, kp_camera_matching_thread, mpts_1, mpts_2); //Extract a list of the (x,y) location of the matches
		
		outimg = Mat::zeros(matching_thread_rgbcamera.rows, matching_thread_rgbcamera.cols, matching_thread_rgbcamera.type());
		
		//Draw Matching Results between Camera and Database Image
		
		
		drawMatches(matching_thread_rgbcamera, kp_camera_matching_thread, img_database, kp_database, matches,outimg,
			Scalar(0,255,0), Scalar(0,0,255),
			std::vector<std::vector<char> >(), DrawMatchesFlags::DRAW_RICH_KEYPOINTS );
		/*
		imshow("BaekAR", outimg);
		cvWaitKey(1);
		*/
		 // need at least 5 matched pairs of points (more are better)
		 if (computeHomography && (mpts_1.size() > 5))
         {
                			
			    if(bInitTracking==true){
					g_mpts_1=mpts_1;
					g_mpts_2=mpts_2;
					
					bInitTracking=false;
				}else if(bThreadTracking1==false){

					
					g_mpts_1=mpts_1;
					g_mpts_2=mpts_2;

				}
				//Find Homography
				// Original used `&&` (skip only when BOTH < 5) — wrong: findHomography
				// requires both inputs to have ≥4 points and equal size. Use `||`.
				if(mpts_1.size()<5 || mpts_2.size()<5 || mpts_1.size()!=mpts_2.size())
					continue;

				Mat H = findHomography(Mat(mpts_1), Mat(mpts_2), RANSAC, 2);

				// RANSAC can fail to find a model and return an empty Mat; guard
				// before perspectiveTransform asserts on it.
				if(H.empty() || H.cols != 3 || H.rows != 3)
					continue;

				//Convert Object Corners to Transformed Object Corners Using Homography Matrix Information
				perspectiveTransform( obj_matching_corners, dst_matching_corners, H);
				
				//Draw Object Contour Line 
				/*
				line( outimg, dst_matching_corners[0], dst_matching_corners[1], Scalar(0, 255, 0), 4 );
				line( outimg, dst_matching_corners[1], dst_matching_corners[2], Scalar( 0, 255, 0), 4 );
				line( outimg, dst_matching_corners[2], dst_matching_corners[3], Scalar( 0, 255, 0), 4 );
				line( outimg, dst_matching_corners[3], dst_matching_corners[0], Scalar( 0, 255, 0), 4 );	
				imshow("Matches", outimg);
				*/
				
				//waitKey(1);	

				dst_matching_corners1=dst_matching_corners;
				if(idxcount==1 && bThreadTracking1==false){
					//threadResults1.
					//3D OBJECT 를 위한 공간

					threadResults1.cf=true;			


					threadResults1.vertex[0].x=dst_matching_corners[0].x;
					threadResults1.vertex[0].y=dst_matching_corners[0].y;
					threadResults1.vertex[1].x=dst_matching_corners[1].x;
					threadResults1.vertex[1].y=dst_matching_corners[1].y;
					threadResults1.vertex[2].x=dst_matching_corners[2].x;
					threadResults1.vertex[2].y=dst_matching_corners[2].y;
					threadResults1.vertex[3].x=dst_matching_corners[3].x;
					threadResults1.vertex[3].y=dst_matching_corners[3].y;


					//bsw 새롭게 추가한 코드
					camera.featuresResult.vertex[0].x=dst_matching_corners[0].x;
					camera.featuresResult.vertex[0].y=dst_matching_corners[0].y;
					camera.featuresResult.vertex[1].x=dst_matching_corners[1].x;
					camera.featuresResult.vertex[1].y=dst_matching_corners[1].y;
					camera.featuresResult.vertex[2].x=dst_matching_corners[2].x;
					camera.featuresResult.vertex[2].y=dst_matching_corners[2].y;
					camera.featuresResult.vertex[3].x=dst_matching_corners[3].x;
					camera.featuresResult.vertex[3].y=dst_matching_corners[3].y;

					double dx, dy;
								
					//CvPoint2D p1, p2;
					//bsw 필요없는 코드일듯
					/*
					for(int i=0; i<4; ++i) {
				
						p1=camera.featuresResult.vertex[i];
						p2=camera.featuresResult.vertex[(i+1)%4];

						dx=p1.x-p2.x;
						dy=p1.y-p2.y;

						camera.featuresResult.line[i][0]=-dy;
						camera.featuresResult.line[i][1]=dx;
						camera.featuresResult.line[i][2]=(p2.x)*(p1.y)-(p1.x)*(p2.y);

				
					}*/

			
					camera.featuresResult.center.x=camera.featuresResult.vertex[0].x+((camera.featuresResult.vertex[1].x-camera.featuresResult.vertex[0].x)/2);
					camera.featuresResult.center.y=camera.featuresResult.vertex[0].y+((camera.featuresResult.vertex[3].y-camera.featuresResult.vertex[0].y)/2);
			
					//matching_thread_rgbcamera.copyTo(gDetectionResult1);
			
					

					/*
					if(bThreadTracking1==false && mbisDetecting==true){
						g_mpts_1=mpts_1;
						g_mpts_2=mpts_2;
					}
			
					bThreadDetection1=mbisDetecting;
					*/
			
				}
				
				mbisDetecting=true;

				
				
		}else
			mbisDetecting=false;

		bThreadDetection1=mbisDetecting;
		
			
		
			

		// Was: explicit ~vector / ~Mat on stack-local objects (UB — next
		// iteration writes through freed storage and heap-corrupts).
		matches.clear();

		desc_camera_matching_thread.release();
		kp_camera_matching_thread.clear();

		matching_thread_rgbcamera.release();
		matching_thread_graycamera.release();
		matching_thread_result.release();

		dst_matching_corners.clear();

	}

	// kp_database / desc_database are auto-storage; let scope exit destruct them.
	
	_endthreadex(0);
	return 0;
	
}



unsigned int ThreadTracking(void *param)
{
		
	int idxcount=*((int*)param);
	
	bool mbistracking=false;
	bool mbisDetecting=false;
	
	bool mbisPrevMatchLoc=false;
	Point prev_matchLoc;
	Mat tracking_thread_rgbcamera, tracking_thread_transformedcamera;
		
	vector<KeyPoint>			kp_database;
	Mat							desc_database;
	Mat							img_database;

	vector<Point2f>				obj_tracking_corners(4);
	vector<Point2f>				dst_tracking_corners(4);
	
	int							match_method;
	int							windowSize=20;

	
	Mat img_database_resize;
	img_database_resize.cols = img_rgbdatabase1.cols/2; 
	img_database_resize.rows = img_rgbdatabase1.rows/2;
	resize(img_rgbdatabase1,img_database_resize,img_database_resize.size());

	//경민 capture2>>tracking_thread_rgbcamera를 capture1으로 수정함
#ifdef _WIN32
	capture1>>tracking_thread_rgbcamera;
#else
	tracking_thread_rgbcamera = getSharedFrame();
#endif
	if(tracking_thread_rgbcamera.empty()) {
		tracking_thread_rgbcamera = Mat::zeros(480, 640, CV_8UC3);
	}

	Mat tracking_thread_result(tracking_thread_rgbcamera.rows-img_database_resize.rows+1,tracking_thread_rgbcamera.cols-img_database_resize.cols,CV_32FC1);


	while(true)
	{
		if(idxcount==1){
			
			mbisDetecting=bThreadDetection1;
		}
		else if(idxcount==2){
				
			mbisDetecting=bThreadDetection2;
		}

		//Only Tracking will be run when Detection Condition Value was True
		if(mbisDetecting==true)
		{

			//경민 capture2를 capture1으로 수정함
#ifdef _WIN32
			capture1>>tracking_thread_rgbcamera;
#else
			tracking_thread_rgbcamera = getSharedFrame();
#endif

			// Skip if no frame available yet
			if(tracking_thread_rgbcamera.empty()) {
#ifndef _WIN32
				struct timespec ts = {0, 50000000}; // 50ms
				nanosleep(&ts, NULL);
#endif
				continue;
			}

			//minMaxLoc을 위한 함수
			double minVal; double maxVal; Point minLoc; Point maxLoc; Point matchLoc;

			match_method=CV_TM_CCOEFF_NORMED;

			//Homography & NCC
			//Cam으로부터 받아오는 영상의 포인트 mpts_2, DB로부터 받아오는 포인트 mpts_1
			// Original used `&&` (skip only when BOTH < 5) — wrong, see matching thread.
			if(g_mpts_2.size()<5 || g_mpts_1.size()<5 || g_mpts_2.size()!=g_mpts_1.size())
			{
				//mbisDetecting=false;

				bThreadTracking1=false;
				//트랙킹을 실패했을 경우 정보를 지워줌
				dst_tracking_corners.clear();
				dst_tracking_corners1=dst_tracking_corners;

				continue;
			}
			Mat HH = findHomography(Mat(g_mpts_2), Mat(g_mpts_1), RANSAC, 2);						//Homography
			// RANSAC can fail and return an empty Mat — guard before invert/perspectiveTransform.
			if(HH.empty() || HH.cols != 3 || HH.rows != 3) {
				bThreadTracking1=false;
				//트랙킹을 실패했을 경우 정보를 지워줌
				dst_tracking_corners.clear();
				dst_tracking_corners1=dst_tracking_corners;
				continue;
			}
			Mat HH_inver;


			invert(HH, HH_inver,DECOMP_LU );//역행렬 계산
			//bIsHomographyInvertMatrix = true;
				
			warpPerspective(tracking_thread_rgbcamera,tracking_thread_transformedcamera,HH,tracking_thread_rgbcamera.size(),INTER_LINEAR,BORDER_CONSTANT);		//Wrapping
			//dilate(Outout,Outout,Mat(3,3,CV_32FC1));
						
			//Prev Frame

			
			if(mbisPrevMatchLoc==false){
				matchTemplate(tracking_thread_transformedcamera,img_database_resize,tracking_thread_result,CV_TM_CCOEFF_NORMED);					//Templete matching

				minMaxLoc( tracking_thread_result, &minVal, &maxVal, &minLoc, &maxLoc, Mat() );

				mbisPrevMatchLoc=true;

				if( match_method  == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED )
				{ 
					matchLoc = minLoc; 

				}
				else
				{	matchLoc = maxLoc; 
				}

				prev_matchLoc=matchLoc;

			}else{
							
				/*
				Rect roi(Point(prev_matchLoc.x, prev_matchLoc.y), Size(img_rgbdatabase1.cols/2+windowSize, img_rgbdatabase1.rows/2+windowSize));
				Mat roi_tracking_thread_transformedcamera=Mat(tracking_thread_transformedcamera, roi);
				matchTemplate(roi_tracking_thread_transformedcamera,img_database_resize,tracking_thread_result,CV_TM_CCOEFF_NORMED);					//Templete matching
				minMaxLoc( tracking_thread_result, &minVal, &maxVal, &minLoc, &maxLoc, Mat() );


				if( match_method  == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED )
				{ 
					matchLoc = minLoc; 

				}
				else
				{	matchLoc = maxLoc; 
				}

				matchLoc+=prev_matchLoc;
				prev_matchLoc=matchLoc;
				namedWindow("wrapping");
				imshow("wrapping", roi_tracking_thread_transformedcamera);
				
				waitKey(1);
				*/
				
				if(prev_matchLoc.x-windowSize>=0 && prev_matchLoc.y-windowSize>=0){
					Rect roi(Point(prev_matchLoc.x-windowSize, prev_matchLoc.y-windowSize), Size(img_rgbdatabase1.cols/2+windowSize*2, img_rgbdatabase1.rows/2+windowSize*2));
					Mat roi_tracking_thread_transformedcamera=Mat(tracking_thread_transformedcamera, roi);
					matchTemplate(roi_tracking_thread_transformedcamera,img_database_resize,tracking_thread_result,CV_TM_CCOEFF_NORMED);					//Templete matching
					minMaxLoc( tracking_thread_result, &minVal, &maxVal, &minLoc, &maxLoc, Mat() );


					if( match_method  == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED )
					{ 
						matchLoc = minLoc; 

					}
					else
					{	matchLoc = maxLoc; 
					}

					matchLoc.x+=(prev_matchLoc.x-windowSize);
					matchLoc.y+=(prev_matchLoc.y-windowSize);
					//matchLoc+=prev_matchLoc;

					prev_matchLoc=matchLoc;
					/*
					namedWindow("wrapping");
					imshow("wrapping", roi_tracking_thread_transformedcamera);

					waitKey(1);
					*/
				}
				else{

					Rect roi(Point(prev_matchLoc.x, prev_matchLoc.y), Size(img_rgbdatabase1.cols/2+windowSize, img_rgbdatabase1.rows/2+windowSize));
					Mat roi_tracking_thread_transformedcamera=Mat(tracking_thread_transformedcamera, roi);
					matchTemplate(roi_tracking_thread_transformedcamera,img_database_resize,tracking_thread_result,CV_TM_CCOEFF_NORMED);					//Templete matching
					minMaxLoc( tracking_thread_result, &minVal, &maxVal, &minLoc, &maxLoc, Mat() );


					if( match_method  == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED )
					{ 
						matchLoc = minLoc; 

					}
					else
					{	matchLoc = maxLoc; 
					}

					matchLoc+=prev_matchLoc;
					prev_matchLoc=matchLoc;
					
					/*
					namedWindow("wrapping");
					imshow("wrapping", roi_tracking_thread_transformedcamera);
					waitKey(1);

					*/


				}
				
				
			}

			//원래 있던 코드 주석처리
			//matchTemplate(tracking_thread_transformedcamera,img_database_resize,tracking_thread_result,CV_TM_CCOEFF_NORMED);					//Templete matching
				
			//Normalization, 2012, 03, 04 by Sungwook
			//normalize( MatchingResult, MatchingResult, 0, 1, NORM_MINMAX, -1, Mat() );


			//minMaxLoc(MatchingResult, &min, &max, NULL, &left_top);								//collation

			//minMaxLoc, 2012, 03, 04 by Sungwook 원래 있던 코드 주석처리
			//minMaxLoc( tracking_thread_result, &minVal, &maxVal, &minLoc, &maxLoc, Mat() );
			
			/*
			if( match_method  == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED )
			{ 
				matchLoc = minLoc; 
								
			}
			else
			{	matchLoc = maxLoc; 
			}*/


			//Rectangle Souce Code 수정 by Sungwook
			//rectangle( wrapImgRGB2, matchLoc, Point( matchLoc.x + DBResizImg.cols , matchLoc.y + DBResizImg.rows ), Scalar::all(0), 2, 8, 0 );
				
			//사각형 그려주는 부분도 수정 2012/03/25~~~~~~~

			//rectangle( tracking_thread_transformedcamera, matchLoc, Point( matchLoc.x + img_rgbdatabase1.cols/2 , matchLoc.y + img_rgbdatabase1.rows/2 ), Scalar::all(0), 2, 8, 0 );
				
			//rectangle( tracking_thread_result, matchLoc, Point( matchLoc.x + img_database_resize.cols , matchLoc.y + img_database_resize.rows ), Scalar::all(0), 2, 8, 0 );
				

			//~~~~~사각형 그려주는 부분도 수정 2012/03/25



			//rectangle(wrapImgRGB2, left_top, Point(left_top.x + imgRGB1.cols/2, left_top.y + imgRGB1.rows/2), CV_RGB(255,0,0)); // 찾은 


			//printf("Min Value is %x\n", min);
			//NCC나 SAD는 MAX값을 기준으로!
			//printf("Max Value is %f\n", maxVal);
				
			//font
			char s_output_result[50];
			sprintf(s_output_result,"max:%f  min:%f",maxVal,minVal);    //우선 sprintf로 문자열 생성
			putText(tracking_thread_transformedcamera, s_output_result, Point(20,50), FONT_HERSHEY_SCRIPT_SIMPLEX, 1,Scalar::all(255),1,2);
			
			/*
			namedWindow("MatchingResult");
			imshow("MatchingResult",tracking_thread_result);
			waitKey(1);
			*/


			/*
			namedWindow("wrapping");
			imshow("wrapping", tracking_thread_transformedcamera);
			waitKey(1);
			*/
			
			//여기서 트랙킹이 실패했을 경우에만 Detection 쓰레드가 호출되도록 수정하자!!  이부분 값을 0.55 -> 0.65로 수정함
			if(maxVal < 0.65){
				//tracking_thread_result.release();
				bThreadTracking1=false;
				mbisPrevMatchLoc=false;

				//트랙킹을 실패했을 경우 정보를 지워줌
				dst_tracking_corners.clear();
				dst_tracking_corners1=dst_tracking_corners;
				
				printf("트랙킹 실패\n");
			}
			else
			{		
				bThreadTracking1=true;

				CvPoint p[4];
				p[0].x = matchLoc.x;
				p[0].y = matchLoc.y;

				p[1].x = matchLoc.x+img_rgbdatabase1.cols/2;
				p[1].y = matchLoc.y;

				p[2].x = matchLoc.x+img_rgbdatabase1.cols/2;
				p[2].y = matchLoc.y+img_rgbdatabase1.rows/2;

				p[3].x = matchLoc.x;
				p[3].y = matchLoc.y+img_rgbdatabase1.rows/2;

				obj_tracking_corners[0] = cvPoint(matchLoc.x,matchLoc.y); 
				obj_tracking_corners[1] = cvPoint( matchLoc.x+img_rgbdatabase1.cols/2, matchLoc.y);
				obj_tracking_corners[2] = cvPoint( matchLoc.x+img_rgbdatabase1.cols/2, matchLoc.y+img_rgbdatabase1.rows/2); 
				obj_tracking_corners[3] = cvPoint( matchLoc.x, matchLoc.y+img_rgbdatabase1.rows/2);
				perspectiveTransform( obj_tracking_corners, dst_tracking_corners, HH_inver);

				//tracking_thread_rgbcamera = tracking_thread_rgbcamera;
				
				/*
				line( tracking_thread_rgbcamera, dst_tracking_corners[0], dst_tracking_corners[1], Scalar(0, 255, 255), 4 );
				line( tracking_thread_rgbcamera, dst_tracking_corners[1], dst_tracking_corners[2], cvScalar( 0, 255, 255), 4 );
				line( tracking_thread_rgbcamera, dst_tracking_corners[2], dst_tracking_corners[3], cvScalar( 0, 255, 255), 4 );
				line( tracking_thread_rgbcamera, dst_tracking_corners[3], dst_tracking_corners[0], cvScalar( 0, 255, 255), 4 );	
				*/
				
				dst_tracking_corners1=dst_tracking_corners;

				/*
				namedWindow("TrackingResult");
				
				imshow("TrackingResult", tracking_thread_rgbcamera);
				waitKey(1);
				*/



				if(idxcount==1){
					//threadResults1.
					//3D OBJECT 를 위한 공간

					threadResults1.cf=true;			

					camera.featuresResult.vertex[0].x=dst_tracking_corners[0].x;
					camera.featuresResult.vertex[0].y=dst_tracking_corners[0].y;
					camera.featuresResult.vertex[1].x=dst_tracking_corners[1].x;
					camera.featuresResult.vertex[1].y=dst_tracking_corners[1].y;
					camera.featuresResult.vertex[2].x=dst_tracking_corners[2].x;
					camera.featuresResult.vertex[2].y=dst_tracking_corners[2].y;
					camera.featuresResult.vertex[3].x=dst_tracking_corners[3].x;
					camera.featuresResult.vertex[3].y=dst_tracking_corners[3].y;

			
					double dx, dy;

			
					CvPoint2D p1, p2;

					//BSW 필요없는 코드일듯
					/*
					for(int i=0; i<4; ++i) {
				
						p1=threadResults1.vertex[i];
						p2=threadResults1.vertex[(i+1)%4];

						dx=p1.x-p2.x;
						dy=p1.y-p2.y;

						threadResults1.line[i][0]=-dy;
						threadResults1.line[i][1]=dx;
						threadResults1.line[i][2]=(p2.x)*(p1.y)-(p1.x)*(p2.y);

				
					}*/

			
					camera.featuresResult.center.x=camera.featuresResult.vertex[0].x+((camera.featuresResult.vertex[1].x-camera.featuresResult.vertex[0].x)/2);
					camera.featuresResult.center.y=camera.featuresResult.vertex[0].y+((camera.featuresResult.vertex[3].y-camera.featuresResult.vertex[0].y)/2);
			
					//matching_thread_rgbcamera.copyTo(gDetectionResult1);
			

					dst_tracking_corners1=dst_tracking_corners;

					/*
					if(bThreadTracking1==false && mbisDetecting==true){
						g_mpts_1=mpts_1;
						g_mpts_2=mpts_2;
					}
			
					bThreadDetection1=mbisDetecting;
					*/
					//초기화
					dst_tracking_corners.clear();
					
				}
				else if(idxcount==2){


					//3D OBJECT 를 위한 공간

					threadResults2.cf=true;

					threadResults2.vertex[0].x=dst_tracking_corners[0].x;
					threadResults2.vertex[0].y=dst_tracking_corners[0].y;
					threadResults2.vertex[1].x=dst_tracking_corners[1].x;
					threadResults2.vertex[1].y=dst_tracking_corners[1].y;
					threadResults2.vertex[2].x=dst_tracking_corners[2].x;
					threadResults2.vertex[2].y=dst_tracking_corners[2].y;
					threadResults2.vertex[3].x=dst_tracking_corners[3].x;
					threadResults2.vertex[3].y=dst_tracking_corners[3].y;


					double dx, dy;
					CvPoint2D p1, p2;
					for(int i=0; i<4; ++i) {

						p1=threadResults2.vertex[i];
						p2=threadResults2.vertex[(i+1)%4];

						dx=p1.x-p2.x;
						dy=p1.y-p2.y;

						threadResults2.line[i][0]=-dy;
						threadResults2.line[i][1]=dx;
						threadResults2.line[i][2]=(p2.x)*(p1.y)-(p1.x)*(p2.y);


					}


					threadResults2.center.x=(threadResults2.vertex[1].x-threadResults2.vertex[0].x)/2;
					threadResults2.center.y=(threadResults2.vertex[3].y-threadResults2.vertex[0].y)/2;



					//matching_thread_rgbcamera.copyTo(mResult2);
			
					dst_tracking_corners2=dst_tracking_corners;

					bThreadDetection2=true;

			
			
				}		
		
			}

			//Release Variables
			//tracking_thread_result.release();
			//g_mpts_1.clear();
			//g_mpts_2.clear();

			tracking_thread_rgbcamera.release();
			tracking_thread_transformedcamera.release();
		}
				
	}			
	_endthreadex(0);
	return 0;

}

// 





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
// macOS entry point — GLFW windowing + OpenGL context

static void glfwKeyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void glfwMouseButtonCallback(GLFWwindow* /*window*/, int button, int action, int /*mods*/)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
		g_mouseDown = (action == GLFW_PRESS);
}

int main(int argc, char* argv[])
{
#ifndef _WIN32
	// Change working directory to the executable's directory so that data files
	// (skin.dis, calibration/, 3dobjects/, etc.) symlinked into the .app bundle
	// are found relative to the binary.
	{
		std::string exePath(argv[0]);
		auto lastSlash = exePath.rfind('/');
		if (lastSlash != std::string::npos) {
			std::string exeDir = exePath.substr(0, lastSlash);
			chdir(exeDir.c_str());
			fprintf(stderr, "BaekAR: Working directory set to %s\n", exeDir.c_str());
			fflush(stderr);
		}
	}
#endif
	// Tell OpenCV to skip its own AVFoundation authorization handling.
	// The .app bundle's Info.plist triggers the macOS permission dialog instead.
	// Without this, cv::VideoCapture::open() blocks waiting for auth that
	// never completes (OpenCV issue #7519).
	setenv("OPENCV_AVFOUNDATION_SKIP_AUTH", "1", 1);

#ifdef __APPLE__
	{
		fprintf(stderr, "BaekAR: requesting camera permission...\n");
		fflush(stderr);
		if (!RequestCameraPermission()) {
			fprintf(stderr, "BaekAR: camera permission denied — engine will run with dummy frames.\n");
			fprintf(stderr, "  Grant access in System Settings > Privacy & Security > Camera, then reset:\n");
			fprintf(stderr, "  tccutil reset Camera com.baekar.engine\n");
			fflush(stderr);
		} else {
			fprintf(stderr, "BaekAR: camera permission granted.\n");
			fflush(stderr);
		}
	}

	// Marker-image picker: thesis "select a feature-detectable photo, render
	// the 3D scene anchored to it" workflow. Pick any image from image/ as
	// the BRISK reference; skipping keeps the hardcoded default.
	{
		std::string chosen = PickMarkerImage();
		if (!chosen.empty()) {
			Filename[0]._strFilename = chosen;
			Filename[1]._strFilename = chosen;
			fprintf(stderr, "BaekAR: marker = %s\n", chosen.c_str());
		} else {
			fprintf(stderr, "BaekAR: marker = %s (default)\n",
			        Filename[0]._strFilename.c_str());
		}
		fflush(stderr);
	}

	// Window-as-AR-texture (thesis novelty): present a stdin picker so the
	// user chooses any on-screen window; ScreenCaptureKit streams its pixels
	// into g_winFrameBuf. Skipping the picker leaves AR running marker-only.
	{
		uint32_t winId = WCPicker_PickWindowID();
		if (winId != 0) {
			g_winStream = WCStream_Open(winId, kWinTexW, kWinTexH);
			if (g_winStream) {
				g_winFrameBuf.assign(kWinTexW * kWinTexH * 4, 0);
				fprintf(stderr, "BaekAR: window stream ready — plane will spawn on first marker pose.\n");
			}
		} else {
			fprintf(stderr, "BaekAR: no window chosen — running marker-only mode.\n");
		}
		fflush(stderr);
	}
#endif

	// Initialize GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	// Create window with OpenGL context (legacy profile for fixed-function pipeline)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	g_window = glfwCreateWindow(640, 480, "BaekAR - Markerless AR Engine", nullptr, nullptr);
	if (!g_window) {
		fprintf(stderr, "Failed to create GLFW window\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(g_window);
	glfwSwapInterval(1); // vsync

	// Set up input callbacks
	glfwSetKeyCallback(g_window, glfwKeyCallback);
	glfwSetMouseButtonCallback(g_window, glfwMouseButtonCallback);

	// Initialize GLUT (needed for glutSolidCone etc. used in init())
	glutInit(&argc, argv);

	// Initialize D3D stub (allocates static gpDevice so GetDevice() is non-null)
	wonjo_dx::AAR3DInitD3D(nullptr);

	// Initialize OpenGL state
	init();

	fprintf(stderr, "BaekAR: OpenGL init done. Press ESC to quit.\n");
	fprintf(stderr, "BaekAR: Camera will initialize when event loop starts...\n");
	fflush(stderr);

	// Main loop
	while (!glfwWindowShouldClose(g_window)) {
		mainLoop();
		glfwSwapBuffers(g_window);
		glfwPollEvents();
	}

	ReleaseEngineMain();
	glfwDestroyWindow(g_window);
	glfwTerminate();
	return 0;
}
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
