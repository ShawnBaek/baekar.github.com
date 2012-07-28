/** \file KatoPoseEstimator.h
\brief Kato & Billinghurst �˰��� ����� �ڼ����� ��� ����
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
* \brief Kato & Billinghurst �˰��� ��� �ڼ����� Ŭ����
* \remark  ī�޶�� �Է¹��� ��Ŀ�� �� ������ ��ǥ�� ������Ʈ�� ���µ� �ʿ��� ��ǥ�� ���
* \param camera ī�޶�
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
	CKatoPoseEstimator(void);		/**<������*/
	~CKatoPoseEstimator(void);		/**<�Ҹ���*/

	/**
	* \fn ARTKFloat calculateTransformationMatrix(CCamera *camera, Results* aResult)
	* \brief Kato & Billinghurst �˰��� ����� ������Ʈ�� ���µ� �ʿ��� ��ǥ�� ���ϴ� �Լ�
	* \remark ī�޶�� �Է¹��� �̱� ��Ŀ�� �� ������ ��ǥ�� ������Ʈ�� ���µ� �ʿ���  ��ǥ�� ����ϴ� �Լ��̴�.
	* \param camera	�Էµ� ī�޶� Ŭ����
	* \param aResult ���� �Ķ���� Ŭ����
	*/
	ARTKFloat calculateTransformationMatrix(CCamera *camera, Results* aResult);
	//void setCamera(CCamera* camera);
	/**
	* \fn ARTKFloat calculateTranformationMatrixMulti(CCamera* camera, Results* results, int marker_num, MultiConfigure* config)
	* \brief Kato & Billinghurst �˰��� ����� ������Ʈ�� ���µ� ��Ƽ ��Ŀ ���� �ʿ��� ��ǥ�� ���ϴ� �Լ�
	* \remark ī�޶�� �Է¹��� ��Ƽ ��Ŀ�� �� ������ ��ǥ�� ������Ʈ�� ���µ� �ʿ���  ��ǥ�� ����ϴ� �Լ��̴�.
	* \param camera	�Էµ� ī�޶� Ŭ����
	* \param results ���� �Ķ���� Ŭ����
	* \param marker_num ��Ŀ ����
	* \param config ��Ƽ��Ŀ configure ������
	*/
	ARTKFloat calculateTranformationMatrixMulti(CCamera* camera, Results* results, int marker_num, MultiConfigure* config);
};
