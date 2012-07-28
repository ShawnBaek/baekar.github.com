#include "wonjo.h"
#include "omp.h"


//#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 )


//Inner Utilities
namespace wonjo_dx
{



	Mesh::Mesh()
	{
		m_AdjBuffer = NULL;
		m_Mesh = NULL;
		m_MtrlBuffer = NULL;
		m_NumMtrl = 0;
	}
	Mesh::~Mesh()
	{
		SAFE_RELEASE(m_AdjBuffer);
		SAFE_RELEASE(m_MtrlBuffer);
		SAFE_RELEASE(m_Mesh);
		while(m_Tex.size())
		{
			SAFE_RELEASE(m_Tex.back());
			m_Tex.pop_back();
		}
	}

	//ResourceManager (Mesh only) : not Textures
	stdext::hash_map<std::string,Mesh*> hsMeshes;
	LPMESH LoadMeshFromFile(LPCSTR location)
	{
		stdext::hash_map<std::string,Mesh*>::iterator itr = hsMeshes.find(location);
		if(itr == hsMeshes.end())
		{
			LPMESH ms = new Mesh;
			HRESULT hr = D3DXLoadMeshFromX(location, D3DXMESH_MANAGED, GetDevice(), 
				&ms->m_AdjBuffer, &ms->m_MtrlBuffer, NULL, &ms->m_NumMtrl, &ms->m_Mesh
				);
			if(FAILED(hr)) 
			{
				delete ms;
				return NULL;
			}
			ms->filename.assign(location);
			std::string FileName = ms->filename;
			
			//load materials & textures
			if( ms->m_MtrlBuffer != NULL && ms->m_NumMtrl != NULL )
			{
				D3DXMATERIAL* mtrls = (D3DXMATERIAL*)ms->m_MtrlBuffer->GetBufferPointer();

				for( int i = 0; i < (signed)ms->m_NumMtrl; i++ )
				{
					mtrls[i].MatD3D.Ambient = mtrls[i].MatD3D.Diffuse;
					ms->Mtrls.push_back(mtrls[i].MatD3D);

					if(mtrls[i].pTextureFilename != NULL)
					{
						LPDIRECT3DTEXTURE9 tex = NULL;
						////
						////
						//LPWSTR lpsz = MultiByteToWideChar(mtrls[i].pTextureFilename);

// 						TCHAR buffer[BUFSIZ];
// 						ZeroMemory(buffer,sizeof(buffer));
// 						MultiByteToWideChar(CP_ACP,MB_COMPOSITE,mtrls[i].pTextureFilename,-1,buffer,BUFSIZ);

						//확장자 떼기
						std::string::iterator fitr;
						while( FileName.size() && FileName[FileName.size()-1] != '\\' )
						{
							fitr = FileName.end();
							--fitr;
							FileName.erase(fitr);
						}

						std::string fullpath = FileName;
						fullpath.append(mtrls[i].pTextureFilename);
						if(FAILED(D3DXCreateTextureFromFile( GetDevice(), fullpath.c_str() , &tex )))
						{
							MessageBox(NULL, "텍스쳐 생성 실패", "에러메시지", NULL );
							//					return false;
							return NULL;
						}
						ms->m_Tex.push_back(tex);
					}
					else
					{
						ms->m_Tex.push_back(0);
					}
				}
			}//end of load materials & textures

			hsMeshes[location] = ms;
			return ms;
		}
		else	//cache hits!
			return itr->second;
	}
	void ReleaseMeshes()
	{
		for(stdext::hash_map<std::string,Mesh*>::iterator itr = hsMeshes.begin() ; itr != hsMeshes.end() ; ++itr)
		{
			delete itr->second;
		}
		hsMeshes.clear();
	}

	static D3DXMATRIXA16 rtv_matScale;
	static D3DXMATRIXA16 rtv_matTrans;
	static D3DXMATRIXA16 rtv_matRot;
	const D3DXMATRIXA16* MakeScaleMatrix(float x, float y, float z)
	{
		D3DXMatrixScaling(&rtv_matScale,x,y,z);
		return &rtv_matScale;
	}

	const D3DXMATRIXA16* MakeTranslationMatrix(float x, float y, float z)
	{
		D3DXMatrixTranslation(&rtv_matTrans,x,y,z);
		return &rtv_matTrans;
	}

	const D3DXMATRIXA16* MakeRotationMatrix(float x, float y, float z)
	{
		D3DXMatrixRotationYawPitchRoll(&rtv_matRot,y,x,z);
		return &rtv_matRot;
	}

	void DrawMesh(LPMESH pms, const D3DXMATRIXA16* matWorld)
	{
		if(!pms) return;
		
		D3DXMATRIXA16 matWorld_backup;
		//GetDevice()->GetTransform(D3DTS_WORLD, &matWorld_backup);
		if(matWorld) GetDevice()->SetTransform(D3DTS_WORLD, matWorld);

		GetDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		GetDevice()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		GetDevice()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		
		GetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);   //디폴트값 = 텍스쳐
		GetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);    // 1과 2에 각각 텍스쳐와 디퓨즈를 넣고
		GetDevice()->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE); // 2개를 모듈레이트 한다 (합성)

		for(int i = 0; i < (signed)pms->Mtrls.size(); i++)
		{
			//////////////////////////////////////////////////////////////////////////
			//		m_Effector->SetUpShader(matWorld, res->m_Tex[i]);	//120109카툰쉐이더rollback
			//////////////////////////////////////////////////////////////////////////
			//mtrls[i].MatD3D.Diffuse.a = 0.4f;
			pms->Mtrls[i].Ambient.a = pms->Mtrls[i].Diffuse.a = 0.95f;
			GetDevice()->SetMaterial( &pms->Mtrls[i] );
			GetDevice()->SetTexture( 0, pms->m_Tex[i] );
			//////////////////////////////////////////////////////////////////////////
			//deprecated source
			//m_Mesh->DrawSubset(i);
			//new source
			pms->m_Mesh->DrawSubset( i );
			//////////////////////////////////////////////////////////////////////////
		}

		//GetDevice()->SetTransform(D3DTS_WORLD, &matWorld_backup);
	}

	void DrawPlane(const D3DXMATRIXA16* matWorld /* = NULL */)
	{
		if(matWorld) GetDevice()->SetTransform(D3DTS_WORLD, matWorld);
		D3DMATERIAL9 mtrl;
		mtrl.Ambient  = D3DXCOLOR( 1.0f, 1.0f, 1.0f, 0);
		mtrl.Diffuse  = D3DXCOLOR( 0.0f, 0.0f, 0.0f, 1.0f);
		mtrl.Specular = D3DXCOLOR( 1.0f, 1.0f, 1.0f, 0);
		mtrl.Emissive = D3DXCOLOR( 1.0f, 1.0f, 1.0f, 0);
		mtrl.Power    = 0.0f;
		gpDevice->SetMaterial(&mtrl);
		//gpDevice->SetTexture( 0, ptxt );
		gpDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
		gpDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		gpDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		gpDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

		//gpDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

		// Render the vertex buffer contents
		gpDevice->SetStreamSource( 0, AAR3DGetModel(), 0, sizeof( CUSTOMVERTEX ) );
		gpDevice->SetFVF( D3DFVF_CUSTOMVERTEX );

		gpDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2 );	//triangle 12	
	}







	LPDIRECT3D9 gpD3D = NULL;
	LPDIRECT3DDEVICE9 gpDevice = NULL;
	LPDIRECT3DTEXTURE9  gpTextureBackground = NULL;
	LPDIRECT3DTEXTURE9  gpTextureBrowser = NULL;

	static int nScreenX, nScreenY;
	static BOOL bInitedOnce = FALSE;
	static LPDIRECT3DVERTEXBUFFER9 pVB = NULL;
	static LPDIRECT3DTEXTURE9 pTXT = NULL;
	static LARGE_INTEGER cpufrq;
	static LARGE_INTEGER LastTime;
	static LPD3DXFONT gFont = NULL;
	static LPD3DXSPRITE gSprite = NULL;
	
	

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
			//pAddr[i].color=0xffffffff;
			pAddr[i].normal = D3DXVECTOR3(0,1,0);

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
//remove boost function
// 	void RenderFPS()
// 	{
// 
// 		if(gSprite) 
// 		{
// 			if(SUCCEEDED(gSprite->Begin(D3DXSPRITE_ALPHABLEND|D3DXSPRITE_SORT_TEXTURE)))
// 			{
// 				std::string fps = "fps : ";
// 				//TCHAR buf[10];
// 
// 				fps.append(boost::lexical_cast<std::string>(GetTimeEllapse()));
// 
// 				//fps.append(buf);
// 
// 				RECT rtPos = { 10, 10, 0, 0 };
// 				gFont->DrawText( gSprite, fps.c_str() , -1, &rtPos, DT_NOCLIP, D3DCOLOR_XRGB(255,255,255));
// 				gSprite->End();
// 			}
// 
// 		}
// 
// 	}

}












//Interfaces
namespace wonjo_dx
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



		//full-screen anti aliasing check
// 		if( SUCCEEDED(gpDevice->CheckDeviceMultiSampleType( D3DADAPTER_DEFAULT, 
// 			D3DDEVTYPE_HAL , D3DFMT_R8G8B8, FALSE, 
// 			D3DMULTISAMPLE_2_SAMPLES, NULL ) ) )
// 		{
// 			d3dpp.MultiSampleType = D3DMULTISAMPLE_2_SAMPLES;
// 		}

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
		//gpDevice->SetRenderState( D3DRS_LIGHTING, TRUE );
		D3DXCOLOR   c(1.0f, 1.0f, 1.0f, 1.0f);
		D3DXVECTOR3 m_vLight = D3DXVECTOR3(1.0f, -1.0f, 0.0f);
		D3DLIGHT9 m_Light;
		::ZeroMemory(&m_Light, sizeof(m_Light));
		m_Light.Type      = D3DLIGHT_DIRECTIONAL;
		m_Light.Ambient   = c * 1.0f;
		m_Light.Diffuse   = c;
		m_Light.Specular  = c * 1.0f;
		m_Light.Direction = m_vLight;
		m_Light.Range = 10000.0f;

		gpDevice->SetLight(0, &m_Light);
		gpDevice->LightEnable(0, true);

		gpDevice->SetRenderState(D3DRS_NORMALIZENORMALS, true);
		gpDevice->SetRenderState(D3DRS_SPECULARENABLE, false);


		gpDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);//왼손좌표계니까 반시계방향 컬링
		gpDevice->SetRenderState(D3DRS_ZENABLE, TRUE);//z버퍼 켜기

		//알파채널 켜기
		gpDevice->SetRenderState( D3DRS_ALPHABLENDENABLE,   TRUE );
		gpDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
		gpDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
		gpDevice->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
		gpDevice->SetRenderState( D3DRS_ALPHAREF,        0x08 );
		gpDevice->SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );

		// Turn on the zbuffer
		gpDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

		D3DCAPS9 caps;
		gpDevice->GetDeviceCaps(&caps);
		DWORD anisotropicLv = caps.MaxAnisotropy;

		gpDevice->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_ANISOTROPIC);
		gpDevice->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_ANISOTROPIC);
		gpDevice->SetSamplerState(0,D3DSAMP_MAXANISOTROPY,anisotropicLv/*이부분이 배율.. 현재는 그래픽카드에서 받아온값*/);

		gpDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		gpDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		gpDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		gpDevice->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);

		QueryPerformanceFrequency(&cpufrq);
		QueryPerformanceCounter(&LastTime);

		D3DXCreateFont( gpDevice, 20, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, 
			OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
			"굴림체", &gFont );		

		D3DXCreateSprite( gpDevice, &gSprite );

		//백그라운드 텍스쳐 생성
		RECT rt;
		GetWindowRect(hWnd,&rt);
		D3DXCreateTexture(gpDevice, rt.right-rt.left, rt.bottom-rt.top, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,&gpTextureBackground);
		//D3DXCreateTexture(gpDevice, 640, 480, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,&gpTextureBackground);
		
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
		SAFE_RELEASE(gpTextureBackground);
	}


	void AAR3DDrawCameraPreview(char* cameradata, int pixelx, int pixely)
	{
		//gpDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 0, 0 ), 1.0f, 0 );
		
		D3DLOCKED_RECT lr;
		if(SUCCEEDED(gpTextureBackground->LockRect(0,&lr,NULL,D3DLOCK_DISCARD)))
		{
			int* ti = (int*)(lr.pBits);
			int i = 0;
			int minimal = 640*480-1;
			for( ; i < pixelx*pixely*3 ; i+=3, --minimal )
			{				
				char a = char(255);
				char b = cameradata[i];
				char g = cameradata[i+1];
				char r = cameradata[i+2];
				ti[minimal] = D3DCOLOR_ARGB(a,r,g,b);
			}
			gpTextureBackground->UnlockRect(0);
			gSprite->Begin(D3DXSPRITE_ALPHABLEND);
			gSprite->Draw(gpTextureBackground,NULL,NULL,&D3DXVECTOR3(0,0,1),D3DCOLOR_ARGB(255,255,255,255));
			gSprite->End();
		}
	}

// 	void AAR3DDrawMesh(LPCSTR DestClassName, LPCSTR DestWindowName, float size)
// 	{
// 		D3DXMATRIXA16 matWorld;
// 		D3DXMatrixScaling(&matWorld,size,size,size);
// 		gpDevice->SetTransform(D3DTS_WORLD, &matWorld);
// 
// 		HWND hSrc = ::FindWindow(DestClassName, DestWindowName);
// 		HDC hSrcDC = GetDC(hSrc);
// 		HDC hdc = GetDC(NULL);
// 
// 		RECT targetRect;
// 		GetClientRect(hSrc,&targetRect);
// 
// 		int width = targetRect.right;
// 		int height = targetRect.bottom;
// 
// 		//LPDIRECT3DTEXTURE9 ptxt = AAR3DTestTexture();
// 		LPDIRECT3DTEXTURE9 ptxt = NULL;
// 		if( SUCCEEDED(D3DXCreateTexture(gpDevice,targetRect.right, targetRect.bottom, 
// 			0, 0, D3DFMT_A8B8G8R8, D3DPOOL_MANAGED,&ptxt)))
// 		{
// 			D3DLOCKED_RECT lr;
// 			HRESULT hr = ptxt->LockRect(0,&lr,NULL,D3DLOCK_DISCARD);
// 			
// 			//	BITMAPINFO bmi;
// 			//	ZeroMemory(&bmi,sizeof(BITMAPINFO));
// 			//	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
// 			//	GetDIBits(hdc, hBitmap, 0, 1, NULL, &bmi,DIB_RGB_COLORS);
// 			//	DWORD d = GetLastError();
// 			HDC memdc = CreateCompatibleDC(hdc);
// 
// 			int* t;
// 
// 			BITMAPINFO bmii;
// 			ZeroMemory(&bmii,sizeof(bmii));
// 			bmii.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
// 			bmii.bmiHeader.biBitCount = 32;
// 			bmii.bmiHeader.biWidth = width;
// 			bmii.bmiHeader.biHeight = height;
// 			bmii.bmiHeader.biPlanes = 1;
// 			
// 
// 			HBITMAP hBitMapUsing = CreateDIBSection(hdc,&bmii,DIB_RGB_COLORS,(void**)&t,NULL,0);
// 
// 			//BitBlt(memdc,0,0,width,height,hdc,0,0,SRCCOPY);
// 
// 			HDC dcTarget = CreateCompatibleDC(hdc);
// 			SelectObject(dcTarget,hBitMapUsing);
// 			RECT rt;
// 			GetClientRect(NULL,&rt);
// 			//			BitBlt(dcTarget,0,0,width,height,hSrcDC,0,0,SRCCOPY);
// 			::PrintWindow(hSrc, dcTarget, 0);
// 			//		memcpy(temp,t,width*height*4);
// 			
// 			//1 가로세로 뒤집힘
// 			//memcpy(lr.pBits,t,sizeof(int)*width*height);
// 
// //#pragma omp parallel for
// 			//2 세로가 뒤집힘
// 			for(int i = 0 ; i < width*height ; ++i)
// 				((int*)lr.pBits)[width*height-i-1] = t[i];
// 						
// 			//for(int y = 0 ; y < height ; ++y)
// 			//{
// 			//	for(int x = 0 ; x < width; ++x)
// 			//	{
// 					
// 					//((int*)lr.pBits)[width*height-i-1] = t[i];
// 					//((int*)lr.pBits)[x*y] = t[width*height-x*y-1];
// 			//	}
// 			//}
// 			
// 			/*
// 			for(int i = 0 ; i < width*height ; ++i)
// 			{
// 				((int*)lr.pBits)[width*height-i-1] = t[i];
// 			}
// 			*/
// 			//			memcpy(lr.pBits,t,width*height*4);
// 
// 			//  			for(int x = 0 ; x < width ; ++x)
// 			//  			{
// 			//  				for(int y = 0 ; y < height ; ++y)
// 			//  				{
// 			//  					int pos = x*targetRect.right + y;
// 			//  					COLORREF color = GetPixel(myDC,y,x);
// 			//  					int a = color >> 24;
// 			//  					color = color << 8;
// 			//  					a = a << 24;
// 			//  					color |= a;
// 			//  					((COLORREF*)lr.pBits)[pos] = color;
// 			//  				}
// 			//  			}
// 
// 			ptxt->UnlockRect(0);
// 			DeleteObject(hBitMapUsing);
// 			DeleteDC(dcTarget);
// 			DeleteDC(memdc);
// 		}
// 		DeleteDC(hdc);
// 
// 
// 
// 
// 		//좌우 뒤집어진 텍스쳐를 반전하기 위한 World Matrix
// 
// 
// 
// 		// Begin the scene
// 		//if( SUCCEEDED( gpDevice->BeginScene() ) )
// 		{
// 			// Setup the world, view, and projection matrices
// 			//			SetupMatrices();
// 			D3DMATERIAL9 mtrl;
// 			mtrl.Ambient  = D3DXCOLOR( 1.0f, 1.0f, 1.0f, 0);
// 			mtrl.Diffuse  = D3DXCOLOR( 0.0f, 0.0f, 0.0f, 1.0f);
// 			mtrl.Specular = D3DXCOLOR( 1.0f, 1.0f, 1.0f, 0);
// 			mtrl.Emissive = D3DXCOLOR( 1.0f, 1.0f, 1.0f, 0);
// 			mtrl.Power    = 0.0f;
// 			gpDevice->SetMaterial(&mtrl);
// 			gpDevice->SetTexture( 0, ptxt );
// 			gpDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
// 			gpDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
// 			gpDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
// 			gpDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
// 
// 			//gpDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
// 
// 			// Render the vertex buffer contents
// 			gpDevice->SetStreamSource( 0, AAR3DGetModel(), 0, sizeof( CUSTOMVERTEX ) );
// 			gpDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
// 			
// 			gpDevice->DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2 );	//triangle 12
// 
// 			//gpDevice->EndScene();
// 		}
// 
// 		//RenderFPS();
// 
// 		
// 		SAFE_RELEASE(ptxt);
// 	}

	HRESULT BeginRender()
	{
		gpDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB( 0, 0, 0 ), 1.0f, 0 );
		return gpDevice->BeginScene();
	}

	void Matrix_LH_RH_Swap(D3DXMATRIXA16* pOut, const D3DXMATRIXA16* pIn)
	{
		const D3DXMATRIXA16& rIn = *pIn;
		D3DXMATRIXA16& rOut = *pOut;

		for(int i = 0 ; i < 16 ; ++i)
		{
			if(i >= 8 && i <= 11)
				rOut[i] = -1*rIn[i];
			rOut[i]=rIn[i];
		}
	}

	void Flip()
	{
		// End the scene
		gpDevice->EndScene();

		// Present the backbuffer contents to the display
		gpDevice->Present( NULL, NULL, NULL, NULL );

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
		wonjo_dx::GetDevice()->GetViewport(&vp);

		D3DXMATRIXA16 proj;
		wonjo_dx::GetDevice()->GetTransform(D3DTS_PROJECTION, &proj);

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
		wonjo_dx::GetDevice()->GetTransform(D3DTS_VIEW, &view);

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