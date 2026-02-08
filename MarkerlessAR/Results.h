/** \file Results.h
\brief 트래킹된 마커의 정보
*/

#pragma once


#ifdef	_WIN32
//	#include <cxcore.h>
#else
	#include <OpenCV/OpenCV.h>
#endif

#include "dll_header.h"

/** \struct Results
* \brief 디텍트된 마커의 구조
* 
* 컨투어를 구한 후의 정보를 저장 
* \ remark 선은 ax+by+c=0로 표현되기 위하여 a,b,c를 사용한다.
* \param area 라벨링된 영역의 픽셀 갯수
* \param id 인식된 마커 번호
* \param dir 마커의 회전 여부를 알려주는 방향정보 (0,1,2,3이 들어옴). 디텍트 된 마커의 선분 순서를 알려줌.arGetTransMat()에서 transformation matrix를 수행하기 위해 중요함.
* \param cf 마커일 가능성
* \param center 마커의 중앙(in ideal screen coordinates)
* \param line 마커의 네 측면을 위한 선분 방정식 (in ideal screen coordinates)
* \param vertex 마커의 edge point (in ideal screen coordinates)
* \param Tcm 카메라 좌표계와 마커 좌표계 간의 트랜스포매이션 매트릭스
*/
typedef struct 
{
	int					ID;				// Detector::readID()에서 계산							/  Detector::doDetecting()에서 저장
	int					dir;				// Detector::readID()에서 계산							/  Detector::doDetecting()에서 저장
	double				cf;				// Detector::readID()에서 계산							/  Detector::doDetecting()에서 저장

	CvPoint2D		vertex[4];	// Detector::MSL_arGetLine2()에서 계산		/  Detector::MSL_arGetLine2()에서 저장
	CvPoint2D		center;		// Detector::GetLocationInfo()에서 계산		/  Detector::GetLocationInfo()에서 저장
	double				area;			// Detector::doDetecting()에서 계산				/  Detector::doDetecting()에서 저장
	double				line[4][3];	// Detector::MSL_arGetLine2()에서 계산		/  Detector::MSL_arGetLine2()에서 저장
	double				Tcm[3][4];	// 
} Results;	// end of struct


/** \struct ARMultiMarkerEach
* \brief 멀티 마커 사용시 각각의 마커 정보
* \param id 인식된 마커 번호
* \param width 마커의 가로 크기
* \param center 마커의 센터
* \param trans[3][4] transform matrix
* \param itrans[3][4] 10. inverse transform matrix
* \param pos3d[4][3] 마커 좌표계에서의 네 포인트의 위치들을 글로벌 좌표계에서의 위치로 바꾼 정보
* \param visible visible 여부
*/
typedef	struct	{
	int				id;
	ARTKFloat		width;
	CvPoint2D		center;
	ARTKFloat		trans[3][4];
	ARTKFloat		itrans[3][4];
	ARTKFloat		pos3d[4][3];
	int				visible;
}	ARMultiMarkerEach;

/** \struct MultiConfigure
* \brief 멀티 마커 Configure 구조체
* \param num_of_markers 마커의 개수
* \param markers 각각의 마커
* \param trans[3][4] transform matrix
* \param prevF 이전 마커
* \param cVisible visible 여부
*/
typedef struct 
{
	int				num_of_markers;
	
	ARMultiMarkerEach	*markers;
	ARTKFloat		trans[3][4];
	int				prevF;
	bool			cVisible;
} MultiConfigure;
