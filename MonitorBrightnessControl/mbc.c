#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#include <initguid.h>
#include <Windows.h>
#include <windowsx.h>
#include <strsafe.h>
#include <physicalmonitorenumerationapi.h>
#include <highlevelmonitorconfigurationapi.h>
#pragma comment(lib, "Dxva2.lib")
#include <Shlobj_core.h>
#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")
#include "resource.h"
#define APPLICATION_NAME TEXT("Monitor Brightness Control")
#define CLASS_NAME TEXT("MonitorBrightnessControl")
#define error_printf printf

#define WIDTH 400
#define HEIGHT 40
#define ID_TRACKBAR 200
#define TBS_ORIENT TBS_HORZ
#define STEP 5

DEFINE_GUID(GUID_NULL,
    0x00000000,
    0x0000, 0x0000, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
);

DEFINE_GUID(__uuidof_TaskbarList,
    0x56FDF344,
    0xFD6D, 0x11d0, 0x95, 0x8A,
    0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90
);

DEFINE_GUID(__uuidof_ITaskbarList,
    0x56FDF342,
    0xFD6D, 0x11d0, 0x95, 0x8A,
    0x00, 0x60, 0x97, 0xC9, 0xA0, 0x90
);

HWND hMainWindow;

BOOL CALLBACK MonitorBrightness(
    HMONITOR unnamedParam1,
    HDC unnamedParam2,
    LPRECT unnamedParam3,
    LPARAM unnamedParam4
)
{
    LPPHYSICAL_MONITOR pPhysicalMonitors = NULL;
    DWORD cPhysicalMonitors;

    // Get the number of physical monitors.
    BOOL bSuccess = GetNumberOfPhysicalMonitorsFromHMONITOR(
        unnamedParam1,
        &cPhysicalMonitors
    );

    if (bSuccess)
    {
        // Allocate the array of PHYSICAL_MONITOR structures.
        pPhysicalMonitors = (LPPHYSICAL_MONITOR)malloc(
            cPhysicalMonitors * sizeof(PHYSICAL_MONITOR));

        if (pPhysicalMonitors != NULL)
        {
            // Get the array.
            bSuccess = GetPhysicalMonitorsFromHMONITOR(
                unnamedParam1, cPhysicalMonitors, pPhysicalMonitors);

            if (unnamedParam4 > 100)
            {
                DWORD min, max, cur = 0;
                GetMonitorBrightness(
                    pPhysicalMonitors[0].hPhysicalMonitor,
                    &min,
                    &cur,
                    &max
                );
                DWORD* c = (DWORD*)unnamedParam4;
                *c = cur;
                return FALSE;
            }

            // Use the monitor handles (not shown).
            SetMonitorBrightness(
                pPhysicalMonitors[0].hPhysicalMonitor, unnamedParam4
            );

            // Close the monitor handles.
            bSuccess = DestroyPhysicalMonitors(
                cPhysicalMonitors,
                pPhysicalMonitors);

            // Free the array.
            free(pPhysicalMonitors);
        }
    }
    return TRUE;
}

// https://gist.github.com/ethanhs/0e157e4003812e99bf5bc7cb6f73459f
void SetWindowBlur(HWND hWnd)
{
    const HINSTANCE hModule = LoadLibrary(TEXT("user32.dll"));
    if (hModule)
    {
        typedef struct
        {
            int nAccentState;
            int nFlags;
            int nColor;
            int nAnimationId;
        } ACCENTPOLICY;
        typedef struct
        {
            int nAttribute;
            PVOID pData;
            ULONG ulDataSize;
        } WINCOMPATTRDATA;
        typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
        const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hModule, "SetWindowCompositionAttribute");
        if (SetWindowCompositionAttribute)
        {
            ACCENTPOLICY policy = { 4, 0, (128 << 24) | (0XB32E9A & 0xFFFFFF), 0 }; // ACCENT_ENABLE_BLURBEHIND=3...
            WINCOMPATTRDATA data = { 19, &policy, sizeof(ACCENTPOLICY) }; // WCA_ACCENT_POLICY=19
            SetWindowCompositionAttribute(hWnd, &data);
        }
        FreeLibrary(hModule);
    }
}

LRESULT CALLBACK WindowProc(
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    switch (uMsg)
    {
    case WM_CTLCOLORSTATIC:
    {
        LONG_PTR ptr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
        HWND hWndTrack = *((HWND*)(ptr));
        if ((HWND)lParam == hWndTrack)
        {
            SetBkColor((HDC)wParam, RGB(0, 0, 0));
            return (BOOL)GetStockObject(BLACK_BRUSH);
        }
        break;
    }
    case WM_CREATE:
    {
        HWND* hWndTrack;
        CREATESTRUCT* pCreate = (CREATESTRUCT*)(lParam);
        hWndTrack = (HWND*)(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)hWndTrack);
    }
    case WM_USER:
    {
        switch (lParam)
        {
        case WM_LBUTTONUP:
        {
            NOTIFYICONIDENTIFIER nii;
            nii.cbSize = sizeof(NOTIFYICONIDENTIFIER);
            nii.hWnd = hwnd;
            nii.uID = 0;
            nii.guidItem = GUID_NULL;
            RECT loc;
            Shell_NotifyIconGetRect(
                &nii,
                &loc
            );
            if (IsWindowVisible(hwnd))
            {
                ShowWindow(
                    hwnd,
                    SW_HIDE
                );
            }
            else
            {
                ShowWindow(
                    hwnd,
                    SW_SHOW
                );
                UpdateWindow(hwnd);
                SetForegroundWindow(hwnd);
                SetWindowPos(
                    hwnd,
                    HWND_TOPMOST,
                    loc.left + (loc.right - loc.left) / 2 - WIDTH / 2,
                    loc.top - HEIGHT,
                    WIDTH,
                    HEIGHT,
                    SWP_SHOWWINDOW | SWP_FRAMECHANGED
                );
            }
            break;
        }
        }
        break;
    }
    case WM_KEYDOWN:
    {
        if (wParam == VK_ESCAPE)
        {
            ShowWindow(
                hwnd,
                SW_HIDE
            );
        }
        else if (wParam == VK_LEFT || wParam == VK_DOWN || wParam == VK_RIGHT || wParam == VK_UP)
        {
            LONG_PTR ptr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
            HWND hWndTrack = *((HWND*)(ptr));
            int val = SendMessage(
                hWndTrack,
                TBM_GETPOS,
                0,
                0
            );
            val += ((wParam == VK_LEFT || wParam == VK_DOWN) ? -STEP : STEP);
            if (val > 100) val = 100;
            if (val < 0) val = 0;
            if (TBS_ORIENT == TBS_VERT)
            {
                val = 100 - val;
            }
            SendMessage(
                hWndTrack,
                TBM_SETPOS,
                TRUE,
                (LPARAM)val
            );
            EnumDisplayMonitors(
                NULL,
                NULL,
                MonitorBrightness,
                val
            );
        }
    }
    case WM_ACTIVATE:
    {
        if (
            wParam == WA_INACTIVE &&
            FindWindow(TEXT("Shell_TrayWnd"), NULL) != GetForegroundWindow()
            )
        {
            ShowWindow(
                hwnd,
                SW_HIDE
            );
        }
        break;
    }
    case WM_HSCROLL:
    case WM_VSCROLL:
    {
        LONG_PTR ptr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
        HWND hWndTrack = *((HWND*)(ptr));
        DWORD val = SendMessage(
            hWndTrack,
            TBM_GETPOS,
            0,
            0
        );
        if (TBS_ORIENT == TBS_VERT)
        {
            val = 100 - val;
        }
        EnumDisplayMonitors(
            NULL,
            NULL,
            MonitorBrightness,
            val
        );
        // disable dashed outline
        if (IsWindowVisible(hwnd)) SendMessage(hWndTrack, WM_CHANGEUISTATE, (WPARAM)(0x10001), (LPARAM)(0));
        break;
    }
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mi = (MINMAXINFO*)lParam;
        mi->ptMinTrackSize.x = 0;
        mi->ptMinTrackSize.y = 0;
        return 0;
    }
    case WM_NCCALCSIZE:
    {
        return 0;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        break;
    }
    default:
    {
        return DefWindowProc(
            hwnd,
            uMsg,
            wParam,
            lParam
        );
    }
    }
    return 0;
}

HWND WINAPI CreateTrackbar(
    HINSTANCE hInstance,
    HWND hwndDlg,  // handle of dialog box (parent window) 
    UINT iMin,     // minimum value in trackbar range 
    UINT iMax     // maximum value in trackbar range 
)
{

    InitCommonControls(); // loads common control's DLL 

    HWND hwndTrack = CreateWindowEx(
        0,                               // no extended styles 
        TRACKBAR_CLASS,                  // class name 
        L"Trackbar Control",              // title (caption) 
        WS_CHILD |
        WS_VISIBLE | TBS_NOTICKS | TBS_BOTH | SS_LEFT | TBS_ORIENT | (TBS_ORIENT == TBS_HORZ ? TBS_DOWNISLEFT : 0) |
        0,              // style 
        0, 0,                          // position 
        WIDTH, HEIGHT,                         // size 
        hwndDlg,                         // parent window 
        ID_TRACKBAR,                     // control identifier 
        hInstance,                       // instance 
        NULL                             // no WM_CREATE parameter 
    );

    SendMessage(hwndTrack, TBM_SETRANGE,
        (WPARAM)TRUE,                   // redraw flag 
        (LPARAM)MAKELONG(iMin, iMax));  // min. & max. positions

    SendMessage(hwndTrack, TBM_SETPAGESIZE,
        0, (LPARAM)4);                  // new page size 

    DWORD val = 0;
    EnumDisplayMonitors(
        NULL,
        NULL,
        MonitorBrightness,
        &val
    );
    if (TBS_ORIENT == TBS_VERT)
    {
        val = 100 - val;
    }
    SendMessage(
        hwndTrack,
        TBM_SETPOS,
        TRUE,
        (LPARAM)val
    );

    SetFocus(hwndTrack);

    return hwndTrack;
}

LRESULT CALLBACK LowLevelMouseProc(
    _In_ int    nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
) 
{
    NOTIFYICONIDENTIFIER nid;
    nid.cbSize = sizeof(nid);
    nid.hWnd = hMainWindow;
    nid.uID = 0;
    nid.guidItem = GUID_NULL;
    RECT rc;
    PMSLLHOOKSTRUCT pMsll = (PMSLLHOOKSTRUCT)lParam;

    if (nCode == HC_ACTION && 
        (wParam == WM_MOUSEWHEEL || wParam == WM_MOUSEHWHEEL) &&
        SUCCEEDED(Shell_NotifyIconGetRect(&nid, &rc)) &&
        PtInRect(&rc, pMsll->pt) &&
        RegisterWindowMessageW(L"ToolbarWindow32") == GetClassLongPtrW(WindowFromPoint(*(POINT*)&rc), GCW_ATOM))
    {
        int factor = GET_WHEEL_DELTA_WPARAM(pMsll->mouseData) / 120;       
        LONG_PTR ptr = GetWindowLongPtrW(hMainWindow, GWLP_USERDATA);
        HWND hWndTrack = *((HWND*)(ptr));
        int val = SendMessageW(hWndTrack, TBM_GETPOS, 0, 0);
        val += (factor * STEP);
        if (val > 100) val = 100;
        if (val < 0) val = 0;
        if (TBS_ORIENT == TBS_VERT) val = 100 - val;
        SendMessageW(hWndTrack, TBM_SETPOS, IsWindowVisible(hMainWindow), (LPARAM)val);
        PostMessageW(hMainWindow, WM_VSCROLL, 0, 0);
    }

    return CallNextHookEx(0, nCode, wParam, lParam);
}

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd
) 
{
    /*
    FILE* conout;
    AllocConsole();
    freopen_s(
        &conout,
        "CONOUT$",
        "w",
        stdout
    );
    */

    int ch;

    SetProcessDpiAwarenessContext(
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
    );

    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        error_printf(
            "%s:%d: %s (0x%x).\n",
            __FILE__,
            __LINE__,
            "CoInitialize",
            hr
        );
        ch = _getch();
        return hr;
    }

    ITaskbarList* pTaskList = NULL;
    hr = CoCreateInstance(
        &__uuidof_TaskbarList,
        NULL,
        CLSCTX_ALL,
        &__uuidof_ITaskbarList,
        (void**)(&pTaskList)
    );
    if (FAILED(hr))
    {
        error_printf(
            "%s:%d: %s (0x%x).\n",
            __FILE__,
            __LINE__,
            "CoCreateInstance",
            hr
        );
        ch = _getch();
        return hr;
    }
    pTaskList->lpVtbl->HrInit(pTaskList);

    HICON icon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

    WNDCLASS wndClass = { 0 };
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = icon;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = CLASS_NAME;

    HWND hWndTrack;
    HWND hWnd = CreateWindowEx(
        0,
        (LPCWSTR)(
            MAKEINTATOM(
                RegisterClass(&wndClass)
            )
            ),
        CLASS_NAME,
        WS_OVERLAPPEDWINDOW,
        0, 0, WIDTH, HEIGHT,
        NULL,
        NULL,
        hInstance,
        &hWndTrack
    );
    hMainWindow = hWnd;
    SetWindowBlur(hWnd);
    hWndTrack = CreateTrackbar(
        hInstance,
        hWnd,
        0, 100
    );

    pTaskList->lpVtbl->DeleteTab(pTaskList, hWnd);

    NOTIFYICONDATA tnid;
    tnid.cbSize = sizeof(NOTIFYICONDATA);
    tnid.hWnd = hWnd;
    tnid.uID = 0;
    tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnid.uCallbackMessage = WM_USER;
    tnid.hIcon = icon;
    StringCbCopyN(
        tnid.szTip,
        sizeof(tnid.szTip),
        APPLICATION_NAME,
        sizeof(APPLICATION_NAME)
    );
    Shell_NotifyIcon(
        NIM_ADD,
        &tnid
    );

    HHOOK hHook = SetWindowsHookExW(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
    MSG msg;
    BOOL bRet;
    while ((bRet = GetMessage(
        &msg,
        NULL,
        0,
        0)) != 0)
    {
        // An error occured
        if (bRet == -1)
        {
            break;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    UnhookWindowsHookEx(hHook);

    Shell_NotifyIcon(
        NIM_DELETE,
        &tnid
    );
    pTaskList->lpVtbl->Release(pTaskList);
    DestroyIcon(icon);
    CoUninitialize();

    return 0;
}