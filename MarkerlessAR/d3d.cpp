#include "wonjo.h"
#include "omp.h"

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)


//Inner Utilities
namespace d3d
{
	LPDIRECT3D9 gpD3D = NULL;
	LPDIRECT3DDEVICE9 gpDevice = NULL;
	static int nScreenX, nScreenY;
	static BOOL bInitedOnce = FALSE;
	static LPDIRECT3DVERTEXBUFFER9 pVB = NULL;
	static LPDIRECT3DTEXTURE9 pTXT = NULL;
	static LARGE_INTEGER cpufrq;
	static LARGE_INTEGER LastTime;
	static LPD3DXFONT gFont = NULL;
	static LPD3DXSPRITE gSprite = NULL;

	struct CUSTOMVERTEX
	{
		D3DXVECTOR3 position; // The position
		D3DCOLOR color;    // The color
#ifndef SHOW_HOW_TO_USE_TCI
		FLOAT tu, tv;   // The texture coordinates
#endif
		void setUV(float u, float v)
		{
			tu = u;
			tv = v;
		}
	};

	LPDIRECT3DDEVICE9 GetDevice()
	{
		return gpDevice;
	}

	void AAR3DMakeDesktopSize()
	{
		RECT rtDesktop;
		GetWindowRect(GetDesktopWindow(), &rtDesktop);
		nScreenX = rtDesktop.right;
		nScreenY = rtDesktop.bottom;
	}

	LPDIRECT3DVERTEXBUFFER9 AAR3DGetModel()
	{
		if(pVB) return pVB;		

		int numvertices = 6;

		if( FAILED( gpDevice->CreateVertexBuffer( numvertices * sizeof( CUSTOMVERTEX ), 
			0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &pVB, NULL ) ) )
			return NULL;


		CUSTOMVERTEX* pAddr;
		if(FAILED(pVB->Lock(0,0,(void**)&pAddr,0))) return NULL;

		for(int i = 0 ; i < numvertices ; ++i)
			pAddr[i].color=0xffffffff;

		//앞면
		pAddr[0].position=D3DXVECTOR3(-1,-1,0);
		pAddr[0].setUV(0,1);

		pAddr[1].position=D3DXVECTOR3(-1,1,0);
		pAddr[1].setUV(0,0);

		pAddr[2].position=D3DXVECTOR3(1,-1,0);
		pAddr[2].setUV(1,1);

		pAddr[3].position=D3DXVECTOR3(1,-1,0);
		pAddr[3].setUV(1,1);

		pAddr[4].position=D3DXVECTOR3(-1,1,0);
		pAddr[4].setUV(0,0);

		pAddr[5].position=D3DXVECTOR3(1,1,0);
		pAddr[5].setUV(1,0);
		/*
		//우면
		pAddr[6].position=D3DXVECTOR3(1,1,-1);
		pAddr[6].setUV(0,0);

		pAddr[7].position=D3DXVECTOR3(1,1,1);
		pAddr[7].setUV(1,0);

		pAddr[8].position=D3DXVECTOR3(1,-1,-1);
		pAddr[8].setUV(0,1);

		pAddr[9].position=D3DXVECTOR3(1,-1,-1);
		pAddr[9].setUV(0,1);

		pAddr[10].position=D3DXVECTOR3(1,1,1);
		pAddr[10].setUV(1,0);

		pAddr[11].position=D3DXVECTOR3(1,-1,1);
		pAddr[11].setUV(1,1);

		//하면
		pAddr[12].position=D3DXVECTOR3(1,-1,1);
		pAddr[12].setUV(0,0);

		pAddr[13].position=D3DXVECTOR3(-1,-1,1);
		pAddr[13].setUV(1,0);

		pAddr[14].position=D3DXVECTOR3(1,-1,-1);
		pAddr[14].setUV(0,1);

		pAddr[15].position=D3DXVECTOR3(1,-1,-1);
		pAddr[15].setUV(0,1);

		pAddr[16].position=D3DXVECTOR3(-1,-1,1);
		pAddr[16].setUV(1,0);

		pAddr[17].position=D3DXVECTOR3(-1,-1,-1);
		pAddr[17].setUV(1,1);

		//좌면
		pAddr[18].position=D3DXVECTOR3(-1,-1,-1);
		pAddr[18].setUV(1,1);

		pAddr[19].position=D3DXVECTOR3(-1,-1,1);
		pAddr[19].setUV(0,1);

		pAddr[20].position=D3DXVECTOR3(-1,1,-1);
		pAddr[20].setUV(1,0);

		pAddr[21].position=D3DXVECTOR3(-1,1,-1);
		pAddr[21].setUV(1,0);

		pAddr[22].position=D3DXVECTOR3(-1,-1,1);
		pAddr[22].setUV(0,1);

		pAddr[23].position=D3DXVECTOR3(-1,1,1);
		pAddr[23].setUV(0,0);


		//뒤면
		pAddr[24].position=D3DXVECTOR3(-1,1,1);
		pAddr[24].setUV(1,0);

		pAddr[25].position=D3DXVECTOR3(-1,-1,1);
		pAddr[25].setUV(1,1);

		pAddr[26].position=D3DXVECTOR3(1,1,1);
		pAddr[26].setUV(0,0);

		pAddr[27].position=D3DXVECTOR3(1,1,1);
		pAddr[27].setUV(0,0);

		pAddr[28].position=D3DXVECTOR3(-1,-1,1);
		pAddr[28].setUV(1,1);

		pAddr[29].position=D3DXVECTOR3(1,-1,1);
		pAddr[29].setUV(0,1);

		//윗면
		pAddr[30].position=D3DXVECTOR3(1,1,1);
		pAddr[30].setUV(1,0);

		pAddr[31].position=D3DXVECTOR3(1,1,-1);
		pAddr[31].setUV(1,1);

		pAddr[32].position=D3DXVECTOR3(-1,1,1);
		pAddr[32].setUV(0,0);

		pAddr[33].position=D3DXVECTOR3(-1,1,1);
		pAddr[33].setUV(0,0);

		pAddr[34].position=D3DXVECTOR3(1,1,-1);
		pAddr[34].setUV(1,1);

		pAddr[35].position=D3DXVECTOR3(-1,1,-1);
		pAddr[35].setUV(0,1);		
		*/
		pVB->Unlock();

		return pVB;
	}

	void RenderFPS()
	{

		if(gSprite) 
		{
			if(SUCCEEDED(gSprite->Begin(D3DXSPRITE_ALPHABLEND|D3DXSPRITE_SORT_TEXTURE)))
			{
				std::string fps = "fps : ";
				//TCHAR buf[10];

				fps.append(boost::lexical_cast<std::string>(GetTimeEllapse()));

				//fps.append(buf);

				RECT rtPos = { 10, 10, 0, 0 };
				gFont->DrawText( gSprite, fps.c_str() , -1, &rtPos, DT_NOCLIP, D3DCOLOR_XRGB(255,255,255));
				gSprite->End();
			}

		}

	}

}












//Interfaces
namespace d3d
{


	LPDIRECT3DTEXTURE9 AAR3DTestTexture()
	{
		if(pTXT) return pTXT;

		if( FAILED( D3DXCreateTextureFromFile( gpDevice, "rose.png", &pTXT) ) ) return NULL;

		return pTXT;
	}

	HRESULT AAR3DInitSystemOnce()
	{
		if(bInitedOnce) return E_FAIL;

		//데스크탑 크기 받아옴.
		AAR3DMakeDesktopSize();


		bInitedOnce = TRUE;
		return S_OK;
	}


	HRESULT AAR3DInitD3D(HWND hWnd)
	{

		AAR3DInitSystemOnce();
		// Create the D3D object.
		if( NULL == ( gpD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
			return E_FAIL;

		// Set up the structure used to create the D3DDevice. Since we are now
		// using more complex geometry, we will create a device with a zbuffer.
		D3DPRESENT_PARAMETERS d3dpp;
		ZeroMemory( &d3dpp, sizeof( d3dpp ) );
		d3dpp.Windowed = TRUE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
		d3dpp.EnableAutoDepthStencil = TRUE;
		d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

		// Create the D3DDevice
		if( FAILED( gpD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			&d3dpp, &gpDevice ) ) )
		{
			return E_FAIL;
		}

		// Turn off culling
		gpDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

		// Turn off D3D lighting
		gpDevice->SetRenderState( D3DRS_LIGHTING, FALSE );

		// Turn on the zbuffer
		gpDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

		D3DCAPS9 caps;
		gpDevice->GetDeviceCaps(&caps);
		DWORD anisotropicLv = caps.MaxAnisotropy;

		gpDevice->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_ANISOTROPIC);
		gpDevice->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_ANISOTROPIC);
		gpDevice->SetSamplerState(0,D3DSAMP_MAXANISOTROPY,anisotropicLv/*이부분이 배율.. 현재는 그래픽카드에서 받아온값*/);


		QueryPerformanceFrequency(&cpufrq);
		QueryPerformanceCounter(&LastTime);

		D3DXCreateFont( gpDevice, 20, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, 
			OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
			"굴림체", &gFont );		

		D3DXCreateSprite( gpDevice, &gSprite );

		return S_OK;
	}


	void AAR3DReleaseSystem()
	{
		SAFE_RELEASE(pTXT);
		SAFE_RELEASE(pVB);
		SAFE_RELEASE(gFont);
		SAFE_RELEASE(gSprite);
		SAFE_RELEASE(gpDevice);
		SAFE_RELEASE(gpD3D);
	}

	void AAR3DTotalProcess(char* cameradata, int pixelx, int pixely, int size)
	{
		//std::cout << cameradata << "is cameradata" << std::endl;
		LPDIRECT3DTEXTURE9 backtxt;

		if(
			//0 && //debugging code
			//int(cameradata) < 30 && 
			
			SUCCEEDED(D3DXCreateTexture(gpDevice, pixelx, pixely, 
			0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,&backtxt)))
		{
			//std::cout << "camera data pointer is " << cameradata << " and size : " << size << std::endl;
			D3DLOCKED_RECT lr;
			if(SUCCEEDED(backtxt->LockRect(0,&lr,NULL,D3DLOCK_DISCARD)))
			{ 
				int* ti = (int*)(lr.pBits);
				char* tc = (char*)(lr.pBits);
//#pragma omp parallel for
				int i = 0;
				int minimal = 0;
				for( ; i < pixelx*pixely*3 ; i+=3, ++minimal )
				{				
					char a = char(255);
					char b = cameradata[i];
					char g = cameradata[i+1];
					char r = cameradata[i+2];
					ti[minimal] = D3DCOLOR_ARGB(a,r,g,b);
				}
				backtxt->UnlockRect(0);
				gpDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 0, 0 ), 1.0f, 0 );
				gSprite->Begin(D3DXSPRITE_ALPHABLEND|D3DXSPRITE_SORT_TEXTURE);
				//RECT SrcRect;
				//SetRect(&SrcRect, 0, 0,pixelx,pixely);
				gSprite->Draw(backtxt,NULL,NULL,&D3DXVECTOR3(0,0,1),D3DCOLOR_ARGB(255,255,255,255));
				gSprite->End();
				backtxt->Release();
			}
			else
				gpDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 0, 255 ), 1.0f, 0 );
		}
		else
			gpDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 0, 255 ), 1.0f, 0 );
		

		D3DXMATRIXA16 matWorld;
		D3DXMatrixScaling(&matWorld,-30.0f,30.0f,30.0f);
		gpDevice->SetTransform(D3DTS_WORLD, &matWorld);

		HWND hSrc = ::FindWindow("IEFrame", NULL);
		//HWND hSrc = ::FindWindow(NULL, "NateOn");
		HDC hSrcDC = GetDC(hSrc);
		HDC hdc = GetDC(NULL);

		RECT targetRect;
		GetClientRect(hSrc,&targetRect);

		int width = targetRect.right;
		int height = targetRect.bottom;

		//LPDIRECT3DTEXTURE9 ptxt = AAR3DTestTexture();
		LPDIRECT3DTEXTURE9 ptxt = NULL;
		if( SUCCEEDED(D3DXCreateTexture(gpDevice,targetRect.right, targetRect.bottom, 
			0, 0, D3DFMT_A8B8G8R8, D3DPOOL_MANAGED,&ptxt)))
		{
			D3DLOCKED_RECT lr;
			HRESULT hr = ptxt->LockRect(0,&lr,NULL,D3DLOCK_DISCARD);


			//	BITMAPINFO bmi;
			//	ZeroMemory(&bmi,sizeof(BITMAPINFO));
			//	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			//	GetDIBits(hdc, hBitmap, 0, 1, NULL, &bmi,DIB_RGB_COLORS);
			//	DWORD d = GetLastError();
			HDC memdc = CreateCompatibleDC(hdc);

			int* t;

			BITMAPINFO bmii;
			ZeroMemory(&bmii,sizeof(bmii));
			bmii.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmii.bmiHeader.biBitCount = 32;
			bmii.bmiHeader.biWidth = width;
			bmii.bmiHeader.biHeight = height;
			bmii.bmiHeader.biPlanes = 1;



			HBITMAP hBitMapUsing = CreateDIBSection(hdc,&bmii,DIB_RGB_COLORS,(void**)&t,NULL,0);

			//BitBlt(memdc,0,0,width,height,hdc,0,0,SRCCOPY);

			HDC dcTarget = CreateCompatibleDC(hdc);
			SelectObject(dcTarget,hBitMapUsing);
			RECT rt;
			GetClientRect(NULL,&rt);
			//			BitBlt(dcTarget,0,0,width,height,hSrcDC,0,0,SRCCOPY);
			::PrintWindow(hSrc, dcTarget, 0);
			//		memcpy(temp,t,width*height*4);
			
			//1 가로세로 뒤집힘
			//memcpy(lr.pBits,t,sizeof(int)*width*height);

//#pragma omp parallel for
			//2 세로가 뒤집힘
			for(int i = 0 ; i < width*height ; ++i)
				((int*)lr.pBits)[width*height-i-1] = t[i];

			
			
			//for(int y = 0 ; y < height ; ++y)
			//{
			//	for(int x = 0 ; x < width; ++x)
			//	{
					
					//((int*)lr.pBits)[width*height-i-1] = t[i];
					//((int*)lr.pBits)[x*y] = t[width*height-x*y-1];
			//	}
			//}
			
			/*
			for(int i = 0 ; i < width*height ; ++i)
			{
				((int*)lr.pBits)[width*height-i-1] = t[i];
			}
			*/
			//			memcpy(lr.pBits,t,width*height*4);

			//  			for(int x = 0 ; x < width ; ++x)
			//  			{
			//  				for(int y = 0 ; y < height ; ++y)
			//  				{
			//  					int pos = x*targetRect.right + y;
			//  					COLORREF color = GetPixel(myDC,y,x);
			//  					int a = color >> 24;
			//  					color = color << 8;
			//  					a = a << 24;
			//  					color |= a;
			//  					((COLORREF*)lr.pBits)[pos] = color;
			//  				}
			//  			}

			ptxt->UnlockRect(0);
			DeleteObject(hBitMapUsing);
			DeleteDC(dcTarget);
			DeleteDC(memdc);
		}
		DeleteDC(hdc);






		// Begin the scene
		if( SUCCEEDED( gpDevice->BeginScene() ) )
		{
			// Setup the world, view, and projection matrices
			//			SetupMatrices();
			cam3D::UpdateView();
			gpDevice->SetTexture( 0, ptxt );
			gpDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
			gpDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
			gpDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
			gpDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

			//gpDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

			// Render the vertex buffer contents
			gpDevice->SetStreamSource( 0, AAR3DGetModel(), 0, sizeof( CUSTOMVERTEX ) );
			gpDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
			
			gpDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2 );	//triangle 12

			// End the scene
			gpDevice->EndScene();
		}

		RenderFPS();

		// Present the backbuffer contents to the display
		gpDevice->Present( NULL, NULL, NULL, NULL );

		SAFE_RELEASE(ptxt);




	}

	double GetTimeEllapse()
	{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);

		LARGE_INTEGER differ;
		differ.QuadPart = now.QuadPart - LastTime.QuadPart;



		double ms = ((double)(differ.QuadPart)/double(cpufrq.QuadPart/1000));
		double fps = 1000/ms;

		LastTime.QuadPart = now.QuadPart;

//		std::cout << fps << std::endl;
		return fps;
	}









	





	//삼각형으로 피킹
	void PickWithTriangle(POINT ptClient, D3DXVECTOR3* triangles, int size)
	{
		//월드를 프로젝션으로 가져옴 (= 월드역, 뷰역)
		D3DXMATRIXA16 worldmatrix;
		D3DXMATRIXA16 viewmatrix;
		D3DXMATRIXA16 worldmatrix_inverse;
		D3DXMATRIXA16 viewmatrix_inverse;
		float detworld = D3DXMatrixDeterminant(&worldmatrix);
		float detview = D3DXMatrixDeterminant(&viewmatrix);
		GetDevice()->GetTransform(D3DTS_WORLD, &worldmatrix);
		GetDevice()->GetTransform(D3DTS_VIEW, &worldmatrix);
		D3DXMatrixInverse(&worldmatrix_inverse,&detworld,&worldmatrix);
		D3DXMatrixInverse(&viewmatrix_inverse,&detview,&viewmatrix);
		
		D3DXVECTOR3* vTransformed = new D3DXVECTOR3[size];
		for(int i = 0 ; i < size ; ++i)
		{
			D3DXVec3TransformCoord(&vTransformed[i],&triangles[i],&worldmatrix_inverse);
			D3DXVec3TransformCoord(&vTransformed[i],&triangles[i],&viewmatrix_inverse);
		}
		
		D3DXVECTOR2 pick_pos = D3DXVECTOR2(ptClient.x - 320 , 240 - ptClient.y);
		pick_pos.x /= 320;
		pick_pos.y /= 240;
		D3DXVECTOR3 RayBegin = D3DXVECTOR3(pick_pos.x,pick_pos.y,0);
		D3DXVECTOR3 RayDir = D3DXVECTOR3(pick_pos.x,pick_pos.y,1);
		//정규화공간상에서의 Ray는 x,y는 같고 z만 다름

		for(int i = 0 ; i < size ; ++i)
		{
			std::cout << "original pos " << triangles[i].x <<","<< triangles[i].y <<","<< triangles[i].z <<"/"<<
				"transformed pos " << vTransformed[i].x <<","<< vTransformed[i].y <<","<< vTransformed[i].z <<std::endl;
		}

		//projection 공간으로의 트랜스폼된 vTransformed
		for(int i = 0 ; i < size ; i+=3 )
		{
			float u, v, dist;
			BOOL picked = D3DXIntersectTri(
				&vTransformed[i]
				,&vTransformed[i+1]
				,&vTransformed[i+2]
				,&RayBegin
				,&RayDir
				,&u
				,&v
				,&dist
			);
			if(picked)
				std::cout << "picked !!!" << u << "/" << v <<std::endl;
		}
		
	}








	//Pick Process
	struct Ray
	{
		D3DXVECTOR3 _origin;
		D3DXVECTOR3 _direction;
	};

	Ray CalcPickingRay(int x, int y)
	{
		float px = 0.0f;
		float py = 0.0f;

		D3DVIEWPORT9 vp;
		d3d::GetDevice()->GetViewport(&vp);

		D3DXMATRIXA16 proj;
		d3d::GetDevice()->GetTransform(D3DTS_PROJECTION, &proj);

		px = ((( 2.0f*x) / vp.Width)  - 1.0f - 0) / proj(0, 0);
		py = (((-2.0f*y) / vp.Height) + 1.0f) / proj(1, 1);

		Ray ray;
		ray._origin    = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		ray._direction = D3DXVECTOR3(px, py, 1.0f);

		return ray;
	}


	void TransformRay(Ray* ray, D3DXMATRIXA16* T)
	{
		// transform the ray's origin, w = 1.
		D3DXVec3TransformCoord(
			&ray->_origin,
			&ray->_origin,
			T);

		// transform the ray's direction, w = 0.
		D3DXVec3TransformNormal(
			&ray->_direction,
			&ray->_direction,
			T);

		// normalize the direction
		D3DXVec3Normalize(&ray->_direction, &ray->_direction);
	}

	D3DXVECTOR3 Picking(POINT MousePosClient)
	{
		D3DXVECTOR3 Picked;
		Ray ray = CalcPickingRay(MousePosClient.x, MousePosClient.y);

		// transform the ray to world space
		D3DXMATRIXA16 view;
		d3d::GetDevice()->GetTransform(D3DTS_VIEW, &view);

		D3DXMATRIXA16 viewInverse;
		D3DXMatrixInverse(&viewInverse,	0, &view);

		TransformRay(&ray, &viewInverse);

		float temp = (-1 * ray._origin.z ) / ray._direction.z;

		Picked.z = 0.0f;
		Picked.x = temp * ray._direction.x + ray._origin.x;
		Picked.y = temp * ray._direction.y + ray._origin.y;

		std::cout << "picked pos is " << Picked.x << "/" << Picked.y << std::endl;
		
		
		return Picked;
	}
}