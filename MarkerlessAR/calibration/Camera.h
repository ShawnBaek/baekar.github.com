/** 
\file Camera.h
\brief 카메라의 입력 설정 헤더 파일
*/

#pragma once
#include <opencv2/opencv.hpp>
#include "../dll_header.h"

#define	defaultFileName	"/camera.txt"

/**
* \class CCamera 
* \brief 카메라 설정 클래스
* \remark  카메라의 올바른 입력을 위해 셋팅
* \param size 카메라 크기
* \param projectionMatrix 카메라의 projectionMatrix
* \param distortionFactor 카메라의 distortionFactor
* \param parameterFile 파라미터가 저장된 파일의 이름이 저장될 변수
* \param img_input 카메라 영상 핸들링 변수
*/
class CCamera
{
public:
	CCamera(void); /**<생성자*/
	CCamera(bool	turnOn); /**<생성자*/

	~CCamera(void); /**<소멸자*/

	// Intrinsic Parameters
	CvSize	size;
	double	projectionMatrix[3][4];
	double	distortionFactor[4];

	char	parameterFile[100];

	bool	loaded;

	// Loading Functions
	/** 
	* \fn bool load	(const char* fileName)
	* \brief 카메라 파라미터를 셋팅하는 함수.
	* \remark 정의된 fileName인 “Camera.dat”의 파일을 오픈하여 Distortion Factor, 화면 크기를 읽어 카메라 사이즈를 세팅하고 Projection Matrix를 읽는다.\n
	* 카메라 사이즈 설정을 사용자가 바꿀 수 있는데 이는 Camera.dat파일의 내부를 수정해 주어야 한다. \n
	* 이 파일은 calibration 데이터와의 mapping을 위하여 camera parameter 파일에서 설정하도록 설계되어 있다. \n
	* 따라서 카메라의 사이즈를 변경하고자 할 때에는 #size 부분에 x크기 (공백) y크기 의 형식으로 입력하면 된다.
	* \param fileName 오픈할 파일명
	*/
	bool load	(const char* fileName);

	/** 
	* \fn bool load()
	* \brief 카메라 파라미터를 셋팅하는 함수.
	* \remark 정의된 fileName인 “Camera.dat”의 파일을 오픈하여 Distortion Factor, 화면 크기를 읽어 카메라 사이즈를 세팅하고 Projection Matrix를 읽는다.\n
	* 카메라 사이즈 설정을 사용자가 바꿀 수 있는데 이는 Camera.dat파일의 내부를 수정해 주어야 한다. \n
	* 이 파일은 calibration 데이터와의 mapping을 위하여 camera parameter 파일에서 설정하도록 설계되어 있다. \n
	* 따라서 카메라의 사이즈를 변경하고자 할 때에는 #size 부분에 x크기 (공백) y크기 의 형식으로 입력하면 된다.
	*/
	bool	load()	{	return load(defaultFileName);		}

	// 보정 

	/** 
	* \fn void observ2Ideal(CvPoint	observed, CvPoint2D *ideal)
	* \brief 카메라로 입력 받은 영상의 좌표를 Ideal한 좌표로 변환하는 함수.
	* \param observed 카메라로 입력 받은 영상의 좌표
	* \param ideal ideal하게 변환된 영상의 좌표
	*/
	void observ2Ideal(CvPoint	observed, CvPoint2D *ideal);

	/** 
	* \fn void ideal2Observ(CvPoint2D ideal, CvPoint *observed)
	* \brief 카메라로 입력 받은 영상의 좌표를 observed한 좌표로 변환하는 함수.
	* \param ideal 입력받은 영상의 좌표
	* \param observed observed하게 변환된 영상의 좌표
	*/
	void ideal2Observ(CvPoint2D ideal, CvPoint *observed);


	/** 
	* \fn void ideal2Observ(CvPoint2D ideal, CvPoint2D *observed)
	* \brief 카메라로 입력 받은 영상의 좌표를 observed한 좌표로 변환하는 함수.
	* \param ideal 입력받은 영상의 좌표
	* \param observed observed하게 변환된 영상의 좌표
	*/
	void ideal2Observ(CvPoint2D ideal, CvPoint2D *observed);

	/** 
	* \fn void observ2Ideal(int ox, int oy, ARTKFloat *ix, ARTKFloat *iy)
	* \brief 카메라로 입력 받은 영상의 좌표를 Ideal한 좌표로 변환하는 함수.
	* \param ox 카메라로 입력 받은 영상의 x좌표
	* \param oy 카메라로 입력 받은 영상의 y좌표
	* \param ix ideal하게 변환된 영상의 x좌표
	* \param iy ideal하게 변환된 영상의 y좌표
	*/
	void observ2Ideal(int ox, int oy, ARTKFloat *ix, ARTKFloat *iy);

};	// end of class

//};};		// end of namespace