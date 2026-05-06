#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NTDDI_VERSION   NTDDI_VISTA
#define _WIN32_WINNT    _WIN32_WINNT_VISTA

#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <tchar.h>

#include <string>
#include <vector>

#include "resource.h"
#include "rpc_client.h"

namespace {
constexpr wchar_t kMutexName[] = L"Local\\TrayApp_{7A3B9F2E-1D4C-4E5A-8F6B-0C2D3E4F5A6B}";
constexpr wchar_t kWindowClass[] = L"TrayAppWindowClass";
constexpr wchar_t kAppTitle[] = L"TrayApp";
constexpr wchar_t kServiceName[] = L"TrayAppService";
constexpr wchar_t kServiceBinaryName[] = L"TrayAppService.exe";
constexpr wchar_t kMenuFile[] = L"\x0424\x0430\x0439\x043B";
constexpr wchar_t kMenuOpen[] = L"\x041E\x0442\x043A\x0440\x044B\x0442\x044C";
constexpr wchar_t kMenuExit[] = L"\x0412\x044B\x0445\x043E\x0434";
}

static HINSTANCE g_hInst = nullptr;
static HWND g_hWnd = nullptr;
static NOTIFYICONDATA g_nid = {};
static HANDLE g_hMutex = nullptr;
static UINT g_wmTaskbarCreated = 0;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static void AddTrayIcon(HWND hWnd);
static void RemoveTrayIcon();
static void ShowTrayContextMenu(HWND hWnd);
static HMENU CreateMainMenu();
static void ShowMainWindow();
static void ExitApp();
static bool EnsureServiceRunningForCurrentLaunch();
static bool WaitForServiceState(SC_HANDLE serviceHandle, DWORD desiredState, DWORD timeoutMs);
static bool IsLaunchedByService();
static DWORD GetParentProcessId();
static std::wstring GetSiblingPath(const wchar_t* fileName);
static bool GetProcessImagePath(HANDLE processHandle, std::wstring* imagePath);
static const wchar_t* GetBaseName(const std::wstring& path);

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                    _In_opt_ HINSTANCE,
                    _In_ LPWSTR lpCmdLine,
                    _In_ int nCmdShow)
{
    if (!EnsureServiceRunningForCurrentLaunch()) {
        return 0;
    }

    if (!IsLaunchedByService()) {
        return 0;
    }

    g_hMutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (!g_hMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (g_hMutex) {
            CloseHandle(g_hMutex);
        }
        return 0;
    }

    g_hInst = hInstance;
    g_wmTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = hInstance;
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kWindowClass;
    windowClass.hIconSm = LoadIconW(nullptr, IDI_APPLICATION);

    if (!RegisterClassExW(&windowClass)) {
        ReleaseMutex(g_hMutex);
        CloseHandle(g_hMutex);
        return 0;
    }

    g_hWnd = CreateWindowExW(
        0,
        kWindowClass,
        kAppTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        640,
        480,
        nullptr,
        CreateMainMenu(),
        hInstance,
        nullptr);

    if (!g_hWnd) {
        ReleaseMutex(g_hMutex);
        CloseHandle(g_hMutex);
        return 0;
    }

    AddTrayIcon(g_hWnd);

    bool startHidden = false;
    if (lpCmdLine && (wcsstr(lpCmdLine, L"--hidden") || wcsstr(lpCmdLine, L"/hidden"))) {
        startHidden = true;
    }

    if (!startHidden) {
        ShowWindow(g_hWnd, nCmdShow);
        UpdateWindow(g_hWnd);
    }

    MSG message = {};
    while (GetMessageW(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    RemoveTrayIcon();
    ReleaseMutex(g_hMutex);
    CloseHandle(g_hMutex);
    return static_cast<int>(message.wParam);
}

static HMENU CreateMainMenu()
{
    HMENU mainMenu = CreateMenu();
    HMENU fileMenu = CreatePopupMenu();
    AppendMenuW(fileMenu, MF_STRING, IDM_FILE_EXIT, kMenuExit);
    AppendMenuW(mainMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(fileMenu), kMenuFile);
    return mainMenu;
}

static void AddTrayIcon(HWND hWnd)
{
    g_nid.cbSize = sizeof(g_nid);
    g_nid.hWnd = hWnd;
    g_nid.uID = IDI_TRAYAPP;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    lstrcpynW(g_nid.szTip, kAppTitle, ARRAYSIZE(g_nid.szTip));
    Shell_NotifyIconW(NIM_ADD, &g_nid);
}

static void RemoveTrayIcon()
{
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

static void ShowTrayContextMenu(HWND hWnd)
{
    POINT cursorPoint = {};
    GetCursorPos(&cursorPoint);

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, IDM_TRAY_OPEN, kMenuOpen);
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, IDM_TRAY_EXIT, kMenuExit);

    SetForegroundWindow(hWnd);
    TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, cursorPoint.x, cursorPoint.y, 0, hWnd, nullptr);
    PostMessageW(hWnd, WM_NULL, 0, 0);

    DestroyMenu(menu);
}

static void ShowMainWindow()
{
    ShowWindow(g_hWnd, SW_SHOW);
    ShowWindow(g_hWnd, SW_RESTORE);
    SetForegroundWindow(g_hWnd);
}

static void ExitApp()
{
    RequestServiceStopViaRpc();
    DestroyWindow(g_hWnd);
}

static bool EnsureServiceRunningForCurrentLaunch()
{
    SC_HANDLE scmHandle = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scmHandle) {
        return false;
    }

    SC_HANDLE serviceHandle = OpenServiceW(
        scmHandle,
        kServiceName,
        SERVICE_QUERY_STATUS | SERVICE_START);
    if (!serviceHandle) {
        CloseServiceHandle(scmHandle);
        return false;
    }

    SERVICE_STATUS_PROCESS serviceStatus = {};
    DWORD bytesNeeded = 0;
    bool shouldContinue = false;

    if (QueryServiceStatusEx(
            serviceHandle,
            SC_STATUS_PROCESS_INFO,
            reinterpret_cast<LPBYTE>(&serviceStatus),
            sizeof(serviceStatus),
            &bytesNeeded)) {
        if (serviceStatus.dwCurrentState == SERVICE_RUNNING) {
            shouldContinue = true;
        } else {
            if (serviceStatus.dwCurrentState == SERVICE_STOPPED) {
                StartServiceW(serviceHandle, 0, nullptr);
            }

            WaitForServiceState(serviceHandle, SERVICE_RUNNING, 15000);
            shouldContinue = false;
        }
    }

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);
    return shouldContinue;
}

static bool WaitForServiceState(SC_HANDLE serviceHandle, DWORD desiredState, DWORD timeoutMs)
{
    const DWORD deadline = GetTickCount() + timeoutMs;

    while (true) {
        SERVICE_STATUS_PROCESS serviceStatus = {};
        DWORD bytesNeeded = 0;
        if (!QueryServiceStatusEx(
                serviceHandle,
                SC_STATUS_PROCESS_INFO,
                reinterpret_cast<LPBYTE>(&serviceStatus),
                sizeof(serviceStatus),
                &bytesNeeded)) {
            return false;
        }

        if (serviceStatus.dwCurrentState == desiredState) {
            return true;
        }

        if (GetTickCount() >= deadline) {
            return false;
        }

        Sleep(250);
    }
}

static bool IsLaunchedByService()
{
    const DWORD parentProcessId = GetParentProcessId();
    if (parentProcessId == 0) {
        return false;
    }

    HANDLE parentHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, parentProcessId);
    if (!parentHandle) {
        return false;
    }

    std::wstring parentImagePath;
    const bool haveParentPath = GetProcessImagePath(parentHandle, &parentImagePath);
    CloseHandle(parentHandle);

    if (!haveParentPath) {
        return false;
    }

    const wchar_t* parentBaseName = GetBaseName(parentImagePath);
    if (_wcsicmp(parentBaseName, kServiceBinaryName) != 0) {
        return false;
    }

    const std::wstring expectedServicePath = GetSiblingPath(kServiceBinaryName);
    return _wcsicmp(parentImagePath.c_str(), expectedServicePath.c_str()) == 0;
}

static DWORD GetParentProcessId()
{
    const DWORD currentProcessId = GetCurrentProcessId();
    HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshotHandle == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W processEntry = {};
    processEntry.dwSize = sizeof(processEntry);

    DWORD parentProcessId = 0;
    if (Process32FirstW(snapshotHandle, &processEntry)) {
        do {
            if (processEntry.th32ProcessID == currentProcessId) {
                parentProcessId = processEntry.th32ParentProcessID;
                break;
            }
        } while (Process32NextW(snapshotHandle, &processEntry));
    }

    CloseHandle(snapshotHandle);
    return parentProcessId;
}

static std::wstring GetSiblingPath(const wchar_t* fileName)
{
    std::vector<wchar_t> modulePath(MAX_PATH, L'\0');
    DWORD length = GetModuleFileNameW(nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
    while (length == modulePath.size()) {
        modulePath.resize(modulePath.size() * 2, L'\0');
        length = GetModuleFileNameW(nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
    }

    std::wstring fullPath(modulePath.data(), length);
    const size_t separator = fullPath.find_last_of(L'\\');
    if (separator != std::wstring::npos) {
        fullPath.erase(separator + 1);
    } else {
        fullPath.clear();
    }

    fullPath += fileName;
    return fullPath;
}

static bool GetProcessImagePath(HANDLE processHandle, std::wstring* imagePath)
{
    if (!imagePath) {
        return false;
    }

    std::vector<wchar_t> buffer(MAX_PATH, L'\0');
    DWORD bufferSize = static_cast<DWORD>(buffer.size());

    while (!QueryFullProcessImageNameW(processHandle, 0, buffer.data(), &bufferSize)) {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            return false;
        }

        buffer.resize(buffer.size() * 2, L'\0');
        bufferSize = static_cast<DWORD>(buffer.size());
    }

    imagePath->assign(buffer.data(), bufferSize);
    return true;
}

static const wchar_t* GetBaseName(const std::wstring& path)
{
    const size_t separator = path.find_last_of(L'\\');
    return (separator == std::wstring::npos) ? path.c_str() : path.c_str() + separator + 1;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == g_wmTaskbarCreated && g_wmTaskbarCreated != 0) {
        AddTrayIcon(hWnd);
        return 0;
    }

    switch (message) {
    case WM_TRAYICON:
        switch (lParam) {
        case WM_LBUTTONUP:
            ShowMainWindow();
            break;
        case WM_RBUTTONUP:
            ShowTrayContextMenu(hWnd);
            break;
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_FILE_EXIT:
        case IDM_TRAY_EXIT:
            ExitApp();
            break;
        case IDM_TRAY_OPEN:
            ShowMainWindow();
            break;
        }
        break;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    return 0;
}
