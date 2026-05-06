#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NTDDI_VERSION   NTDDI_VISTA
#define _WIN32_WINNT    _WIN32_WINNT_VISTA

#include <windows.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <rpc.h>

#include <string>
#include <vector>

#include "trayapp_rpc.h"

namespace {
constexpr wchar_t kServiceName[] = L"TrayAppService";
constexpr wchar_t kServiceDisplayName[] = L"TrayApp Background Service";
constexpr wchar_t kServiceDescription[] = L"Launches TrayApp in user sessions and exposes an RPC stop endpoint.";
constexpr wchar_t kTrayAppBinaryName[] = L"TrayApp.exe";
constexpr wchar_t kRpcProtocolSequence[] = L"ncalrpc";
constexpr wchar_t kRpcEndpoint[] = L"TrayAppServiceRpcEndpoint";
}

struct LaunchedProcess {
    DWORD sessionId;
    DWORD processId;
    HANDLE processHandle;
};

static SERVICE_STATUS g_serviceStatus = {};
static SERVICE_STATUS_HANDLE g_serviceStatusHandle = nullptr;
static HANDLE g_stopEvent = nullptr;
static CRITICAL_SECTION g_processLock = {};
static bool g_processLockInitialized = false;
static std::vector<LaunchedProcess> g_launchedProcesses;

static void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
static DWORD WINAPI ServiceCtrlHandler(DWORD control, DWORD eventType, LPVOID eventData, LPVOID context);
static void SetCurrentServiceStatus(DWORD state, DWORD exitCode = NO_ERROR);
static bool InitializeRpcServer();
static void ShutdownRpcServer();
static void RequestServiceShutdown();
static void LaunchTrayAppInSession(DWORD sessionId);
static void LaunchTrayAppInAllSessions();
static void TerminateLaunchedProcesses();
static void CleanupTrackedProcessesLocked();
static bool HasRunningProcessForSessionLocked(DWORD sessionId);
static std::wstring GetSiblingPath(const wchar_t* fileName);
static std::wstring GetDirectoryName(const std::wstring& path);
static int InstallService();
static int UninstallService();

extern "C" void TrayAppRpcStopService(handle_t)
{
    RequestServiceShutdown();
}

int wmain(int argc, wchar_t* argv[])
{
    SERVICE_TABLE_ENTRYW dispatchTable[] = {
        { const_cast<LPWSTR>(kServiceName), ServiceMain },
        { nullptr, nullptr }
    };

    if (StartServiceCtrlDispatcherW(dispatchTable)) {
        return 0;
    }

    if (argc >= 2) {
        if (_wcsicmp(argv[1], L"install") == 0) {
            return InstallService();
        }

        if (_wcsicmp(argv[1], L"uninstall") == 0) {
            return UninstallService();
        }
    }

    return 1;
}

static void WINAPI ServiceMain(DWORD, LPTSTR*)
{
    g_serviceStatusHandle = RegisterServiceCtrlHandlerExW(kServiceName, ServiceCtrlHandler, nullptr);
    if (!g_serviceStatusHandle) {
        return;
    }

    SetCurrentServiceStatus(SERVICE_START_PENDING);

    g_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_stopEvent) {
        SetCurrentServiceStatus(SERVICE_STOPPED, GetLastError());
        return;
    }

    InitializeCriticalSection(&g_processLock);
    g_processLockInitialized = true;

    if (!InitializeRpcServer()) {
        if (g_processLockInitialized) {
            DeleteCriticalSection(&g_processLock);
            g_processLockInitialized = false;
        }
        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;
        SetCurrentServiceStatus(SERVICE_STOPPED, ERROR_GEN_FAILURE);
        return;
    }

    LaunchTrayAppInAllSessions();
    SetCurrentServiceStatus(SERVICE_RUNNING);

    const RPC_STATUS listenStatus = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);

    SetCurrentServiceStatus(SERVICE_STOP_PENDING);
    ShutdownRpcServer();
    TerminateLaunchedProcesses();

    if (g_processLockInitialized) {
        DeleteCriticalSection(&g_processLock);
        g_processLockInitialized = false;
    }

    if (g_stopEvent) {
        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;
    }

    const DWORD exitCode = (listenStatus == RPC_S_OK || listenStatus == RPC_S_ALREADY_LISTENING)
        ? NO_ERROR
        : listenStatus;
    SetCurrentServiceStatus(SERVICE_STOPPED, exitCode);
}

static DWORD WINAPI ServiceCtrlHandler(DWORD control, DWORD eventType, LPVOID eventData, LPVOID)
{
    switch (control) {
    case SERVICE_CONTROL_SESSIONCHANGE:
        if (eventType == WTS_SESSION_LOGON && eventData) {
            const auto* notification = reinterpret_cast<WTSSESSION_NOTIFICATION*>(eventData);
            if (notification->dwSessionId != 0) {
                LaunchTrayAppInSession(notification->dwSessionId);
            }
        }
        return NO_ERROR;

    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        return NO_ERROR;

    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;

    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
}

static void SetCurrentServiceStatus(DWORD state, DWORD exitCode)
{
    g_serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_serviceStatus.dwCurrentState = state;
    g_serviceStatus.dwWin32ExitCode = exitCode;
    g_serviceStatus.dwControlsAccepted = (state == SERVICE_RUNNING) ? SERVICE_ACCEPT_SESSIONCHANGE : 0;
    g_serviceStatus.dwCheckPoint = 0;
    g_serviceStatus.dwWaitHint = 0;
    ::SetServiceStatus(g_serviceStatusHandle, &g_serviceStatus);
}

static bool InitializeRpcServer()
{
    RPC_STATUS status = RpcServerUseProtseqEpW(
        reinterpret_cast<RPC_WSTR>(const_cast<wchar_t*>(kRpcProtocolSequence)),
        RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
        reinterpret_cast<RPC_WSTR>(const_cast<wchar_t*>(kRpcEndpoint)),
        nullptr);
    if (status != RPC_S_OK) {
        return false;
    }

    status = RpcServerRegisterIf2(
        TrayAppRpc_v1_0_s_ifspec,
        nullptr,
        nullptr,
        RPC_IF_ALLOW_LOCAL_ONLY,
        RPC_C_LISTEN_MAX_CALLS_DEFAULT,
        static_cast<unsigned int>(-1),
        nullptr);
    return status == RPC_S_OK;
}

static void ShutdownRpcServer()
{
    RpcServerUnregisterIf(nullptr, nullptr, FALSE);
}

static void RequestServiceShutdown()
{
    if (g_stopEvent) {
        SetEvent(g_stopEvent);
    }

    RpcMgmtStopServerListening(nullptr);
}

static void LaunchTrayAppInSession(DWORD sessionId)
{
    if (sessionId == 0 || !g_processLockInitialized) {
        return;
    }

    EnterCriticalSection(&g_processLock);
    CleanupTrackedProcessesLocked();
    if (HasRunningProcessForSessionLocked(sessionId)) {
        LeaveCriticalSection(&g_processLock);
        return;
    }
    LeaveCriticalSection(&g_processLock);

    HANDLE userToken = nullptr;
    if (!WTSQueryUserToken(sessionId, &userToken)) {
        return;
    }

    HANDLE primaryToken = nullptr;
    if (!DuplicateTokenEx(
            userToken,
            MAXIMUM_ALLOWED,
            nullptr,
            SecurityImpersonation,
            TokenPrimary,
            &primaryToken)) {
        CloseHandle(userToken);
        return;
    }

    LPVOID environmentBlock = nullptr;
    CreateEnvironmentBlock(&environmentBlock, primaryToken, FALSE);

    const std::wstring appPath = GetSiblingPath(kTrayAppBinaryName);
    const std::wstring workingDirectory = GetDirectoryName(appPath);
    const std::wstring commandLine = L"\"" + appPath + L"\" --hidden";
    std::vector<wchar_t> commandLineBuffer(commandLine.begin(), commandLine.end());
    commandLineBuffer.push_back(L'\0');

    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.lpDesktop = const_cast<LPWSTR>(L"winsta0\\default");
    startupInfo.dwFlags = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION processInformation = {};
    const BOOL created = CreateProcessAsUserW(
        primaryToken,
        appPath.c_str(),
        commandLineBuffer.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_UNICODE_ENVIRONMENT,
        environmentBlock,
        workingDirectory.empty() ? nullptr : workingDirectory.c_str(),
        &startupInfo,
        &processInformation);

    if (environmentBlock) {
        DestroyEnvironmentBlock(environmentBlock);
    }
    CloseHandle(primaryToken);
    CloseHandle(userToken);

    if (!created) {
        return;
    }

    CloseHandle(processInformation.hThread);

    EnterCriticalSection(&g_processLock);
    CleanupTrackedProcessesLocked();
    g_launchedProcesses.push_back({ sessionId, processInformation.dwProcessId, processInformation.hProcess });
    LeaveCriticalSection(&g_processLock);
}

static void LaunchTrayAppInAllSessions()
{
    PWTS_SESSION_INFOW sessionInfo = nullptr;
    DWORD sessionCount = 0;
    if (!WTSEnumerateSessionsW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &sessionInfo, &sessionCount)) {
        return;
    }

    for (DWORD index = 0; index < sessionCount; ++index) {
        if (sessionInfo[index].SessionId == 0) {
            continue;
        }

        LaunchTrayAppInSession(sessionInfo[index].SessionId);
    }

    WTSFreeMemory(sessionInfo);
}

static void TerminateLaunchedProcesses()
{
    if (!g_processLockInitialized) {
        return;
    }

    EnterCriticalSection(&g_processLock);
    for (LaunchedProcess& processInfo : g_launchedProcesses) {
        if (processInfo.processHandle) {
            TerminateProcess(processInfo.processHandle, 0);
            WaitForSingleObject(processInfo.processHandle, 5000);
            CloseHandle(processInfo.processHandle);
            processInfo.processHandle = nullptr;
        }
    }
    g_launchedProcesses.clear();
    LeaveCriticalSection(&g_processLock);
}

static void CleanupTrackedProcessesLocked()
{
    size_t writeIndex = 0;
    for (size_t readIndex = 0; readIndex < g_launchedProcesses.size(); ++readIndex) {
        LaunchedProcess& processInfo = g_launchedProcesses[readIndex];
        if (!processInfo.processHandle || WaitForSingleObject(processInfo.processHandle, 0) == WAIT_OBJECT_0) {
            if (processInfo.processHandle) {
                CloseHandle(processInfo.processHandle);
            }
            continue;
        }

        if (writeIndex != readIndex) {
            g_launchedProcesses[writeIndex] = processInfo;
        }
        ++writeIndex;
    }

    g_launchedProcesses.resize(writeIndex);
}

static bool HasRunningProcessForSessionLocked(DWORD sessionId)
{
    for (const LaunchedProcess& processInfo : g_launchedProcesses) {
        if (processInfo.sessionId == sessionId &&
            processInfo.processHandle &&
            WaitForSingleObject(processInfo.processHandle, 0) == WAIT_TIMEOUT) {
            return true;
        }
    }

    return false;
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

static std::wstring GetDirectoryName(const std::wstring& path)
{
    const size_t separator = path.find_last_of(L'\\');
    if (separator == std::wstring::npos) {
        return L"";
    }

    return path.substr(0, separator);
}

static int InstallService()
{
    SC_HANDLE scmHandle = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!scmHandle) {
        return 1;
    }

    const std::wstring servicePath = GetSiblingPath(L"TrayAppService.exe");

    SC_HANDLE serviceHandle = CreateServiceW(
        scmHandle,
        kServiceName,
        kServiceDisplayName,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        servicePath.c_str(),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    if (!serviceHandle) {
        const DWORD error = GetLastError();
        CloseServiceHandle(scmHandle);
        wprintf(L"Failed to install service: %lu\n", error);
        return 1;
    }

    SERVICE_DESCRIPTIONW description = {};
    description.lpDescription = const_cast<LPWSTR>(kServiceDescription);
    ChangeServiceConfig2W(serviceHandle, SERVICE_CONFIG_DESCRIPTION, &description);

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);
    wprintf(L"Service installed successfully.\n");
    return 0;
}

static int UninstallService()
{
    SC_HANDLE scmHandle = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scmHandle) {
        return 1;
    }

    SC_HANDLE serviceHandle = OpenServiceW(
        scmHandle,
        kServiceName,
        DELETE | SERVICE_QUERY_STATUS);
    if (!serviceHandle) {
        CloseServiceHandle(scmHandle);
        return 1;
    }

    const bool deleted = DeleteService(serviceHandle) == TRUE;
    if (deleted) {
        wprintf(L"Service uninstalled successfully.\n");
    } else {
        wprintf(L"Failed to uninstall service: %lu\n", GetLastError());
    }

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);
    return deleted ? 0 : 1;
}
