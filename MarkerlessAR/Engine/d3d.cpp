#include "wonjo.h"

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

		if( FAILED( gpDevice->CreateVertexBuffer( 36 * sizeof( CUSTOMVERTEX ), 
			0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &pVB, NULL ) ) )
			return NULL;


		CUSTOMVERTEX* pAddr;
		if(FAILED(pVB->Lock(0,0,(void**)&pAddr,0))) return NULL;

		for(int i = 0 ; i < 36 ; ++i)
			pAddr[i].color=0xffffffff;

		//앞면
		pAddr[0].position=D3DXVECTOR3(-1,-1,-1);
		pAddr[0].setUV(0,1);

		pAddr[1].position=D3DXVECTOR3(-1,1,-1);
		pAddr[1].setUV(0,0);

		pAddr[2].position=D3DXVECTOR3(1,-1,-1);
		pAddr[2].setUV(1,1);

		pAddr[3].position=D3DXVECTOR3(1,-1,-1);
		pAddr[3].setUV(1,1);

		pAddr[4].position=D3DXVECTOR3(-1,1,-1);
		pAddr[4].setUV(0,0);

		pAddr[5].position=D3DXVECTOR3(1,1,-1);
		pAddr[5].setUV(1,0);

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

		pVB->Unlock();

		return pVB;
	}

	void RenderFPS()
	{

		if(gSprite) 
		{
			if(SUCCEEDED(gSprite->Begin(D3DXSPRITE_ALPHABLEND|D3DXSPRITE_SORT_TEXTURE)))
			{
				std::wstring fps = _T("fps : ");
				//TCHAR buf[10];

				fps.append(boost::lexical_cast<std::wstring>(GetTimeEllapse()));

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

		if( FAILED( D3DXCreateTextureFromFile( gpDevice, _T("rose.png"), &pTXT) ) ) return NULL;

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
			_T("굴림체"), &gFont );		

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

	void AAR3DTotalProcess()
	{
		gpDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 0, 255 ), 1.0f, 0 );




		HWND hSrc = ::FindWindow(NULL, _T("NateOn"));
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
#pragma omp parallel for
			for(int i = 0 ; i < width*height ; ++i)
			{
				((int*)lr.pBits)[width*height-i-1] = t[i];
			}
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
			ReleaseDC(NULL,dcTarget);
			ReleaseDC(NULL,memdc);
		}
		ReleaseDC(hSrc,hdc);






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
			// Render the vertex buffer contents
			gpDevice->SetStreamSource( 0, AAR3DGetModel(), 0, sizeof( CUSTOMVERTEX ) );
			gpDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
			gpDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 12 );	//triangle 12

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

		std::cout << fps << std::endl;
		return fps;
	}
}