#include <windows.h>
#include <gdiplus.h>
#include <iostream>

#pragma comment(lib,"gdiplus.lib")

using namespace Gdiplus;

POINT startPt;
POINT endPt;
bool selecting=false;

ULONG_PTR gdiplusToken;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_LBUTTONDOWN:
        selecting=true;
        startPt.x=LOWORD(lParam);
        startPt.y=HIWORD(lParam);
        endPt=startPt;
        break;

        case WM_MOUSEMOVE:
        if(selecting)
        {
            endPt.x=LOWORD(lParam);
            endPt.y=HIWORD(lParam);
            InvalidateRect(hwnd,NULL,TRUE);
        }
        break;

        case WM_LBUTTONUP:
        selecting=false;

        {
            int x=min(startPt.x,endPt.x);
            int y=min(startPt.y,endPt.y);
            int w=abs(startPt.x-endPt.x);
            int h=abs(startPt.y-endPt.y);

            HDC hScreen=GetDC(NULL);
            HDC hDC=CreateCompatibleDC(hScreen);
            HBITMAP hBitmap=CreateCompatibleBitmap(hScreen,w,h);

            SelectObject(hDC,hBitmap);

            BitBlt(hDC,0,0,w,h,hScreen,x,y,SRCCOPY);

            Bitmap bmp(hBitmap,NULL);

            CLSID pngClsid;
            UINT num, size;
            GetImageEncodersSize(&num,&size);

            ImageCodecInfo* pImageCodecInfo=(ImageCodecInfo*)(malloc(size));
            GetImageEncoders(num,size,pImageCodecInfo);

            for(UINT j=0;j<num;++j)
            {
                if(wcscmp(pImageCodecInfo[j].MimeType,L"image/png")==0)
                {
                    pngClsid=pImageCodecInfo[j].Clsid;
                    break;
                }
            }

            bmp.Save(L"screenshot.png",&pngClsid,NULL);

            free(pImageCodecInfo);

            DeleteDC(hDC);
            ReleaseDC(NULL,hScreen);
            DeleteObject(hBitmap);

            PostQuitMessage(0);
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc=BeginPaint(hwnd,&ps);

            if(selecting)
            {
                Rectangle(hdc,
                          startPt.x,
                          startPt.y,
                          endPt.x,
                          endPt.y);
            }

            EndPaint(hwnd,&ps);
        }
        break;

        case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd,msg,wParam,lParam);
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE,LPSTR,int)
{
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken,&gdiplusStartupInput,NULL);

    RegisterHotKey(NULL,1,0,VK_MULTIPLY); // * dugme

    MSG msg;

    while(GetMessage(&msg,NULL,0,0))
    {
        if(msg.message==WM_HOTKEY)
        {
            WNDCLASS wc={0};

            wc.lpfnWndProc=WndProc;
            wc.hInstance=hInstance;
            wc.lpszClassName="snip";

            RegisterClass(&wc);

            HWND hwnd=CreateWindowEx(
                WS_EX_TOPMOST|WS_EX_LAYERED,
                "snip",
                "",
                WS_POPUP,
                0,0,
                GetSystemMetrics(SM_CXSCREEN),
                GetSystemMetrics(SM_CYSCREEN),
                NULL,NULL,hInstance,NULL);

            SetLayeredWindowAttributes(hwnd,0,120,LWA_ALPHA);

            ShowWindow(hwnd,SW_SHOW);
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);

    return 0;
}
