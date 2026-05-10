#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NTDDI_VERSION   NTDDI_VISTA
#define _WIN32_WINNT    _WIN32_WINNT_VISTA

#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>

#include <cstdio>
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
constexpr UINT_PTR kStateRefreshTimerId = 1;
constexpr UINT_PTR kTrayRetryTimerId = 2;
constexpr UINT kStateRefreshIntervalMs = 5000;
constexpr UINT kTrayRetryIntervalMs = 1000;
constexpr UINT WM_APP_INITIALIZE = WM_APP + 1;

const wchar_t* StatusCodeToMessage(TrayAppRpcStatusCode statusCode)
{
    switch (statusCode) {
    case TRAYAPP_RPC_STATUS_OK:
        return L"";
    case TRAYAPP_RPC_STATUS_NOT_AUTHENTICATED:
        return L"Войдите в учетную запись";
    case TRAYAPP_RPC_STATUS_INVALID_CREDENTIALS:
        return L"Не удалось выполнить аутентификацию";
    case TRAYAPP_RPC_STATUS_NETWORK_ERROR:
        return L"Сервер недоступен";
    case TRAYAPP_RPC_STATUS_NO_LICENSE:
        return L"Лицензия отсутствует";
    case TRAYAPP_RPC_STATUS_LICENSE_BLOCKED:
        return L"Лицензия заблокирована";
    case TRAYAPP_RPC_STATUS_LICENSE_EXPIRED:
        return L"Срок действия лицензии истек";
    case TRAYAPP_RPC_STATUS_ACTIVATION_FAILED:
        return L"Не удалось активировать продукт";
    default:
        return L"Внутренняя ошибка службы";
    }
}
}

static HINSTANCE g_hInst = nullptr;
static HWND g_hWnd = nullptr;
static NOTIFYICONDATA g_nid = {};
static HANDLE g_hMutex = nullptr;
static UINT g_wmTaskbarCreated = 0;
static HFONT g_uiFont = nullptr;

static HWND g_statusLabel = nullptr;
static HWND g_userLabel = nullptr;
static HWND g_licenseLabel = nullptr;
static HWND g_avLabel = nullptr;
static HWND g_dbLabel = nullptr;
static HWND g_emailEdit = nullptr;
static HWND g_passwordEdit = nullptr;
static HWND g_loginButton = nullptr;
static HWND g_activateEdit = nullptr;
static HWND g_activateButton = nullptr;
static HWND g_logoutButton = nullptr;
static HWND g_scanFileEdit = nullptr;
static HWND g_scanFileButton = nullptr;
static HWND g_scanDirEdit = nullptr;
static HWND g_scanDirButton = nullptr;
static HWND g_scanResultsEdit = nullptr;
static HWND g_scanFixedButton = nullptr;
static HWND g_scheduleLabel = nullptr;
static HWND g_scheduleEdit = nullptr;
static HWND g_scheduleEnableButton = nullptr;
static HWND g_scheduleDisableButton = nullptr;
static HWND g_monitorLabel = nullptr;
static HWND g_monitorEdit = nullptr;
static HWND g_monitorAddButton = nullptr;
static HWND g_monitorRemoveButton = nullptr;

static TrayAppAuthState g_authState = {};
static TrayAppLicenseState g_licenseState = {};
static TrayAppAvDatabaseInfo g_avDatabaseInfo = {};
static TrayAppScheduledScanState g_scheduledScanState = {};
static TrayAppMonitoringState g_monitoringState = {};
static bool g_trayIconAdded = false;

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
static DWORD GetRunningServiceProcessId();
static DWORD GetParentProcessId();
static std::wstring GetSiblingPath(const wchar_t* fileName);
static bool GetProcessImagePath(HANDLE processHandle, std::wstring* imagePath);
static const wchar_t* GetBaseName(const std::wstring& path);
static void CreateUiControls(HWND hWnd);
static void LayoutControls(HWND hWnd);
static void ApplyControlFont(HWND control);
static void RefreshStateFromService();
static void UpdateUiFromState();
static void HandleLogin();
static void HandleActivate();
static void HandleLogout();
static void HandleScanFile();
static void HandleScanDirectory();
static void HandleScanFixedDisks();
static void HandleEnableSchedule();
static void HandleDisableSchedule();
static void HandleAddMonitor();
static void HandleRemoveMonitor();
static std::wstring GetControlText(HWND control);
static void ShowRpcFailureMessage(const wchar_t* action);
static void WriteDebugLog(const wchar_t* message);
static void WriteLastErrorDebugLog(const wchar_t* message);
static void ScheduleTrayIconRetry(HWND hWnd);
static void UpdateScanResultsText(const TrayAppScanResult& result);
static const wchar_t* ScanObjectTypeToText(TrayAppScanObjectType objectType);

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                    _In_opt_ HINSTANCE,
                    _In_ LPWSTR lpCmdLine,
                    _In_ int nCmdShow)
{
    WriteDebugLog(L"wWinMain entered");

    const bool launchedByService = IsLaunchedByService();
    if (!launchedByService) {
        WriteDebugLog(L"IsLaunchedByService returned false");
        if (!EnsureServiceRunningForCurrentLaunch()) {
            WriteDebugLog(L"EnsureServiceRunningForCurrentLaunch returned false, exiting");
            return 0;
        }
        WriteDebugLog(L"EnsureServiceRunningForCurrentLaunch returned true; exiting because app was not launched by service");
        return 0;
    }

    WriteDebugLog(L"IsLaunchedByService returned true");

    g_hMutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (!g_hMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (!g_hMutex) {
            WriteLastErrorDebugLog(L"CreateMutexW failed");
        } else {
            WriteDebugLog(L"CreateMutexW reported already existing instance, exiting");
        }
        if (g_hMutex) {
            CloseHandle(g_hMutex);
        }
        return 0;
    }
    WriteDebugLog(L"CreateMutexW succeeded");

    g_hInst = hInstance;
    g_wmTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    NONCLIENTMETRICSW metrics = {};
    metrics.cbSize = sizeof(metrics);
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0)) {
        g_uiFont = CreateFontIndirectW(&metrics.lfMessageFont);
    }

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
        WriteLastErrorDebugLog(L"RegisterClassExW failed");
        if (g_uiFont) {
            DeleteObject(g_uiFont);
        }
        ReleaseMutex(g_hMutex);
        CloseHandle(g_hMutex);
        return 0;
    }
    WriteDebugLog(L"RegisterClassExW succeeded");

    g_hWnd = CreateWindowExW(
        0,
        kWindowClass,
        kAppTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        980,
        860,
        nullptr,
        CreateMainMenu(),
        hInstance,
        nullptr);

    if (!g_hWnd) {
        WriteLastErrorDebugLog(L"CreateWindowExW failed");
        if (g_uiFont) {
            DeleteObject(g_uiFont);
        }
        ReleaseMutex(g_hMutex);
        CloseHandle(g_hMutex);
        return 0;
    }
    WriteDebugLog(L"CreateWindowExW succeeded");

    AddTrayIcon(g_hWnd);
    WriteDebugLog(L"AddTrayIcon called");

    bool startHidden = false;
    if (lpCmdLine && (wcsstr(lpCmdLine, L"--hidden") || wcsstr(lpCmdLine, L"/hidden"))) {
        startHidden = true;
    }
    WriteDebugLog(startHidden ? L"Starting hidden" : L"Starting visible");

    if (!startHidden) {
        ShowWindow(g_hWnd, nCmdShow);
        UpdateWindow(g_hWnd);
        WriteDebugLog(L"ShowWindow and UpdateWindow completed");
    }

    MSG message = {};
    while (GetMessageW(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    RemoveTrayIcon();
    WriteDebugLog(L"Message loop finished, exiting");
    if (g_uiFont) {
        DeleteObject(g_uiFont);
    }
    ReleaseMutex(g_hMutex);
    CloseHandle(g_hMutex);
    return static_cast<int>(message.wParam);
}

static void WriteDebugLog(const wchar_t* message)
{
    const std::wstring logPath = GetSiblingPath(L"TrayApp-debug.log");
    FILE* fileHandle = nullptr;
    if (_wfopen_s(&fileHandle, logPath.c_str(), L"a+, ccs=UTF-8") != 0 || !fileHandle) {
        return;
    }

    SYSTEMTIME systemTime = {};
    GetLocalTime(&systemTime);
    fwprintf(
        fileHandle,
        L"[%04u-%02u-%02u %02u:%02u:%02u] %ls\n",
        systemTime.wYear,
        systemTime.wMonth,
        systemTime.wDay,
        systemTime.wHour,
        systemTime.wMinute,
        systemTime.wSecond,
        message ? message : L"");
    fclose(fileHandle);
}

static void WriteLastErrorDebugLog(const wchar_t* message)
{
    wchar_t buffer[256] = {};
    _snwprintf_s(
        buffer,
        _countof(buffer),
        _TRUNCATE,
        L"%ls, GetLastError=%lu",
        message ? message : L"",
        GetLastError());
    WriteDebugLog(buffer);
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
    if (Shell_NotifyIconW(NIM_ADD, &g_nid)) {
        g_trayIconAdded = true;
        KillTimer(hWnd, kTrayRetryTimerId);
        WriteDebugLog(L"Shell_NotifyIconW(NIM_ADD) succeeded");
    } else {
        g_trayIconAdded = false;
        WriteLastErrorDebugLog(L"Shell_NotifyIconW(NIM_ADD) failed");
        ScheduleTrayIconRetry(hWnd);
    }
}

static void RemoveTrayIcon()
{
    if (g_trayIconAdded) {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_trayIconAdded = false;
    }
}

static void ScheduleTrayIconRetry(HWND hWnd)
{
    SetTimer(hWnd, kTrayRetryTimerId, kTrayRetryIntervalMs, nullptr);
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
        WriteLastErrorDebugLog(L"OpenSCManagerW failed");
        return false;
    }
    WriteDebugLog(L"OpenSCManagerW succeeded");

    SC_HANDLE serviceHandle = OpenServiceW(
        scmHandle,
        kServiceName,
        SERVICE_QUERY_STATUS | SERVICE_START);
    if (!serviceHandle) {
        WriteLastErrorDebugLog(L"OpenServiceW failed");
        CloseServiceHandle(scmHandle);
        return false;
    }
    WriteDebugLog(L"OpenServiceW succeeded");

    SERVICE_STATUS_PROCESS serviceStatus = {};
    DWORD bytesNeeded = 0;
    bool shouldContinue = false;
    bool startedServiceFromThisProcess = false;

    if (QueryServiceStatusEx(
            serviceHandle,
            SC_STATUS_PROCESS_INFO,
            reinterpret_cast<LPBYTE>(&serviceStatus),
            sizeof(serviceStatus),
            &bytesNeeded)) {
        wchar_t stateBuffer[128] = {};
        _snwprintf_s(
            stateBuffer,
            _countof(stateBuffer),
            _TRUNCATE,
            L"QueryServiceStatusEx succeeded, currentState=%lu",
            serviceStatus.dwCurrentState);
        WriteDebugLog(stateBuffer);

        if (serviceStatus.dwCurrentState == SERVICE_RUNNING) {
            shouldContinue = true;
        } else {
            if (serviceStatus.dwCurrentState == SERVICE_STOPPED) {
                if (StartServiceW(serviceHandle, 0, nullptr)) {
                    startedServiceFromThisProcess = true;
                    WriteDebugLog(L"StartServiceW succeeded");
                } else {
                    WriteLastErrorDebugLog(L"StartServiceW failed");
                }
            }

            if (WaitForServiceState(serviceHandle, SERVICE_RUNNING, 15000)) {
                WriteDebugLog(L"WaitForServiceState returned true");
                shouldContinue = !startedServiceFromThisProcess;
            } else {
                WriteDebugLog(L"WaitForServiceState returned false");
            }
        }
    } else {
        WriteLastErrorDebugLog(L"QueryServiceStatusEx failed");
    }

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);
    WriteDebugLog(shouldContinue ? L"EnsureServiceRunningForCurrentLaunch final result=true" : L"EnsureServiceRunningForCurrentLaunch final result=false");
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
        WriteDebugLog(L"IsLaunchedByService: parentProcessId is 0");
        return false;
    }

    const DWORD serviceProcessId = GetRunningServiceProcessId();
    if (serviceProcessId != 0 && parentProcessId == serviceProcessId) {
        WriteDebugLog(L"IsLaunchedByService: parent PID matches running service PID");
        return true;
    }

    HANDLE parentHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, parentProcessId);
    if (!parentHandle) {
        WriteLastErrorDebugLog(L"IsLaunchedByService: OpenProcess for parent failed");
        return false;
    }

    std::wstring parentImagePath;
    if (!GetProcessImagePath(parentHandle, &parentImagePath)) {
        return false;
    }
    CloseHandle(parentHandle);

    const wchar_t* parentBaseName = GetBaseName(parentImagePath);
    if (_wcsicmp(parentBaseName, kServiceBinaryName) != 0) {
        WriteDebugLog(L"IsLaunchedByService: parent base name is not TrayAppService.exe");
        return false;
    }

    const std::wstring expectedServicePath = GetSiblingPath(kServiceBinaryName);
    const bool matchesPath = _wcsicmp(parentImagePath.c_str(), expectedServicePath.c_str()) == 0;
    WriteDebugLog(matchesPath
        ? L"IsLaunchedByService: parent path matches expected service path"
        : L"IsLaunchedByService: parent path does not match expected service path");
    return matchesPath;
}

static DWORD GetRunningServiceProcessId()
{
    SC_HANDLE scmHandle = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scmHandle) {
        return 0;
    }

    SC_HANDLE serviceHandle = OpenServiceW(scmHandle, kServiceName, SERVICE_QUERY_STATUS);
    if (!serviceHandle) {
        CloseServiceHandle(scmHandle);
        return 0;
    }

    SERVICE_STATUS_PROCESS serviceStatus = {};
    DWORD bytesNeeded = 0;
    DWORD processId = 0;
    if (QueryServiceStatusEx(
            serviceHandle,
            SC_STATUS_PROCESS_INFO,
            reinterpret_cast<LPBYTE>(&serviceStatus),
            sizeof(serviceStatus),
            &bytesNeeded) &&
        serviceStatus.dwCurrentState == SERVICE_RUNNING) {
        processId = serviceStatus.dwProcessId;
    }

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);
    return processId;
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

static void ApplyControlFont(HWND control)
{
    if (control && g_uiFont) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_uiFont), TRUE);
    }
}

static void CreateUiControls(HWND hWnd)
{
    g_statusLabel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_STATUS_LABEL), g_hInst, nullptr);
    g_userLabel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_USER_LABEL), g_hInst, nullptr);
    g_licenseLabel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_LICENSE_LABEL), g_hInst, nullptr);
    g_avLabel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_AV_LABEL), g_hInst, nullptr);
    g_dbLabel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_DB_LABEL), g_hInst, nullptr);
    g_emailEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_EMAIL_EDIT), g_hInst, nullptr);
    g_passwordEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_PASSWORD, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_PASSWORD_EDIT), g_hInst, nullptr);
    g_loginButton = CreateWindowW(L"BUTTON", L"Войти", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_LOGIN_BUTTON), g_hInst, nullptr);
    g_activateEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_ACTIVATE_EDIT), g_hInst, nullptr);
    g_activateButton = CreateWindowW(L"BUTTON", L"Активировать", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_ACTIVATE_BUTTON), g_hInst, nullptr);
    g_logoutButton = CreateWindowW(L"BUTTON", L"Выйти из аккаунта", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_LOGOUT_BUTTON), g_hInst, nullptr);
    g_scanFileEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_SCAN_FILE_EDIT), g_hInst, nullptr);
    g_scanFileButton = CreateWindowW(L"BUTTON", L"Сканировать файл", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_SCAN_FILE_BTN), g_hInst, nullptr);
    g_scanDirEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_SCAN_DIR_EDIT), g_hInst, nullptr);
    g_scanDirButton = CreateWindowW(L"BUTTON", L"Сканировать папку", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_SCAN_DIR_BTN), g_hInst, nullptr);
    g_scanFixedButton = CreateWindowW(L"BUTTON", L"Scan fixed disks", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_SCAN_FIXED_BTN), g_hInst, nullptr);
    g_scheduleLabel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_SCHEDULE_LABEL), g_hInst, nullptr);
    g_scheduleEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"30", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_SCHEDULE_EDIT), g_hInst, nullptr);
    g_scheduleEnableButton = CreateWindowW(L"BUTTON", L"Enable schedule", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_SCHEDULE_ON_BTN), g_hInst, nullptr);
    g_scheduleDisableButton = CreateWindowW(L"BUTTON", L"Disable schedule", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_SCHEDULE_OFF_BTN), g_hInst, nullptr);
    g_monitorLabel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_MONITOR_LABEL), g_hInst, nullptr);
    g_monitorEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_MONITOR_EDIT), g_hInst, nullptr);
    g_monitorAddButton = CreateWindowW(L"BUTTON", L"Add monitor", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_MONITOR_ADD_BTN), g_hInst, nullptr);
    g_monitorRemoveButton = CreateWindowW(L"BUTTON", L"Remove monitor", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_MONITOR_DEL_BTN), g_hInst, nullptr);
    g_scanResultsEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL, 0, 0, 0, 0, hWnd, reinterpret_cast<HMENU>(IDC_SCAN_RESULTS), g_hInst, nullptr);

    ApplyControlFont(g_statusLabel);
    ApplyControlFont(g_userLabel);
    ApplyControlFont(g_licenseLabel);
    ApplyControlFont(g_avLabel);
    ApplyControlFont(g_dbLabel);
    ApplyControlFont(g_emailEdit);
    ApplyControlFont(g_passwordEdit);
    ApplyControlFont(g_loginButton);
    ApplyControlFont(g_activateEdit);
    ApplyControlFont(g_activateButton);
    ApplyControlFont(g_logoutButton);
    ApplyControlFont(g_scanFileEdit);
    ApplyControlFont(g_scanFileButton);
    ApplyControlFont(g_scanDirEdit);
    ApplyControlFont(g_scanDirButton);
    ApplyControlFont(g_scanFixedButton);
    ApplyControlFont(g_scheduleLabel);
    ApplyControlFont(g_scheduleEdit);
    ApplyControlFont(g_scheduleEnableButton);
    ApplyControlFont(g_scheduleDisableButton);
    ApplyControlFont(g_monitorLabel);
    ApplyControlFont(g_monitorEdit);
    ApplyControlFont(g_monitorAddButton);
    ApplyControlFont(g_monitorRemoveButton);
    ApplyControlFont(g_scanResultsEdit);
}

static void LayoutControls(HWND hWnd)
{
    RECT clientRect = {};
    GetClientRect(hWnd, &clientRect);

    const int left = 24;
    const int top = 24;
    const int width = max(460, clientRect.right - 48);
    const int editWidth = max(320, width - 190);
    const int buttonLeft = left + editWidth + 12;
    const int labelHeight = 24;
    const int editHeight = 28;
    const int buttonHeight = 30;
    const int gap = 12;

    MoveWindow(g_statusLabel, left, top, width, labelHeight, TRUE);
    MoveWindow(g_userLabel, left, top + 36, width, labelHeight, TRUE);
    MoveWindow(g_licenseLabel, left, top + 72, width, labelHeight, TRUE);
    MoveWindow(g_avLabel, left, top + 108, width, labelHeight, TRUE);
    MoveWindow(g_dbLabel, left, top + 144, width, labelHeight, TRUE);

    MoveWindow(g_emailEdit, left, top + 190, 320, editHeight, TRUE);
    MoveWindow(g_passwordEdit, left, top + 190 + editHeight + gap, 320, editHeight, TRUE);
    MoveWindow(g_loginButton, left, top + 190 + (editHeight + gap) * 2, 160, buttonHeight, TRUE);

    MoveWindow(g_activateEdit, left, top + 318, 320, editHeight, TRUE);
    MoveWindow(g_activateButton, left, top + 318 + editHeight + gap, 160, buttonHeight, TRUE);

    MoveWindow(g_logoutButton, left, top + 318 + (editHeight + gap) * 2, 190, buttonHeight, TRUE);

    MoveWindow(g_scanFileEdit, left, top + 430, editWidth, editHeight, TRUE);
    MoveWindow(g_scanFileButton, buttonLeft, top + 430, 170, buttonHeight, TRUE);

    MoveWindow(g_scanDirEdit, left, top + 430 + editHeight + gap, editWidth, editHeight, TRUE);
    MoveWindow(g_scanDirButton, buttonLeft, top + 430 + editHeight + gap, 170, buttonHeight, TRUE);

    MoveWindow(g_scanFixedButton, left, top + 430 + (editHeight + gap) * 2, 220, buttonHeight, TRUE);

    MoveWindow(g_scheduleLabel, left, top + 430 + (editHeight + gap) * 2 + 46, width, labelHeight, TRUE);
    MoveWindow(g_scheduleEdit, left, top + 430 + (editHeight + gap) * 2 + 78, 120, editHeight, TRUE);
    MoveWindow(g_scheduleEnableButton, left + 140, top + 430 + (editHeight + gap) * 2 + 76, 170, buttonHeight, TRUE);
    MoveWindow(g_scheduleDisableButton, left + 320, top + 430 + (editHeight + gap) * 2 + 76, 170, buttonHeight, TRUE);

    MoveWindow(g_monitorLabel, left, top + 430 + (editHeight + gap) * 2 + 120, width, labelHeight, TRUE);
    MoveWindow(g_monitorEdit, left, top + 430 + (editHeight + gap) * 2 + 152, editWidth, editHeight, TRUE);
    MoveWindow(g_monitorAddButton, buttonLeft, top + 430 + (editHeight + gap) * 2 + 150, 170, buttonHeight, TRUE);
    MoveWindow(g_monitorRemoveButton, buttonLeft, top + 430 + (editHeight + gap) * 2 + 150 + buttonHeight + gap, 170, buttonHeight, TRUE);

    MoveWindow(g_scanResultsEdit, left, top + 430 + (editHeight + gap) * 2 + 204, width, max(120, clientRect.bottom - (top + 430 + (editHeight + gap) * 2 + 204) - 24), TRUE);
}

static std::wstring GetControlText(HWND control)
{
    const int length = GetWindowTextLengthW(control);
    std::wstring value(length + 1, L'\0');
    GetWindowTextW(control, value.data(), length + 1);
    value.resize(length);
    return value;
}

static void RefreshStateFromService()
{
    ZeroMemory(&g_authState, sizeof(g_authState));
    ZeroMemory(&g_licenseState, sizeof(g_licenseState));
    ZeroMemory(&g_avDatabaseInfo, sizeof(g_avDatabaseInfo));
    ZeroMemory(&g_scheduledScanState, sizeof(g_scheduledScanState));
    ZeroMemory(&g_monitoringState, sizeof(g_monitoringState));

    if (!GetAuthStateViaRpc(&g_authState)) {
        g_authState.statusCode = TRAYAPP_RPC_STATUS_NETWORK_ERROR;
        lstrcpynW(g_authState.message, L"Служба недоступна", ARRAYSIZE(g_authState.message));
        UpdateUiFromState();
        return;
    }

    if (g_authState.authenticated) {
        if (!GetLicenseStateViaRpc(&g_licenseState)) {
            g_licenseState.statusCode = TRAYAPP_RPC_STATUS_NETWORK_ERROR;
            lstrcpynW(g_licenseState.message, L"Не удалось получить статус лицензии", ARRAYSIZE(g_licenseState.message));
        }
        if (!GetAvDatabaseInfoViaRpc(&g_avDatabaseInfo)) {
            g_avDatabaseInfo.statusCode = TRAYAPP_RPC_STATUS_NETWORK_ERROR;
            lstrcpynW(g_avDatabaseInfo.message, L"Не удалось получить информацию о базах", ARRAYSIZE(g_avDatabaseInfo.message));
        }
        GetScheduledScanStateViaRpc(&g_scheduledScanState);
        GetMonitoringStateViaRpc(&g_monitoringState);
    } else {
        g_licenseState.statusCode = TRAYAPP_RPC_STATUS_NO_LICENSE;
        g_licenseState.antivirusEnabled = FALSE;
        g_avDatabaseInfo.statusCode = TRAYAPP_RPC_STATUS_NO_LICENSE;
    }

    UpdateUiFromState();
}

static void UpdateUiFromState()
{
    std::wstring statusText;
    if (g_authState.authenticated) {
        statusText = L"Пользователь аутентифицирован";
    } else {
        statusText = g_authState.message[0] ? g_authState.message : StatusCodeToMessage(g_authState.statusCode);
    }

    if (g_authState.authenticated && !g_licenseState.hasLicense) {
        const wchar_t* licenseMessage = g_licenseState.message[0] ? g_licenseState.message : StatusCodeToMessage(g_licenseState.statusCode);
        if (*licenseMessage != 0) {
            statusText = licenseMessage;
        }
    }

    SetWindowTextW(g_statusLabel, statusText.c_str());

    std::wstring userText = g_authState.authenticated
        ? (std::wstring(L"Пользователь: ") + g_authState.email)
        : L"Пользователь не аутентифицирован";
    SetWindowTextW(g_userLabel, userText.c_str());

    std::wstring licenseText;
    if (g_authState.authenticated && g_licenseState.hasLicense && !g_licenseState.blocked && !g_licenseState.expired) {
        licenseText = L"Лицензия активна до: ";
        licenseText += g_licenseState.expiresAtText;
    } else {
        const wchar_t* licenseMessage = g_licenseState.message[0] ? g_licenseState.message : StatusCodeToMessage(g_licenseState.statusCode);
        licenseText = L"Лицензия: ";
        licenseText += (*licenseMessage != 0) ? licenseMessage : L"отсутствует";
    }
    SetWindowTextW(g_licenseLabel, licenseText.c_str());

    const bool antivirusEnabled = (g_authState.authenticated != FALSE) &&
        (g_licenseState.hasLicense != FALSE) &&
        (g_licenseState.blocked == FALSE) &&
        (g_licenseState.expired == FALSE);
    SetWindowTextW(g_avLabel, antivirusEnabled ? L"Функциональность антивируса разблокирована" : L"Функциональность антивируса заблокирована");

    std::wstring dbText = L"Базы: ";
    if (g_avDatabaseInfo.loaded) {
        dbText += L"выпуск ";
        dbText += g_avDatabaseInfo.releaseDateText;
        dbText += L", записей: ";
        dbText += std::to_wstring(g_avDatabaseInfo.recordCount);
    } else {
        dbText += (g_avDatabaseInfo.message[0] != 0) ? g_avDatabaseInfo.message : L"не загружены";
    }
    SetWindowTextW(g_dbLabel, dbText.c_str());

    std::wstring scheduleText = L"Расписание: ";
    if (g_scheduledScanState.enabled) {
        scheduleText += L"enabled, every ";
        scheduleText += std::to_wstring(g_scheduledScanState.intervalMinutes);
        scheduleText += L" min";
        if (g_scheduledScanState.nextRunText[0] != 0) {
            scheduleText += L", следующий запуск ";
            scheduleText += g_scheduledScanState.nextRunText;
        }
        if (g_scheduledScanState.lastResult.scannedFileCount != 0 || g_scheduledScanState.lastResult.detectedFileCount != 0) {
            scheduleText += L", последняя проверка ";
            scheduleText += std::to_wstring(g_scheduledScanState.lastResult.scannedFileCount);
            scheduleText += L"/";
            scheduleText += std::to_wstring(g_scheduledScanState.lastResult.detectedFileCount);
        }
    } else {
        scheduleText += L"отключено";
    }
    SetWindowTextW(g_scheduleLabel, scheduleText.c_str());

    std::wstring monitorText = L"Мониторинг: ";
    monitorText += std::to_wstring(g_monitoringState.directoryCount);
    monitorText += L" директорий";
    if (g_monitoringState.directories[0] != 0) {
        monitorText += L" [";
        monitorText += g_monitoringState.directories;
        monitorText += L"]";
    }
    if (g_monitoringState.lastResult.targetPath[0] != 0) {
        monitorText += L", последнее: ";
        monitorText += (g_monitoringState.lastResult.malicious ? L"обнаружено" : L"чисто");
    }
    SetWindowTextW(g_monitorLabel, monitorText.c_str());

    const BOOL showLogin = (g_authState.authenticated == FALSE);
    ShowWindow(g_emailEdit, showLogin ? SW_SHOW : SW_HIDE);
    ShowWindow(g_passwordEdit, showLogin ? SW_SHOW : SW_HIDE);
    ShowWindow(g_loginButton, showLogin ? SW_SHOW : SW_HIDE);

    const BOOL showActivation = (!showLogin) &&
        (g_licenseState.hasLicense == FALSE || g_licenseState.blocked != FALSE || g_licenseState.expired != FALSE);
    ShowWindow(g_activateEdit, showActivation ? SW_SHOW : SW_HIDE);
    ShowWindow(g_activateButton, showActivation ? SW_SHOW : SW_HIDE);

    ShowWindow(g_logoutButton, g_authState.authenticated ? SW_SHOW : SW_HIDE);
    EnableWindow(g_scanFileButton, antivirusEnabled ? TRUE : FALSE);
    EnableWindow(g_scanDirButton, antivirusEnabled ? TRUE : FALSE);
    EnableWindow(g_scanFixedButton, antivirusEnabled ? TRUE : FALSE);
    EnableWindow(g_scheduleEdit, antivirusEnabled ? TRUE : FALSE);
    EnableWindow(g_scheduleEnableButton, antivirusEnabled ? TRUE : FALSE);
    EnableWindow(g_scheduleDisableButton, antivirusEnabled ? TRUE : FALSE);
    EnableWindow(g_monitorEdit, antivirusEnabled ? TRUE : FALSE);
    EnableWindow(g_monitorAddButton, antivirusEnabled ? TRUE : FALSE);
    EnableWindow(g_monitorRemoveButton, antivirusEnabled ? TRUE : FALSE);
}

static void ShowRpcFailureMessage(const wchar_t* action)
{
    std::wstring message = action;
    message += L": служба недоступна";
    MessageBoxW(g_hWnd, message.c_str(), kAppTitle, MB_ICONERROR | MB_OK);
}

static void HandleLogin()
{
    const std::wstring email = GetControlText(g_emailEdit);
    const std::wstring password = GetControlText(g_passwordEdit);

    if (email.empty() || password.empty()) {
        MessageBoxW(g_hWnd, L"Введите email и пароль", kAppTitle, MB_ICONWARNING | MB_OK);
        return;
    }

    TrayAppAuthState authState = {};
    if (!LoginViaRpc(email, password, &authState)) {
        ShowRpcFailureMessage(L"Не удалось выполнить вход");
        return;
    }

    if (!authState.authenticated) {
        const wchar_t* message = authState.message[0] ? authState.message : StatusCodeToMessage(authState.statusCode);
        MessageBoxW(g_hWnd, message, kAppTitle, MB_ICONERROR | MB_OK);
        g_authState = authState;
        ZeroMemory(&g_licenseState, sizeof(g_licenseState));
        UpdateUiFromState();
        return;
    }

    RefreshStateFromService();
}

static void HandleActivate()
{
    const std::wstring activationCode = GetControlText(g_activateEdit);
    if (activationCode.empty()) {
        MessageBoxW(g_hWnd, L"Введите код активации", kAppTitle, MB_ICONWARNING | MB_OK);
        return;
    }

    TrayAppLicenseState licenseState = {};
    if (!ActivateLicenseViaRpc(activationCode, &licenseState)) {
        ShowRpcFailureMessage(L"Не удалось активировать продукт");
        return;
    }

    if (!licenseState.hasLicense || licenseState.blocked || licenseState.expired) {
        const wchar_t* message = licenseState.message[0] ? licenseState.message : StatusCodeToMessage(licenseState.statusCode);
        MessageBoxW(g_hWnd, message, kAppTitle, MB_ICONERROR | MB_OK);
        g_licenseState = licenseState;
        UpdateUiFromState();
        return;
    }

    SetWindowTextW(g_activateEdit, L"");
    g_licenseState = licenseState;
    UpdateUiFromState();
}

static void HandleLogout()
{
    TrayAppOperationResult result = {};
    if (!LogoutViaRpc(&result)) {
        ShowRpcFailureMessage(L"Не удалось выполнить выход");
        return;
    }

    SetWindowTextW(g_passwordEdit, L"");
    SetWindowTextW(g_activateEdit, L"");
    RefreshStateFromService();
}

static void HandleScanFile()
{
    const std::wstring path = GetControlText(g_scanFileEdit);
    if (path.empty()) {
        MessageBoxW(g_hWnd, L"Введите путь к файлу для сканирования", kAppTitle, MB_ICONWARNING | MB_OK);
        return;
    }

    TrayAppScanResult result = {};
    if (!ScanFileViaRpc(path, &result)) {
        ShowRpcFailureMessage(L"Не удалось выполнить сканирование файла");
        return;
    }

    UpdateScanResultsText(result);
}

static void HandleScanDirectory()
{
    const std::wstring path = GetControlText(g_scanDirEdit);
    if (path.empty()) {
        MessageBoxW(g_hWnd, L"Введите путь к директории для сканирования", kAppTitle, MB_ICONWARNING | MB_OK);
        return;
    }

    TrayAppScanResult result = {};
    if (!ScanDirectoryViaRpc(path, &result)) {
        ShowRpcFailureMessage(L"Не удалось выполнить сканирование папки");
        return;
    }

    UpdateScanResultsText(result);
}

static void HandleScanFixedDisks()
{
    TrayAppScanResult result = {};
    if (!ScanFixedDisksViaRpc(&result)) {
        ShowRpcFailureMessage(L"Не удалось выполнить сканирование дисков");
        return;
    }

    UpdateScanResultsText(result);
}

static void HandleEnableSchedule()
{
    const std::wstring intervalText = GetControlText(g_scheduleEdit);
    const unsigned long intervalMinutes = intervalText.empty() ? 0UL : wcstoul(intervalText.c_str(), nullptr, 10);
    if (intervalMinutes == 0) {
        MessageBoxW(g_hWnd, L"Введите интервал в минутах", kAppTitle, MB_ICONWARNING | MB_OK);
        return;
    }

    TrayAppOperationResult result = {};
    if (!ConfigureScheduledScanViaRpc(intervalMinutes, true, &result)) {
        ShowRpcFailureMessage(L"Не удалось включить расписание");
        return;
    }

    RefreshStateFromService();
}

static void HandleDisableSchedule()
{
    TrayAppOperationResult result = {};
    if (!ConfigureScheduledScanViaRpc(0, false, &result)) {
        ShowRpcFailureMessage(L"Не удалось отключить расписание");
        return;
    }

    RefreshStateFromService();
}

static void HandleAddMonitor()
{
    const std::wstring path = GetControlText(g_monitorEdit);
    if (path.empty()) {
        MessageBoxW(g_hWnd, L"Введите путь к директории для мониторинга", kAppTitle, MB_ICONWARNING | MB_OK);
        return;
    }

    TrayAppOperationResult result = {};
    if (!AddMonitoredDirectoryViaRpc(path, &result)) {
        ShowRpcFailureMessage(L"Не удалось добавить директорию в мониторинг");
        return;
    }

    if (result.statusCode != TRAYAPP_RPC_STATUS_OK) {
        MessageBoxW(g_hWnd, result.message, kAppTitle, MB_ICONERROR | MB_OK);
    }
    RefreshStateFromService();
}

static void HandleRemoveMonitor()
{
    const std::wstring path = GetControlText(g_monitorEdit);
    if (path.empty()) {
        MessageBoxW(g_hWnd, L"Введите путь к директории для удаления из мониторинга", kAppTitle, MB_ICONWARNING | MB_OK);
        return;
    }

    TrayAppOperationResult result = {};
    if (!RemoveMonitoredDirectoryViaRpc(path, &result)) {
        ShowRpcFailureMessage(L"Не удалось удалить директорию из мониторинга");
        return;
    }

    if (result.statusCode != TRAYAPP_RPC_STATUS_OK) {
        MessageBoxW(g_hWnd, result.message, kAppTitle, MB_ICONERROR | MB_OK);
    }
    RefreshStateFromService();
}

static const wchar_t* ScanObjectTypeToText(TrayAppScanObjectType objectType)
{
    switch (objectType) {
    case TRAYAPP_SCAN_OBJECT_PE:
        return L"PE";
    case TRAYAPP_SCAN_OBJECT_POWERSHELL:
        return L"PowerShell";
    default:
        return L"Unknown";
    }
}

static void UpdateScanResultsText(const TrayAppScanResult& result)
{
    std::wstring text = L"Результат сканирования\r\n";
    text += L"Цель: ";
    text += result.targetPath;
    text += L"\r\n";
    text += L"Статус: ";
    text += (result.completed ? L"завершено" : L"ошибка");
    text += L"\r\n";
    text += L"Просканировано файлов: ";
    text += std::to_wstring(result.scannedFileCount);
    text += L"\r\n";
    text += L"Обнаружено угроз: ";
    text += std::to_wstring(result.detectedFileCount);
    text += L"\r\n";
    text += L"Тип объекта: ";
    text += ScanObjectTypeToText(result.objectType);
    text += L"\r\n";

    if (result.malicious) {
        text += L"Детект: да\r\n";
        text += L"Файл: ";
        text += result.detectedPath;
        text += L"\r\n";
        text += L"Смещение: ";
        text += std::to_wstring(result.firstMatchOffset);
        text += L"\r\n";
    } else {
        text += L"Детект: нет\r\n";
    }

    if (result.details[0] != 0) {
        text += L"Подробности: ";
        text += result.details;
        text += L"\r\n";
    }

    SetWindowTextW(g_scanResultsEdit, text.c_str());
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == g_wmTaskbarCreated && g_wmTaskbarCreated != 0) {
        AddTrayIcon(hWnd);
        return 0;
    }

    switch (message) {
    case WM_CREATE:
        CreateUiControls(hWnd);
        LayoutControls(hWnd);
        SetTimer(hWnd, kStateRefreshTimerId, kStateRefreshIntervalMs, nullptr);
        PostMessageW(hWnd, WM_APP_INITIALIZE, 0, 0);
        break;

    case WM_SIZE:
        LayoutControls(hWnd);
        break;

    case WM_APP_INITIALIZE:
        WriteDebugLog(L"WM_APP_INITIALIZE received");
        RefreshStateFromService();
        break;

    case WM_TIMER:
        if (wParam == kStateRefreshTimerId) {
            RefreshStateFromService();
        } else if (wParam == kTrayRetryTimerId && !g_trayIconAdded) {
            WriteDebugLog(L"Retrying tray icon add");
            AddTrayIcon(hWnd);
        }
        break;

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
        case IDC_LOGIN_BUTTON:
            HandleLogin();
            break;
        case IDC_ACTIVATE_BUTTON:
            HandleActivate();
            break;
        case IDC_LOGOUT_BUTTON:
            HandleLogout();
            break;
        case IDC_SCAN_FILE_BTN:
            HandleScanFile();
            break;
        case IDC_SCAN_DIR_BTN:
            HandleScanDirectory();
            break;
        case IDC_SCAN_FIXED_BTN:
            HandleScanFixedDisks();
            break;
        case IDC_SCHEDULE_ON_BTN:
            HandleEnableSchedule();
            break;
        case IDC_SCHEDULE_OFF_BTN:
            HandleDisableSchedule();
            break;
        case IDC_MONITOR_ADD_BTN:
            HandleAddMonitor();
            break;
        case IDC_MONITOR_DEL_BTN:
            HandleRemoveMonitor();
            break;
        }
        break;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        KillTimer(hWnd, kStateRefreshTimerId);
        KillTimer(hWnd, kTrayRetryTimerId);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }

    return 0;
}




