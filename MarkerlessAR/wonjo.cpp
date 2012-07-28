#include "wonjo.h"
<<<<<<< .mine
=======
//#include "d3dx9.h"
//#pragma comment(lib,"d3dx9.lib")
>>>>>>> .r52

//legacy opengl include
#include "gl/glut.h"
#pragma comment(lib,"glut32.lib")


namespace wonjo_dx
{
<<<<<<< .mine
=======
	static BOOL TextureCreated = FALSE;
	

	double matView[16];
	double matProj[16];
	D3DXMATRIXA16 ChangeD3DXMatrix(const double* pIn);
	double lastSize;
	D3DXVECTOR3 lastPoses[4];
>>>>>>> .r52
<<<<<<< .mine
	
	TexturingWith::TexturingWith(const char* className, const char* WindowName)
	{
		HWND hSrc = NULL;
		HDC hSrcDC = NULL;
		HDC hdc = NULL;
		HBITMAP hBitMapUsing = NULL;
		HDC dcTarget = NULL;
		HDC memdc = NULL;
		int width = 0;
		int height = 0;
		

		SetUp();
=======
>>>>>>> .r52

<<<<<<< .mine
		GetBitmapData(className, WindowName);
	}
	TexturingWith::~TexturingWith()
	{
		ReleaseBitmapData();
	}


	void TexturingWith::SetUp()
	{
	}

	void TexturingWith::GetBitmapData(const char* className, const char* WindowName)
	{
		hSrc = ::FindWindow(className, WindowName);
		hSrcDC = GetDC(hSrc);
		hdc = GetDC(NULL);

		RECT targetRect;
		GetClientRect(hSrc,&targetRect);

		width = targetRect.right;
		height = targetRect.bottom;
		memdc = CreateCompatibleDC(hdc);

		int* t;

		BITMAPINFO bmii;
		ZeroMemory(&bmii,sizeof(bmii));
		bmii.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmii.bmiHeader.biBitCount = 32;
		bmii.bmiHeader.biWidth = width;
		bmii.bmiHeader.biHeight = height;
		bmii.bmiHeader.biPlanes = 1;

		hBitMapUsing = CreateDIBSection(hdc,&bmii,DIB_RGB_COLORS,(void**)&t,NULL,0);

		dcTarget = CreateCompatibleDC(hdc);
		SelectObject(dcTarget,hBitMapUsing);
		RECT rt;
		GetClientRect(NULL,&rt);
		::PrintWindow(hSrc, dcTarget, 0);

		//gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width,	height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, t );


		DeleteDC(dcTarget);
		DeleteDC(memdc);
		ReleaseDC(NULL,hdc);
		ReleaseDC(hSrc,hSrcDC);
	}

	void TexturingWith::ReleaseBitmapData()
	{
		DeleteObject(hBitMapUsing);
		//
	}
=======
	//remove class TexturingWith
// 	GLuint TexturingWith::texture = 0;
// 	HWND TexturingWith::lastWnd = NULL;
// 	TexturingWith::TexturingWith(const char* className, const char* WindowName)
// 	{
// 		HWND hSrc = NULL;
// 		HDC hSrcDC = NULL;
// 		HDC hdc = NULL;
// 		HBITMAP hBitMapUsing = NULL;
// 		HDC dcTarget = NULL;
// 		HDC memdc = NULL;
// 		int width = 0;
// 		int height = 0;
// 		GLuint texture = 0;
// 		
// 		
// 		CreateTexture();
// 		SetUp();
// 
// 		GetBitmapData(className, WindowName);
// 	}
// 	TexturingWith::~TexturingWith()
// 	{
// 		ReleaseBitmapData();
// 	}
// 
// 
// 	void TexturingWith::SetUp()
// 	{
// 		glEnable( GL_TEXTURE_2D );
// 		glBindTexture( GL_TEXTURE_2D, texture );
// 	}
// 
// 	void TexturingWith::CreateTexture()
// 	{
// 		if(TextureCreated) return;
// 
// 		// allocate a texture name
// 		glGenTextures( 1, &texture );
// 
// 		// select our current texture
// 		glBindTexture( GL_TEXTURE_2D, texture );
// 
// 		// select modulate to mix texture with color for shading
// 		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
// 
// 		// when texture area is small, bilinear filter the closest MIP map
// 		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
// 			GL_LINEAR_MIPMAP_NEAREST );
// 		// when texture area is large, bilinear filter the first MIP map
// 		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
// 
// 		// if wrap is true, the texture wraps over at the edges (repeat)
// 		//       ... false, the texture ends at the edges (clamp)
// 		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
// 		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
// 
// 		// build our texture MIP maps
// 		//gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width, 	height, GL_RGB, GL_UNSIGNED_BYTE, data );
// 
// 		// free buffer
// 		//		free( data );
// 		//SetUp();
// 		// 
// 		TextureCreated = TRUE;
// 	}
// 
// 	void TexturingWith::ReleaseTexture()
// 	{
// 		if(!TextureCreated) return;
// 		glDeleteTextures(1, &texture);
// 		TextureCreated = FALSE;
// 	}
// 
// 	void TexturingWith::GetBitmapData(const char* className, const char* WindowName)
// 	{
// 		hSrc = ::FindWindow(className, WindowName);
// 		lastWnd = hSrc;
// 		hSrcDC = GetDC(hSrc);
// 		hdc = GetDC(NULL);
// 
// 		RECT targetRect;
// 		GetClientRect(hSrc,&targetRect);
// 
// 		width = targetRect.right;
// 		height = targetRect.bottom;
// 		memdc = CreateCompatibleDC(hdc);
// 
// 		int* t;
// 
// 		BITMAPINFO bmii;
// 		ZeroMemory(&bmii,sizeof(bmii));
// 		bmii.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
// 		bmii.bmiHeader.biBitCount = 32;
// 		bmii.bmiHeader.biWidth = width;
// 		bmii.bmiHeader.biHeight = height;
// 		bmii.bmiHeader.biPlanes = 1;
// 
// 		hBitMapUsing = CreateDIBSection(hdc,&bmii,DIB_RGB_COLORS,(void**)&t,NULL,0);
// 
// 		dcTarget = CreateCompatibleDC(hdc);
// 		SelectObject(dcTarget,hBitMapUsing);
// 		RECT rt;
// 		GetClientRect(NULL,&rt);
// 		::PrintWindow(hSrc, dcTarget, 0);
// 
// 		gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width,	height, GL_BGRA_EXT, GL_UNSIGNED_BYTE, t );
// 
// 
// 		DeleteDC(dcTarget);
// 		DeleteDC(memdc);
// 		ReleaseDC(NULL,hdc);
// 		ReleaseDC(hSrc,hSrcDC);
// 	}
// 
// 	void TexturingWith::ReleaseBitmapData()
// 	{
// 		DeleteObject(hBitMapUsing);
// 		//
// 	}
>>>>>>> .r52







<<<<<<< .mine
	
=======
		glEnd();
	}

	void Plane(double size)
	{
		wonjo_dx::lastSize = size;
		glBegin(GL_QUADS);
		glTexCoord2i(0, 0); glVertex3f(-1*size, -1*size, 0);
		glTexCoord2i(0, 1); glVertex3f(-1*size,  1*size, 0);
		glTexCoord2i(1, 1); glVertex3f( 1*size,  1*size, 0);
		glTexCoord2i(1, 0); glVertex3f( 1*size, -1*size, 0);
		glEnd();
	}

// 	void TexturingWith::Plane(double size)
// 	{
// 		wonjo_dx::lastSize = size;
// 		glBegin(GL_QUADS);
// 
// 		lastPoses[0] = D3DXVECTOR3(-1*size*((double)width/(double)height), -1*size*((double)height/(double)width), -0*size);
// 		glTexCoord2i(0, 0); glVertex3f(-1*size*((double)width/(double)height), -1*size*((double)height/(double)width), -0*size);
// 
// 		lastPoses[1] = D3DXVECTOR3(-1*size*((double)width/(double)height),  1*size*((double)height/(double)width), -0*size);
// 		glTexCoord2i(0, 1); glVertex3f(-1*size*((double)width/(double)height),  1*size*((double)height/(double)width), -0*size);
// 
// 		lastPoses[2] = D3DXVECTOR3(1*size*((double)width/(double)height),  1*size*((double)height/(double)width), -0*size);
// 		glTexCoord2i(1, 1); glVertex3f( 1*size*((double)width/(double)height),  1*size*((double)height/(double)width), -0*size);
// 
// 		lastPoses[3] = D3DXVECTOR3( 1*size*((double)width/(double)height), -1*size*((double)height/(double)width), -0*size);
// 		glTexCoord2i(1, 0); glVertex3f( 1*size*((double)width/(double)height), -1*size*((double)height/(double)width), -0*size);
// 
// 		glEnd();
// 	}

	void Plane(double size, double corners[4][3])
	{
		glBegin(GL_QUADS);
		glTexCoord2i(0, 0); glVertex3f(size*(corners[0][0]), size*(corners[0][1]), -0*size);
		glTexCoord2i(0, 1); glVertex3f(size*(corners[1][0]), size*(corners[1][1]), -0*size);
		glTexCoord2i(1, 1); glVertex3f(size*(corners[2][0]), size*(corners[2][1]), -0*size);
		glTexCoord2i(1, 0); glVertex3f(size*(corners[3][0]), size*(corners[3][1]), -0*size);
		glEnd();
	}
















	D3DXMATRIXA16 ChangeD3DXMatrix(const double* pIn)
	{
		D3DXMATRIXA16 rtv;
		rtv._11 = float(pIn[0]);
		rtv._12 = float(pIn[1]);
		rtv._13 = float(pIn[2]);
		rtv._14 = float(pIn[3]);
		rtv._21 = float(pIn[4]);
		rtv._22 = float(pIn[5]);
		rtv._23 = float(pIn[6]);
		rtv._24 = float(pIn[7]);
		rtv._31 = float(pIn[8]);
		rtv._32 = float(pIn[9]);
		rtv._33 = float(pIn[10]);
		rtv._34 = float(pIn[11]);
		rtv._41 = float(pIn[12]);
		rtv._42 = float(pIn[13]);
		rtv._43 = float(pIn[14]);
		rtv._44 = float(pIn[15]);
		return rtv;
	}

	double* ChangeLinearMatrix(double* pOut, const D3DXMATRIXA16* pIn)
	{
		pOut[0] = pIn->_11;
		pOut[1] = pIn->_12;
		pOut[2] = pIn->_13;
		pOut[3] = pIn->_14;
		pOut[4] = pIn->_21;
		pOut[5] = pIn->_22;
		pOut[6] = pIn->_23;
		pOut[7] = pIn->_24;
		pOut[8] = pIn->_31;
		pOut[9] = pIn->_32;
		pOut[10] = pIn->_33;
		pOut[11] = pIn->_34;
		pOut[12] = pIn->_41;
		pOut[13] = pIn->_42;
		pOut[14] = pIn->_43;
		pOut[15] = pIn->_44;
		return pOut;
	}

	double* MultiMatrix(double* pOut, const double* pFirst, const double* pSecond)
	{
		D3DXMATRIXA16 p1 = ChangeD3DXMatrix(pFirst);
		D3DXMATRIXA16 p2 = ChangeD3DXMatrix(pSecond);
		D3DXMATRIXA16 pOutMatrix = p1*p2;
		ChangeLinearMatrix(pOut,&pOutMatrix);
		return pOut;
	}

	double* InverseMatrix(double* pOut, const double* pIn)
	{
		D3DXMATRIXA16 matIn = ChangeD3DXMatrix(pIn);
		float det = D3DXMatrixDeterminant(&matIn);
		D3DXMATRIXA16 out;
		D3DXMatrixInverse(&out,&det,&matIn);
		ChangeLinearMatrix(pOut,&out);
		return pOut;
	}


	static int KeyTable[0xFF];
	static BOOL bInitedIOSystem = FALSE;
	void InitIO()
	{
		if(bInitedIOSystem) return;
		ZeroMemory(KeyTable,sizeof(KeyTable));
		bInitedIOSystem = TRUE;
	}

	
	//picking ProcessIO
	void ProcessIO()
	{
		if(bInitedIOSystem) InitIO();

		if( (GetKeyState(VK_LBUTTON) & 0x8000)
			&& (!KeyTable[VK_LBUTTON])
			) 
		{
			KeyTable[VK_LBUTTON] = TRUE;
		
			POINT pt;
			GetCursorPos(&pt);

			

			//영역을 벗어나면 취급하지 않음.
			RECT myArea;
			GetWindowRect(NULL,&myArea);
			std::cout << "pt is x" << pt.x <<"/y"<<pt.y << "\tand myArea left" << myArea.left << " right"  << myArea.right << " top" << myArea.top << " bottom" << myArea.bottom << std::endl;
			if(pt.x > myArea.right || pt.x < myArea.left
				|| pt.y < myArea.top || pt.y > myArea.bottom)
			{
				return;
			}
			
			//스크린내부의 영역으로 처리함.
			ScreenToClient(NULL,&pt);
			D3DXVECTOR3 vecpicked = Picking(pt);

			vecpicked/=GetScaleXYZ();
			//피킹됐는지 확인.
			if(fabs(vecpicked.x) <= 1.0f && fabs(vecpicked.y) <= 1.0f )
			{
				std::cout << "sendmessage with scale" << GetScaleXYZ() << " / pre_mousepos" << vecpicked.x << "/" << vecpicked.y << std::endl;

 				//-1 ~ 1 (뒤집힌 x)
 				vecpicked.x -= 1.0f;
				vecpicked.x *= -1;

				vecpicked.y -= 1.0f;
				vecpicked.y *= -1;

				vecpicked/=2.0f;
// 				
// 
// 				//1 ~ -1
// 				vecpicked.y -= 1.0f; //0~-2
// 				vecpicked.y *= -1;	//0~2
// 
 				std::cout << "sendmessage with mousepos (calculating...)" << vecpicked.x << "/" << vecpicked.y << std::endl;
// 
 				RECT rt;
 				GetWindowRect(GetTargetHWND(),&rt);
				
				std::cout << "Rectangle " << GetTargetHWND() <<" x " << rt.right - rt.left << " /y " << rt.bottom - rt.top << std::endl;
 				vecpicked.x *= (rt.right - rt.left);
 				vecpicked.y *= (rt.bottom - rt.top);
 				std::cout << "sendmessage with mousepos" << vecpicked.x << "/" << vecpicked.y << std::endl;


 				POINT pt;
 				GetCursorPos(&pt);
 				SetCursorPos(rt.left+vecpicked.x,rt.top+vecpicked.y);
 				std::cout << "final destination" << rt.left+vecpicked.x << " / " << rt.right + vecpicked.y << std::endl;
 				mouse_event(MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_ABSOLUTE	,rt.left+vecpicked.x , rt.top+vecpicked.y, 0, 0);
 				Sleep(150);
 				mouse_event(MOUSEEVENTF_LEFTUP|MOUSEEVENTF_ABSOLUTE		,rt.left+vecpicked.x , rt.top+vecpicked.y, 0, 0);
				mouse_event(MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_ABSOLUTE	,rt.left+vecpicked.x , rt.top+vecpicked.y, 0, 0);
				Sleep(150);
				mouse_event(MOUSEEVENTF_LEFTUP|MOUSEEVENTF_ABSOLUTE		,rt.left+vecpicked.x , rt.top+vecpicked.y, 0, 0);
 				SetCursorPos(pt.x,pt.y);
			}
			
		}
		else
		{
			KeyTable[VK_LBUTTON] = FALSE;
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


























// opengl picking function
// 
// 	void MouseEvent(int iButton, int iState, int x, int y)
// 	{
// 		if(iState == GLUT_DOWN)
// 		{
// 			if(iButton == GLUT_LEFT_BUTTON)
// 			{
// 				
// 				
// 				
// 				if(TexturingWith::lastWnd)
// 				{
// 					POINT p; 
// 					p.x = x;
// 					p.y = y;
// 					D3DXVECTOR3 vec = PickXZPlane(p);
// 
// 					vec/=30.0f;	//plane's size
// 					
// 					//만약 두개가 1.0f라면 픽됨
// 					if(fabs(vec.x) <= 1.0f 	&& fabs(vec.y) <= 1.0f  )
// 					{
// 						std::cout << "sendmessage with pre_mousepos" << vec.x << "/" << vec.y << std::endl;
// 
// 						//-1 ~ 1
// 						vec.x += 1.0f;
// 
// 						//1 ~ -1
// 						vec.y -= 1.0f; //0~-2
// 						vec.y *= -1;	//0~2
// 						
// 						RECT rt;
// 						GetWindowRect(TexturingWith::lastWnd,&rt);
// 						vec.x *= (rt.right - rt.left)/2;
// 						vec.y *= (rt.bottom - rt.top)/2;
// 						std::cout << "sendmessage with mousepos" << vec.x << "/" << vec.y << std::endl;
// 						//SendMessage(TexturingWith::lastWnd,WM_PARENTNOTIFY,WM_LBUTTONDOWN,MAKELPARAM(vec.x,vec.y));
// 						//SendMessage(TexturingWith::lastWnd,WM_MOUSEACTIVATE,WM_LBUTTONDOWN,MAKELPARAM(HTCLIENT,MA_ACTIVATE));
// 						//SendMessage(TexturingWith::lastWnd,WM_SETCURSOR,TexturingWith::lastWnd,MAKELPARAM(HTCLIENT,WM_LBUTTONDOWN));
// 						//SendMessage(TexturingWith::lastWnd,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(vec.x,vec.y));
// 						//Sleep(50);
// 						//SendMessage(TexturingWith::lastWnd,WM_LBUTTONUP,MK_LBUTTON,MAKELPARAM(vec.x,vec.y));
// 						//SendMessage(TexturingWith::lastWnd,WM_LBUTTONDOWN,NULL,MAKELPARAM(vec.x+rt.left,vec.y+rt.top));
// 						//SendMessage(TexturingWith::lastWnd,WM_COMMAND,MK_LBUTTON,MAKELPARAM(vec.x,vec.y));
// // 						INPUT input;
// // 						ZeroMemory(&input, sizeof(input));
// // 						input.type = INPUT_MOUSE;
// // 						input.mi.dx = rt.left+vec.x;
// // 						input.mi.dy = rt.top+vec.y;
// // 						input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_MOVE|MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_LEFTUP;
// // 						SendInput(1,&input,sizeof(input));
//  						POINT pt;
// 						GetCursorPos(&pt);
// 						SetCursorPos(rt.left+vec.x,rt.top+vec.y);
// 						std::cout << rt.left+vec.x << " / " << rt.right + vec.y << std::endl;
// 						mouse_event(MOUSEEVENTF_LEFTDOWN|MOUSEEVENTF_ABSOLUTE	,rt.left+vec.x , rt.top+vec.y, 0, 0);
// 						Sleep(50);
// 						mouse_event(MOUSEEVENTF_LEFTUP|MOUSEEVENTF_ABSOLUTE		,rt.left+vec.x , rt.top+vec.y, 0, 0);
// 						SetCursorPos(pt.x,pt.y);
// 					}
// 				}
// 				
// 				
// 				
// 
// 				return;
// 				/*
// 				//구 코드. 여전히 잘 동작하지만 VGM에서 따온 코드를 사용하자.
// 				D3DXVECTOR2 pick_pos = D3DXVECTOR2(x - 320 , 240 - y);
// 				pick_pos.x /= 320;
// 				pick_pos.y /= 240;
// 				//opengl projection 좌표로 변경됨 
// 				
// 				//std::cout << "------------------------------------------------------" << std::endl;
// 				//std::cout << "picked pos regulared pos = " << pick_pos.x << "/" << pick_pos.y << std::endl;
// 				
// 				//TargetMove(x,y);
// 				
// 				D3DXVECTOR3 normalizedPicked0 = D3DXVECTOR3(pick_pos.x, pick_pos.y, 0);		//near	(in normalized)
// 				D3DXVECTOR3 normalizedPicked1 = D3DXVECTOR3(pick_pos.x, pick_pos.y, 1);		//far	(in normalized)
// 
// 				D3DXMATRIXA16 inversedProj = ChangeD3DXMatrix(InverseMatrix(matProj,matProj));
// 				D3DXVECTOR3 pickedInView0;
// 				D3DXVECTOR3 pickedInView1;
// 				D3DXVec3TransformCoord(&pickedInView0,&normalizedPicked0,&inversedProj);	//near	(in view)
// 				D3DXVec3TransformCoord(&pickedInView1,&normalizedPicked1,&inversedProj);	//far	(in view)
// 				
// 				
// 				D3DXVECTOR3 pickedInWorld0;
// 				D3DXVECTOR3 pickedInWorld1;
// 				D3DXMATRIXA16 inversedView = ChangeD3DXMatrix(InverseMatrix(matView,matView));
// 				D3DXVec3TransformCoord(&pickedInWorld0,&pickedInView0,&inversedView);		//near	(in world)
// 				D3DXVec3TransformCoord(&pickedInWorld1,&pickedInView1,&inversedView);		//far	(int world)
// 				
// 				
// 				//std::cout << " ----------------------- \n ";
// 				//std::cout << "picked pos 0 = " << pickedInWorld0.x << "\t/" << pickedInWorld0.y << "\t/" << pickedInWorld0.z << std::endl;
// 				//std::cout << "picked pos 1 = " << pickedInWorld1.x << "\t/" << pickedInWorld1.y << "\t/" << pickedInWorld1.z << std::endl;
// 				
// 
// 				//src-dest 의 직선이 z=0 평면과 이루는 교점을 찾음.
// 				// x-x0   y-y0   z-z0
// 				// ---- = ---- = ----
// 				// x1-x0  y1-y0  z1-z0
// 				
// 				//z=0의 z값
// 				float z_constant = (0-pickedInWorld0.z) / (pickedInWorld1.z-pickedInWorld0.z);
// 				// y = zc * (y1-y0)+y0
// 				// x = zc * (z1-z0)+z0
// 				float yvalue = z_constant*(pickedInWorld1.y-pickedInWorld0.y)+pickedInWorld0.y;
// 				float xvalue = z_constant*(pickedInWorld1.x-pickedInWorld0.x)+pickedInWorld0.x;
// 
// 				std::cout << "picked pos x=" << xvalue << "\t/" << yvalue << std::endl;
// 				*/
// 			}
// 		}	
// 	}

	double* ViewMatrix(double* setView)
	{
		if(setView) memcpy(matView,setView,sizeof(matView));
		return matView;
	}
	double* ProjMatrix(double* setProj)
	{
		if(setProj) memcpy(matProj,setProj,sizeof(matProj));
		return matProj;
	}




>>>>>>> .r52
}