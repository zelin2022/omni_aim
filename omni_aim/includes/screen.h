#pragma once

#include "framework.h"
#include "Objidl.h"
#include <fstream>
#include "gdiplus.h"
#pragma comment(lib,"gdiplus.lib")

#include <vector>
#include <string>
#include "gdiplusinit.h"
//#include "Gdiplusimaging.h"
class Screen {
private:
	Gdiplus::Rect rect;
	Gdiplus::PixelFormat pixel_format = PixelFormat48bppRGB;
public:
	static BITMAPINFOHEADER createBitmapHeader(int width, int height);
	static HBITMAP GdiPlusScreenCapture(HWND hWnd);
	static bool saveToMemory(HBITMAP* hbitmap, std::vector<BYTE>& data);
	static bool saveToFile(HBITMAP* hbitmap, std::wstring filename);
	static bool saveToFile(std::vector<BYTE>& data, std::wstring filename);



	void set_rect_capture_zone(INT x, INT y, INT w, INT h);
	void bitmap_to_raw(HBITMAP* hbitmap, Gdiplus::BitmapData* out);
	void get_int32_by_3_per_pixel_image(Gdiplus::BitmapData* src, int32_t* buffer);
};