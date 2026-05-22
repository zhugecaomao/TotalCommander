#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdio>
#include <cstring>

// Typedefs for plugin functions
typedef HWND(__stdcall* ListLoadW_t)(HWND, WCHAR*, int);
typedef void(__stdcall* ListCloseWindow_t)(HWND);
typedef void(__stdcall* ListGetDetectString_t)(char*, int);

static LRESULT CALLBACK HostWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    if (msg == WM_SIZE) {
        // Resize the plugin child window
        HWND child = GetWindow(hwnd, GW_CHILD);
        if (child) {
            MoveWindow(child, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 2) {
        wprintf(L"Usage: test_dxf.exe <path-to-dxf-file>\n");
        return 1;
    }

    // Load plugin DLL
    HMODULE hDll = LoadLibraryW(L"dxfViewer.wlx64");
    if (!hDll) hDll = LoadLibraryW(L"dxfViewer.wlx");
    if (!hDll) {
        wprintf(L"Error: Cannot load dxfViewer.wlx64 or dxfViewer.wlx\n");
        return 1;
    }

    auto pfnListLoadW = (ListLoadW_t)GetProcAddress(hDll, "ListLoadW");
    auto pfnListCloseWindow = (ListCloseWindow_t)GetProcAddress(hDll, "ListCloseWindow");
    auto pfnListGetDetectString = (ListGetDetectString_t)GetProcAddress(hDll, "ListGetDetectString");

    if (!pfnListLoadW || !pfnListCloseWindow) {
        wprintf(L"Error: Cannot find ListLoadW/ListCloseWindow exports\n");
        FreeLibrary(hDll);
        return 1;
    }

    // Print detect string
    if (pfnListGetDetectString) {
        char detect[256] = {};
        pfnListGetDetectString(detect, sizeof(detect));
        printf("DetectString: %s\n", detect);
    }

    // Register host window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = HostWndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"TestDxfHost";
    RegisterClassExW(&wc);

    // Create host window
    HWND hHost = CreateWindowExW(0, L"TestDxfHost", L"DXF Viewer Test",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);

    // Call ListLoadW
    HWND hPlugin = pfnListLoadW(hHost, argv[1], 0);
    if (!hPlugin) {
        wprintf(L"Error: ListLoadW returned NULL\n");
        DestroyWindow(hHost);
        FreeLibrary(hDll);
        return 1;
    }

    wprintf(L"Plugin window created: %p\n", hPlugin);

    // Message loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    pfnListCloseWindow(hPlugin);
    FreeLibrary(hDll);
    return 0;
}
