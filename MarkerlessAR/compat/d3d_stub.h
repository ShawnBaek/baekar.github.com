// d3d_stub.h — Minimal DirectX 9 type stubs for macOS cross-compilation
// These are no-op stubs to allow the codebase to compile on macOS.
// Actual 3D rendering will be done via OpenGL in Sprint 4.
#pragma once

#ifndef _WIN32

#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include "win32_stub.h"

// ---- D3D Vector/Matrix types ----

struct D3DXVECTOR2 {
    float x, y;
    D3DXVECTOR2() : x(0), y(0) {}
    D3DXVECTOR2(float _x, float _y) : x(_x), y(_y) {}
};

struct D3DXVECTOR3 {
    float x, y, z;
    D3DXVECTOR3() : x(0), y(0), z(0) {}
    D3DXVECTOR3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    D3DXVECTOR3 operator+(const D3DXVECTOR3& o) const { return D3DXVECTOR3(x+o.x, y+o.y, z+o.z); }
    D3DXVECTOR3 operator-(const D3DXVECTOR3& o) const { return D3DXVECTOR3(x-o.x, y-o.y, z-o.z); }
    D3DXVECTOR3 operator*(float s) const { return D3DXVECTOR3(x*s, y*s, z*s); }
    D3DXVECTOR3 operator/(float s) const { return D3DXVECTOR3(x/s, y/s, z/s); }
    D3DXVECTOR3& operator/=(float s) { x/=s; y/=s; z/=s; return *this; }
};

struct D3DXCOLOR {
    float r, g, b, a;
    D3DXCOLOR() : r(0), g(0), b(0), a(0) {}
    D3DXCOLOR(float _r, float _g, float _b, float _a) : r(_r), g(_g), b(_b), a(_a) {}
    D3DXCOLOR operator*(float s) const { return D3DXCOLOR(r*s, g*s, b*s, a*s); }
};

struct D3DXMATRIXA16 {
    union {
        struct {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
        float flat[16];
    };
    D3DXMATRIXA16() { memset(flat, 0, sizeof(flat)); _11 = _22 = _33 = _44 = 1.0f; }
    float& operator[](int i) { return flat[i]; }
    float operator[](int i) const { return flat[i]; }
    float operator()(int r, int c) const { return m[r][c]; }
    D3DXMATRIXA16 operator*(const D3DXMATRIXA16& o) const {
        D3DXMATRIXA16 res;
        memset(res.flat, 0, sizeof(res.flat));
        for(int i=0;i<4;i++) for(int j=0;j<4;j++) for(int k=0;k<4;k++)
            res.m[i][j] += m[i][k] * o.m[k][j];
        return res;
    }
    D3DXMATRIXA16& operator*=(const D3DXMATRIXA16& o) { *this = *this * o; return *this; }
};

// ---- D3D Interface stubs (opaque pointers) ----

struct IDirect3DDevice9_stub; // forward declare
struct IDirect3DVertexBuffer9_stub;

struct IDirect3D9_stub {
    HRESULT CreateDevice(UINT, int, HWND, DWORD, void*, IDirect3DDevice9_stub**) { return E_FAIL; }
};
struct IDirect3DDevice9_stub {
    HRESULT SetTransform(int, const D3DXMATRIXA16*) { return S_OK; }
    HRESULT GetTransform(int, D3DXMATRIXA16*) { return S_OK; }
    HRESULT SetRenderState(int, DWORD) { return S_OK; }
    HRESULT SetMaterial(const void*) { return S_OK; }
    HRESULT SetTexture(DWORD, void*) { return S_OK; }
    HRESULT SetTextureStageState(DWORD, int, int) { return S_OK; }
    HRESULT SetStreamSource(UINT, IDirect3DVertexBuffer9_stub*, UINT, UINT) { return S_OK; }
    HRESULT SetFVF(DWORD) { return S_OK; }
    HRESULT DrawPrimitive(int, UINT, UINT) { return S_OK; }
    HRESULT SetLight(DWORD, const void*) { return S_OK; }
    HRESULT LightEnable(DWORD, BOOL) { return S_OK; }
    HRESULT SetSamplerState(DWORD, int, DWORD) { return S_OK; }
    HRESULT GetDeviceCaps(void*) { return S_OK; }
    HRESULT GetViewport(void*) { return S_OK; }
    HRESULT CreateVertexBuffer(UINT, DWORD, DWORD, int, IDirect3DVertexBuffer9_stub**, void*) { return E_FAIL; }
    HRESULT Clear(DWORD, void*, DWORD, DWORD, float, DWORD) { return S_OK; }
    HRESULT BeginScene() { return S_OK; }
    HRESULT EndScene() { return S_OK; }
    HRESULT Present(void*, void*, void*, void*) { return S_OK; }
};
struct IDirect3DTexture9_stub {
    HRESULT LockRect(UINT, void*, void*, DWORD) { return E_FAIL; }
    HRESULT UnlockRect(UINT) { return S_OK; }
};
struct IDirect3DVertexBuffer9_stub {
    HRESULT Lock(UINT, UINT, void**, DWORD) { return E_FAIL; }
    HRESULT Unlock() { return S_OK; }
};
struct ID3DXBuffer_stub {
    void* GetBufferPointer() { return nullptr; }
};
struct ID3DXMesh_stub {
    HRESULT DrawSubset(DWORD) { return S_OK; }
};
struct ID3DXFont_stub { int dummy; };
struct ID3DXSprite_stub {
    HRESULT Begin(DWORD) { return S_OK; }
    HRESULT Draw(void*, void*, void*, void*, DWORD) { return S_OK; }
    HRESULT End() { return S_OK; }
};

typedef IDirect3D9_stub* LPDIRECT3D9;
typedef IDirect3DDevice9_stub* LPDIRECT3DDEVICE9;
typedef IDirect3DTexture9_stub* LPDIRECT3DTEXTURE9;
typedef IDirect3DVertexBuffer9_stub* LPDIRECT3DVERTEXBUFFER9;
typedef ID3DXBuffer_stub* LPD3DXBUFFER;
typedef ID3DXMesh_stub* LPD3DXMESH;
typedef ID3DXFont_stub* LPD3DXFONT;
typedef ID3DXSprite_stub* LPD3DXSPRITE;

// ---- D3D Material/Light structures ----

struct D3DXCOLOR; // forward declare
struct D3DCOLORVALUE {
    float r, g, b, a;
    D3DCOLORVALUE() : r(0), g(0), b(0), a(0) {}
    D3DCOLORVALUE(float _r, float _g, float _b, float _a) : r(_r), g(_g), b(_b), a(_a) {}
    inline D3DCOLORVALUE& operator=(const D3DXCOLOR& rhs);
};

struct D3DMATERIAL9 {
    D3DCOLORVALUE Diffuse;
    D3DCOLORVALUE Ambient;
    D3DCOLORVALUE Specular;
    D3DCOLORVALUE Emissive;
    float Power;
};

struct D3DLIGHT9 {
    int Type;
    D3DCOLORVALUE Ambient;
    D3DCOLORVALUE Diffuse;
    D3DCOLORVALUE Specular;
    D3DXVECTOR3 Direction;
    float Range;
};

struct D3DCAPS9 { DWORD MaxAnisotropy; };
struct D3DLOCKED_RECT { int Pitch; void* pBits; };
struct D3DPRESENT_PARAMETERS {
    BOOL Windowed;
    int SwapEffect;
    int BackBufferFormat;
    BOOL EnableAutoDepthStencil;
    int AutoDepthStencilFormat;
    int MultiSampleType;
};
struct D3DVIEWPORT9 { DWORD X, Y, Width, Height; float MinZ, MaxZ; };
struct D3DXMATERIAL {
    D3DMATERIAL9 MatD3D;
    char* pTextureFilename;
};

// ---- D3D Constants ----

#define D3D_SDK_VERSION 32
#define D3DFVF_XYZ      0x002
#define D3DFVF_NORMAL   0x010
#define D3DFVF_DIFFUSE  0x040
#define D3DFVF_TEX1     0x100
#define D3DSWAPEFFECT_DISCARD 1
#define D3DFMT_UNKNOWN 0
#define D3DFMT_D16 0
#define D3DFMT_A8R8G8B8 0
#define D3DFMT_A8B8G8R8 0
#define D3DADAPTER_DEFAULT 0
#define D3DDEVTYPE_HAL 0
#define D3DCREATE_SOFTWARE_VERTEXPROCESSING 0
#define D3DPOOL_DEFAULT 0
#define D3DPOOL_MANAGED 0
#define D3DXMESH_MANAGED 0
#define D3DRS_CULLMODE 0
#define D3DRS_LIGHTING 0
#define D3DRS_NORMALIZENORMALS 0
#define D3DRS_SPECULARENABLE 0
#define D3DRS_ZENABLE 0
#define D3DRS_ALPHABLENDENABLE 0
#define D3DRS_SRCBLEND 0
#define D3DRS_DESTBLEND 0
#define D3DRS_ALPHATESTENABLE 0
#define D3DRS_ALPHAREF 0
#define D3DRS_ALPHAFUNC 0
#define D3DRS_FILLMODE 0
#define D3DCULL_NONE 0
#define D3DCULL_CCW 0
#define D3DBLEND_SRCALPHA 0
#define D3DBLEND_INVSRCALPHA 0
#define D3DCMP_GREATEREQUAL 0
#define D3DFILL_WIREFRAME 0
#define D3DPT_TRIANGLELIST 0
#define D3DLIGHT_DIRECTIONAL 0
#define D3DTSS_COLOROP 0
#define D3DTSS_COLORARG1 0
#define D3DTSS_COLORARG2 0
#define D3DTSS_ALPHAOP 0
#define D3DTSS_ALPHAARG1 0
#define D3DTSS_ALPHAARG2 0
#define D3DTOP_MODULATE 0
#define D3DTOP_DISABLE 0
#define D3DTA_TEXTURE 0
#define D3DTA_DIFFUSE 0
#define D3DTS_WORLD 0
#define D3DTS_VIEW 0
#define D3DTS_PROJECTION 0
#define D3DLOCK_DISCARD 0
#define D3DSAMP_MINFILTER 0
#define D3DSAMP_MAGFILTER 0
#define D3DSAMP_MAXANISOTROPY 0
#define D3DTEXF_ANISOTROPIC 0
#define D3DCLEAR_TARGET 0
#define D3DCLEAR_ZBUFFER 0
#define D3DXSPRITE_ALPHABLEND 0
#define D3DXSPRITE_SORT_TEXTURE 0
#define D3DCOLOR_XRGB(r,g,b) ((DWORD)(0xFF000000|((r)<<16)|((g)<<8)|(b)))
#define D3DCOLOR_ARGB(a,r,g,b) ((DWORD)(((a)<<24)|((r)<<16)|((g)<<8)|(b)))

// ---- D3DX Math functions (stubs) ----

inline LPDIRECT3D9 Direct3DCreate9(UINT) { return nullptr; }
inline void D3DXMatrixScaling(D3DXMATRIXA16* m, float x, float y, float z) {
    memset(m->flat, 0, sizeof(m->flat)); m->_11=x; m->_22=y; m->_33=z; m->_44=1;
}
inline void D3DXMatrixTranslation(D3DXMATRIXA16* m, float x, float y, float z) {
    memset(m->flat, 0, sizeof(m->flat)); m->_11=m->_22=m->_33=m->_44=1; m->_41=x; m->_42=y; m->_43=z;
}
inline void D3DXMatrixRotationYawPitchRoll(D3DXMATRIXA16*, float, float, float) {}
inline float D3DXMatrixDeterminant(const D3DXMATRIXA16*) { return 1.0f; }
inline D3DXMATRIXA16* D3DXMatrixInverse(D3DXMATRIXA16* out, float*, const D3DXMATRIXA16*) { return out; }
inline void D3DXVec3TransformCoord(D3DXVECTOR3*, const D3DXVECTOR3*, const D3DXMATRIXA16*) {}
inline void D3DXVec3TransformNormal(D3DXVECTOR3*, const D3DXVECTOR3*, const D3DXMATRIXA16*) {}
inline void D3DXVec3Normalize(D3DXVECTOR3* out, const D3DXVECTOR3* in) {
    float len = sqrtf(in->x*in->x + in->y*in->y + in->z*in->z);
    if(len > 0) { out->x = in->x/len; out->y = in->y/len; out->z = in->z/len; }
}
inline HRESULT D3DXLoadMeshFromX(LPCSTR, DWORD, LPDIRECT3DDEVICE9, LPD3DXBUFFER*, LPD3DXBUFFER*, void*, DWORD*, LPD3DXMESH*) { return E_FAIL; }
inline HRESULT D3DXCreateTextureFromFile(LPDIRECT3DDEVICE9, LPCSTR, LPDIRECT3DTEXTURE9*) { return E_FAIL; }
inline HRESULT D3DXCreateTexture(LPDIRECT3DDEVICE9, UINT, UINT, UINT, DWORD, int, int, LPDIRECT3DTEXTURE9*) { return E_FAIL; }
inline HRESULT D3DXCreateFont(LPDIRECT3DDEVICE9, int, UINT, UINT, UINT, BOOL, DWORD, DWORD, DWORD, DWORD, LPCSTR, LPD3DXFONT*) { return E_FAIL; }
inline HRESULT D3DXCreateSprite(LPDIRECT3DDEVICE9, LPD3DXSPRITE*) { return E_FAIL; }
inline D3DXMATRIXA16* D3DXMatrixLookAtLH(D3DXMATRIXA16* pOut, const D3DXVECTOR3* pEye, const D3DXVECTOR3* pAt, const D3DXVECTOR3* pUp) {
    // zaxis = normal(At - Eye)
    D3DXVECTOR3 zaxis = {pAt->x - pEye->x, pAt->y - pEye->y, pAt->z - pEye->z};
    float zlen = sqrtf(zaxis.x*zaxis.x + zaxis.y*zaxis.y + zaxis.z*zaxis.z);
    if (zlen > 0) { zaxis.x /= zlen; zaxis.y /= zlen; zaxis.z /= zlen; }
    // xaxis = normal(cross(Up, zaxis))
    D3DXVECTOR3 xaxis = {
        pUp->y * zaxis.z - pUp->z * zaxis.y,
        pUp->z * zaxis.x - pUp->x * zaxis.z,
        pUp->x * zaxis.y - pUp->y * zaxis.x
    };
    float xlen = sqrtf(xaxis.x*xaxis.x + xaxis.y*xaxis.y + xaxis.z*xaxis.z);
    if (xlen > 0) { xaxis.x /= xlen; xaxis.y /= xlen; xaxis.z /= xlen; }
    // yaxis = cross(zaxis, xaxis)
    D3DXVECTOR3 yaxis = {
        zaxis.y * xaxis.z - zaxis.z * xaxis.y,
        zaxis.z * xaxis.x - zaxis.x * xaxis.z,
        zaxis.x * xaxis.y - zaxis.y * xaxis.x
    };
    memset(pOut->flat, 0, sizeof(pOut->flat));
    pOut->_11 = xaxis.x;  pOut->_12 = yaxis.x;  pOut->_13 = zaxis.x;  pOut->_14 = 0;
    pOut->_21 = xaxis.y;  pOut->_22 = yaxis.y;  pOut->_23 = zaxis.y;  pOut->_24 = 0;
    pOut->_31 = xaxis.z;  pOut->_32 = yaxis.z;  pOut->_33 = zaxis.z;  pOut->_34 = 0;
    pOut->_41 = -(xaxis.x*pEye->x + xaxis.y*pEye->y + xaxis.z*pEye->z);
    pOut->_42 = -(yaxis.x*pEye->x + yaxis.y*pEye->y + yaxis.z*pEye->z);
    pOut->_43 = -(zaxis.x*pEye->x + zaxis.y*pEye->y + zaxis.z*pEye->z);
    pOut->_44 = 1;
    return pOut;
}
inline BOOL D3DXIntersectTri(const D3DXVECTOR3*, const D3DXVECTOR3*, const D3DXVECTOR3*, const D3DXVECTOR3*, const D3DXVECTOR3*, float*, float*, float*) { return FALSE; }

// Deferred definition of D3DCOLORVALUE::operator= (needs D3DXCOLOR to be complete)
inline D3DCOLORVALUE& D3DCOLORVALUE::operator=(const D3DXCOLOR& rhs) {
    r = rhs.r; g = rhs.g; b = rhs.b; a = rhs.a;
    return *this;
}

#endif // !_WIN32
