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

#include "HandyAR/HandyAR.h"

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
    "../image/yejin.jpg",
	"../image/yejin.jpg",
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
    IplImage * tempImage = cvLoadImage( EARTH_TEXTURE_FILENAME, 1 );
    // convert image R and B channel
    cvConvertImage( tempImage, tempImage, CV_CVTIMG_SWAP_RB );
    cvFlip( tempImage );
    glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, tempImage->width, tempImage->height, GL_RGB, GL_UNSIGNED_BYTE, tempImage->imageData );
    cvReleaseImage( &tempImage );

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

static void mainLoop(void)
{
	if( GetKeyState(VK_LBUTTON) & 0x8000 )
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(NULL, &pt);
		wonjo_dx::Picking(pt);
	}
	
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
		cvLine(image, dst_tracking_corners1[0], dst_tracking_corners1[1], Scalar( 0, 255, 255), 4);
		cvLine(image, dst_tracking_corners1[1], dst_tracking_corners1[2], Scalar( 0, 255, 255), 4);
		cvLine(image, dst_tracking_corners1[2], dst_tracking_corners1[3], Scalar( 0, 255, 255), 4);
		cvLine(image, dst_tracking_corners1[3], dst_tracking_corners1[0], Scalar( 0, 255, 255), 4);
	}
	else if(bThreadDetection1==true)
	{
		cvLine(image, dst_matching_corners1[0], dst_matching_corners1[1], Scalar( 255, 255, 255), 4);
		cvLine(image, dst_matching_corners1[1], dst_matching_corners1[2], Scalar( 255, 255, 255), 4);
		cvLine(image, dst_matching_corners1[2], dst_matching_corners1[3], Scalar( 255, 255, 255), 4);
		cvLine(image, dst_matching_corners1[3], dst_matching_corners1[0], Scalar( 255, 255, 255), 4);
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
			wonjo_dx::AAR3DTexturing text("IEFrame",NULL);

			
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
		
		}//end of processing marker
	
		
	}



	//120325 cwj handy AR 추가
	//HandyAR에 대한 프로시져
	
	//////////////////////////////////////////////////////////////////////////
	//손 끝점 찾아서 그림.
	// capture
	IplImage * frame = 0;

	gCapture.CaptureFrame();
	frame = gCapture.QueryFrame();
	if ( !frame )
	{
		return;
	}
	gFingertipPoseEstimation.OnCapture( frame, gCapture.QueryTickCount() );

	
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
		wonjo_dx::GetDevice()->SetTransform(D3DTS_PROJECTION,&matProj);
		wonjo_dx::GetDevice()->SetTransform(D3DTS_VIEW,&matView);	
		//wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("Arrow3Axis.X"), 1000000000.0f);
		//wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("hand000.X"), 10000000);
		wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("Arrow3Axis.X"), wonjo_dx::MakeScaleMatrix(10.0f,10.0f,10.0f));

		
		//native hand
		//wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("hand001.X"), wonjo_dx::MakeScaleMatrix(5.0f,5.0f,5.0f));
		
		//z축으로 조금 내림...
		D3DXMATRIXA16 matWorld = *wonjo_dx::MakeScaleMatrix(5.0f,5.0f,5.0f) * (*wonjo_dx::MakeTranslationMatrix(0,0,200));	 
		wonjo_dx::DrawMesh(wonjo_dx::LoadMeshFromFile("hand001.X"), &matWorld);

		//wonjo_dx::AAR3DDrawMesh("IEFrame",NULL,35.0f);

		if ( gFingertipPoseEstimation.QueryValidPose() )
		{
			std::cout << " Valid Fingertip!! " <<std::endl;
		
		}
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

LRESULT CALLBACK WndProc(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam);
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
    if ( fInputVideoFile )
    {
        // Capture From File
        if ( !gCapture.Initialize( fFlipFrame, -1, "record_debug.avi" ) )
        {
            fprintf( stderr, "capture initialization failed.\n" );
            return -1;
        }
    } else
    {
        // Live Capture From Camera
        if ( !gCapture.Initialize( fFlipFrame ) )
        {
            fprintf( stderr, "capture initialization failed.\n" );
            return -1;
        }
    }


	// OpenGL loop
	//120325 cwj
	//char *myargv [1];
	//int myargc=1;
	//myargv [0]=strdup ("Myappname");


    //glutInit(&myargc, &myargv[0]);
    //glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
    //glutInitWindowPosition( 100, 100 );
	//~120325 cwj

    // Init Capture
    IplImage * frame = 0;
    gCapture.CaptureFrame();
    frame = gCapture.QueryFrame();
    if ( !frame )
    {
        fprintf( stderr, "failed to capture a frame...\n" );
        return -1;
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
    if ( gFingertipPoseEstimation.Initialize( frame, "../calibration.txt" ) == false )
    {
        return -1;
    }

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
	
	
	detector = new BriskFeatureDetector(60,2);
	descriptorExtractor = new BriskDescriptorExtractor();
	
	//BFMatcher matcher(NORM_L2);
	if(hamming){
		//descriptorMatcher1 = new BFMatcher(NORM_L1);
		//descriptorMatcher2 = new BFMatcher(NORM_L1);
		
		
		//descriptorMatcher2 = new BruteForceMatcher<HammingSse>(); 

		
		descriptorMatcher1 = new BruteForceMatcher<HammingSse>(); 
		descriptorMatcher2 = new BruteForceMatcher<HammingSse>(); 
		
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
	img_rgbdatabase1=imread(Filename[0]._strFilename.c_str(), 1);
	//2번은 오버라이드하지않는다.
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
		
	//경민 여기서 capture_C에 값이 들어오나 1차적으로 확인좀 해죠
	capture_C = cvCaptureFromCAM(0);
	cvSetCaptureProperty(capture_C , CV_CAP_PROP_FRAME_WIDTH, 640);
	cvSetCaptureProperty(capture_C , CV_CAP_PROP_FRAME_HEIGHT, 480);


	//remove opengl
// 	glutInit(&argc, argv);
// 	init();
 	camera.load("../calibration/calibration.txt");
// 	glViewport(0, 0, 640, 480);
// 
// 	glutDisplayFunc(mainLoop);				
// 	glutIdleFunc( mainLoop );
// 
// 	//glutKeyboardFunc(keyEvent);
// 
// 	glutMainLoop();
	
 	//~remove opengl
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
	namedWindow("BaekAR", CV_WINDOW_AUTOSIZE|CV_GUI_NORMAL);

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
		
		
		drawMatches(matching_thread_rgbcamera, kp_camera_matching_thread, img_database, kp_database, matches,outimg,
			Scalar(0,255,0), Scalar(0,0,255),
			std::vector<std::vector<char> >(), DrawMatchesFlags::DRAW_RICH_KEYPOINTS );
		
		imshow("BaekAR", outimg);
		cvWaitKey(1);
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

	//경민 capture2>>tracking_thread_rgbcamera를 capture1으로 수정함... 왜냐하면 카메라 영상을 받는 변수가 너무 많아서 꼬인거 같아서 수정함
	capture1>>tracking_thread_rgbcamera;

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
			capture1>>tracking_thread_rgbcamera;

			
			//minMaxLoc을 위한 함수
			double minVal; double maxVal; Point minLoc; Point maxLoc; Point matchLoc;

			match_method=CV_TM_CCOEFF_NORMED;

			//Homography & NCC
			//Cam으로부터 받아오는 영상의 포인트 mpts_2, DB로부터 받아오는 포인트 mpts_1
			if(g_mpts_2.size()<5 && g_mpts_1.size()<5)
			{
				//mbisDetecting=false;

				bThreadTracking1=false;
				//트랙킹을 실패했을 경우 정보를 지워줌
				dst_tracking_corners.clear();
				dst_tracking_corners1=dst_tracking_corners;

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


	//초기화
	if(FAILED(wonjo_dx::AAR3DInitD3D(hWnd))) 
	{
		MessageBox(NULL,"DirectX Device Failed.\nthe application will be terminated.","BaekAR",MB_OK);
		return 0;	//실패시 윈도우 끝내버림.
	}
	//~initialize directX
	//////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////// 


	InitializeEngineMain();

	//direct X code mainloop
	PeekMessage( &Message, NULL, 0U, 0U, PM_REMOVE );
	while(true) {	//main loop start

		//윈도우 핸들링이 들어올 때 처리
		if( PeekMessage( &Message, NULL, 0U, 0U, PM_REMOVE ) )
		{
			TranslateMessage( &Message );
			DispatchMessage( &Message );
			continue;
		}
		//메시지가 들어오지 않았을 때 처리 (main loop)
		else mainLoop();
	}	//end of main loop
	//~direct X code

	ReleaseEngineMain();
	return 0;
}






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
