#include <windows.h>
#include <gdiplus.h>
#include <filesystem>
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>

#pragma comment(lib,"gdiplus.lib")

using namespace Gdiplus;

POINT startPt, endPt;
bool selecting = false;
HWND hwndOverlay;

ULONG_PTR gdiplusToken;

// Funkcija za dobijanje CLSID PNG encoder-a
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0;
    UINT size = 0;
    GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j)
    {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}

// Funkcija za screenshot
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

    // Naziv fajla: screenshots/screen_YYYYMMDD_HHMMSS.png
    auto t = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(t);
    std::tm tm;
    localtime_s(&tm, &tt);
    std::stringstream ss;
    ss << "screenshots/screen_"
       << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".png";
    std::wstring filename(ss.str().begin(), ss.str().end());

    bmp.Save(filename.c_str(), &pngClsid, NULL);

    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(NULL, hScreen);
}

// Overlay Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (selecting)
        {
            // Prozirni overlay + pravougaonik
            HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 255));
            SetBkMode(hdc, TRANSPARENT);
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Registracija hotkey *
    RegisterHotKey(NULL, 1, 0, VK_MULTIPLY);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "SnipOverlay";

    RegisterClass(&wc);

    hwndOverlay = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        "SnipOverlay",
        "Snipping Tool",
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        NULL, NULL, hInstance, NULL
    );

    SetLayeredWindowAttributes(hwndOverlay, 0, 120, LWA_ALPHA);
    ShowWindow(hwndOverlay, SW_SHOW);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_HOTKEY)
        {
            int w = GetSystemMetrics(SM_CXSCREEN);
            int h = GetSystemMetrics(SM_CYSCREEN);
            SaveScreenshot(0, 0, w, h); // cijeli ekran
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return 0;
}
