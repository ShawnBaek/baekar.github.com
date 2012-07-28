/** \file PoseEstimator.h
\brief Ideal한 좌표를 얻는 자세추정 헤더 파일
*/

#pragma once

#ifdef EXPORTAPI
#undef	EXPORTAPI
#define EXPORTAPI __declspec(dllexport)
#else
#define EXPORTAPI __declspec(dllimport)
#endif


#include "../Results.h"
#include "../calibration/Camera.h"

/** \class CPoseEstimator
* \brief 자세추정 클래스
* \remark  카메라로 입력받은 마커의 네 꼭지점 좌표에 오브젝트를 띄우는데 필요한 좌표로 계산
* \param camera 카메라
*/
class CPoseEstimator
{
protected:
	CCamera	*camera;
	const static int DEFAULT_MARKER_WIDTH = 80;

	//// 내부 함수들
	//ARTKFloat CPoseEstimator::arGetTransMat3( ARTKFloat rot[3][3], ARTKFloat ppos2d[][2],
	//																				   ARTKFloat ppos3d[][2], int num, ARTKFloat conv[3][4],
	//																				   CCamera *camera );

public:
	CPoseEstimator(void); /**<생성자*/
	~CPoseEstimator(void); /**<소멸자*/
	//CPoseEstimator(CCamera	*camera);

	/**
	* \fn ARTKFloat calculateTransformationMatrix(CCamera *camera, Results* aResult)
	* \brief 오브젝트를 띄우는데 필요한 좌표를 구하는 함수
	* \remark 카메라로 입력받은 싱글 마커의 네 꼭지점 좌표를 오브젝트를 띄우는데 필요한  좌표로 계산하는 함수이다.
	* \param camera	입력된 카메라 클래스
	* \param aResult 계산된 파라미터 클래스
	*/
	virtual ARTKFloat calculateTransformationMatrix(CCamera *camera, Results* aResult) { return 0.0;}

	//void setCamera(CCamera* camera);

	/**
	* \fn ARTKFloat calculateTranformationMatrixMulti(CCamera* camera, Results* results, int marker_num, MultiConfigure* config)
	* \brief 오브젝트를 띄우는데 멀티 마커 사용시 필요한 좌표를 구하는 함수
	* \remark 카메라로 입력받은 멀티 마커의 네 꼭지점 좌표를 오브젝트를 띄우는데 필요한  좌표로 계산하는 함수이다.
	* \param camera	입력된 카메라 클래스
	* \param results 계산된 파라미터 클래스
	* \param marker_num 마커 개수
	* \param config 멀티마커 configure 데이터
	*/
	virtual ARTKFloat calculateTranformationMatrixMulti(CCamera* camera, Results* results, int marker_num, MultiConfigure* config) { return 0.0; }
};	// end of class

