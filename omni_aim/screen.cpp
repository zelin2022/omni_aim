#include "screen.h"
#include <algorithm>

BITMAPINFOHEADER Screen::createBitmapHeader(int width, int height)

{

    BITMAPINFOHEADER  bi;


    // create a bitmap

    bi.biSize = sizeof(BITMAPINFOHEADER);

    bi.biWidth = width;

    bi.biHeight = -height;  //this is the line that makes it draw upside down or not

    bi.biPlanes = 1;

    bi.biBitCount = 32;

    bi.biCompression = BI_RGB;

    bi.biSizeImage = 0;

    bi.biXPelsPerMeter = 0;

    bi.biYPelsPerMeter = 0;

    bi.biClrUsed = 0;

    bi.biClrImportant = 0;


    return bi;

}


HBITMAP Screen::GdiPlusScreenCapture(HWND hWnd)

{

    // get handles to a device context (DC)

    HDC hwindowDC = GetDC(nullptr);

    HDC hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);

    SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);


    // define scale, height and width

    int scale = 1;

    int screenx = 0;

    int screeny = 0;

    int width = 1920;

    int height = 1080;


    // create a bitmap

    HBITMAP hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);

    BITMAPINFOHEADER bi = createBitmapHeader(width, height);


    // use the previously created device context with the bitmap

    SelectObject(hwindowCompatibleDC, hbwindow);


    // Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that call HeapAlloc using a handle to the process's default heap.

    // Therefore, GlobalAlloc and LocalAlloc have greater overhead than HeapAlloc.

    DWORD dwBmpSize = ((width * bi.biBitCount + 31) / 32) * 4 * height;

    HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);

    char* lpbitmap = (char*)GlobalLock(hDIB);


    // copy from the window device context to the bitmap device context

    StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, screenx, screeny, width, height, SRCCOPY);   //change SRCCOPY to NOTSRCCOPY for wacky colors !

    GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, lpbitmap, (BITMAPINFO*)&bi, DIB_RGB_COLORS);


    // avoid memory leak

    DeleteDC(hwindowCompatibleDC);

    ReleaseDC(hWnd, hwindowDC);


    return hbwindow;

}


bool Screen::saveToMemory(HBITMAP* hbitmap, std::vector<BYTE>& data)
{

    auto bmp = Gdiplus::Bitmap::FromHBITMAP(*hbitmap, nullptr);

    if (!bmp) {
        return false;
    }


    // write to IStream

    IStream *istream = nullptr;

    CreateStreamOnHGlobal(NULL, TRUE, &istream);


    // define encoding

    CLSID clsid;

    CLSIDFromString(L"{557cf401-1a04-11d3-9a73-0000f81ef32e}", &clsid); 



    auto status = bmp->Save(istream, &clsid, NULL);

    if (status) {
        return false;
    }
    LARGE_INTEGER   li = { 0 };
    ULARGE_INTEGER sz;
    istream->Seek(li, STREAM_SEEK_END, &sz); // get data size by seeking to the end of stream
    istream->Seek(li, STREAM_SEEK_SET, nullptr); // seek back to the start
    data.resize(sz.QuadPart);
    ULONG out;
    istream->Read(data.data(), sz.QuadPart, &out);

    if (sz.QuadPart != out) {
        std::string mout("mismatch read size: want ");
        mout += std::to_string(sz.QuadPart);
        mout += ", actual ";
        mout += std::to_string(out);
        auto wmout = std::wstring(mout.begin(), mout.end());
        MessageBox(nullptr, wmout.c_str(), TEXT("status"), MB_OK);
        return false;
    }
    


    istream->Release();

    return true;

}


bool Screen::saveToFile(HBITMAP* hbitmap, std::wstring filename){
    auto bmp = Gdiplus::Bitmap::FromHBITMAP(*hbitmap, nullptr);
    if (!bmp) {
        return false;
    }
    // define encoding

    CLSID clsid;

    CLSIDFromString(L"{557cf401-1a04-11d3-9a73-0000f81ef32e}", &clsid);

    auto status2 = bmp->Save(filename.c_str(), &clsid, NULL);
    return true;
}
bool Screen::saveToFile(std::vector<BYTE>& data, std::wstring filename){
    std::ofstream os(filename, std::ios_base::out | std::ios_base::binary);
    os.write((const char*)data.data(), data.size());
    return true;
}


void Screen::set_rect_capture_zone(INT x, INT y, INT w, INT h) {
    this->rect = Gdiplus::Rect(x, y, w, h);
}

void Screen::bitmap_to_raw(HBITMAP* hbitmap, Gdiplus::BitmapData* out){
    auto bmp = Gdiplus::Bitmap::FromHBITMAP(*hbitmap, nullptr);
    if (bmp == nullptr) {
        throw std::runtime_error("converted into null bmp");
    }

    Gdiplus::Status s = bmp->LockBits(&this->rect, Gdiplus::ImageLockModeRead, this->pixel_format, out);
    if (s == Gdiplus::Status::Ok) {
    }
    else {
        Gdiplus::Size sz;
        this->rect.GetSize(&sz);
        Gdiplus::Point pt;
        this->rect.GetLocation(&pt);
        std::string msg("status: ");
        msg += std::to_string(s);
        msg += "\nSize: ";
        msg += std::to_string(sz.Width);
        msg += ", ";
        msg += std::to_string(sz.Height);
        msg += "\nLocation: ";
        msg += std::to_string(pt.X);
        msg += ", ";
        msg += std::to_string(pt.Y);
        auto wmout = std::wstring(msg.begin(), msg.end());
        MessageBox(nullptr, wmout.c_str(), TEXT("bad status on LockBits"), MB_OK);
    }

}

void Screen::get_int32_by_3_per_pixel_image(Gdiplus::BitmapData* src, int32_t* buffer) {

    // src->scan0 will be in PixelFormat48bppRGB or 48 bits per pixel, meaning each color is 16 bits

    int total_pixel = src->Width * src->Height;

    // 1 pixel is 32*3 = 96 bits or 12 bytes

    try {
        std::copy((int16_t*)src->Scan0, (int16_t*)src->Scan0 + total_pixel*3, buffer);
    }
    catch (std::exception &e) {
        std::string msg("exception: ");
        msg += e.what();
        auto wmout = std::wstring(msg.begin(), msg.end());
        MessageBox(nullptr, wmout.c_str(), TEXT("std copy"), MB_OK);
    }
}
