#include "wonjo.h"

#ifndef _WIN32
#define GL_SILENCE_DEPRECATION
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif


namespace wonjo_dx
{
	D3DXVECTOR3 pos;
	D3DXVECTOR3 up;
	D3DXVECTOR3 lookAt;
	static D3DXMATRIXA16 matView;
	static D3DXMATRIXA16 matProj;

#ifndef _WIN32
	// Helper: transpose D3DXMATRIXA16 (row-major) to float[16] (column-major) for OpenGL
	static void TransposeToGL(const D3DXMATRIXA16& src, float* dst) {
		for(int r = 0; r < 4; ++r)
			for(int c = 0; c < 4; ++c)
				dst[c*4+r] = src.m[r][c];
	}
#endif

	void SetProjectionMatrix(double* proj)
	{
		if(proj)
		{
			for(int i = 0 ; i < 16 ; ++i) matProj[i] = proj[i];
		}

#ifdef _WIN32
		if(GetDevice()) GetDevice()->SetTransform(D3DTS_PROJECTION, &matProj);
#else
		// double* data is already in OpenGL column-major format
		float fproj[16];
		for(int i = 0; i < 16; ++i) fproj[i] = matProj[i];
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(fproj);
#endif
	}

	void SetModelViewMatrix(double* view)
	{
		if(view)
		{
			for(int i = 0 ; i < 16 ; ++i) matView[i] = view[i];
		}

#ifdef _WIN32
		//double* view가 opengl 의 matrix로 넘어오기 때문에 directx 좌표계에서는 좌우를 flip해야한다.
		D3DXMATRIXA16 matFlip;
		D3DXMatrixScaling(&matFlip,-1,1,1);
		matView *= matFlip;

		if(GetDevice()) GetDevice()->SetTransform(D3DTS_VIEW, &matView);
#else
		// double* data is already in OpenGL column-major format — no flip needed
		float fview[16];
		for(int i = 0; i < 16; ++i) fview[i] = matView[i];
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(fview);
#endif
	}

	void SetProjectionMatrix(const D3DXMATRIXA16* proj)
	{
		matProj = *proj;

#ifdef _WIN32
		if(GetDevice()) GetDevice()->SetTransform(D3DTS_PROJECTION, &matProj);
#else
		// D3DXMATRIXA16 is row-major; OpenGL expects column-major — transpose
		float fproj[16];
		TransposeToGL(matProj, fproj);
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(fproj);
#endif
	}

	void SetModelViewMatrix(const D3DXMATRIXA16* view)
	{
		matView = *view;

#ifdef _WIN32
		//double* view가 opengl 의 matrix로 넘어오기 때문에 directx 좌표계에서는 좌우를 flip해야한다.
// 		D3DXMATRIXA16 matFlip;
// 		D3DXMatrixScaling(&matFlip,-1,1,1);
// 		matView *= matFlip;

		if(GetDevice()) GetDevice()->SetTransform(D3DTS_VIEW, &matView);
#else
		// D3DXMATRIXA16 is row-major; OpenGL expects column-major — transpose
		float fview[16];
		TransposeToGL(matView, fview);
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(fview);
#endif
	}
}
