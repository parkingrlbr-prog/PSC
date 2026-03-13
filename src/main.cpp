#include <windows.h>
#include <iostream>
#include <filesystem>

#pragma comment(lib,"gdi32.lib")

void SaveScreenshot(int x,int y,int w,int h)
{
    HDC hScreen = GetDC(NULL);
    HDC hDC = CreateCompatibleDC(hScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreen,w,h);

    SelectObject(hDC,hBitmap);

    BitBlt(hDC,0,0,w,h,hScreen,x,y,SRCCOPY);

    std::filesystem::create_directory("screenshots");

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    infoHeader.biSize = sizeof(infoHeader);
    infoHeader.biWidth = w;
    infoHeader.biHeight = -h;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = BI_RGB;

    int bmpSize = w*h*3;

    char* bmpData = new char[bmpSize];

    GetDIBits(hScreen,hBitmap,0,h,bmpData,(BITMAPINFO*)&infoHeader,DIB_RGB_COLORS);

    HANDLE file = CreateFile("screenshots/screen.bmp",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

    DWORD written;

    fileHeader.bfType = 0x4D42;
    fileHeader.bfOffBits = sizeof(fileHeader)+sizeof(infoHeader);
    fileHeader.bfSize = bmpSize+fileHeader.bfOffBits;

    WriteFile(file,&fileHeader,sizeof(fileHeader),&written,NULL);
    WriteFile(file,&infoHeader,sizeof(infoHeader),&written,NULL);
    WriteFile(file,bmpData,bmpSize,&written,NULL);

    CloseHandle(file);

    delete[] bmpData;

    DeleteObject(hBitmap);
    DeleteDC(hDC);
    ReleaseDC(NULL,hScreen);
}

int main()
{
    std::cout<<"PRINT SCREEN = screenshot\n";

    while(true)
    {
        if(GetAsyncKeyState(VK_SNAPSHOT) & 1)
        {
            std::cout<<"1 = cijeli ekran\n";
            std::cout<<"2 = dio ekrana\n";

            int choice;
            std::cin>>choice;

            if(choice==1)
            {
                int w = GetSystemMetrics(SM_CXSCREEN);
                int h = GetSystemMetrics(SM_CYSCREEN);

                SaveScreenshot(0,0,w,h);
            }

            if(choice==2)
            {
                int x,y,w,h;

                std::cout<<"X: ";
                std::cin>>x;

                std::cout<<"Y: ";
                std::cin>>y;

                std::cout<<"Width: ";
                std::cin>>w;

                std::cout<<"Height: ";
                std::cin>>h;

                SaveScreenshot(x,y,w,h);
            }

            std::cout<<"Screenshot sacuvan\n";
        }

        Sleep(100);
    }
}
