#include "wonjo.h"


namespace wonjo_dx
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 up;
	D3DXVECTOR3 lookAt;
	static D3DXMATRIXA16 matView;
	static D3DXMATRIXA16 matProj;

	void SetProjectionMatrix(double* proj)
	{
		if(proj)
		{
			for(int i = 0 ; i < 16 ; ++i) matProj[i] = proj[i];
		}

		if(GetDevice()) GetDevice()->SetTransform(D3DTS_PROJECTION, &matProj);
	}
	
	void SetModelViewMatrix(double* view)
	{
		if(view)
		{
			for(int i = 0 ; i < 16 ; ++i) matView[i] = view[i];
		}

		//double* view가 opengl 의 matrix로 넘어오기 때문에 directx 좌표계에서는 좌우를 flip해야한다.
		D3DXMATRIXA16 matFlip;
		D3DXMatrixScaling(&matFlip,-1,1,1);
		matView *= matFlip;

		if(GetDevice()) GetDevice()->SetTransform(D3DTS_VIEW, &matView);
	}

	void SetProjectionMatrix(const D3DXMATRIXA16* proj)
	{
		matProj = *proj;

		if(GetDevice()) GetDevice()->SetTransform(D3DTS_PROJECTION, &matProj);
	}

	void SetModelViewMatrix(const D3DXMATRIXA16* view)
	{
		matView = *view;

		//double* view가 opengl 의 matrix로 넘어오기 때문에 directx 좌표계에서는 좌우를 flip해야한다.
// 		D3DXMATRIXA16 matFlip;
// 		D3DXMatrixScaling(&matFlip,-1,1,1);
// 		matView *= matFlip;

		if(GetDevice()) GetDevice()->SetTransform(D3DTS_VIEW, &matView);
	}
}