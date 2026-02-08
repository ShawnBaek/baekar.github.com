#include "global_include.h"
#pragma once


namespace wonjo_dx
{

	struct CUSTOMVERTEX
	{
		D3DXVECTOR3 position; // The position
		D3DXVECTOR3 normal;		//normal
		FLOAT tu, tv;   // The texture coordinates
		void setUV(float u, float v)
		{
			tu = u;
			tv = v;
		}
	};

	extern BOOL bDrawHand;
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
	HRESULT BeginRender();
	LPDIRECT3DTEXTURE9 AAR3DTestTexture();
	double GetTimeEllapse();
	LPDIRECT3DDEVICE9 GetDevice();
	D3DXVECTOR3 Picking(POINT MousePosClient);
	void PickWithTriangle(POINT ptClient, D3DXVECTOR3* triangles, int size);
	LPDIRECT3DVERTEXBUFFER9 AAR3DGetModel();
	//directx camera system
	void SetProjectionMatrix(double* proj = NULL);
	void SetModelViewMatrix(double* view);
	void SetProjectionMatrix(const D3DXMATRIXA16* proj);
	void SetModelViewMatrix(const D3DXMATRIXA16* view);

	void Matrix_LH_RH_Swap(D3DXMATRIXA16* pOut, const D3DXMATRIXA16* pIn);

	class Mesh;
	typedef Mesh* LPMESH;
	LPMESH LoadMeshFromFile(LPCSTR location);
	void ReleaseMeshes();
	class Mesh
	{
		friend void ReleaseMeshes();
		friend LPMESH LoadMeshFromFile(LPCSTR location);
	public:
		LPD3DXBUFFER m_AdjBuffer;
		LPD3DXBUFFER m_MtrlBuffer;
		LPD3DXMESH m_Mesh;
		DWORD m_NumMtrl;
		std::vector<D3DMATERIAL9> Mtrls;
		std::string filename;
		std::vector<LPDIRECT3DTEXTURE9> m_Tex;
	private:
		Mesh();
		~Mesh();
	};
	void DrawMesh(LPMESH pms, const D3DXMATRIXA16* matWorld = NULL);
	void DrawPlane(const D3DXMATRIXA16* matWorld = NULL);
	const D3DXMATRIXA16* MakeScaleMatrix(float x, float y, float z);
	const D3DXMATRIXA16* MakeTranslationMatrix(float x, float y, float z);
	const D3DXMATRIXA16* MakeRotationMatrix(float x, float y, float z);

	
	class AAR3DTexturing
	{
		BOOL bSucceeded;
		HWND hSrc;
		HDC hSrcDC;
		HDC hdc;
	public:
		RECT targetRect;
		HDC memdc;
		HDC dcTarget;
		LPDIRECT3DTEXTURE9 ptxt;
		HBITMAP hBitMapUsing;
		BOOL isSucceeded();
		
		AAR3DTexturing(LPCSTR DestClassName, LPCSTR DestWIndowName);
		~AAR3DTexturing();
	};

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


	
}
