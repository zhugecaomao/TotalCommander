#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>

#include <windows.h>
#include <windowsx.h>
using std::min;
using std::max;
#include <gdiplus.h>
#include <atomic>
#include <thread>
#include <string>
#include <memory>

#include "listplug.h"
#include "dxf_collector.h"
#include "dxf_renderer.h"
#include "libdwgr.h"

#pragma comment(lib, "gdiplus.lib")

extern HINSTANCE g_hInst;

// ============================================================================
// GDI+ lifetime management
// ============================================================================

static ULONG_PTR g_gdiplusToken = 0;
static int g_gdiplusRefCount = 0;
static CRITICAL_SECTION g_gdiplusCS;
static bool g_gdiplusCSInit = false;

static void GdiplusAddRef() {
    if (!g_gdiplusCSInit) {
        InitializeCriticalSection(&g_gdiplusCS);
        g_gdiplusCSInit = true;
    }
    EnterCriticalSection(&g_gdiplusCS);
    if (g_gdiplusRefCount == 0) {
        Gdiplus::GdiplusStartupInput input;
        Gdiplus::GdiplusStartup(&g_gdiplusToken, &input, nullptr);
    }
    g_gdiplusRefCount++;
    LeaveCriticalSection(&g_gdiplusCS);
}

static void GdiplusRelease() {
    EnterCriticalSection(&g_gdiplusCS);
    g_gdiplusRefCount--;
    if (g_gdiplusRefCount == 0) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
    LeaveCriticalSection(&g_gdiplusCS);
}

// ============================================================================
// Per-window plugin state
// ============================================================================

static const wchar_t WND_CLASS[] = L"WlxDxfWndClass";
static const UINT WM_DXF_LOAD_DONE = WM_USER + 100;

struct PluginState {
    // Parsed DXF data
    std::unique_ptr<DxfCollector> collector;
    DxfRenderer renderer;

    // Background loading
    std::thread loadThread;
    std::atomic<int> loadState{0};  // 0=running, 1=cancel, 2=done
    int loadGen = 0;

    // Window state
    bool loaded = false;
    bool darkMode = false;

    // Mouse interaction
    bool dragging = false;
    POINT lastMouse = {};
    int dragButton = 0; // 1=left (pan), 2=right

    ~PluginState() {
        if (loadThread.joinable()) {
            int expected = 0;
            loadState.compare_exchange_strong(expected, 1);
            loadThread.join();
        }
    }
};

// ============================================================================
// Background loading thread
// ============================================================================

static void LoadThreadProc(HWND hwnd, std::wstring filePath, PluginState* state, int gen) {
    auto collector = std::make_unique<DxfCollector>();

    // Convert wide path to narrow for libdxfrw
    int len = WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string narrowPath(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, filePath.c_str(), -1, narrowPath.data(), len, nullptr, nullptr);

    // Determine format by file extension
    bool isDwg = false;
    {
        size_t dot = filePath.rfind(L'.');
        if (dot != std::wstring::npos) {
            std::wstring ext = filePath.substr(dot);
            for (auto& c : ext) c = towlower(c);
            isDwg = (ext == L".dwg");
        }
    }

    if (isDwg) {
        dwgR dwg(narrowPath.c_str());
        dwg.read(collector.get(), false);
    } else {
        dxfRW dxf(narrowPath.c_str());
        dxf.read(collector.get(), false);
    }

    // Check if cancelled
    int expected = 0;
    if (state->loadState.compare_exchange_strong(expected, 2)) {
        // Success — transfer ownership via window message
        PostMessageW(hwnd, WM_DXF_LOAD_DONE, (WPARAM)gen, (LPARAM)collector.release());
    } else {
        // Cancelled (state was 1) — we own cleanup, but collector auto-destructs
    }
}

// ============================================================================
// Window procedure
// ============================================================================

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* s = reinterpret_cast<PluginState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_DXF_LOAD_DONE: {
        if (!s) break;
        int gen = (int)wParam;
        if (gen != s->loadGen) {
            // Stale message from old load — free the collector
            delete reinterpret_cast<DxfCollector*>(lParam);
            break;
        }
        if (s->loadThread.joinable())
            s->loadThread.join();

        s->collector.reset(reinterpret_cast<DxfCollector*>(lParam));
        s->loaded = true;

        // Fit to view
        RECT rc;
        GetClientRect(hwnd, &rc);
        s->renderer.fitToView(*s->collector, rc.right, rc.bottom);
        s->renderer.darkMode = s->darkMode;

        InvalidateRect(hwnd, nullptr, FALSE);
        break;
    }

    case WM_SIZE: {
        if (s && s->loaded && s->collector) {
            // Re-fit on resize
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            s->renderer.fitToView(*s->collector, w, h);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);

        if (s && s->loaded && s->collector) {
            // Double-buffered GDI+ render
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
            HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

            {
                Gdiplus::Graphics g(memDC);
                s->renderer.render(g, *s->collector, w, h);
            }

            BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
            SelectObject(memDC, oldBmp);
            DeleteObject(memBmp);
            DeleteDC(memDC);
        } else {
            // Loading or error state
            HBRUSH bgBrush = CreateSolidBrush(
                (s && s->darkMode) ? RGB(30, 30, 30) : RGB(255, 255, 255));
            FillRect(hdc, &rc, bgBrush);
            DeleteObject(bgBrush);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, (s && s->darkMode) ? RGB(200, 200, 200) : RGB(80, 80, 80));
            DrawTextW(hdc, L"Loading...", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        if (!s || !s->loaded) break;
        s->dragging = true;
        s->dragButton = 1;
        s->lastMouse = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        SetCapture(hwnd);
        return 0;
    }

    case WM_RBUTTONDOWN: {
        if (!s || !s->loaded) break;
        s->dragging = true;
        s->dragButton = 2;
        s->lastMouse = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        SetCapture(hwnd);
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (!s || !s->dragging) break;
        POINT cur = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        double dx = (double)(cur.x - s->lastMouse.x);
        double dy = (double)(cur.y - s->lastMouse.y);
        s->lastMouse = cur;

        if (s->dragButton == 1) {
            // Pan: screen pixels → world units
            s->renderer.panX += dx / s->renderer.zoom;
            s->renderer.panY -= dy / s->renderer.zoom;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONUP:
    case WM_RBUTTONUP: {
        if (s && s->dragging) {
            s->dragging = false;
            ReleaseCapture();
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        if (!s || !s->loaded) break;
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        double factor = (delta > 0) ? 1.1 : 1.0 / 1.1;

        // Zoom centered on cursor position
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ScreenToClient(hwnd, &pt);

        RECT rc;
        GetClientRect(hwnd, &rc);
        int h = rc.bottom - rc.top;

        // World position under cursor before zoom
        double wx = pt.x / s->renderer.zoom - s->renderer.panX;
        double wy = (h - pt.y) / s->renderer.zoom - s->renderer.panY;

        s->renderer.zoom *= factor;
        if (s->renderer.zoom < 1e-6) s->renderer.zoom = 1e-6;
        if (s->renderer.zoom > 1e8) s->renderer.zoom = 1e8;

        // Adjust pan so world point stays under cursor
        s->renderer.panX = pt.x / s->renderer.zoom - wx;
        s->renderer.panY = (h - pt.y) / s->renderer.zoom - wy;

        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONDBLCLK: {
        if (!s || !s->loaded || !s->collector) break;
        RECT rc;
        GetClientRect(hwnd, &rc);
        s->renderer.fitToView(*s->collector, rc.right, rc.bottom);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_KEYDOWN: {
        if (!s || !s->loaded) break;
        double panStep = 50.0 / s->renderer.zoom;
        switch (wParam) {
        case VK_LEFT:  s->renderer.panX -= panStep; InvalidateRect(hwnd, nullptr, FALSE); break;
        case VK_RIGHT: s->renderer.panX += panStep; InvalidateRect(hwnd, nullptr, FALSE); break;
        case VK_UP:    s->renderer.panY += panStep; InvalidateRect(hwnd, nullptr, FALSE); break;
        case VK_DOWN:  s->renderer.panY -= panStep; InvalidateRect(hwnd, nullptr, FALSE); break;
        case VK_ADD:
        case VK_OEM_PLUS:
            s->renderer.zoom *= 1.1;
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case VK_SUBTRACT:
        case VK_OEM_MINUS:
            s->renderer.zoom /= 1.1;
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        case VK_HOME: {
            RECT rc;
            GetClientRect(hwnd, &rc);
            s->renderer.fitToView(*s->collector, rc.right, rc.bottom);
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        }
        }
        return 0;
    }

    case WM_NCDESTROY: {
        if (s) {
            GdiplusRelease();
            delete s;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        }
        break;
    }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ============================================================================
// Window class registration
// ============================================================================

static bool g_classRegistered = false;

static void EnsureClassRegistered() {
    if (g_classRegistered) return;

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = g_hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = WND_CLASS;
    RegisterClassExW(&wc);
    g_classRegistered = true;
}

// ============================================================================
// WLX Plugin API exports
// ============================================================================

extern "C" {

void __stdcall ListGetDetectString(char* DetectString, int maxlen) {
    strncpy_s(DetectString, maxlen, "EXT=\"DXF\" | EXT=\"DWG\"", _TRUNCATE);
}

void __stdcall ListSetDefaultParams(ListDefaultParamStruct* /*dps*/) {
    // Nothing to configure
}

HWND __stdcall ListLoadW(HWND ParentWin, WCHAR* FileToLoad, int ShowFlags) {
    EnsureClassRegistered();
    GdiplusAddRef();

    RECT rc;
    GetClientRect(ParentWin, &rc);

    HWND hwnd = CreateWindowExW(0, WND_CLASS, L"",
        WS_CHILD | WS_VISIBLE,
        0, 0, rc.right - rc.left, rc.bottom - rc.top,
        ParentWin, nullptr, g_hInst, nullptr);

    if (!hwnd) {
        GdiplusRelease();
        return nullptr;
    }

    auto* s = new PluginState();
    s->darkMode = (ShowFlags & lcp_darkmode) != 0;
    s->renderer.darkMode = s->darkMode;
    s->loadGen = 1;
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)s);

    // Start background loading
    std::wstring path(FileToLoad);
    s->loadThread = std::thread(LoadThreadProc, hwnd, std::move(path), s, s->loadGen);

    return hwnd;
}

HWND __stdcall ListLoad(HWND ParentWin, char* FileToLoad, int ShowFlags) {
    int wlen = MultiByteToWideChar(CP_ACP, 0, FileToLoad, -1, nullptr, 0);
    std::wstring wpath(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, FileToLoad, -1, wpath.data(), wlen);
    return ListLoadW(ParentWin, wpath.data(), ShowFlags);
}

int __stdcall ListLoadNextW(HWND ParentWin, HWND PluginWin, WCHAR* FileToLoad, int ShowFlags) {
    (void)ParentWin;
    auto* s = reinterpret_cast<PluginState*>(GetWindowLongPtrW(PluginWin, GWLP_USERDATA));
    if (!s) return LISTPLUGIN_ERROR;

    // Cancel previous load
    if (s->loadThread.joinable()) {
        int expected = 0;
        if (!s->loadState.compare_exchange_strong(expected, 1)) {
            // Already done or cancelled
        }
        s->loadThread.join();
    }

    // Reset state
    s->loaded = false;
    s->collector.reset();
    s->loadState.store(0);
    s->loadGen++;
    s->darkMode = (ShowFlags & lcp_darkmode) != 0;
    s->renderer.darkMode = s->darkMode;

    // Start new load
    std::wstring path(FileToLoad);
    s->loadThread = std::thread(LoadThreadProc, PluginWin, std::move(path), s, s->loadGen);

    InvalidateRect(PluginWin, nullptr, TRUE);
    return LISTPLUGIN_OK;
}

int __stdcall ListLoadNext(HWND ParentWin, HWND PluginWin, char* FileToLoad, int ShowFlags) {
    int wlen = MultiByteToWideChar(CP_ACP, 0, FileToLoad, -1, nullptr, 0);
    std::wstring wpath(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, FileToLoad, -1, wpath.data(), wlen);
    return ListLoadNextW(ParentWin, PluginWin, wpath.data(), ShowFlags);
}

void __stdcall ListCloseWindow(HWND ListWin) {
    DestroyWindow(ListWin);
}

int __stdcall ListSendCommand(HWND ListWin, int Command, int /*Parameter*/) {
    if (Command == lc_copy) {
        auto* s = reinterpret_cast<PluginState*>(GetWindowLongPtrW(ListWin, GWLP_USERDATA));
        if (!s || !s->loaded || !s->collector) return LISTPLUGIN_ERROR;

        RECT rc;
        GetClientRect(ListWin, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        // Render to bitmap and copy to clipboard
        HDC hdc = GetDC(ListWin);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, bmp);

        {
            Gdiplus::Graphics g(memDC);
            s->renderer.render(g, *s->collector, w, h);
        }

        SelectObject(memDC, oldBmp);
        DeleteDC(memDC);
        ReleaseDC(ListWin, hdc);

        if (OpenClipboard(ListWin)) {
            EmptyClipboard();
            SetClipboardData(CF_BITMAP, bmp);
            CloseClipboard();
        } else {
            DeleteObject(bmp);
        }

        return LISTPLUGIN_OK;
    }
    return LISTPLUGIN_ERROR;
}

HBITMAP __stdcall ListGetPreviewBitmapW(WCHAR* FileToLoad, int width, int height,
    char* /*contentbuf*/, int /*contentbuflen*/)
{
    // Parse the DXF/DWG file
    DxfCollector collector;
    int len = WideCharToMultiByte(CP_UTF8, 0, FileToLoad, -1, nullptr, 0, nullptr, nullptr);
    std::string narrowPath(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, FileToLoad, -1, narrowPath.data(), len, nullptr, nullptr);

    // Determine format by file extension
    bool isDwg = false;
    {
        size_t dot = narrowPath.rfind('.');
        if (dot != std::string::npos) {
            std::string ext = narrowPath.substr(dot);
            for (auto& c : ext) c = (char)tolower((unsigned char)c);
            isDwg = (ext == ".dwg");
        }
    }

    bool readOk;
    if (isDwg) {
        dwgR dwg(narrowPath.c_str());
        readOk = dwg.read(&collector, false);
    } else {
        dxfRW dxf(narrowPath.c_str());
        readOk = dxf.read(&collector, false);
    }
    if (!readOk)
        return nullptr;

    if (collector.primitives.empty())
        return nullptr;

    GdiplusAddRef();

    // Create DIB section
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP hBmp = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!hBmp) {
        GdiplusRelease();
        return nullptr;
    }

    HDC memDC = CreateCompatibleDC(nullptr);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, hBmp);

    {
        Gdiplus::Graphics g(memDC);
        DxfRenderer renderer;
        renderer.darkMode = false;
        renderer.fitToView(collector, width, height);
        renderer.render(g, collector, width, height);
    }

    SelectObject(memDC, oldBmp);
    DeleteDC(memDC);
    GdiplusRelease();

    return hBmp;
}

HBITMAP __stdcall ListGetPreviewBitmap(char* FileToLoad, int width, int height,
    char* contentbuf, int contentbuflen)
{
    int wlen = MultiByteToWideChar(CP_ACP, 0, FileToLoad, -1, nullptr, 0);
    std::wstring wpath(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, FileToLoad, -1, wpath.data(), wlen);
    return ListGetPreviewBitmapW(wpath.data(), width, height, contentbuf, contentbuflen);
}

} // extern "C"
