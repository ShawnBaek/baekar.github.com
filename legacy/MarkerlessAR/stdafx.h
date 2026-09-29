// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 및 프로젝트 관련 포함 파일이
// 들어 있는 포함 파일입니다.
// (EN: Precompiled header — standard system includes and project includes
//  that are used frequently but rarely changed.)

#pragma once

// targetver.h removed — Windows SDK version targeting not needed on macOS
// <tchar.h> removed — Windows Unicode/ANSI abstraction not needed on macOS

#include <stdio.h>

// TODO: 프로그램에 필요한 추가 헤더는 여기에서 참조합니다.
// (EN: Reference additional headers needed by the program here.)

#include <iostream>
#include <string>
#include <time.h>
#include <vector>
// <atlstr.h> removed — MSVC ATL string library not available on macOS; use std::string

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ostream;

using namespace cv;
using namespace std;