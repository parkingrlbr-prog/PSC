#include <windows.h>
#include <iostream>
#include <filesystem>

#pragma comment(lib, "gdi32.lib")

void screenshot()
{
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HDC hScreen = GetDC(NULL);
    HDC hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, width, height);

    SelectObject(hDC, hBitmap);

    BitBlt(hDC,0,0,width,height,hScreen,0,0,SRCCOPY);

    std::filesystem::create_directory("screenshots");

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    infoHeader.biSize = sizeof(infoHeader);
    infoHeader.biWidth = width;
    infoHeader.biHeight = -height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = BI_RGB;

    int bmpSize = width * height * 3;

    char* bmpData = new char[bmpSize];

    GetDIBits(hScreen,hBitmap,0,height,bmpData,(BITMAPINFO*)&infoHeader,DIB_RGB_COLORS);

    HANDLE file = CreateFile("screenshots/screen.bmp",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

    DWORD written;

    fileHeader.bfType = 0x4D42;
    fileHeader.bfOffBits = sizeof(fileHeader) + sizeof(infoHeader);
    fileHeader.bfSize = bmpSize + fileHeader.bfOffBits;

    WriteFile(file,&fileHeader,sizeof(fileHeader),&written,NULL);
    WriteFile(file,&infoHeader,sizeof(infoHeader),&written,NULL);
    WriteFile(file,bmpData,bmpSize,&written,NULL);

    CloseHandle(file);

    delete[] bmpData;

    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(NULL,hScreen);

    std::cout << "Screenshot sacuvan\n";
}

int main()
{
    std::cout<<"F12 = screenshot\n";

    while(true)
    {
        if(GetAsyncKeyState(VK_F12) & 1)
        {
            screenshot();
        }

        Sleep(100);
    }
}
