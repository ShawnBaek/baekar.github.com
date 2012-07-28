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

		//double* view�� opengl �� matrix�� �Ѿ���� ������ directx ��ǥ�迡���� �¿츦 flip�ؾ��Ѵ�.
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

		//double* view�� opengl �� matrix�� �Ѿ���� ������ directx ��ǥ�迡���� �¿츦 flip�ؾ��Ѵ�.
// 		D3DXMATRIXA16 matFlip;
// 		D3DXMatrixScaling(&matFlip,-1,1,1);
// 		matView *= matFlip;

		if(GetDevice()) GetDevice()->SetTransform(D3DTS_VIEW, &matView);
	}
}