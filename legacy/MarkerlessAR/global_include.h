#pragma once
//너무 많은 warning들을 일단 disable (EN: Disable too many warnings for now)
// MSVC #pragma warning directives removed for cross-platform build:
// 4996: This function or variable may be unsafe
// 4819: 현재 코드 페이지(949)에서 표시할 수 없는 문자가 파일에 들어 있습니다 (EN: File contains characters not displayable in current code page 949)
// 4101: 참조되지 않은 지역 변수입니다 (EN: Unreferenced local variable)
// 4003: 'max' 매크로의 실제 매개 변수가 부족합니다 (EN: Not enough actual parameters for 'max' macro)
// 4244: 암시적 타입캐스트 경고 (EN: Implicit typecast warning)

//120325 cwj 최상위 헤더파일 directx (EN: Top-level header file for DirectX — now removed for macOS port)

//#define _TESTMODE
//#define _DRAW_HAND
//#define _LOGGING

// windows.h removed — not available on macOS
#include <iostream>
#include <unordered_map>  // replaced MSVC stdext::hash_map
#include <string>
//#include <boost/lexical_cast.hpp>

// DirectX headers and lib pragmas removed for macOS port
// (d3d9.h, d3dx9.h, dxguid.lib, d3d9.lib, d3dx9.lib)

//radian transform
#define Deg2Rad(n) ((n)*0.01745329252222f)
#define Rad2Deg(n) ((n)*57.2957795056010466467050759f)
#define SAFE_RELEASE(p) {if(p) {(p) = NULL;}}


//~120325 cwj
