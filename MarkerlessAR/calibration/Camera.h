/** 
\file Camera.h
\brief ī�޶��� �Է� ���� ��� ����
*/

#pragma once
#include <opencv2/opencv.hpp>
#include "../dll_header.h"

#define	defaultFileName	"/camera.txt"

/**
* \class CCamera 
* \brief ī�޶� ���� Ŭ����
* \remark  ī�޶��� �ùٸ� �Է��� ���� ����
* \param size ī�޶� ũ��
* \param projectionMatrix ī�޶��� projectionMatrix
* \param distortionFactor ī�޶��� distortionFactor
* \param parameterFile �Ķ���Ͱ� ����� ������ �̸��� ����� ����
* \param img_input ī�޶� ���� �ڵ鸵 ����
*/
class CCamera
{
public:
	CCamera(void); /**<������*/
	CCamera(bool	turnOn); /**<������*/

	~CCamera(void); /**<�Ҹ���*/

	// Intrinsic Parameters
	CvSize	size;
	double	projectionMatrix[3][4];
	double	distortionFactor[4];

	char	parameterFile[100];

	bool	loaded;

	// Loading Functions
	/** 
	* \fn bool load	(const char* fileName)
	* \brief ī�޶� �Ķ���͸� �����ϴ� �Լ�.
	* \remark ���ǵ� fileName�� ��Camera.dat���� ������ �����Ͽ� Distortion Factor, ȭ�� ũ�⸦ �о� ī�޶� ����� �����ϰ� Projection Matrix�� �д´�.\n
	* ī�޶� ������ ������ ����ڰ� �ٲ� �� �ִµ� �̴� Camera.dat������ ���θ� ������ �־�� �Ѵ�. \n
	* �� ������ calibration �����Ϳ��� mapping�� ���Ͽ� camera parameter ���Ͽ��� �����ϵ��� ����Ǿ� �ִ�. \n
	* ���� ī�޶��� ����� �����ϰ��� �� ������ #size �κп� xũ�� (����) yũ�� �� �������� �Է��ϸ� �ȴ�.
	* \param fileName ������ ���ϸ�
	*/
	bool load	(const char* fileName);

	/** 
	* \fn bool load()
	* \brief ī�޶� �Ķ���͸� �����ϴ� �Լ�.
	* \remark ���ǵ� fileName�� ��Camera.dat���� ������ �����Ͽ� Distortion Factor, ȭ�� ũ�⸦ �о� ī�޶� ����� �����ϰ� Projection Matrix�� �д´�.\n
	* ī�޶� ������ ������ ����ڰ� �ٲ� �� �ִµ� �̴� Camera.dat������ ���θ� ������ �־�� �Ѵ�. \n
	* �� ������ calibration �����Ϳ��� mapping�� ���Ͽ� camera parameter ���Ͽ��� �����ϵ��� ����Ǿ� �ִ�. \n
	* ���� ī�޶��� ����� �����ϰ��� �� ������ #size �κп� xũ�� (����) yũ�� �� �������� �Է��ϸ� �ȴ�.
	*/
	bool	load()	{	return load(defaultFileName);		}

	// ���� 

	/** 
	* \fn void observ2Ideal(CvPoint	observed, CvPoint2D *ideal)
	* \brief ī�޶�� �Է� ���� ������ ��ǥ�� Ideal�� ��ǥ�� ��ȯ�ϴ� �Լ�.
	* \param observed ī�޶�� �Է� ���� ������ ��ǥ
	* \param ideal ideal�ϰ� ��ȯ�� ������ ��ǥ
	*/
	void observ2Ideal(CvPoint	observed, CvPoint2D *ideal);

	/** 
	* \fn void ideal2Observ(CvPoint2D ideal, CvPoint *observed)
	* \brief ī�޶�� �Է� ���� ������ ��ǥ�� observed�� ��ǥ�� ��ȯ�ϴ� �Լ�.
	* \param ideal �Է¹��� ������ ��ǥ
	* \param observed observed�ϰ� ��ȯ�� ������ ��ǥ
	*/
	void ideal2Observ(CvPoint2D ideal, CvPoint *observed);


	/** 
	* \fn void ideal2Observ(CvPoint2D ideal, CvPoint2D *observed)
	* \brief ī�޶�� �Է� ���� ������ ��ǥ�� observed�� ��ǥ�� ��ȯ�ϴ� �Լ�.
	* \param ideal �Է¹��� ������ ��ǥ
	* \param observed observed�ϰ� ��ȯ�� ������ ��ǥ
	*/
	void ideal2Observ(CvPoint2D ideal, CvPoint2D *observed);

	/** 
	* \fn void observ2Ideal(int ox, int oy, ARTKFloat *ix, ARTKFloat *iy)
	* \brief ī�޶�� �Է� ���� ������ ��ǥ�� Ideal�� ��ǥ�� ��ȯ�ϴ� �Լ�.
	* \param ox ī�޶�� �Է� ���� ������ x��ǥ
	* \param oy ī�޶�� �Է� ���� ������ y��ǥ
	* \param ix ideal�ϰ� ��ȯ�� ������ x��ǥ
	* \param iy ideal�ϰ� ��ȯ�� ������ y��ǥ
	*/
	void observ2Ideal(int ox, int oy, ARTKFloat *ix, ARTKFloat *iy);

};	// end of class

//};};		// end of namespace