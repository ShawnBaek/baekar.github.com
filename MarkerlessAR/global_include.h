#pragma once
//�ʹ� ���� warning���� �ϴ� disable
#pragma warning(disable:4996)	//This function or variable may be unsafe. Consider using fopen_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
#pragma warning(disable:4819)	//���� �ڵ� ������(949)���� ǥ���� �� ���� ���ڰ� ���Ͽ� ��� �ֽ��ϴ�. �����Ͱ� �սǵ��� �ʰ� �Ϸ��� �ش� ������ �����ڵ� �������� �����Ͻʽÿ�.
#pragma warning(disable:4101)	//�������� ���� ���� �����Դϴ�.
#pragma warning(disable:4003)	//'max' ��ũ���� ���� �Ű� ������ �����մϴ�.
#pragma warning(disable:4244)	//�Ͻ��� Ÿ��ĳ��Ʈ ���
//120325 cwj �ֻ��� ������� directx

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