// omni_aim.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "omni_aim.h"
// my includes
#include "screen.h"
#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include "gdiplusinit.h"
#include "predictor.h"
#include "aim.h"

#include <sstream> // debug

using namespace Gdiplus;
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

UINT G_USE_KEY = VK_OEM_3;
UINT G_CALIBRATE_KEY = 0x50; // p key
INT G_RECT_WIDTH = 768;
void draw_control(HWND&);
void set_key(HWND&);
void cap_predict_aim(HWND&);
void init_gdiplus();
void cap_screen(HWND& h, int32_t* buffer);
void init_image_buffer(int32_t*& ptr);
Predictor* G_PREDICTOR = nullptr;
Screen* G_SCREEN = nullptr;
int32_t* G_input_blob_buffer = nullptr;
void aim_at_prediction(std::vector<Humanoid>& p_result);

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_OMNIAIM, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_OMNIAIM));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_OMNIAIM));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_OMNIAIM);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 400, 600, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   init_gdiplus();
   set_key(hWnd);
   G_SCREEN = new Screen();
   //
   G_SCREEN->set_rect_capture_zone(1920 / 2 - G_RECT_WIDTH / 2, 1080 / 2 - G_RECT_WIDTH / 2, G_RECT_WIDTH, G_RECT_WIDTH);
   //
   G_PREDICTOR = new Predictor();
   G_PREDICTOR->load();
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_HOTKEY:
        if (HIWORD(lParam) == G_USE_KEY) {
            cap_predict_aim(hWnd);
        }
        if (HIWORD(lParam) == G_CALIBRATE_KEY) {
            Aim::move_cursor(1920/2, 0);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            draw_control(hWnd);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        delete(G_PREDICTOR);
        delete(G_SCREEN);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


void draw_control(HWND& h) {
    // display size
    // precision high/low
    // bind key
}

void init_gdiplus() {
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

void set_key(HWND& h) {
    RegisterHotKey(h, 0, 0, G_USE_KEY);
    RegisterHotKey(h, 0, 0, G_CALIBRATE_KEY);
}



// DEBUG
void print_pointer(void* a) {
    const void* address = static_cast<const void*>(a);
    std::stringstream ss;
    ss << address;
    std::string msg = ss.str();
    std::wstring wmsg = std::wstring(msg.begin(), msg.end());
    MessageBox(nullptr, wmsg.c_str(), TEXT("pp0"), MB_OK);
}

void cap_predict_aim(HWND& h) {
    if (G_input_blob_buffer == nullptr) {
        init_image_buffer(G_input_blob_buffer);
    }
    // capture screen
    cap_screen(h, G_input_blob_buffer);

    //std::ofstream osf;
    //osf.open("K:\saveimg2", std::ios_base::out | std::ios_base::binary);
    //osf.write((const char*)G_input_blob_buffer, G_RECT_WIDTH * G_RECT_WIDTH * 3 * sizeof(int32_t));
    //osf << std::flush;
    //osf.close();

    // setup
    G_PREDICTOR->setup(G_input_blob_buffer, G_RECT_WIDTH);




    // predict 
    std::vector<Humanoid> predict_result = G_PREDICTOR->predict();
    G_input_blob_buffer = nullptr;
    // print_pointer(G_input_blob_buffer);
    // 
    // aim and click
    aim_at_prediction(predict_result);
}


// DEBUG
void print_pixel_data(int32_t* big_buff, int16_t* small_buff, int c) {

    for (int i = 0; i < G_RECT_WIDTH * G_RECT_WIDTH * 3; i++) {
        if (big_buff[i] != small_buff[i]) {
            std::string msg;
            msg += "big: " + std::to_string(big_buff[i]) + "\n";
            msg += "small: " + std::to_string(small_buff[i]) + "\n";
            std::wstring ws(msg.begin(), msg.end());
            MessageBox(nullptr, ws.c_str(), TEXT("COMPARE wrong"), MB_OK);
        }
    }

    std::string msg;
    msg += "big: " + std::to_string(big_buff[c]) + "\n";
    msg += "small: " + std::to_string(small_buff[c]) + "\n";
    std::wstring ws(msg.begin(), msg.end());
    MessageBox(nullptr, ws.c_str(), TEXT("COMPARE"), MB_OK);
}

void cap_screen(HWND& h, int32_t* buffer) {
    HBITMAP img = Screen::GdiPlusScreenCapture(h);
    Gdiplus::BitmapData* bitmapData = new Gdiplus::BitmapData;
    G_SCREEN->bitmap_to_raw(&img, bitmapData);
    if (bitmapData == nullptr) {
        throw std::runtime_error("captured BitmapData is nullptr");
    }
    G_SCREEN->get_int32_by_3_per_pixel_image(bitmapData, buffer);

    // print_pixel_data(buffer,(int16_t*) bitmapData->Scan0, 7721);
    delete(bitmapData);
}

void init_image_buffer(int32_t *& ptr) {
    if (ptr != nullptr) {
        free(ptr);
    }
    ptr = (int32_t * )malloc( G_RECT_WIDTH * G_RECT_WIDTH * 3 * sizeof(int32_t));
}


float distanceCenterNOSQRT(std::pair<float, float> point) {
    return pow(point.first - 0.5, 2) + pow(point.second - 0.5, 2);
}

void aim_at_prediction(std::vector<Humanoid> & p_result) {
    unsigned int joint_to_aim = 0; // aim at nose 
    float score_threshold = 0.30;  // 35% higher certainty
    std::vector<std::pair<float, float>> valids;

    // std::string msg;
    for (auto& h : p_result) {
        if (h.parts[0].score > score_threshold) {
            valids.push_back({h.parts[0].x, h.parts[0].y});
            // msg += "x: " + std::to_string(h.parts[0].x) + " y: " + std::to_string(h.parts[0].y) + " s: "+ std::to_string(h.parts[0].score) + " \n";
        }
    }
    if (valids.size() == 0) {
        return;
    }
    auto it_max = std::max_element(valids.begin(), valids.end(), [] (auto& a, auto &b) {
        return distanceCenterNOSQRT(a) > distanceCenterNOSQRT(b);
        });

    // msg += "cloest: x: " + std::to_string(it_max->first) + " y: " + std::to_string(it_max->second) + "\n";
    // std::wstring wmsg(msg.begin(), msg.end());
    // MessageBox(nullptr, wmsg.c_str(), TEXT("POINTS DISTANCE"), MB_OK);

    long raw_distance_y = (it_max->first - 0.5) * G_RECT_WIDTH;
    long raw_distance_x = (it_max->second - 0.5) * G_RECT_WIDTH;
    Aim::move_cursor(raw_distance_x, raw_distance_y);
    if ((GetKeyState(VK_LBUTTON) & 0x8000) == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0); // Left click
    }
};