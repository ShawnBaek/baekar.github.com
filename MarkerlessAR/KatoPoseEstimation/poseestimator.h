/** \file PoseEstimator.h
\brief Ideal�� ��ǥ�� ��� �ڼ����� ��� ����
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
* \brief �ڼ����� Ŭ����
* \remark  ī�޶�� �Է¹��� ��Ŀ�� �� ������ ��ǥ�� ������Ʈ�� ���µ� �ʿ��� ��ǥ�� ���
* \param camera ī�޶�
*/
class CPoseEstimator
{
protected:
	CCamera	*camera;
	const static int DEFAULT_MARKER_WIDTH = 80;

	//// ���� �Լ���
	//ARTKFloat CPoseEstimator::arGetTransMat3( ARTKFloat rot[3][3], ARTKFloat ppos2d[][2],
	//																				   ARTKFloat ppos3d[][2], int num, ARTKFloat conv[3][4],
	//																				   CCamera *camera );

public:
	CPoseEstimator(void); /**<������*/
	~CPoseEstimator(void); /**<�Ҹ���*/
	//CPoseEstimator(CCamera	*camera);

	/**
	* \fn ARTKFloat calculateTransformationMatrix(CCamera *camera, Results* aResult)
	* \brief ������Ʈ�� ���µ� �ʿ��� ��ǥ�� ���ϴ� �Լ�
	* \remark ī�޶�� �Է¹��� �̱� ��Ŀ�� �� ������ ��ǥ�� ������Ʈ�� ���µ� �ʿ���  ��ǥ�� ����ϴ� �Լ��̴�.
	* \param camera	�Էµ� ī�޶� Ŭ����
	* \param aResult ���� �Ķ���� Ŭ����
	*/
	virtual ARTKFloat calculateTransformationMatrix(CCamera *camera, Results* aResult) { return 0.0;}

	//void setCamera(CCamera* camera);

	/**
	* \fn ARTKFloat calculateTranformationMatrixMulti(CCamera* camera, Results* results, int marker_num, MultiConfigure* config)
	* \brief ������Ʈ�� ���µ� ��Ƽ ��Ŀ ���� �ʿ��� ��ǥ�� ���ϴ� �Լ�
	* \remark ī�޶�� �Է¹��� ��Ƽ ��Ŀ�� �� ������ ��ǥ�� ������Ʈ�� ���µ� �ʿ���  ��ǥ�� ����ϴ� �Լ��̴�.
	* \param camera	�Էµ� ī�޶� Ŭ����
	* \param results ���� �Ķ���� Ŭ����
	* \param marker_num ��Ŀ ����
	* \param config ��Ƽ��Ŀ configure ������
	*/
	virtual ARTKFloat calculateTranformationMatrixMulti(CCamera* camera, Results* results, int marker_num, MultiConfigure* config) { return 0.0; }
};	// end of class

