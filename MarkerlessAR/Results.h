/** \file Results.h
\brief Ʈ��ŷ�� ��Ŀ�� ����
*/

#pragma once


#ifdef	_WIN32
//	#include <cxcore.h>
#else
	#include <OpenCV/OpenCV.h>
#endif

#include "dll_header.h"

/** \struct Results
* \brief ����Ʈ�� ��Ŀ�� ����
* 
* ����� ���� ���� ������ ���� 
* \ remark ���� ax+by+c=0�� ǥ���Ǳ� ���Ͽ� a,b,c�� ����Ѵ�.
* \param area �󺧸��� ������ �ȼ� ����
* \param id �νĵ� ��Ŀ ��ȣ
* \param dir ��Ŀ�� ȸ�� ���θ� �˷��ִ� �������� (0,1,2,3�� ����). ����Ʈ �� ��Ŀ�� ���� ������ �˷���.arGetTransMat()���� transformation matrix�� �����ϱ� ���� �߿���.
* \param cf ��Ŀ�� ���ɼ�
* \param center ��Ŀ�� �߾�(in ideal screen coordinates)
* \param line ��Ŀ�� �� ������ ���� ���� ������ (in ideal screen coordinates)
* \param vertex ��Ŀ�� edge point (in ideal screen coordinates)
* \param Tcm ī�޶� ��ǥ��� ��Ŀ ��ǥ�� ���� Ʈ���������̼� ��Ʈ����
*/
typedef struct 
{
	int					ID;				// Detector::readID()���� ���							/  Detector::doDetecting()���� ����
	int					dir;				// Detector::readID()���� ���							/  Detector::doDetecting()���� ����
	double				cf;				// Detector::readID()���� ���							/  Detector::doDetecting()���� ����

	CvPoint2D		vertex[4];	// Detector::MSL_arGetLine2()���� ���		/  Detector::MSL_arGetLine2()���� ����
	CvPoint2D		center;		// Detector::GetLocationInfo()���� ���		/  Detector::GetLocationInfo()���� ����
	double				area;			// Detector::doDetecting()���� ���				/  Detector::doDetecting()���� ����
	double				line[4][3];	// Detector::MSL_arGetLine2()���� ���		/  Detector::MSL_arGetLine2()���� ����
	double				Tcm[3][4];	// 
} Results;	// end of struct


/** \struct ARMultiMarkerEach
* \brief ��Ƽ ��Ŀ ���� ������ ��Ŀ ����
* \param id �νĵ� ��Ŀ ��ȣ
* \param width ��Ŀ�� ���� ũ��
* \param center ��Ŀ�� ����
* \param trans[3][4] transform matrix
* \param itrans[3][4] 10. inverse transform matrix
* \param pos3d[4][3] ��Ŀ ��ǥ�迡���� �� ����Ʈ�� ��ġ���� �۷ι� ��ǥ�迡���� ��ġ�� �ٲ� ����
* \param visible visible ����
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
* \brief ��Ƽ ��Ŀ Configure ����ü
* \param num_of_markers ��Ŀ�� ����
* \param markers ������ ��Ŀ
* \param trans[3][4] transform matrix
* \param prevF ���� ��Ŀ
* \param cVisible visible ����
*/
typedef struct 
{
	int				num_of_markers;
	
	ARMultiMarkerEach	*markers;
	ARTKFloat		trans[3][4];
	int				prevF;
	bool			cVisible;
} MultiConfigure;
