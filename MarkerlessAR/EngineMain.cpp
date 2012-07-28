#define _STDINT
#include <windows.h>
#include <process.h>
#include <fstream>

#include <opencv2/opencv.hpp>
#include <opencv2/gpu/gpu.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <iostream>
#include <list>

#include "stdint.h"
#include "brisk/brisk.h"
//#include "projection.h"
#include "brisk/Matcher.h"

#include "brisk/MatchVerifier.hpp"
#include "brisk/GroundTruth.hpp"

#include <gl/glut.h>
#include "SungwookUtility.hpp"
#include "SungwookFeature.hpp"
#include "SungwookAR.hpp"
#include "calibration/Camera.h"
#include "KatoPoseEstimation/KatoPoseEstimator.h"
#include "wonjo.h"
//#include <boost/lexical_cast.hpp>
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
VideoCapture	capture(0);
VideoCapture	capture1(0);
VideoCapture	capture2(0);

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

CvFont	font;
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
    "../image/iu2.jpg",
	"../image/iu1.jpg",
};


static void init( void )
{
	camera.load("../calibration/camera.dat");
	//ccAR.init();
<<<<<<< .mine
	
=======
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(640, 480);
	glutCreateWindow("");

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(getProjectionMatrix());
	glMatrixMode(GL_MODELVIEW);
//	wonjo_dx::TexturingWith::CreateTexture();
//	glutMouseFunc(wonjo_dx::MouseEvent);
>>>>>>> .r52
}


//for test
double modelview16[16];
double* testModelView()
{
	modelview16[0] = 0.99981194128147199;
	modelview16[1] = -0.018552073020307029;
	modelview16[2] = -0.0056482437645758912;
	modelview16[3] = 0;
	modelview16[4] = -0.017918288667907761;
	modelview16[5] = -0.99513878692707813;
	modelview16[6] = 0.096838678659495664;
	modelview16[7] = 0;
	modelview16[8] = -0.0074173446858294976;
	modelview16[9] = -0.096719260439442609;
	modelview16[10] = -0.99528406380181678;
	modelview16[11] = 0;
	modelview16[12] = -49.206718431201708;
	modelview16[13] = 14.710132728454928;
	modelview16[14] = 226.63746113148767;
	modelview16[15] = 1;
	return modelview16;
}

namespace wonjo_dx
{
	static void drawDX()
	{
		//if(bThreadDetection1==false) return;
		
		double viewbuff[16];
//		double projbuff[16];
		memcpy(viewbuff,getModelViewMatrix(0),sizeof(viewbuff));
//		memcpy(projbuff,getProjectionMatrix(),sizeof(projbuff));
//		SetProjectionMatrix(projbuff);
		SetProjectionMatrix(getProjectionMatrix());
		SetModelViewMatrix(viewbuff);
//		std::cout << "viewbuff is : ";
//		for(int i = 0 ; i < 16 ; ++i)
//			std::cout << viewbuff[i] << " ";
//		std::cout << std::endl;
		wonjo_dx::ProcessIO();
		
		AAR3DDrawMesh("IEFrame",NULL, 35.0f );
	}
}

static void draw1( void )
{

	/*   detection의 성공여부로 변경하자.
	if(!decodeSuccess)
		return;
	*/
	
	/*
	if(bThreadDetection1==false)
		return;
	*/

<<<<<<< .mine
=======
	//최원조 네이트온으로 일단 함.
	//wonjo_dx::TexturingWith texturing(NULL,"NateOn");
	//wonjo_dx::TexturingWith texturing("IEFrame",NULL);
	//wonjo_dx::TexturingWith texturing("WebCtrl32","http://www.naver.com");

	//threadResults1.line[0][0]

	double projbuff[16];
	double viewbuff[16];

	////	argDrawMode3D();
	//glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	////	argDrawMode3D();
	glMatrixMode(GL_PROJECTION);
	memcpy(projbuff,wonjo_dx::ProjMatrix(getProjectionMatrix()),sizeof(projbuff));
	//glLoadMatrixd( getProjectionMatrix() );
	glLoadMatrixd( projbuff );
	glClearDepth( 1.0 );
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glMatrixMode(GL_MODELVIEW);
	memcpy(viewbuff,wonjo_dx::ViewMatrix(getModelViewMatrix(0)),sizeof(viewbuff));
	//glLoadMatrixd( getModelViewMatrix(0) );
	glLoadMatrixd( viewbuff );
	//glMatrixMode(GL_MODELVIEW);
	glTranslatef( 0.0, 0.0, 0.0 );

	glEnable(GL_LINEAR);
	glEnable(GL_LINEAR_MIPMAP_NEAREST);
	//glutSolidCube(60.0);
	//texturing.Plane(30.0);
	wonjo_dx::Plane(36);
>>>>>>> .r52
	
<<<<<<< .mine
=======
	
	//wonjo_dx::Cube(30);
	
	
	glDisable( GL_LIGHTING );
	glDisable( GL_DEPTH_TEST );
>>>>>>> .r52
#ifdef _LOGGING
	//remove boost
// 	FILE* fp = fopen("c:\\_PROJECTS\\dtdt.txt","at");
// 	std::string str;
// 	str.assign("//--------------\n");
// 	for(int i = 0 ; i < 16 ; ++i)
// 	{
// 		str.append("modelview16[");
// 		str.append(boost::lexical_cast<std::string>(i));
// 		str.append("] = ");
// 		str.append(boost::lexical_cast<std::string>(viewbuff[i]));
// 		str.append(";\n");
// 	}
// 	fputs(str.c_str(),fp);
// 	fclose(fp);
#endif
	
	
}


static void draw2( void )
{

	/*   detection의 성공여부로 변경하자.
	if(!decodeSuccess)
		return;
	*/
	
	
	
	if(bThreadDetection2==false)
		return;
	

	double    gl_para[16];
	GLfloat   mat_ambient[]     = {0.0, 0.0, 1.0, 1.0};
	GLfloat   mat_flash[]       = {0.0, 0.0, 1.0, 1.0};
	GLfloat   mat_flash_shiny[] = {50.0};
	GLfloat   light_position[]  = {100.0,-200.0,200.0,0.0};
	GLfloat   ambi[]            = {0.1, 0.1, 0.1, 0.1};
	GLfloat   lightZeroColor[]  = {0.9, 0.9, 0.9, 0.1};

	////	argDrawMode3D();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	////	argDrawMode3D();
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd( getProjectionMatrix() );

	glClearDepth( 1.0 );
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glMatrixMode(GL_MODELVIEW);

	


	glLoadMatrixd( getModelViewMatrix(1)  );

		

//	glEnable(GL_LIGHTING);
//	glEnable(GL_LIGHT0);
//	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
//	glLightfv(GL_LIGHT0, GL_AMBIENT, ambi);
//	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightZeroColor);
//	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_flash);
//	glMaterialfv(GL_FRONT, GL_SHININESS, mat_flash_shiny);	
//	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMatrixMode(GL_MODELVIEW);
	glTranslatef( 0.0, 0.0, 25.0 );



	glutSolidCube(60.0);


	//glutSolidCone(40.0, 80.0, 100, 50);
	
		//glutSolidCone(40.0, 80.0, 100, 50);
	/*
	switch(ccAR.ID) {
	case	0xa3e34053:		
		//printf("사각형!\t");
		glutSolidCube(60.0);
		break;
	case	0x90600381:		
		//printf("원뿔!\t");
		glutSolidCone(40.0, 80.0, 100, 50);
		break;
	default:
		//printf("구!!!\t");
		glutSolidSphere(40.0, 100, 50);

	}*/
	glDisable( GL_LIGHTING );

	glDisable( GL_DEPTH_TEST );
		
}


static void mainLoop(void)
{
	IplImage		*img_input;
	IplImage		img_output;
	img_input = cvQueryFrame(capture_C);
	
	//Mat img_input;
	//capture>>img_input;

	

	//여기서 사각형을 그려주는 것으로 수정
	if(bThreadTracking1==true)
		{
			
			cvLine(img_input, dst_tracking_corners1[0], dst_tracking_corners1[1], Scalar( 0, 255, 255), 4);
			cvLine(img_input, dst_tracking_corners1[1], dst_tracking_corners1[2], Scalar( 0, 255, 255), 4);
			cvLine(img_input, dst_tracking_corners1[2], dst_tracking_corners1[3], Scalar( 0, 255, 255), 4);
			cvLine(img_input, dst_tracking_corners1[3], dst_tracking_corners1[0], Scalar( 0, 255, 255), 4);

			/*
			line( mInput, dst_tracking_corners1[0], dst_tracking_corners1[1], Scalar( 0, 255, 255), 4 );
			line( mInput, dst_tracking_corners1[1], dst_tracking_corners1[2], Scalar( 0, 255, 255), 4 );
			line( mInput, dst_tracking_corners1[2], dst_tracking_corners1[3], Scalar( 0, 255, 255), 4 );
			line( mInput, dst_tracking_corners1[3], dst_tracking_corners1[0], Scalar( 0, 255, 255), 4 );	
			*/
		}else
		{
			cvLine(img_input, dst_matching_corners1[0], dst_matching_corners1[1], Scalar( 255, 255, 255), 4);
			cvLine(img_input, dst_matching_corners1[1], dst_matching_corners1[2], Scalar( 255, 255, 255), 4);
			cvLine(img_input, dst_matching_corners1[2], dst_matching_corners1[3], Scalar( 255, 255, 255), 4);
			cvLine(img_input, dst_matching_corners1[3], dst_matching_corners1[0], Scalar( 255, 255, 255), 4);

			/*
			line( mInput, dst_matching_corners1[0], dst_matching_corners1[1], Scalar( 255, 255, 255), 4 );
			line( mInput, dst_matching_corners1[1], dst_matching_corners1[2], Scalar( 255, 255, 255), 4 );
			line( mInput, dst_matching_corners1[2], dst_matching_corners1[3], Scalar( 255, 255, 255), 4 );
			line( mInput, dst_matching_corners1[3], dst_matching_corners1[0], Scalar( 255, 255, 255), 4 );	
			*/
		}




	if(img_input)
	{
		//EnterCriticalSection(&hCriticalSection[0]);
		//img_input=&IplImage(mInput);
		cvFlip(img_input);
		
		//remove opengl
// 		//////argDrawMode2D();
// 		glMatrixMode(GL_MODELVIEW);
// 		glLoadIdentity();
// 		glMatrixMode(GL_PROJECTION);
// 		glLoadIdentity();
// //		glOrtho(0, 640, 0, 480, -1.0, 1.0);
// 		glViewport(0, 0, 640, 480);
// 		//////argDrawMode2D();
// 
// 		//////argDispImage((ARUint8*)img_binary->imageData, 0,0 );
 		//~remove opengl

		//directx rendering
		
		//glDrawPixels( 640, 480, GL_BGR_EXT, GL_UNSIGNED_BYTE, img_input->imageData);
		
		char buf[640*480*3];
		ZeroMemory(buf,640*480*3);
		memcpy(buf,img_input->imageData,640*480*3);

//		wonjo_dx::AAR3DDrawCameraPreview(img_input->imageData,640,480);
		wonjo_dx::AAR3DDrawCameraPreview(buf,640,480);
		

		//여기서 마커에 대한 추적을 해줘야 된다.

		doTrack(img_input->imageData);
		//poseEstimator.calculateTransformationMatrix(&camera, &threadResults1);
		wonjo_dx::drawDX();
		//draw1();
		//draw2();
		
		//LeaveCriticalSection(&hCriticalSection[0]);
	}

	//remove gl
	//glutSwapBuffers();
	//directx
	wonjo_dx::Flip();

}


double* getModelViewMatrix(int index) {
//#ifdef _TESTMODE
//	return testModelView();
//#endif
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
	if(bThreadDetection1==true)
		poseEstimator.calculateTransformationMatrix(&camera, &threadResults1);
	if(bThreadDetection2==true)
		poseEstimator.calculateTransformationMatrix(&camera, &threadResults2);
	
}
HINSTANCE g_hInst;
LRESULT CALLBACK WndProc(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam);
LPCSTR lpszClass="Class";

<<<<<<< .mine
//int main(int argc, char* argv[]) 
INT APIENTRY WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nCmdShow )
{
=======
LRESULT CALLBACK WndProc(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam);
LPCSTR lpszClass = "BaekAR Application : State of Art Argumented Reality Browser";
//int main(int argc, char* argv[]) 
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
>>>>>>> .r52

//#ifdef _TESTMODE
	AllocConsole();
	freopen("CONOUT$","wt",stdout);	
//#endif

	
<<<<<<< .mine
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst=hInstance;

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
	ShowWindow(hWnd,nCmdShow);
	



	//초기화
	if(FAILED(d3d::AAR3DInitD3D(hWnd))) return 0;	//실패시 윈도우 끝내버림.
	cam3D::AAR3DInitSystem();
	cam3D::pos = D3DXVECTOR3(5,0,-1.5f);
	cam3D::up = D3DXVECTOR3(0,1,0);
	cam3D::lookAt = D3DXVECTOR3(0,0,0);
	cam3D::UpdateProjection();
	//초기화끝

=======

	//초기화
	if(FAILED(wonjo_dx::AAR3DInitD3D(hWnd))) 
	{
		MessageBox(NULL,"DirectX Device Failed.\nthe application will be terminated.","BaekAR",MB_OK);
		return 0;	//실패시 윈도우 끝내버림.
	}
	//~initialize directX
	//////////////////////////////////////////////////////////////////////////
	


>>>>>>> .r52
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
	
	
	detector = new BriskFeatureDetector(60,2);
	descriptorExtractor = new BriskDescriptorExtractor();
	

	if(hamming){
		descriptorMatcher1 = new BruteForceMatcher<HammingSse>(); 
		descriptorMatcher2 = new BruteForceMatcher<HammingSse>(); 
	}
	else{
		descriptorMatcher1 = new BruteForceMatcher<L2<float> >();
		descriptorMatcher1 = new BruteForceMatcher<L2<float> >();
	}

	//Create BRISK Database

	//Read from Filename Structure 
	img_rgbdatabase1=imread(Filename[0]._strFilename.c_str(), 1);
	img_rgbdatabase2=imread(Filename[1]._strFilename.c_str(), 1);
		
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
		
	//Detect Keypoints from img_centerdatase using AGAST Feature Detector
	detector->detect(img_centerdatabase1,kp_database1);
	//detector->detect(img_centerdatabase2,kp_database2);
	

	//Create Description from img_centerdatabase1 using BRISK Feature Descriptor
	descriptorExtractor->compute(img_centerdatabase1,kp_database1,desc_database1);
	//descriptorExtractor->compute(img_centerdatabase2,kp_database2,desc_database2);
	
	
	capture >> gDetectionResult1;
	
	int val1=1, val2=2, val3=3, val4=4, val5=5, val6=6,val7=7, val8=8, val9=9, val10=10, val11=11, val12=12;

	hMatchingThread[0] = (HANDLE)_beginthreadex( NULL, 0, &ThreadBRISKMatching, &val1, 0, &uMatchingThreadID[0] );
	
	//hMatchingThread[1] = (HANDLE)_beginthreadex( NULL, 0, &ThreadBRISKMatching, &val2, 0, &uMatchingThreadID[1] );
	
	hTrackingThread[0] = (HANDLE)_beginthreadex( NULL, 0, &ThreadTracking, &val1, 0, &uTrackingThreadID[0] );

	

	//hDrawThread=(HANDLE)_beginthreadex( NULL, 0, &ThreadDraw, 0, 0, &uDrawThreadID);
		

	capture_C = cvCaptureFromCAM(0);
	cvSetCaptureProperty(capture_C , CV_CAP_PROP_FRAME_WIDTH, 640);
	cvSetCaptureProperty(capture_C , CV_CAP_PROP_FRAME_HEIGHT, 480);

<<<<<<< .mine
	
	/////////////////////////////////////////////////////////////////////////////////
=======
>>>>>>> .r52

<<<<<<< .mine


	PeekMessage( &Message, NULL, 0U, 0U, PM_REMOVE );

	while(true) {
=======
	//remove opengl
// 	glutInit(&argc, argv);
// 	init();
 	camera.load("../calibration/camera.dat");
// 	glViewport(0, 0, 640, 480);
// 
// 	glutDisplayFunc(mainLoop);				
// 	glutIdleFunc( mainLoop );
// 
// 	//glutKeyboardFunc(keyEvent);
// 
// 	glutMainLoop();
>>>>>>> .r52
		
		//윈도우 핸들링이 들어올 때 처리
		if( PeekMessage( &Message, NULL, 0U, 0U, PM_REMOVE ) )
		{
			TranslateMessage( &Message );
			DispatchMessage( &Message );
			continue;
		}
		//위에서 continue 이므로, else 부분.
 	//~remove opengl
	
	//direct X code mainloop
	PeekMessage( &Message, NULL, 0U, 0U, PM_REMOVE );
	while(true) {	//main loop start

		//윈도우 핸들링이 들어올 때 처리
		if( PeekMessage( &Message, NULL, 0U, 0U, PM_REMOVE ) )
<<<<<<< .mine
//		{
//			printf("the number of Keypoints @ Query Image: %d\t", kpQuery.size());
//			Mat	canvasTrain,	canvasQuery;
			
//			drawKeypoints(mQuery, kpQuery, canvasQuery);
//		}
=======
		{
			TranslateMessage( &Message );
			DispatchMessage( &Message );
			continue;
		}
>>>>>>> .r52
		//메시지가 들어오지 않았을 때 처리 (main loop)
		else mainLoop();
	}	//end of main loop
	//~direct X code
		

	/////////////////////////////////////////////////////////////////////////////////

<<<<<<< .mine
		//cout << "Detection and Description : " << time_detect_and_describing << " ms\t";
		//cout << "Matching : " << time_matching << " ms\t";
		//cout << "TOTAL : " << time_total_consuming << " ms\t";
		cout << "FPS : " << fps << " frames" << endl;
		//printf("the number of Keypoints @ Query Image: %d\t", kpQuery.size());
// 시간 측정 코드: 시간 측정 필요하면 여기 주석 풀자
	#endif

		IplImage* img_input = cvQueryFrame(capture_C);
		//실제 processing
		//여기서 사각형을 그려주는 것으로 수정
		if(bThreadTracking1==true)
		{
			
			cvLine(img_input, dst_tracking_corners1[0], dst_tracking_corners1[1], Scalar( 0, 255, 255), 4);
			cvLine(img_input, dst_tracking_corners1[1], dst_tracking_corners1[2], Scalar( 0, 255, 255), 4);
			cvLine(img_input, dst_tracking_corners1[2], dst_tracking_corners1[3], Scalar( 0, 255, 255), 4);
			cvLine(img_input, dst_tracking_corners1[3], dst_tracking_corners1[0], Scalar( 0, 255, 255), 4);

			/*
			line( mInput, dst_tracking_corners1[0], dst_tracking_corners1[1], Scalar( 0, 255, 255), 4 );
			line( mInput, dst_tracking_corners1[1], dst_tracking_corners1[2], Scalar( 0, 255, 255), 4 );
			line( mInput, dst_tracking_corners1[2], dst_tracking_corners1[3], Scalar( 0, 255, 255), 4 );
			line( mInput, dst_tracking_corners1[3], dst_tracking_corners1[0], Scalar( 0, 255, 255), 4 );	
			*/
		}else
		{
			cvLine(img_input, dst_matching_corners1[0], dst_matching_corners1[1], Scalar( 0, 255, 255), 4);
			cvLine(img_input, dst_matching_corners1[1], dst_matching_corners1[2], Scalar( 0, 255, 255), 4);
			cvLine(img_input, dst_matching_corners1[2], dst_matching_corners1[3], Scalar( 0, 255, 255), 4);
			cvLine(img_input, dst_matching_corners1[3], dst_matching_corners1[0], Scalar( 0, 255, 255), 4);

			/*
			line( mInput, dst_matching_corners1[0], dst_matching_corners1[1], Scalar( 255, 255, 255), 4 );
			line( mInput, dst_matching_corners1[1], dst_matching_corners1[2], Scalar( 255, 255, 255), 4 );
			line( mInput, dst_matching_corners1[2], dst_matching_corners1[3], Scalar( 255, 255, 255), 4 );
			line( mInput, dst_matching_corners1[3], dst_matching_corners1[0], Scalar( 255, 255, 255), 4 );	
			*/
		}
	
		
		if(img_input) 		//draw1 로 그림
		{
			cvFlip(img_input);
#ifndef _DEBUG
			if(bThreadDetection1)
#endif
			{
				//char pch[640*480*3];
				//ZeroMemory(pch,sizeof(pch));
				//memcpy(pch,img_input->imageData,sizeof(pch));

				double * modelview = getModelViewMatrix(0);
				std::cout << "-------------------------\n";
				for(int i = 0 ; i < 16 ; ++i) std::cout << float(modelview[i]);
				std::cout << std::endl;
				cam3D::UpdateView(modelview);
				d3d::AAR3DTotalProcess(img_input->imageData,640,480,img_input->imageSize);
				//d3d::AAR3DTotalProcess(pch,640,480,img_input->imageSize);
			}
		}


	#ifdef	CAMERA
		//ch = waitKey(10);
		
		/*
		WaitForSingleObject(hMutex, INFINITE); 
		//imshow("NFT", mResult);
		//cvWaitKey(1);
		showImage("MSL", mResult, 10, 0, 0);
		ReleaseMutex(hMutex);
		*/

		

	}	// end of while
	#else
		waitKey(0);
		}
	#endif
	CloseHandle( hDrawThread);
=======
	
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
>>>>>>> .r52
	CloseHandle( hMatchingThread[0] );	
	CloseHandle( hMatchingThread[1] );

	//클린업
	d3d::AAR3DReleaseSystem();
	
	//CloseHandle( hTrackingThread[0] );

	//mslNFT_Log.close();
	return 0;
}

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
		d3d::AAR3DReleaseSystem();
		ExitProcess(0);
		PostQuitMessage(0);
		return 0;
	case WM_LBUTTONDOWN:
		//std::cout << "click" << std::endl;
		{
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(hWnd, &pt);
			d3d::Picking(pt);
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
			d3d::AAR3DReleaseSystem();
			ExitProcess(0);
			PostQuitMessage(0);
			return 0;
		}
	}
	return(DefWindowProc(hWnd,iMessage,wParam,lParam));
}


unsigned __stdcall ThreadDraw(void *param)
{
	namedWindow("BaekAR", CV_WINDOW_AUTOSIZE|CV_GUI_NORMAL);
	while(true){
		
		
		capture1>>mInput;
	

		//Detection or Tracking 체크

		if(bThreadTracking1==true)
		{
			
			line( mInput, dst_tracking_corners1[0], dst_tracking_corners1[1], Scalar( 0, 255, 255), 4 );
			line( mInput, dst_tracking_corners1[1], dst_tracking_corners1[2], Scalar( 0, 255, 255), 4 );
			line( mInput, dst_tracking_corners1[2], dst_tracking_corners1[3], Scalar( 0, 255, 255), 4 );
			line( mInput, dst_tracking_corners1[3], dst_tracking_corners1[0], Scalar( 0, 255, 255), 4 );	

		}else
		{

			line( mInput, dst_matching_corners1[0], dst_matching_corners1[1], Scalar( 255, 255, 255), 4 );
			line( mInput, dst_matching_corners1[1], dst_matching_corners1[2], Scalar( 255, 255, 255), 4 );
			line( mInput, dst_matching_corners1[2], dst_matching_corners1[3], Scalar( 255, 255, 255), 4 );
			line( mInput, dst_matching_corners1[3], dst_matching_corners1[0], Scalar( 255, 255, 255), 4 );	

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



unsigned __stdcall ThreadBRISKMatching(void *param)
{
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
 			
			capture1>>matching_thread_rgbcamera;		
			mbistracking=bThreadTracking1;
						
		}
		else if(idxcount==2){
			
			capture2>>matching_thread_rgbcamera;		
			mbistracking=bThreadTracking2;
		}
		

		//Convert Camera Input with RGB to Camera Input with GRAY
		cvtColor(matching_thread_rgbcamera, matching_thread_graycamera, CV_BGR2GRAY);
<<<<<<< .mine
#ifdef _DEBUG
		return 0;
#endif
=======
		
//#ifdef _DEBUG
//		return 0;
//#endif
>>>>>>> .r52
		detector->detect(matching_thread_graycamera,kp_camera_matching_thread);
		descriptorExtractor->compute(matching_thread_graycamera,kp_camera_matching_thread,desc_camera_matching_thread);
		
		matching_thread_result=matching_thread_rgbcamera;
		std::vector<std::vector<DMatch> > matches;
		
		//Matching /

		if(idxcount==1){
 			if(hamming){
				descriptorMatcher1->radiusMatch(desc_camera_matching_thread,desc_database,matches,100.0);
				descriptorMatcher1->radiusMatch(desc_camera_matching_thread,desc_database,matches,100.0);
			}
			else{
				descriptorMatcher1->radiusMatch(desc_camera_matching_thread,desc_database,matches,0.21);
				descriptorMatcher1->radiusMatch(desc_camera_matching_thread,desc_database,matches,100.0);
			}

		}
		else if(idxcount==2){
			if(hamming){
				descriptorMatcher1->radiusMatch(desc_camera_matching_thread,desc_database,matches,100.0);
				descriptorMatcher1->radiusMatch(desc_camera_matching_thread,desc_database,matches,100.0);
			}
			else{
				descriptorMatcher1->radiusMatch(desc_camera_matching_thread,desc_database,matches,0.21);
				descriptorMatcher1->radiusMatch(desc_camera_matching_thread,desc_database,matches,100.0);
			}
		}
		
			
		//Compute Homography
		vector<Point2f> mpts_1, mpts_2;		// train이 1 query가 2
		CvPoint dst_corners[4];
		matches2points(matches, kp_database, kp_camera_matching_thread, mpts_1, mpts_2); //Extract a list of the (x,y) location of the matches
		
		outimg = Mat::zeros(matching_thread_rgbcamera.rows, matching_thread_rgbcamera.cols, matching_thread_rgbcamera.type());
		
		//Draw Matching Results between Camera and Database Image
		
		/*
		drawMatches(matching_thread_rgbcamera, kp_camera_matching_thread, img_database, kp_database, matches,outimg,
			Scalar(0,255,0), Scalar(0,0,255),
			std::vector<std::vector<char> >(), DrawMatchesFlags::DRAW_RICH_KEYPOINTS );
		*/

		 // need at least 5 matched pairs of points (more are better)
		 if (computeHomography && (mpts_1.size() > 20))
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
				if(mpts_1.size()<5 && mpts_2.size()<5)
					continue;
				
				Mat H = findHomography(Mat(mpts_1), Mat(mpts_2), RANSAC, 2);

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

			
					double dx, dy;

			
					CvPoint2D p1, p2;

			
					for(int i=0; i<4; ++i) {
				
						p1=threadResults1.vertex[i];
						p2=threadResults1.vertex[(i+1)%4];

						dx=p1.x-p2.x;
						dy=p1.y-p2.y;

						threadResults1.line[i][0]=-dy;
						threadResults1.line[i][1]=dx;
						threadResults1.line[i][2]=(p2.x)*(p1.y)-(p1.x)*(p2.y);

				
					}

			
					threadResults1.center.x=threadResults1.vertex[0].x+((threadResults1.vertex[1].x-threadResults1.vertex[0].x)/2);
					threadResults1.center.y=threadResults1.vertex[0].y+((threadResults1.vertex[3].y-threadResults1.vertex[0].y)/2);
			
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
		
			
		
			

		matches.~vector<vector<DMatch>>();
	
		desc_camera_matching_thread.release();
		kp_camera_matching_thread.~vector<KeyPoint>();

		matching_thread_rgbcamera.release();
		matching_thread_graycamera.release();
		matching_thread_result.release();

		dst_matching_corners.clear();
		
	}


	kp_database.~vector<KeyPoint>();
	desc_database.~Mat();
	
	_endthreadex(0);
	return 0;
	
}



unsigned __stdcall ThreadTracking(void *param)
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

	capture2>>tracking_thread_rgbcamera;

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

			capture2>>tracking_thread_rgbcamera;

			
			//minMaxLoc을 위한 함수
			double minVal; double maxVal; Point minLoc; Point maxLoc; Point matchLoc;

			match_method=CV_TM_CCOEFF_NORMED;

			//Homography & NCC
			//Cam으로부터 받아오는 영상의 포인트 mpts_2, DB로부터 받아오는 포인트 mpts_1
			if(g_mpts_2.size()<5 && g_mpts_1.size()<5)
			{
				//mbisDetecting=false;

				bThreadTracking1=false;
				continue;
			}
			Mat HH = findHomography(Mat(g_mpts_2), Mat(g_mpts_1), RANSAC, 2);						//Homography
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
			
			//여기서 트랙킹이 실패했을 경우에만 Detection 쓰레드가 호출되도록 수정하자!!
			if(maxVal < 0.55){
				//tracking_thread_result.release();
				bThreadTracking1=false;
				mbisPrevMatchLoc=false;
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
				line( tracking_thread_rgbcamera, dst_tracking_corners[1], dst_tracking_corners[2], Scalar( 0, 255, 255), 4 );
				line( tracking_thread_rgbcamera, dst_tracking_corners[2], dst_tracking_corners[3], Scalar( 0, 255, 255), 4 );
				line( tracking_thread_rgbcamera, dst_tracking_corners[3], dst_tracking_corners[0], Scalar( 0, 255, 255), 4 );	
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

					threadResults1.vertex[0].x=dst_tracking_corners[0].x;
					threadResults1.vertex[0].y=dst_tracking_corners[0].y;
					threadResults1.vertex[1].x=dst_tracking_corners[1].x;
					threadResults1.vertex[1].y=dst_tracking_corners[1].y;
					threadResults1.vertex[2].x=dst_tracking_corners[2].x;
					threadResults1.vertex[2].y=dst_tracking_corners[2].y;
					threadResults1.vertex[3].x=dst_tracking_corners[3].x;
					threadResults1.vertex[3].y=dst_tracking_corners[3].y;

			
					double dx, dy;

			
					CvPoint2D p1, p2;

			
					for(int i=0; i<4; ++i) {
				
						p1=threadResults1.vertex[i];
						p2=threadResults1.vertex[(i+1)%4];

						dx=p1.x-p2.x;
						dy=p1.y-p2.y;

						threadResults1.line[i][0]=-dy;
						threadResults1.line[i][1]=dx;
						threadResults1.line[i][2]=(p2.x)*(p1.y)-(p1.x)*(p2.y);

				
					}

			
					threadResults1.center.x=threadResults1.vertex[0].x+((threadResults1.vertex[1].x-threadResults1.vertex[0].x)/2);
					threadResults1.center.y=threadResults1.vertex[0].y+((threadResults1.vertex[3].y-threadResults1.vertex[0].y)/2);
			
					//matching_thread_rgbcamera.copyTo(gDetectionResult1);
			

					dst_tracking_corners1=dst_tracking_corners;

					/*
					if(bThreadTracking1==false && mbisDetecting==true){
						g_mpts_1=mpts_1;
						g_mpts_2=mpts_2;
					}
			
					bThreadDetection1=mbisDetecting;
					*/
			
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
//			POINT pt;
//			GetCursorPos(&pt);
//			ScreenToClient(hWnd, &pt);
//			wonjo_dx::Picking(pt);
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
