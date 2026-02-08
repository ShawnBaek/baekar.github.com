/** 
\file Camera.cpp
\brief 카메라의 입력 설정
*/

#include "Camera.h"
#include <stdio.h>

#ifdef	_WIN32
//#include <cv.h>
//#include <cxcore.h>
//#include <highgui.h>
#include <opencv2/opencv.hpp>
using namespace cv;
#else
#include <OpenCV/OpenCV.h>
#endif

CCamera::CCamera(void)
{
	size = cvSize(640, 480);	// 우선 사이즈 초기화 나중에 바꿔도 됨

	intrinsic   = cvMat( 3, 3, CV_32FC1, camera_intrinsic   );
    distortion  = cvMat( 4, 1, CV_32FC1, camera_distortion  );
    rotation    = cvMat( 3, 1, CV_32FC1, camera_rotation    );
    translation = cvMat( 3, 1, CV_32FC1, camera_translation );

    rotation3by3 = cvMat(3, 3, CV_32FC1, camera_rotation3by3);

	
	


    P = cvMat( 3, 4, CV_32FC1, projection_matrix );

    for ( int i = 0 ; i < 3 ; i ++ )
    {
        camera_rotation[i] = 0;
        camera_translation[i] = 0;
    }


	_fValidPose = false;
    _FeaturesRotationMat = cvMat( 3, 1, CV_32FC1, _FeaturesRotation );
    _FeaturesRotation3by3Mat = cvMat( 3, 3, CV_32FC1, _FeaturesRotation3by3 );
    _FeaturesTranslationMat = cvMat( 3, 1, CV_32FC1, _FeaturesTranslation );
    _FeaturesQuaternionMat = cvMat( 4, 1, CV_32FC1, _FeaturesQuaternion );

    //
    // Camera Transform
    //
    _CameraCenterTMat = cvMat( 3, 1, CV_32FC1, _CameraCenterT );
    _CameraCenterRMat = cvMat( 3, 3, CV_32FC1, _CameraCenterR );


	_CameraCenterRTransPoseMat = cvMat( 3, 3, CV_32FC1, _CameraCenterRTransPose );

	_CameraHomographyMat = cvMat( 3, 3, CV_32FC1, _CameraHomography );







	//삼성 카메라
	m_IntrinsicParam = cvCreateMat(3, 3, CV_64FC1);
	cvZero(m_IntrinsicParam);
	cvmSet(m_IntrinsicParam, 0, 0, 664.0);
	cvmSet(m_IntrinsicParam, 1, 1, 673.4);
	cvmSet(m_IntrinsicParam, 0, 2, 293.0);
	cvmSet(m_IntrinsicParam, 1, 2, 247.4);
	cvmSet(m_IntrinsicParam, 2, 2, 1);

	m_IntrinsicParamInv = cvCreateMat(3, 3, CV_64FC1);
	cvZero(m_IntrinsicParamInv);
	cvInvert(m_IntrinsicParam, m_IntrinsicParamInv);

	//마커의 월드 좌표를 입력한다. 이 값은 상수여야 하며 물리적으로 측정된 값이어야 한다(mm단위).
	//또한 좌표의 입력 순서는 마커의 4개 점 기준으로 시계방향으로 입력한다.
	/*
	m_RealCornerPosition = cvCreateMat(4, 2, CV_64FC1);
	cvmSet(m_RealCornerPosition, 0, 0, -40); cvmSet(m_RealCornerPosition, 0, 1, -40);
	cvmSet(m_RealCornerPosition, 1, 0, 40); cvmSet(m_RealCornerPosition, 1, 1, -40);
	cvmSet(m_RealCornerPosition, 2, 0, 40); cvmSet(m_RealCornerPosition, 2, 1, 40);
	cvmSet(m_RealCornerPosition, 3, 0, -40); cvmSet(m_RealCornerPosition, 3, 1, 40);
	*/


	m_ExtrinsicParam = cvCreateMat(4, 3, CV_64FC1);
	cvZero(m_ExtrinsicParam);
	m_MatHomography = cvCreateMat(3, 3, CV_64FC1);
	cvZero(m_MatHomography);
	m_vecRotX = cvCreateMat(3, 1, CV_64FC1);
	cvZero(m_vecRotX);
	m_vecRotY = cvCreateMat(3, 1, CV_64FC1);
	cvZero(m_vecRotY);
	m_vecRotZ = cvCreateMat(3, 1, CV_64FC1);
	cvZero(m_vecRotZ);
	m_vecTrans = cvCreateMat(3, 1, CV_64FC1);
	cvZero(m_vecTrans);
	m_Rotation3x3 = cvCreateMat(3, 3, CV_64FC1);
	cvZero(m_Rotation3x3);
	m_Rotation3x1 = cvCreateMat(3, 1, CV_64FC1);
	cvZero(m_Rotation3x1);
	m_RotationTranspose3x3 = cvCreateMat(3, 3, CV_64FC1);
	cvZero(m_RotationTranspose3x3);



    _fIntrinsic = false;

}

CCamera::CCamera(bool	turnOn) {
	size = cvSize(640, 480);	// 우선 사이즈 초기화 나중에 바꿔도 됨

	intrinsic   = cvMat( 3, 3, CV_32FC1, camera_intrinsic   );
    distortion  = cvMat( 4, 1, CV_32FC1, camera_distortion  );
    rotation    = cvMat( 3, 1, CV_32FC1, camera_rotation    );
    translation = cvMat( 3, 1, CV_32FC1, camera_translation );

    rotation3by3 = cvMat(3, 3, CV_32FC1, camera_rotation3by3);

    P = cvMat( 3, 4, CV_32FC1, projection_matrix );

    for ( int i = 0 ; i < 3 ; i ++ )
    {
        camera_rotation[i] = 0;
        camera_translation[i] = 0;
    }

	_fValidPose = false;
    _FeaturesRotationMat = cvMat( 3, 1, CV_32FC1, _FeaturesRotation );
    _FeaturesRotation3by3Mat = cvMat( 3, 3, CV_32FC1, _FeaturesRotation3by3 );
    _FeaturesTranslationMat = cvMat( 3, 1, CV_32FC1, _FeaturesTranslation );
    _FeaturesQuaternionMat = cvMat( 4, 1, CV_32FC1, _FeaturesQuaternion );

    //
    // Camera Transform
    //
    _CameraCenterTMat = cvMat( 3, 1, CV_32FC1, _CameraCenterT );
    _CameraCenterRMat = cvMat( 3, 3, CV_32FC1, _CameraCenterR );



	_CameraCenterRTransPoseMat = cvMat( 3, 3, CV_32FC1, _CameraCenterRTransPose );
	

	_CameraHomographyMat = cvMat( 3, 3, CV_32FC1, _CameraHomography );

	
	//삼성 카메라
	m_IntrinsicParam = cvCreateMat(3, 3, CV_64FC1);
	cvZero(m_IntrinsicParam);
	cvmSet(m_IntrinsicParam, 0, 0, 664.0);
	cvmSet(m_IntrinsicParam, 1, 1, 673.4);
	cvmSet(m_IntrinsicParam, 0, 2, 293.0);
	cvmSet(m_IntrinsicParam, 1, 2, 247.4);
	cvmSet(m_IntrinsicParam, 2, 2, 1);

	m_IntrinsicParamInv = cvCreateMat(3, 3, CV_64FC1);
	cvZero(m_IntrinsicParamInv);
	cvInvert(m_IntrinsicParam, m_IntrinsicParamInv);

	//마커의 월드 좌표를 입력한다. 이 값은 상수여야 하며 물리적으로 측정된 값이어야 한다(mm단위).
	//또한 좌표의 입력 순서는 마커의 4개 점 기준으로 시계방향으로 입력한다.
	/*
	m_RealCornerPosition = cvCreateMat(4, 2, CV_64FC1);
	cvmSet(m_RealCornerPosition, 0, 0, -40); cvmSet(m_RealCornerPosition, 0, 1, -40);
	cvmSet(m_RealCornerPosition, 1, 0, 40); cvmSet(m_RealCornerPosition, 1, 1, -40);
	cvmSet(m_RealCornerPosition, 2, 0, 40); cvmSet(m_RealCornerPosition, 2, 1, 40);
	cvmSet(m_RealCornerPosition, 3, 0, -40); cvmSet(m_RealCornerPosition, 3, 1, 40);
	*/


	m_ExtrinsicParam = cvCreateMat(4, 3, CV_64FC1);
	cvZero(m_ExtrinsicParam);
	m_MatHomography = cvCreateMat(3, 3, CV_64FC1);
	cvZero(m_MatHomography);
	m_vecRotX = cvCreateMat(3, 1, CV_64FC1);
	cvZero(m_vecRotX);
	m_vecRotY = cvCreateMat(3, 1, CV_64FC1);
	cvZero(m_vecRotY);
	m_vecRotZ = cvCreateMat(3, 1, CV_64FC1);
	cvZero(m_vecRotZ);
	m_vecTrans = cvCreateMat(3, 1, CV_64FC1);
	cvZero(m_vecTrans);
	m_Rotation3x3 = cvCreateMat(3, 3, CV_64FC1);
	cvZero(m_Rotation3x3);
	m_Rotation3x1 = cvCreateMat(3, 1, CV_64FC1);
	cvZero(m_Rotation3x1);
	m_RotationTranspose3x3 = cvCreateMat(3, 3, CV_64FC1);
	cvZero(m_RotationTranspose3x3);

    _fIntrinsic = false;


}


CCamera::~CCamera(void)
{
}

#define	removeComments()			do {fgets(line, 100, fp);} while( !strcmp(line, "\n") );

bool CCamera::load(const char* fileName)
{
	
	FILE *fp = fopen( fileName, "rt");
	if (fp == NULL)
    {
        //goto Finished;

		printf("Cannot Read BaekAR Calibration.txt\n");
		return false;
    }

    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            camera_intrinsic[i][j] = 0;
    camera_intrinsic[2][2] = 1;

    fscanf(fp, "%f", &(camera_intrinsic[0][0]));
    fscanf(fp, "%f", &(camera_intrinsic[1][1]));
    fscanf(fp, "%f", &(camera_intrinsic[0][2]));
    fscanf(fp, "%f", &(camera_intrinsic[1][2]));

    for (int i=0; i<4; i++)
        fscanf(fp, "%f", &(camera_distortion[i]));
    fclose(fp);

    printf("fx=%.5f fy=%.5f cx=%.5f cy=%.5f\n", camera_intrinsic[0][0], camera_intrinsic[1][1], camera_intrinsic[0][2], camera_intrinsic[1][2]);
    printf("k1=%.5f k2=%.5f p1=%.5f p2=%.5f\n", camera_distortion[0], camera_distortion[1], camera_distortion[2], camera_distortion[3]);

    _fIntrinsic = true;

	
	// Size read
	this->size.width = 640;
	this->size.height = 480;

	
	loaded = true;

	return true;
}

#define PD_LOOP 3

void CCamera::observ2Ideal(CvPoint	observed, CvPoint2D *ideal)
{
	ARTKFloat  z02, z0, p, q, z, px, py;
	register int     i;

	px = observed.x - this->distortionFactor[0];
	py = observed.y - this->distortionFactor[1];
	p = this->distortionFactor[2]/(ARTKFloat)100000000.0;
	z02 = px*px+ py*py;
	q = z0 = (ARTKFloat)sqrt(px*px+ py*py);

	for( i = 1; ; i++ ) {
		if( z0 != 0.0 ) {
			z = z0 - (((ARTKFloat)1.0 - p*z02)*z0 - q) / ((ARTKFloat)1.0 - (ARTKFloat)3.0*p*z02);
			px = px * z / z0;
			py = py * z / z0;
		}
		else {
			px = 0.0;
			py = 0.0;
			break;
		}
		if( i == PD_LOOP ) break;

		z02 = px*px+ py*py;
		z0 = (ARTKFloat)sqrt(px*px+ py*py);
	}

	ideal->x = px / this->distortionFactor[3] + this->distortionFactor[0];
	ideal->y = py / this->distortionFactor[3] + this->distortionFactor[1];
}

void CCamera::observ2Ideal(int ox, int oy, ARTKFloat *ix, ARTKFloat *iy)
{
	ARTKFloat  z02, z0, p, q, z, px, py;
	register int     i;

	px = ox - this->distortionFactor[0];
	py = oy - this->distortionFactor[1];
	p = this->distortionFactor[2]/(ARTKFloat)100000000.0;
	z02 = px*px+ py*py;
	q = z0 = (ARTKFloat)sqrt(px*px+ py*py);

	for( i = 1; ; i++ ) {
		if( z0 != 0.0 ) {
			z = z0 - (((ARTKFloat)1.0 - p*z02)*z0 - q) / ((ARTKFloat)1.0 - (ARTKFloat)3.0*p*z02);
			px = px * z / z0;
			py = py * z / z0;
		}
		else {
			px = 0.0;
			py = 0.0;
			break;
		}
		if( i == PD_LOOP ) break;

		z02 = px*px+ py*py;
		z0 = (ARTKFloat)sqrt(px*px+ py*py);
	}

	*ix = px / this->distortionFactor[3] + this->distortionFactor[0];
	*iy = py / this->distortionFactor[3] + this->distortionFactor[1];
}

void CCamera::ideal2Observ(CvPoint2D ideal, CvPoint *observed)
{
	ARTKFloat    x, y, d;

	x = (ideal.x - this->distortionFactor[0]) * this->distortionFactor[3];
	y = (ideal.y - this->distortionFactor[1]) * this->distortionFactor[3];
	if( x == 0.0 && y == 0.0 ) {
		observed->x = this->distortionFactor[0];
		observed->y = this->distortionFactor[1];
	}
	else {
		d = (ARTKFloat)1.0 - this->distortionFactor[2]/(ARTKFloat)100000000.0 * (x*x+y*y);
		observed->x = x * d + this->distortionFactor[0];
		observed->y = y * d + this->distortionFactor[1];
	}
}

void CCamera::ideal2Observ(CvPoint2D ideal, CvPoint2D *observed)
{
	ARTKFloat    x, y, d;

	x = (ideal.x - this->distortionFactor[0]) * this->distortionFactor[3];
	y = (ideal.y - this->distortionFactor[1]) * this->distortionFactor[3];
	if( x == 0.0 && y == 0.0 ) {
		observed->x = this->distortionFactor[0];
		observed->y = this->distortionFactor[1];
	}
	else {
		d = (ARTKFloat)1.0 - this->distortionFactor[2]/(ARTKFloat)100000000.0 * (x*x+y*y);
		observed->x = x * d + this->distortionFactor[0];
		observed->y = y * d + this->distortionFactor[1];
	}
}




CvMat CCamera::QueryRotationMat()
{
    return _FeaturesRotationMat;
}

CvMat CCamera::QueryTranslationMat()
{
    return _FeaturesTranslationMat;
}

float * CCamera::QueryModelViewMat()
{
/*    // OpenGL ModelView Matrix of World-2-Camera
    for ( int i = 0 ; i < 3 ; i ++ )
    {
        for ( int j = 0 ; j < 3 ; j ++ )
            _ModelView[j*4+i] = _FingerRotation3by3[i][j];
        //_ModelView[8+i] *= -1;
        _ModelView[12+i] = _FingerTranslation[i];
    }
    //_ModelView[14] *= -1;
    _ModelView[3] = _ModelView[7] = _ModelView[11] = 0;
    _ModelView[15] = 1; /**/

    return _ModelView;
}




D3DXMATRIXA16* CCamera::D3DXMakeProjectionMatrix(D3DXMATRIXA16* pOut)
{
	
	double camera_fx = camera_intrinsic[0][0];
	double camera_fy = camera_intrinsic[1][1];
	double camera_ox = camera_intrinsic[0][2];
	double camera_oy = camera_intrinsic[1][2];
	double camera_opengl_dRight = (double)(640-camera_ox)/(double)(camera_fx);
	double camera_opengl_dLeft = -camera_ox/camera_fx;
	double camera_opengl_dTop = (double)(480-camera_oy)/(double)(camera_fy);
	double camera_opengl_dBottom = -camera_oy/camera_fy;

	D3DXMATRIXA16& matProj = *pOut;
	
	float fnear = 1.0f;
	float ffar = 102500.0f;
	float left = float(camera_opengl_dLeft);
	float right = float(camera_opengl_dRight);
	float top = float(camera_opengl_dTop);
	float bottom = float(camera_opengl_dBottom);
	
	//3d math에 의해제작
	matProj[0] = (2*fnear)/(right-left);
	matProj[1] = 0;
	matProj[2] = (right+left)/(right-left);
	matProj[3] = 0;

	matProj[4] = 0;
	matProj[5] = (2*fnear)/(top-bottom);
	matProj[6] = (top+bottom)/(top-bottom);
	matProj[7] = 0;

	matProj[8] = 0;
	matProj[9] = 0;
	matProj[10] = (-1*((ffar+fnear)/(ffar-fnear)));
	matProj[11] = (-1*((2*ffar*fnear)/(ffar-fnear)));

	matProj[12] = 0;
	matProj[13] = 0;
	matProj[14] = -1;
	matProj[15] = 0;

// 	matProj[0] = 2.075020f;
// 	matProj[1] = 0.000000f;
// 	matProj[2] = 0.000000f;
// 	matProj[3] = 0.000000f;	
// 	matProj[4] = 0.000000f;
// 	matProj[5] = 2.805921f;
// 	matProj[6] = 0.000000f;
// 	matProj[7] = 0.000000f;	
// 	matProj[8] = 0.084239f;
// 	matProj[9] = -0.031158f;
// 	matProj[10] = -1.000020f;
// 	matProj[11] = -1.000000f;	
// 	matProj[12] = 0.000000f;
// 	matProj[13] = 0.000000f;
// 	matProj[14] = -2.000020f;
// 	matProj[15] = 0.000000f;
// 	
	wonjo_dx::Matrix_LH_RH_Swap(&matProj,&matProj);

// 	matProj[0] = 2.190473345f;
// 	matProj[1] = 0.0;
// 	matProj[2] = 0.0;
// 	matProj[3] = 0.0;
// 	matProj[4] = 0.0;
// 	matProj[5] = -3.025392424f;
// 	matProj[6] = 0.0;
// 	matProj[7] = 0.0;
// 	matProj[8] = -0.0109375f;
// 	matProj[9] = -0.010416667f;
// 	matProj[10] = 1.02020202f;
// 	matProj[11] = 1.0;
// 	matProj[12] = 0.0;
// 	matProj[13] = 0.0;
// 	matProj[14] = -101.010101f;
// 	matProj[15] = 0.0;


	//파일에 로그 찍어!
	{
		FILE* fp;
		fp = fopen("projectionlog.txt","a+t");
		if(fp)
		{
			fprintf(fp,"----------------------------------\nMatrix is \n");
			for(int i = 0 ; i < 4 ; ++i)
			{
				for(int j = 0 ; j < 4 ; ++j)
				{
					fprintf(fp,"%f\t",matProj[i*4+j]);
				}
				fprintf(fp,"\n");
			}
			fprintf(fp,"with camera projection : left_end(%f), right_end(%f), upper_end(%f), lower_end(%f)\n", left, right, top, bottom );
			fprintf(fp,"                       : near(%f), far(%f)\n", fnear, ffar );
			fclose(fp);
		}
	}
	
	return &matProj;
}








//ModelView를 대체하는 함수
D3DXMATRIXA16* CCamera::D3DXMakeViewMatrix(D3DXMATRIXA16* pOut)
{
	D3DXMATRIXA16& matView = *pOut;

	/* DLT
	D3DXVECTOR3 eye( _CameraCenterRTransPose[0][2], _CameraCenterRTransPose[1][2], _CameraCenterRTransPose[2][2]
	);
	
	D3DXVECTOR3 center(	//translationvertector x, y, z
		_FeaturesTranslation[0], _FeaturesTranslation[1], _FeaturesTranslation[2]
	);
	
	D3DXVECTOR3 up(
		_CameraCenterRTransPose[0][1], _CameraCenterRTransPose[1][1], _CameraCenterRTransPose[2][1]
	);*/

	/* Zhang*/
	
	
// 	D3DXVECTOR3 eye( m_Rotation3x3->data.db[2], m_Rotation3x3->data.db[5], m_Rotation3x3->data.db[8]
// 	);
// 	
// 	D3DXVECTOR3 center(	//translationvertector x, y, z
// 		m_vecTrans->data.db[0], m_vecTrans->data.db[1], m_vecTrans->data.db[2]
// 	);
// 	
// 	D3DXVECTOR3 up(
// 		m_Rotation3x3->data.db[1], m_Rotation3x3->data.db[4], m_Rotation3x3->data.db[7]);
	
	/*
// 	D3DXVECTOR3 eye( m_Rotation3x3->data.db[6], m_Rotation3x3->data.db[7], m_Rotation3x3->data.db[8]
// 	);
// 	
// 	D3DXVECTOR3 center(	//translationvertector x, y, z
// 		m_vecTrans->data.db[0], m_vecTrans->data.db[1], m_vecTrans->data.db[2]
// 	);
// 	
// 	D3DXVECTOR3 up(
// 		m_Rotation3x3->data.db[3], m_Rotation3x3->data.db[4], m_Rotation3x3->data.db[7]);
// 
*/

	//원조야 이건 R3x3에 대한 transpose m_RotationTranspose3x3->data.db로 갖다 써


	//순수 좌표계
	D3DXVECTOR3 eye( 
		m_vecTrans->data.db[0]
		,m_vecTrans->data.db[1]
		,m_vecTrans->data.db[2]
	);

	D3DXVECTOR3 right(
		m_Rotation3x3->data.db[0]
	,m_Rotation3x3->data.db[3]
	,m_Rotation3x3->data.db[6]
	);

	D3DXVECTOR3 up(
		m_Rotation3x3->data.db[1]
	,m_Rotation3x3->data.db[4]
	,m_Rotation3x3->data.db[7]
	);
	


	D3DXVECTOR3 view(
		m_Rotation3x3->data.db[2]
		,m_Rotation3x3->data.db[5]
		,m_Rotation3x3->data.db[8]
	);

	D3DXVECTOR3 LookAt(
		m_Rotation3x3->data.db[2] + m_vecTrans->data.db[0]
		,m_Rotation3x3->data.db[5] + m_vecTrans->data.db[1]
		,m_Rotation3x3->data.db[8] + m_vecTrans->data.db[2]
	);


	
// 	D3DXVECTOR3 up;
// 	D3DXVec3Cross(&up,&right,&view);
// 	D3DXVec3Cross(&up,&view,&right);
// 
// 	//DirectX 좌표계로 이동.
 	D3DXVECTOR3 dxeye(-eye.x, eye.y, -eye.z);
 	D3DXVECTOR3 dxview(view.x, view.y, -view.z);
// 	//D3DXVECTOR3 dxview(0,0,1);
// 	//D3DXVECTOR3 dxup(0,1,0);
 	D3DXVECTOR3 dxup(up.x,-up.y,up.z);
 	D3DXVECTOR3 dxlook = dxeye + dxview;
 	D3DXMatrixLookAtLH(pOut,&dxeye,&dxlook,&dxup);

//  	D3DXMatrixLookAtRH(pOut,&eye,&LookAt,&up);
// 	wonjo_dx::Matrix_LH_RH_Swap(pOut,pOut);
// 	
// 	matView[0] = -matView[0];
// 	matView[1] = -matView[1];
// 	matView[2] = -matView[2];
// 	matView[3] = -matView[3];
// 	matView[8] = -matView[8];
// 	matView[9] = -matView[9];
// 	matView[10] = -matView[10];
// 	matView[11] = -matView[11];


// 	(
// 		-m_Rotation3x3->data.db[1]
// 		,m_Rotation3x3->data.db[4]
// 		,-m_Rotation3x3->data.db[7]
// 	);

	
 	std::cout << "eye = (" << eye.x << "," << eye.y << "," << eye.z << ")" << std::endl;		
 	std::cout << "036 = (" << m_Rotation3x3->data.db[0] << "," << m_Rotation3x3->data.db[3] << "," << m_Rotation3x3->data.db[6] << std::endl;
 	std::cout << "147 = (" << m_Rotation3x3->data.db[1] << "," << m_Rotation3x3->data.db[4] << "," << m_Rotation3x3->data.db[7] << std::endl;
 	std::cout << "258 = (" << m_Rotation3x3->data.db[2] << "," << m_Rotation3x3->data.db[5] << "," << m_Rotation3x3->data.db[8] << std::endl;

	

		
// 
// #ifdef _TESTMODE
// 	static int stackint = 1;
// 	++stackint;
// 
// 	std::cout << "eye = (" << eye.x << "," << eye.y << "," << eye.z << std::endl;
// 	std::cout << "center = (" << center.x << "," << center.y << "," << center.z << std::endl;
// 	std::cout << "up = (" << up.x << "," << up.y << "," << up.z << std::endl;
// 
// 	static float z = 0;
// 
// 	eye.x = 0;
// 	eye.y = 0;
// 	eye.z = --z;
// 
// 	center.x = 0;
// 	center.y = 0;
// 	center.z = 0;
// 
// 	up.x = 0;
// 	up.y = 1;
// 	up.z = 0;
// 	//D3DXMatrixLookAtLH(pOut,&eye,&center,&up);
// 	{
// 		FILE* fp;
// 		fp = fopen("test_view_factors.txt","a+t");
// 		if(fp)
// 		{
// 			fprintf(fp,"-----------------------------------\n");
// 			//fprintf(fp,"test_view_factors : %d\n",stackint);
// 
// 			fprintf(fp,"eye = %f, %f, %f\n",eye.x,eye.y,eye.z);
// 			fprintf(fp,"center = %f, %f, %f\n",LookAt.x,LookAt.y,LookAt.z);
// 			fprintf(fp,"up = %f, %f, %f\n",up.x,up.y,up.z);
// 
// 			fprintf(fp,"matrix \n");
// 			for(int i = 0 ; i < 4 ; ++i)
// 			{
// 				for(int j = 0 ; j < 4 ; ++j)
// 				{
// 					fprintf(fp,"%f\t",matView[i*4+j]);
// 				}
// 				fprintf(fp,"\n");
// 			}
// 
// 			fclose(fp);
// 		}
// 	}
// #else
// 	//D3DXMatrixLookAtLH(pOut,&eye,&center,&up);
// //	D3DXMatrixMultiply(pOut,pOut,wonjo_dx::MakeScaleMatrix(-1,-1,1));
// #endif
	
	
	

	
	//D3DXMatrixLookAtRH(pOut,&center,&eye,&up);


// 	gluLookAt(pOut,eye.x,eye.y,eye.z,center.x,center.y,center.z,up.x,up.y,up.z);
// 	wonjo_dx::Matrix_LH_RH_Swap(pOut,pOut);
	//D3DXMatrixMultiply(pOut,pOut,wonjo_dx::MakeScaleMatrix(-1,-1,1));

	//파일에 로그 찍어!!
	{
		FILE* fp;
		fp = fopen("viewlog.txt","a+t");
		if(fp)
		{
			fprintf(fp,"----------------------------------\nMatrix is \n");
			for(int i = 0 ; i < 4 ; ++i)
			{
				for(int j = 0 ; j < 4 ; ++j)
				{
					fprintf(fp,"%f\t",matView[i*4+j]);
				}
				fprintf(fp,"\n");
			}
			fprintf(fp,"with camera position : %f\t%f\t%f\n", eye.x, eye.y, eye.z );
			fprintf(fp,"with camera view direction : %f\t%f\t%f\n", LookAt.x, LookAt.y, LookAt.z );
			fprintf(fp,"with camera up : %f\t%f\t%f\n", up.x, up.y, up.z );
			fclose(fp);
		}
	}
	return pOut;
}
//~120325 cwj




//BaekAR의 Pose Estimation을 구하는 부분
void CCamera::featurePoseEstimation()
{

	CvPoint3D	ppos3d[4];
   
	//의심 호모지니어스 좌표계
	float iFeaturesPoints[4][2];
	float oFeaturesPoints[4][3];

	
	for ( int i = 0 ; i < 4 ; i ++ )
    {
		iFeaturesPoints[i][0] = featuresResult.vertex[i].x;
        iFeaturesPoints[i][1] = featuresResult.vertex[i].y;
		

    }

    CvMat imageMat = cvMat( 4, 2, CV_32FC1, iFeaturesPoints );
	
	ppos3d[0].x = 0 - FEATURE_WIDTH/2.0;
    ppos3d[0].y = 0 + FEATURE_HEIGHT/2.0;
	ppos3d[0].z = 0;

    ppos3d[1].x = 0 + FEATURE_WIDTH/2.0;
    ppos3d[1].y = 0 + FEATURE_HEIGHT/2.0;
    ppos3d[1].z = 0;

	ppos3d[2].x = 0 + FEATURE_WIDTH/2.0;
    ppos3d[2].y = 0 - FEATURE_HEIGHT/2.0;
    ppos3d[2].z = 0;

	ppos3d[3].x = 0 - FEATURE_WIDTH/2.0;
    ppos3d[3].y = 0 - FEATURE_HEIGHT/2.0;
	ppos3d[3].z = 0;


	for ( int i = 0 ; i < 4 ; i ++ )
    {
		oFeaturesPoints[i][0] = ppos3d[i].x;
        oFeaturesPoints[i][1] = ppos3d[i].y;
		oFeaturesPoints[i][2] = 0;
		
    }
	
	//
    // Prepare Features Coordinates ( object points )
    //
	
    CvMat objectMat = cvMat( 4, 3, CV_32FC1, oFeaturesPoints );

   // CvMat imageMat = cvMat( 4, 2, CV_32FC1, ppos2d );
	
	


	/* DLT 방식 
	cvFindExtrinsicCameraParams2(
            &objectMat,
            &imageMat,
            &intrinsic,
            &distortion,
            &_FeaturesRotationMat,
            &_FeaturesTranslationMat );
		       
    cvRodrigues2( &_FeaturesRotationMat, &_FeaturesRotation3by3Mat );
	cvTranspose(&_FeaturesRotation3by3Mat, &_CameraCenterRTransPoseMat);

	*/

	//다른 방법 장 방법으로 시도
	
	ZhangCalibration(&objectMat, &imageMat);

	//cvInvert( &_FeaturesRotation3by3Mat, &_CameraCenterRMat );
		
}



CvMat* CCamera::calcHomography(CvMat* src, CvMat* dst)
{
	cvFindHomography(src, dst, m_MatHomography);
	

	return m_MatHomography;
}


void CCamera::ZhangCalibration(CvMat* m_RealCornerPosition, CvMat* imgCornerPosition)
{
	calcHomography(m_RealCornerPosition, imgCornerPosition);

	//호모그래피로부터 카메라 외부 파라미터를 복구한다.
	m_vecRotX->data.db[0] = m_MatHomography->data.db[0];
	m_vecRotX->data.db[1] = m_MatHomography->data.db[3];
	m_vecRotX->data.db[2] = m_MatHomography->data.db[6];

	m_vecRotY->data.db[0] = m_MatHomography->data.db[1];
	m_vecRotY->data.db[1] = m_MatHomography->data.db[4];
	m_vecRotY->data.db[2] = m_MatHomography->data.db[7];

	m_vecTrans->data.db[0] = m_MatHomography->data.db[2];
	m_vecTrans->data.db[1] = m_MatHomography->data.db[5];
	m_vecTrans->data.db[2] = m_MatHomography->data.db[8];

	cvMatMul(m_IntrinsicParamInv, m_vecRotX, m_vecRotX);
	cvMatMul(m_IntrinsicParamInv, m_vecRotY, m_vecRotY);
	cvMatMul(m_IntrinsicParamInv, m_vecTrans, m_vecTrans);

	double lamda = 1/(sqrt(m_vecRotX->data.db[0] * m_vecRotX->data.db[0] + m_vecRotX->data.db[1] * m_vecRotX->data.db[1] + m_vecRotX->data.db[2] * m_vecRotX->data.db[2]));


	for(int i = 0 ; i < 3 ; ++i) {
		m_vecRotX->data.db[i] = m_vecRotX->data.db[i] * lamda;
		m_vecRotY->data.db[i] = m_vecRotY->data.db[i] * lamda;
		m_vecTrans->data.db[i] = m_vecTrans->data.db[i] * lamda;
	}

	/*
	cout<<"Translation"<<endl;
	cout<<m_vecTrans->data.db[0]<<", "<<m_vecTrans->data.db[1]<<", "<<m_vecTrans->data.db[2]<<", "<<endl;
	*/
	cvCrossProduct(m_vecRotX, m_vecRotY, m_vecRotZ);

	m_Rotation3x3->data.db[0] = m_vecRotX->data.db[0]; m_Rotation3x3->data.db[1] = m_vecRotY->data.db[0]; m_Rotation3x3->data.db[2] = m_vecRotZ->data.db[0]; 
	m_Rotation3x3->data.db[3] = m_vecRotX->data.db[1]; m_Rotation3x3->data.db[4] = m_vecRotY->data.db[1]; m_Rotation3x3->data.db[5] = m_vecRotZ->data.db[1]; 
	m_Rotation3x3->data.db[6] = m_vecRotX->data.db[2]; m_Rotation3x3->data.db[7] = m_vecRotY->data.db[2]; m_Rotation3x3->data.db[8] = m_vecRotZ->data.db[2]; 

	cvRodrigues2(m_Rotation3x3, m_Rotation3x1);


	//Transpose Matrix 연산 추가 by 성욱

	cvTranspose(m_Rotation3x3, m_RotationTranspose3x3);

	//카메라 외부 파라미터 셋팅
	m_ExtrinsicParam->data.db[0] = m_vecRotX->data.db[0]; m_ExtrinsicParam->data.db[1] = m_vecRotY->data.db[0]; m_ExtrinsicParam->data.db[2] = m_vecRotZ->data.db[0]; m_ExtrinsicParam->data.db[3] = m_vecTrans->data.db[0];
	m_ExtrinsicParam->data.db[4] = m_vecRotX->data.db[1]; m_ExtrinsicParam->data.db[5] = m_vecRotY->data.db[1]; m_ExtrinsicParam->data.db[6] = m_vecRotZ->data.db[1]; m_ExtrinsicParam->data.db[7] = m_vecTrans->data.db[1];
	m_ExtrinsicParam->data.db[8] = m_vecRotX->data.db[2]; m_ExtrinsicParam->data.db[9] = m_vecRotY->data.db[2]; m_ExtrinsicParam->data.db[10] = m_vecRotZ->data.db[2]; m_ExtrinsicParam->data.db[11] = m_vecTrans->data.db[2];
}



