#include "External_Include.h"



//Message Loop
LRESULT CALLBACK WndProc(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam);



INT APIENTRY WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nShowCmd )
{
	//console
	AllocConsole();
	freopen("CONOUT$","wt",stdout);	

	//////////////////////////////////////////////////////////////////////////
	//initialize directX
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	//g_hInst=hInstance;

	WndClass.cbClsExtra=0;
	WndClass.cbWndExtra=0;
	WndClass.hbrBackground=(HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor=LoadCursor(NULL,IDC_ARROW);
	WndClass.hIcon=LoadIcon(NULL,IDI_APPLICATION);
	WndClass.hInstance=hInstance;
	WndClass.lpfnWndProc=(WNDPROC)WndProc;
	WndClass.lpszClassName=GLOBAL::lpszClass;
	WndClass.lpszMenuName=NULL;
	WndClass.style=CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	hWnd=CreateWindow(GLOBAL::lpszClass,GLOBAL::lpszClass,WS_POPUPWINDOW,
		CW_USEDEFAULT,CW_USEDEFAULT,640,480,
		NULL,(HMENU)NULL,hInstance,NULL);
	ShowWindow(hWnd,nShowCmd);


	//초기화
	if(FAILED(wonjo_dx::AAR3DInitD3D(hWnd))) 
	{
		MessageBox(NULL,"DirectX Device Failed.\nthe application will be terminated.","BaekAR",MB_OK);
		return 0;	//실패시 윈도우 끝내버림.
	}

	//direct X code mainloop
	PeekMessage( &Message, NULL, 0U, 0U, PM_REMOVE );
	while(true) {	//main loop start

		//윈도우 핸들링이 들어올 때 처리
		if( PeekMessage( &Message, NULL, 0U, 0U, PM_REMOVE ) )
		{
			TranslateMessage( &Message );
			DispatchMessage( &Message );
			continue;
		}
		//메시지가 들어오지 않았을 때 처리 (main loop)
		else GLOBAL::mainLoop();
	}	//end of main loop
	//~direct X code
}








//Message Loop
LRESULT CALLBACK WndProc(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	switch(iMessage) {
	case WM_CREATE:
		return 0;
	case WM_PAINT:
		hdc=BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_DESTROY:
		wonjo_dx::AAR3DReleaseSystem();
		ExitProcess(0);
		PostQuitMessage(0);
		return 0;
	case WM_LBUTTONDOWN:
		//std::cout << "click" << std::endl;
		{
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(hWnd, &pt);
			wonjo_dx::Picking(pt);
			/*
			D3DXVECTOR3 triangles[6];
			triangles[0]=D3DXVECTOR3(-30,-30,0);
			triangles[1]=D3DXVECTOR3(-30,30,0);
			triangles[2]=D3DXVECTOR3(30,-30,0);
			triangles[3]=D3DXVECTOR3(30,-30,0);
			triangles[4]=D3DXVECTOR3(-30,30,0);
			triangles[5]=D3DXVECTOR3(30,30,0);
			d3d::PickWithTriangle(pt,triangles,6);
			*/
		}
		return 0;
	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_ESCAPE:
			wonjo_dx::AAR3DReleaseSystem();
			ExitProcess(0);
			PostQuitMessage(0);
			return 0;
		}
	}
	return(DefWindowProc(hWnd,iMessage,wParam,lParam));
}
