#pragma once
// Captures only the test-owned dialog, never the user's desktop or TS3 client.
void captureDialog(HWND dialog,const wchar_t* path) {
    RedrawWindow(dialog,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN|RDW_UPDATENOW|RDW_ERASE);
    RECT rect{};GetWindowRect(dialog,&rect);int w=rect.right-rect.left,h=rect.bottom-rect.top;
    HDC source=GetDC(dialog),memory=CreateCompatibleDC(source);HBITMAP bitmap=CreateCompatibleBitmap(source,w,h);
    HGDIOBJ previous=SelectObject(memory,bitmap);assert(PrintWindow(dialog,memory,0));SelectObject(memory,previous);
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    std::vector<unsigned char> pixels(static_cast<size_t>(w)*h*4);assert(GetDIBits(memory,bitmap,0,h,pixels.data(),&info,DIB_RGB_COLORS));
    BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+static_cast<DWORD>(pixels.size());
    std::ofstream file(std::filesystem::path(path),std::ios::binary);file.write(reinterpret_cast<const char*>(&header),sizeof(header));file.write(reinterpret_cast<const char*>(&info.bmiHeader),sizeof(BITMAPINFOHEADER));file.write(reinterpret_cast<const char*>(pixels.data()),pixels.size());
    DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(dialog,source);
}
