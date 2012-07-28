#include "wonjo.h"


namespace cam3D
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 up;
	D3DXVECTOR3 lookAt;
	static D3DXMATRIXA16 matView;
	static D3DXMATRIXA16 matProj;


	void UpdateProjection()
	{
		if(d3d::gpDevice)
		{
			d3d::gpDevice->SetTransform(D3DTS_PROJECTION, &matProj);
		}
	}

	void SetView(double* dbmatView)
	{
		if(dbmatView)
		{
			//D3DXMatrixLookAtLH(&matView,&pos,&lookAt,&up);
#ifdef _TESTMODE
			matView[0] = 0.99981194128147199;
			matView[1] = -0.018552073020307029;
			matView[2] = -0.0056482437645758912;
			matView[3] = 0;
			matView[4] = -0.017918288667907761;
			matView[5] = -0.99513878692707813;
			matView[6] = 0.096838678659495664;
			matView[7] = 0;
			matView[8] = -0.0074173446858294976;
			matView[9] = -0.096719260439442609;
			matView[10] = -0.99528406380181678;
			matView[11] = 0;
			matView[12] = -49.206718431201708;
			matView[13] = 14.710132728454928;
			matView[14] = 226.63746113148767;
			matView[15] = 1;
#else
			for(int i = 0 ; i < 16 ; ++i) 
			{
				matView[i] = (float)dbmatView[i];
				std::cout << matView[i] << " " ;
			}
			std::cout << std::endl;
#endif
		}
	}

	void UpdateView(double* dbmatView /* = NULL */)
	{
		if(d3d::gpDevice) 
		{
			SetView(dbmatView);
			d3d::gpDevice->SetTransform(D3DTS_VIEW, &matView);
		}
	}

	void AAR3DInitSystem(/*float FieldOfViewX, float FieldOfViewY, float sightLength*/)
	{

		//float fovx = Deg2Rad(FieldOfViewX);
		//float fovy = Deg2Rad(FieldOfViewY);
		//D3DXMatrixPerspectiveFovLH(&matProj, 60, Deg2Rad(tanf(60/2))/Deg2Rad(tanf(60/2)), 1.0f, 10000.0f);
		
		matProj[0] = 2.190473345f;
		matProj[1] = 0.0;
		matProj[2] = 0.0;
		matProj[3] = 0.0;
		matProj[4] = 0.0;
		matProj[5] = -3.025392424f;
		matProj[6] = 0.0;
		matProj[7] = 0.0;
		matProj[8] = -0.0109375f;
		matProj[9] = -0.010416667f;
		matProj[10] = 1.02020202f;
		matProj[11] = 1.0;
		matProj[12] = 0.0;
		matProj[13] = 0.0;
		matProj[14] = -101.010101f;
		matProj[15] = 1;
	}




}