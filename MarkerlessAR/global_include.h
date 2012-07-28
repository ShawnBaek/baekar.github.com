#pragma once
//너무 많은 warning들을 일단 disable
#pragma warning(disable:4996)	//This function or variable may be unsafe. Consider using fopen_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
#pragma warning(disable:4819)	//현재 코드 페이지(949)에서 표시할 수 없는 문자가 파일에 들어 있습니다. 데이터가 손실되지 않게 하려면 해당 파일을 유니코드 형식으로 저장하십시오.
#pragma warning(disable:4101)	//참조되지 않은 지역 변수입니다.
#pragma warning(disable:4003)	//'max' 매크로의 실제 매개 변수가 부족합니다.
#pragma warning(disable:4244)	//암시적 타입캐스트 경고
//120325 cwj 최상위 헤더파일 directx

//#define _TESTMODE
//#define _DRAW_HAND
//#define _LOGGING
#include "windows.h"
#include <iostream>
#include <hash_map>
#include <string>
//#include <boost/lexical_cast.hpp>

//import directx
#include "d3d9.h"
#include "d3dx9.h"
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"dxguid.lib")

//radian transform
#define Deg2Rad(n) ((n)*0.01745329252222f)
#define Rad2Deg(n) ((n)*57.2957795056010466467050759f)
#define SAFE_RELEASE(p) {if(p) {(p)->Release(); (p)=NULL;}}


//~120325 cwj