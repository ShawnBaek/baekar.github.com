#pragma once
//#define _TESTMODE
//#define _LOGGING
#include "windows.h"
#include <iostream>
<<<<<<< .mine
#include <boost/lexical_cast.hpp>
=======
//#include <boost/lexical_cast.hpp>
>>>>>>> .r52

//import directx
#include "d3d9.h"
#include "d3dx9.h"
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"dxguid.lib")

<<<<<<< .mine



//radian transform
#define Deg2Rad(n) ((n)*0.01745329252222f)
#define Rad2Deg(n) ((n)*57.2957795056010466467050759f)



#define SAFE_RELEASE(p) {if(p) {(p)->Release(); (p)=NULL;}}

namespace d3d
{
	extern LPDIRECT3D9 gpD3D;
	extern LPDIRECT3DDEVICE9 gpDevice;
	HRESULT AAR3DInitD3D(HWND hWnd);
	void AAR3DReleaseSystem();
	void AAR3DTotalProcess(char* cameradata, int pixelx, int pixely, int size);
	LPDIRECT3DTEXTURE9 AAR3DTestTexture();
	double GetTimeEllapse();
	LPDIRECT3DDEVICE9 GetDevice();
	D3DXVECTOR3 Picking(POINT MousePosClient);
	void PickWithTriangle(POINT ptClient, D3DXVECTOR3* triangles, int size);
}





namespace cam3D
{
	extern D3DXVECTOR3 pos;
	extern D3DXVECTOR3 up;
	extern D3DXVECTOR3 lookAt;
	void AAR3DInitSystem();
	void UpdateView(double* dbmatView = NULL);
	void UpdateProjection();
	
}





namespace WONJO
=======
//radian transform
#define Deg2Rad(n) ((n)*0.01745329252222f)
#define Rad2Deg(n) ((n)*57.2957795056010466467050759f)
#define SAFE_RELEASE(p) {if(p) {(p)->Release(); (p)=NULL;}}

namespace wonjo_dx
>>>>>>> .r52
{
<<<<<<< .mine

	class TexturingWith
	{
	public:
		TexturingWith(const char* className, const char* WindowName);
		~TexturingWith();
		void Plane(double size);
		BOOL isPicked(int screenx, int screeny);
	private:
		HWND hSrc;
		HDC hSrcDC;
		HDC hdc;
		HBITMAP hBitMapUsing;
		HDC dcTarget;
		HDC memdc;
		int width;
		int height;
		void GetBitmapData(const char* className, const char* WindowName);
		void ReleaseBitmapData();	
		void SetUp();
		
	};
=======
	//directx system
	extern LPDIRECT3D9 gpD3D;
	extern LPDIRECT3DDEVICE9 gpDevice;
	extern LPDIRECT3DTEXTURE9  gpTextureBackground;
	//extern LPDIRECT3DTEXTURE9  gpTextureBrowser;
	HRESULT AAR3DInitD3D(HWND hWnd);
	void AAR3DReleaseSystem();
	void AAR3DDrawMesh(LPCSTR DestClassName, LPCSTR DestWindowName, float size);
	void AAR3DDrawCameraPreview(char* cameradata, int pixelx, int pixely);
	void Flip();
	LPDIRECT3DTEXTURE9 AAR3DTestTexture();
	double GetTimeEllapse();
	LPDIRECT3DDEVICE9 GetDevice();
	D3DXVECTOR3 Picking(POINT MousePosClient);
	void PickWithTriangle(POINT ptClient, D3DXVECTOR3* triangles, int size);
	float GetScaleXYZ();
	HWND GetTargetHWND();
>>>>>>> .r52
<<<<<<< .mine
=======

	//directx camera system
	void SetProjectionMatrix(double* proj = NULL);
	void SetModelViewMatrix(double* view);
		
	//iosystem
	void ProcessIO();
	BOOL CalcPickWithVertex(int x, int y);

// 	class TexturingWith
// 	{
// 	public:
// 		TexturingWith(const char* className, const char* WindowName);
// 		~TexturingWith();
// 		void Plane(double size);
// 		static void CreateTexture();
// 		static void ReleaseTexture();
// 		BOOL isPicked(int screenx, int screeny);
// 		static HWND lastWnd;
// 	private:
// 		HWND hSrc;
// 		HDC hSrcDC;
// 		HDC hdc;
// 		HBITMAP hBitMapUsing;
// 		HDC dcTarget;
// 		HDC memdc;
// 		int width;
// 		int height;
// 		static GLuint texture;
// 		void GetBitmapData(const char* className, const char* WindowName);
// 		void ReleaseBitmapData();
// 		void FreeTexture(GLuint texture);
// 		void SetUp();
// 	};

	//void MouseEvent(int iButton, int iState, int x, int y);
	void Cube(double size);
	void Plane(double size);
	void Plane(double size, double corners[4][3]);
	double* MultiMatrix(double* pOut, const double* pFirst, const double* pSecond);
	double* InverseMatrix(double* pOut, const double* pIn);
	void CalcPickingVertex(float* outF1, float* outF2, const POINT pickedpt, const int viewPortX, const int viewPortY, const double* mdproj, const double* mdview);
	double* ViewMatrix(double* setView = NULL);
	double* ProjMatrix(double* setProj = NULL);


>>>>>>> .r52
	
}
