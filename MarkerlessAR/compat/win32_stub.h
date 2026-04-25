// win32_stub.h — Minimal Win32 type stubs for macOS cross-compilation
// These are no-op stubs to allow the codebase to compile on macOS.
// Actual functionality will be replaced in later sprints (GLFW, std::thread).
#pragma once

#ifndef _WIN32

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <mutex>

// Basic Windows types
typedef int BOOL;
typedef unsigned long DWORD;
typedef unsigned short WORD;
typedef long LONG;
typedef long long LONGLONG;
typedef unsigned int UINT;
typedef int INT;
typedef float FLOAT;
typedef void* HANDLE;
typedef void* HWND;
typedef void* HDC;
typedef void* HBITMAP;
typedef void* HINSTANCE;
typedef void* HMENU;
typedef void* HBRUSH;
typedef void* HCURSOR;
typedef void* HICON;
typedef void* HGDIOBJ;
typedef const char* LPCSTR;
typedef const char* LPCTSTR;
typedef char* LPSTR;
typedef unsigned char BYTE;
typedef long HRESULT;
typedef unsigned long WPARAM;
typedef long LPARAM;
typedef long LRESULT;
typedef unsigned int UINT_PTR;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define S_OK    ((HRESULT)0L)
#define E_FAIL  ((HRESULT)0x80004005L)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define FAILED(hr)    (((HRESULT)(hr)) < 0)

// RECT and POINT
struct RECT { LONG left, top, right, bottom; };
struct POINT { LONG x, y; };

// LARGE_INTEGER for timing
union LARGE_INTEGER {
    struct { DWORD LowPart; LONG HighPart; };
    long long QuadPart;
};

// Critical Section — wraps std::mutex for real thread synchronization
struct CRITICAL_SECTION {
    std::mutex mtx;
};

inline void InitializeCriticalSection(CRITICAL_SECTION*) {} // std::mutex is default-constructed
inline void EnterCriticalSection(CRITICAL_SECTION* cs) { cs->mtx.lock(); }
inline void LeaveCriticalSection(CRITICAL_SECTION* cs) { cs->mtx.unlock(); }
inline void DeleteCriticalSection(CRITICAL_SECTION*) {}

// Thread handling — wraps std::thread for real thread creation
inline HANDLE CreateMutex(void*, BOOL, const char*) { return nullptr; }
inline void CloseHandle(HANDLE h) {
    // If h is a thread handle (std::thread*), detach and delete it
    // Threads in BaekAR run infinite loops, so we detach rather than join
    auto* t = static_cast<std::thread*>(h);
    if (t) {
        if (t->joinable()) t->detach();
        delete t;
    }
}
typedef unsigned (*_beginthreadex_proc)(void*);
inline uintptr_t _beginthreadex(void*, unsigned, _beginthreadex_proc proc, void* arg, unsigned, unsigned*) {
    auto* t = new std::thread([proc, arg]() { proc(arg); });
    return reinterpret_cast<uintptr_t>(t);
}
inline void _endthreadex(unsigned) {} // no-op — thread exits when function returns

// Window stubs (will be replaced by GLFW in Sprint 3)
#define CALLBACK
#define APIENTRY
#define __in
#define __in_opt
#define WINAPI

typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);

struct WNDCLASS {
    UINT style;
    WNDPROC lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCSTR lpszMenuName;
    LPCSTR lpszClassName;
};

struct PAINTSTRUCT { int dummy; };
struct MSG { int dummy; };

// Window style constants
#define CS_HREDRAW 0
#define CS_VREDRAW 0
#define WS_POPUPWINDOW 0
#define CW_USEDEFAULT 0
#define IDC_ARROW nullptr
#define IDI_APPLICATION nullptr
#define MB_OK 0
#define WHITE_BRUSH 0
#define PM_REMOVE 0

// Windows message constants
#define WM_CREATE 0x0001
#define WM_DESTROY 0x0002
#define WM_PAINT 0x000F
#define WM_LBUTTONDOWN 0x0201
#define WM_KEYDOWN 0x0100
#define VK_ESCAPE 0x1B
#define VK_LBUTTON 0x01

inline short GetKeyState(int) { return 0; }
inline BOOL GetCursorPos(POINT*) { return FALSE; }
inline BOOL ScreenToClient(HWND, POINT*) { return FALSE; }

// Stub window functions
inline BOOL RegisterClass(const WNDCLASS*) { return TRUE; }
inline HWND CreateWindow(LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, void*) { return nullptr; }
inline BOOL ShowWindow(HWND, int) { return TRUE; }
inline BOOL PeekMessage(MSG*, HWND, UINT, UINT, UINT) { return FALSE; }
inline BOOL TranslateMessage(const MSG*) { return TRUE; }
inline LONG DispatchMessage(const MSG*) { return 0; }
inline HDC BeginPaint(HWND, PAINTSTRUCT*) { return nullptr; }
inline BOOL EndPaint(HWND, const PAINTSTRUCT*) { return TRUE; }
inline void PostQuitMessage(int) {}
inline void ExitProcess(UINT) { exit(0); }
inline LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
inline HGDIOBJ GetStockObject(int) { return nullptr; }
inline HCURSOR LoadCursor(HINSTANCE, LPCSTR) { return nullptr; }
inline HICON LoadIcon(HINSTANCE, LPCSTR) { return nullptr; }
inline int MessageBox(HWND, LPCSTR, LPCSTR, UINT) { return 0; }
inline void AllocConsole() {}
inline HWND GetDesktopWindow() { return nullptr; }
inline BOOL GetWindowRect(HWND, RECT*) { return FALSE; }
inline BOOL GetClientRect(HWND, RECT*) { return FALSE; }
inline HWND FindWindow(LPCSTR, LPCSTR) { return nullptr; }
inline HDC GetDC(HWND) { return nullptr; }
inline int ReleaseDC(HWND, HDC) { return 0; }
inline HDC CreateCompatibleDC(HDC) { return nullptr; }
inline BOOL DeleteDC(HDC) { return TRUE; }
inline HGDIOBJ SelectObject(HDC, HGDIOBJ) { return nullptr; }
inline BOOL DeleteObject(HGDIOBJ) { return TRUE; }
inline HBITMAP CreateDIBSection(HDC, void*, UINT, void**, HANDLE, DWORD) { return nullptr; }
inline BOOL PrintWindow(HWND, HDC, UINT) { return FALSE; }
inline BOOL QueryPerformanceFrequency(LARGE_INTEGER*) { return FALSE; }
inline BOOL QueryPerformanceCounter(LARGE_INTEGER*) { return FALSE; }

struct BITMAPINFOHEADER {
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
};
struct BITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
};
#define DIB_RGB_COLORS 0
#define ZeroMemory(p, sz) memset((p), 0, (sz))

// Font weight
#define FW_BOLD 700
#define DEFAULT_CHARSET 0
#define OUT_DEFAULT_PRECIS 0
#define DEFAULT_QUALITY 0
#define DEFAULT_PITCH 0
#define FF_DONTCARE 0

#endif // !_WIN32
