#include <windows.h>
#include <gdiplus.h>
#include <filesystem>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>

#pragma comment(lib,"gdiplus.lib")

using namespace Gdiplus;

// ------------------------ GLOBAL VARIABLES ------------------------
HWND hwndOverlay = nullptr;
POINT startPt, endPt;
bool selecting = false;
ULONG_PTR gdiplusToken;

// ------------------------ PNG HELPER ------------------------
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0, size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)malloc(size);
    GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j)
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    free(pImageCodecInfo);
    return -1;
}

// ------------------------ SCREENSHOT FUNCTION ------------------------
void SaveScreenshot(int x, int y, int w, int h)
{
    HDC hScreen = GetDC(NULL);
    HDC hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
    SelectObject(hDC, hBitmap);
    BitBlt(hDC, 0, 0, w, h, hScreen, x, y, SRCCOPY);

    std::filesystem::create_directory("screenshots");

    Bitmap bmp(hBitmap, NULL);
    CLSID pngClsid;
    GetEncoderClsid(L"image/png", &pngClsid);

    // Naziv: screenshots/screen_YYYYMMDD_HHMMSS.png
    auto t = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(t);
    std::tm tm;
    localtime_s(&tm, &tt);
    std::wstringstream ss;
    ss << L"screenshots/screen_"
       << std::put_time(&tm, L"%Y%m%d_%H%M%S") << L".png";

    bmp.Save(ss.str().c_str(), &pngClsid, NULL);

    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);
}

// ------------------------ OVERLAY WINDOW PROCEDURE ------------------------
LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_LBUTTONDOWN:
        selecting = true;
        startPt.x = LOWORD(lParam);
        startPt.y = HIWORD(lParam);
        endPt = startPt;
        SetCapture(hwnd);
        break;

    case WM_MOUSEMOVE:
        if (selecting)
        {
            endPt.x = LOWORD(lParam);
            endPt.y = HIWORD(lParam);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;

    case WM_LBUTTONUP:
        if (selecting)
        {
            selecting = false;
            ReleaseCapture();
            endPt.x = LOWORD(lParam);
            endPt.y = HIWORD(lParam);

            int x = min(startPt.x, endPt.x);
            int y = min(startPt.y, endPt.y);
            int w = abs(startPt.x - endPt.x);
            int h = abs(startPt.y - endPt.y);

            SaveScreenshot(x, y, w, h);
            ShowWindow(hwnd, SW_HIDE); // Sakrij overlay nakon snimanja
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (selecting)
        {
            HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 255));
            FrameRect(hdc, &RECT{ startPt.x,startPt.y,endPt.x,endPt.y }, hBrush);
            DeleteObject(hBrush);
        }
        EndPaint(hwnd, &ps);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ------------------------ CONTROL PANEL WINDOW ------------------------
LRESULT CALLBACK ControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 1: // Snip Region
        {
            ShowWindow(hwndOverlay, SW_SHOW);
        }
        break;
        case 2: // Full Screen
        {
            int w = GetSystemMetrics(SM_CXSCREEN);
            int h = GetSystemMetrics(SM_CYSCREEN);
            SaveScreenshot(0, 0, w, h);
        }
        break;
        case 3: // Exit
            PostQuitMessage(0);
            break;
        }
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ------------------------ MAIN ------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Registracija hotkey *
    RegisterHotKey(NULL, 1, 0, VK_MULTIPLY);

    // Registracija overlay prozora
    WNDCLASS wcOverlay = { 0 };
    wcOverlay.lpfnWndProc = OverlayWndProc;
    wcOverlay.hInstance = hInstance;
    wcOverlay.lpszClassName = "SnipOverlay";
    RegisterClass(&wcOverlay);

    hwndOverlay = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        "SnipOverlay",
        "",
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInstance, NULL
    );
    SetLayeredWindowAttributes(hwndOverlay, 0, 120, LWA_ALPHA);
    ShowWindow(hwndOverlay, SW_HIDE); // Sakrij overlay dok se ne klikne Snip Region

    // Registracija kontrolnog prozora
    WNDCLASS wcControl = { 0 };
    wcControl.lpfnWndProc = ControlWndProc;
    wcControl.hInstance = hInstance;
    wcControl.lpszClassName = "ControlWindow";
    RegisterClass(&wcControl);

    HWND hwndControl = CreateWindow(
        "ControlWindow",
        "Snipping Tool Control",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        250, 120,
        NULL, NULL, hInstance, NULL
    );

    // Dugmad
    CreateWindow("BUTTON", "Snip Region", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        20, 20, 100, 30, hwndControl, (HMENU)1, hInstance, NULL);
    CreateWindow("BUTTON", "Full Screen", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        130, 20, 100, 30, hwndControl, (HMENU)2, hInstance, NULL);
    CreateWindow("BUTTON", "Exit", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        75, 60, 100, 30, hwndControl, (HMENU)3, hInstance, NULL);

    ShowWindow(hwndControl, SW_SHOW);
    UpdateWindow(hwndControl);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_HOTKEY && msg.wParam == 1)
        {
            int w = GetSystemMetrics(SM_CXSCREEN);
            int h = GetSystemMetrics(SM_CYSCREEN);
            SaveScreenshot(0, 0, w, h);
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return 0;
}
