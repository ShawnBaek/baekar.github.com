/** \file KatoPoseEstimator.h
\brief Kato & Billinghurst 알고리즘 기반의 자세추정 헤더 파일
*/

#ifdef EXPORTAPI
#undef	EXPORTAPI
#define EXPORTAPI __declspec(dllexport)
#else
#define EXPORTAPI __declspec(dllimport)
#endif

#pragma once

#include "poseestimator.h"
#include "../Results.h"
#include "../calibration/Camera.h"


/** \class CKatoPoseEstimator
* \brief Kato & Billinghurst 알고리즘 기반 자세추정 클래스
* \remark  카메라로 입력받은 마커의 네 꼭지점 좌표에 오브젝트를 띄우는데 필요한 좌표로 계산
* \param camera 카메라
*/
class CKatoPoseEstimator :
	public CPoseEstimator
{
protected:
	const	int		MAX_LOOP_COUNT;
	const	float	MAX_FIT_ERROR;
	const	int		QUAD_MARKER_STONE;
	const	static	int		P_MAX = 500;

	int arGetInitPose( Results *aResult, double rot[3][3] );
	int check_dir( double dir[3], CvPoint2D	*st, CvPoint2D	*ed);
	int check_rotation( double rot[2][3] );
	CvPoint2D	*ppos2d[4], ppos3d[4];

	ARTKFloat arGetTransMat3(	double rot[3][3], CvPoint2D *ppos2d[4], CvPoint3D ppos3d[],
		/*double ppos2d[][2],double ppos3d[][2],*/ int num, double conv[3][4] );
	ARTKFloat arGetTransMatSub( double rot[3][3], CvPoint2D *ppos2d[4], CvPoint3D pos3d[],
											/*double ppos2d[][2],double pos3d[][3], */
											int num, double conv[3][4]);
	ARTKFloat arModifyMatrix( double rot[3][3], double trans[3], CvPoint3D	vertex[], CvPoint2D pos2d[], int num );
	int arGetNewMatrix( double a, double b, double c, double trans[3], double trans2[3][4], double ret[3][4] );
























public:
	CKatoPoseEstimator(void);		/**<생성자*/
	~CKatoPoseEstimator(void);		/**<소멸자*/

	/**
	* \fn ARTKFloat calculateTransformationMatrix(CCamera *camera, Results* aResult)
	* \brief Kato & Billinghurst 알고리즘 기반의 오브젝트를 띄우는데 필요한 좌표를 구하는 함수
	* \remark 카메라로 입력받은 싱글 마커의 네 꼭지점 좌표를 오브젝트를 띄우는데 필요한  좌표로 계산하는 함수이다.
	* \param camera	입력된 카메라 클래스
	* \param aResult 계산된 파라미터 클래스
	*/
	ARTKFloat calculateTransformationMatrix(CCamera *camera, Results* aResult);
	//void setCamera(CCamera* camera);
	/**
	* \fn ARTKFloat calculateTranformationMatrixMulti(CCamera* camera, Results* results, int marker_num, MultiConfigure* config)
	* \brief Kato & Billinghurst 알고리즘 기반의 오브젝트를 띄우는데 멀티 마커 사용시 필요한 좌표를 구하는 함수
	* \remark 카메라로 입력받은 멀티 마커의 네 꼭지점 좌표를 오브젝트를 띄우는데 필요한  좌표로 계산하는 함수이다.
	* \param camera	입력된 카메라 클래스
	* \param results 계산된 파라미터 클래스
	* \param marker_num 마커 개수
	* \param config 멀티마커 configure 데이터
	*/
	ARTKFloat calculateTranformationMatrixMulti(CCamera* camera, Results* results, int marker_num, MultiConfigure* config);
};
