#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NTDDI_VERSION   NTDDI_VISTA
#define _WIN32_WINNT    _WIN32_WINNT_VISTA

#include <windows.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <rpc.h>

#include <ctime>
#include <fstream>
#include <string>
#include <vector>

#include "backend_client.h"
#include "trayapp_rpc.h"

namespace {
constexpr wchar_t kServiceName[] = L"TrayAppService";
constexpr wchar_t kServiceDisplayName[] = L"TrayApp Background Service";
constexpr wchar_t kServiceDescription[] = L"Launches TrayApp in user sessions and keeps auth/license state in memory.";
constexpr wchar_t kTrayAppBinaryName[] = L"TrayApp.exe";
constexpr wchar_t kRpcProtocolSequence[] = L"ncalrpc";
constexpr wchar_t kRpcEndpoint[] = L"TrayAppServiceRpcEndpoint";

struct LaunchedProcess {
    DWORD sessionId;
    DWORD processId;
    HANDLE processHandle;
};

struct ServiceState {
    AuthSessionData auth;
    LicenseTicketData license;
    TrayAppRpcStatusCode authStatus = TRAYAPP_RPC_STATUS_NOT_AUTHENTICATED;
    TrayAppRpcStatusCode licenseStatus = TRAYAPP_RPC_STATUS_NO_LICENSE;
    std::wstring authMessage = L"Войдите в учетную запись";
    std::wstring licenseMessage = L"Лицензия отсутствует";
};
}

static SERVICE_STATUS g_serviceStatus = {};
static SERVICE_STATUS_HANDLE g_serviceStatusHandle = nullptr;
static HANDLE g_stopEvent = nullptr;
static HANDLE g_workerThread = nullptr;
static CRITICAL_SECTION g_processLock = {};
static CRITICAL_SECTION g_stateLock = {};
static bool g_processLockInitialized = false;
static bool g_stateLockInitialized = false;
static std::vector<LaunchedProcess> g_launchedProcesses;
static ServiceState g_serviceState = {};

static void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
static DWORD WINAPI ServiceCtrlHandler(DWORD control, DWORD eventType, LPVOID eventData, LPVOID context);
static DWORD WINAPI BackgroundWorkerThread(LPVOID parameter);
static void SetCurrentServiceStatus(DWORD state, DWORD exitCode = NO_ERROR);
static bool InitializeRpcServer();
static void ShutdownRpcServer();
static void RequestServiceShutdown();
static void LogServiceEvent(WORD type, const std::wstring& message);
static void AppendDebugLog(const std::wstring& message);
static void LaunchTrayAppInSession(DWORD sessionId);
static void LaunchTrayAppInAllSessions();
static void TerminateLaunchedProcesses();
static void CleanupTrackedProcessesLocked();
static bool HasRunningProcessForSessionLocked(DWORD sessionId);
static std::wstring GetSiblingPath(const wchar_t* fileName);
static std::wstring GetDirectoryName(const std::wstring& path);
static int InstallService();
static int UninstallService();
static long long GetNowUnixSeconds();
static void CopyTextToRpcBuffer(const std::wstring& text, wchar_t* buffer, size_t bufferCount);
static void FillAuthState(TrayAppAuthState* state);
static void FillLicenseState(TrayAppLicenseState* state);
static void FillOperationResult(TrayAppOperationResult* result, TrayAppRpcStatusCode statusCode, const std::wstring& message);
static void ClearLicenseStateLocked(TrayAppRpcStatusCode statusCode, const std::wstring& message);
static void ClearAuthStateLocked(TrayAppRpcStatusCode statusCode, const std::wstring& message);
static TrayAppRpcStatusCode ToRpcStatusCode(int statusCode);
static bool IsLicenseUsableLocked();
static void UpdateLicenseStateLocked(const LicenseTicketData& licenseData, TrayAppRpcStatusCode statusCode, const std::wstring& message);
static void UpdateAuthStateLocked(const AuthSessionData& authData, TrayAppRpcStatusCode statusCode, const std::wstring& message);

extern "C" void TrayAppRpcStopService(handle_t)
{
    RequestServiceShutdown();
}

extern "C" void TrayAppRpcGetAuthState(handle_t, TrayAppAuthState* state)
{
    FillAuthState(state);
}

extern "C" void TrayAppRpcLogin(handle_t, wchar_t* email, wchar_t* password, TrayAppAuthState* state)
{
    if (!state || !email || !password) {
        return;
    }

    AuthSessionData authData = {};
    const BackendResult result = BackendLogin(email, password, &authData);

    EnterCriticalSection(&g_stateLock);
    if (result.statusCode == kStatusOk) {
        UpdateAuthStateLocked(authData, TRAYAPP_RPC_STATUS_OK, L"");
        ClearLicenseStateLocked(TRAYAPP_RPC_STATUS_NO_LICENSE, L"Лицензия отсутствует");
    } else {
        ClearAuthStateLocked(ToRpcStatusCode(result.statusCode), result.message);
        ClearLicenseStateLocked(TRAYAPP_RPC_STATUS_NO_LICENSE, L"Лицензия отсутствует");
    }
    LeaveCriticalSection(&g_stateLock);
    FillAuthState(state);
}

extern "C" void TrayAppRpcLogout(handle_t, TrayAppOperationResult* result)
{
    EnterCriticalSection(&g_stateLock);
    ClearAuthStateLocked(TRAYAPP_RPC_STATUS_NOT_AUTHENTICATED, L"Войдите в учетную запись");
    ClearLicenseStateLocked(TRAYAPP_RPC_STATUS_NO_LICENSE, L"Лицензия отсутствует");
    LeaveCriticalSection(&g_stateLock);

    FillOperationResult(result, TRAYAPP_RPC_STATUS_OK, L"");
}

extern "C" void TrayAppRpcGetLicenseState(handle_t, TrayAppLicenseState* state)
{
    FillLicenseState(state);
}

extern "C" void TrayAppRpcActivateLicense(handle_t, wchar_t* activationCode, TrayAppLicenseState* state)
{
    if (!state || !activationCode) {
        return;
    }

    AuthSessionData authSnapshot = {};
    EnterCriticalSection(&g_stateLock);
    if (!g_serviceState.auth.authenticated) {
        FillLicenseState(state);
        LeaveCriticalSection(&g_stateLock);
        return;
    }
    authSnapshot = g_serviceState.auth;
    LeaveCriticalSection(&g_stateLock);

    LicenseTicketData licenseData = {};
    const BackendResult result = BackendActivateLicense(authSnapshot, activationCode, &licenseData);

    EnterCriticalSection(&g_stateLock);
    if (result.statusCode == kStatusOk) {
        UpdateLicenseStateLocked(licenseData, TRAYAPP_RPC_STATUS_OK, L"");
    } else {
        ClearLicenseStateLocked(ToRpcStatusCode(result.statusCode), result.message);
    }
    LeaveCriticalSection(&g_stateLock);
    FillLicenseState(state);
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
    InitializeCriticalSection(&g_stateLock);
    g_processLockInitialized = true;
    g_stateLockInitialized = true;

    EnterCriticalSection(&g_stateLock);
    ClearAuthStateLocked(TRAYAPP_RPC_STATUS_NOT_AUTHENTICATED, L"Войдите в учетную запись");
    ClearLicenseStateLocked(TRAYAPP_RPC_STATUS_NO_LICENSE, L"Лицензия отсутствует");
    LeaveCriticalSection(&g_stateLock);

    if (!InitializeRpcServer()) {
        if (g_stateLockInitialized) {
            DeleteCriticalSection(&g_stateLock);
            g_stateLockInitialized = false;
        }
        if (g_processLockInitialized) {
            DeleteCriticalSection(&g_processLock);
            g_processLockInitialized = false;
        }
        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;
        SetCurrentServiceStatus(SERVICE_STOPPED, ERROR_GEN_FAILURE);
        return;
    }

    g_workerThread = CreateThread(nullptr, 0, BackgroundWorkerThread, nullptr, 0, nullptr);

    const RPC_STATUS listenStatus = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
    if (listenStatus != RPC_S_OK && listenStatus != RPC_S_ALREADY_LISTENING) {
        SetCurrentServiceStatus(SERVICE_STOP_PENDING);
        RequestServiceShutdown();
    } else {
        SetCurrentServiceStatus(SERVICE_RUNNING);
        LaunchTrayAppInAllSessions();
        WaitForSingleObject(g_stopEvent, INFINITE);
        SetCurrentServiceStatus(SERVICE_STOP_PENDING);
        RpcMgmtStopServerListening(nullptr);
        RpcMgmtWaitServerListen();
    }

    if (g_workerThread) {
        WaitForSingleObject(g_workerThread, 5000);
        CloseHandle(g_workerThread);
        g_workerThread = nullptr;
    }

    ShutdownRpcServer();
    TerminateLaunchedProcesses();

    if (g_stateLockInitialized) {
        DeleteCriticalSection(&g_stateLock);
        g_stateLockInitialized = false;
    }
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

static DWORD WINAPI BackgroundWorkerThread(LPVOID)
{
    while (WaitForSingleObject(g_stopEvent, 1000) == WAIT_TIMEOUT) {
        AuthSessionData authSnapshot = {};
        bool shouldRefreshTokens = false;
        bool shouldRefreshLicense = false;

        EnterCriticalSection(&g_stateLock);
        const long long now = GetNowUnixSeconds();
        if (g_serviceState.auth.authenticated) {
            authSnapshot = g_serviceState.auth;
            shouldRefreshTokens = (g_serviceState.auth.accessTokenExpiresAtUnixSeconds != 0) &&
                (now >= g_serviceState.auth.accessTokenExpiresAtUnixSeconds - 60);
            shouldRefreshLicense = IsLicenseUsableLocked() &&
                (g_serviceState.license.nextRefreshUnixSeconds != 0) &&
                (now >= g_serviceState.license.nextRefreshUnixSeconds);
        }
        LeaveCriticalSection(&g_stateLock);

        if (shouldRefreshTokens) {
            AuthSessionData refreshedAuth = {};
            const BackendResult refreshResult = BackendRefreshTokens(authSnapshot.refreshToken, &refreshedAuth);
            EnterCriticalSection(&g_stateLock);
            if (refreshResult.statusCode == kStatusOk) {
                UpdateAuthStateLocked(refreshedAuth, TRAYAPP_RPC_STATUS_OK, L"");
            } else {
                ClearAuthStateLocked(ToRpcStatusCode(refreshResult.statusCode), refreshResult.message);
                ClearLicenseStateLocked(TRAYAPP_RPC_STATUS_NO_LICENSE, L"Лицензия отсутствует");
                LeaveCriticalSection(&g_stateLock);
                continue;
            }
            authSnapshot = g_serviceState.auth;
            LeaveCriticalSection(&g_stateLock);
        }

        if (shouldRefreshLicense) {
            LicenseTicketData refreshedLicense = {};
            const BackendResult licenseResult = BackendCheckLicense(authSnapshot, &refreshedLicense);
            EnterCriticalSection(&g_stateLock);
            if (licenseResult.statusCode == kStatusOk) {
                UpdateLicenseStateLocked(refreshedLicense, TRAYAPP_RPC_STATUS_OK, L"");
            } else {
                ClearLicenseStateLocked(ToRpcStatusCode(licenseResult.statusCode), licenseResult.message);
            }
            LeaveCriticalSection(&g_stateLock);
        }
    }

    return 0;
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
}

static void LogServiceEvent(WORD type, const std::wstring& message)
{
    HANDLE eventSource = RegisterEventSourceW(nullptr, kServiceName);
    if (!eventSource) {
        return;
    }

    LPCWSTR strings[1] = { message.c_str() };
    ReportEventW(
        eventSource,
        type,
        0,
        0,
        nullptr,
        1,
        0,
        strings,
        nullptr);
    DeregisterEventSource(eventSource);
}

static void AppendDebugLog(const std::wstring& message)
{
    const std::wstring logPath = GetSiblingPath(L"TrayAppService-debug.log");
    SYSTEMTIME now = {};
    GetLocalTime(&now);

    std::wofstream stream(logPath, std::ios::app);
    if (!stream.is_open()) {
        return;
    }

    stream
        << L'['
        << now.wYear << L'-'
        << (now.wMonth < 10 ? L"0" : L"") << now.wMonth << L'-'
        << (now.wDay < 10 ? L"0" : L"") << now.wDay << L' '
        << (now.wHour < 10 ? L"0" : L"") << now.wHour << L':'
        << (now.wMinute < 10 ? L"0" : L"") << now.wMinute << L':'
        << (now.wSecond < 10 ? L"0" : L"") << now.wSecond
        << L"] "
        << message
        << std::endl;
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

    AppendDebugLog(L"LaunchTrayAppInSession start, session=" + std::to_wstring(sessionId));

    HANDLE userToken = nullptr;
    if (!WTSQueryUserToken(sessionId, &userToken)) {
        const DWORD error = GetLastError();
        AppendDebugLog(L"WTSQueryUserToken failed, session=" + std::to_wstring(sessionId) + L", error=" + std::to_wstring(error));
        LogServiceEvent(EVENTLOG_ERROR_TYPE, L"WTSQueryUserToken failed for session " + std::to_wstring(sessionId) + L", error=" + std::to_wstring(error));
        return;
    }
    AppendDebugLog(L"WTSQueryUserToken ok, session=" + std::to_wstring(sessionId));

    HANDLE primaryToken = nullptr;
    if (!DuplicateTokenEx(
            userToken,
            MAXIMUM_ALLOWED,
            nullptr,
            SecurityImpersonation,
            TokenPrimary,
            &primaryToken)) {
        const DWORD error = GetLastError();
        AppendDebugLog(L"DuplicateTokenEx failed, session=" + std::to_wstring(sessionId) + L", error=" + std::to_wstring(error));
        LogServiceEvent(EVENTLOG_ERROR_TYPE, L"DuplicateTokenEx failed for session " + std::to_wstring(sessionId) + L", error=" + std::to_wstring(error));
        CloseHandle(userToken);
        return;
    }
    AppendDebugLog(L"DuplicateTokenEx ok, session=" + std::to_wstring(sessionId));

    LPVOID environmentBlock = nullptr;
    if (!CreateEnvironmentBlock(&environmentBlock, primaryToken, FALSE)) {
        const DWORD error = GetLastError();
        AppendDebugLog(L"CreateEnvironmentBlock failed, session=" + std::to_wstring(sessionId) + L", error=" + std::to_wstring(error));
        LogServiceEvent(EVENTLOG_WARNING_TYPE, L"CreateEnvironmentBlock failed for session " + std::to_wstring(sessionId) + L", error=" + std::to_wstring(error));
    } else {
        AppendDebugLog(L"CreateEnvironmentBlock ok, session=" + std::to_wstring(sessionId));
    }

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
        const DWORD error = GetLastError();
        AppendDebugLog(L"CreateProcessAsUserW failed, session=" + std::to_wstring(sessionId) + L", error=" + std::to_wstring(error) + L", path=" + appPath);
        LogServiceEvent(EVENTLOG_ERROR_TYPE, L"CreateProcessAsUserW failed for session " + std::to_wstring(sessionId) + L", error=" + std::to_wstring(error) + L", path=" + appPath);
        return;
    }

    CloseHandle(processInformation.hThread);
    AppendDebugLog(L"CreateProcessAsUserW ok, session=" + std::to_wstring(sessionId) + L", pid=" + std::to_wstring(processInformation.dwProcessId));
    LogServiceEvent(EVENTLOG_INFORMATION_TYPE, L"TrayApp launched in session " + std::to_wstring(sessionId) + L", pid=" + std::to_wstring(processInformation.dwProcessId));

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

static long long GetNowUnixSeconds()
{
    return static_cast<long long>(time(nullptr));
}

static void CopyTextToRpcBuffer(const std::wstring& text, wchar_t* buffer, size_t bufferCount)
{
    if (!buffer || bufferCount == 0) {
        return;
    }
    lstrcpynW(buffer, text.c_str(), static_cast<int>(bufferCount));
}

static void FillAuthState(TrayAppAuthState* state)
{
    if (!state) {
        return;
    }

    ZeroMemory(state, sizeof(*state));
    EnterCriticalSection(&g_stateLock);
    state->statusCode = g_serviceState.authStatus;
    state->authenticated = g_serviceState.auth.authenticated ? TRUE : FALSE;
    state->antivirusEnabled = IsLicenseUsableLocked() ? TRUE : FALSE;
    CopyTextToRpcBuffer(g_serviceState.auth.email, state->email, ARRAYSIZE(state->email));
    CopyTextToRpcBuffer(g_serviceState.authMessage, state->message, ARRAYSIZE(state->message));
    LeaveCriticalSection(&g_stateLock);
}

static void FillLicenseState(TrayAppLicenseState* state)
{
    if (!state) {
        return;
    }

    ZeroMemory(state, sizeof(*state));
    EnterCriticalSection(&g_stateLock);
    state->statusCode = g_serviceState.licenseStatus;
    state->hasLicense = g_serviceState.license.hasLicense ? TRUE : FALSE;
    state->blocked = g_serviceState.license.blocked ? TRUE : FALSE;
    state->expired = g_serviceState.license.expired ? TRUE : FALSE;
    state->antivirusEnabled = IsLicenseUsableLocked() ? TRUE : FALSE;
    state->expiresAtUnixSeconds = g_serviceState.license.expiresAtUnixSeconds;
    CopyTextToRpcBuffer(g_serviceState.license.expiresAtText, state->expiresAtText, ARRAYSIZE(state->expiresAtText));
    CopyTextToRpcBuffer(g_serviceState.licenseMessage, state->message, ARRAYSIZE(state->message));
    LeaveCriticalSection(&g_stateLock);
}

static void FillOperationResult(TrayAppOperationResult* result, TrayAppRpcStatusCode statusCode, const std::wstring& message)
{
    if (!result) {
        return;
    }
    ZeroMemory(result, sizeof(*result));
    result->statusCode = statusCode;
    CopyTextToRpcBuffer(message, result->message, ARRAYSIZE(result->message));
}

static void ClearLicenseStateLocked(TrayAppRpcStatusCode statusCode, const std::wstring& message)
{
    g_serviceState.license = {};
    g_serviceState.licenseStatus = statusCode;
    g_serviceState.licenseMessage = message;
}

static void ClearAuthStateLocked(TrayAppRpcStatusCode statusCode, const std::wstring& message)
{
    g_serviceState.auth = {};
    g_serviceState.authStatus = statusCode;
    g_serviceState.authMessage = message;
}

static TrayAppRpcStatusCode ToRpcStatusCode(int statusCode)
{
    switch (statusCode) {
    case kStatusOk:
        return TRAYAPP_RPC_STATUS_OK;
    case kStatusNotAuthenticated:
        return TRAYAPP_RPC_STATUS_NOT_AUTHENTICATED;
    case kStatusInvalidCredentials:
        return TRAYAPP_RPC_STATUS_INVALID_CREDENTIALS;
    case kStatusNetworkError:
        return TRAYAPP_RPC_STATUS_NETWORK_ERROR;
    case kStatusNoLicense:
        return TRAYAPP_RPC_STATUS_NO_LICENSE;
    case kStatusLicenseBlocked:
        return TRAYAPP_RPC_STATUS_LICENSE_BLOCKED;
    case kStatusLicenseExpired:
        return TRAYAPP_RPC_STATUS_LICENSE_EXPIRED;
    case kStatusActivationFailed:
        return TRAYAPP_RPC_STATUS_ACTIVATION_FAILED;
    default:
        return TRAYAPP_RPC_STATUS_SERVER_ERROR;
    }
}

static bool IsLicenseUsableLocked()
{
    return g_serviceState.auth.authenticated &&
        g_serviceState.license.hasLicense &&
        !g_serviceState.license.blocked &&
        !g_serviceState.license.expired;
}

static void UpdateLicenseStateLocked(const LicenseTicketData& licenseData, TrayAppRpcStatusCode statusCode, const std::wstring& message)
{
    g_serviceState.license = licenseData;
    g_serviceState.licenseStatus = statusCode;
    g_serviceState.licenseMessage = message.empty() ? licenseData.message : message;
    if (statusCode == TRAYAPP_RPC_STATUS_OK) {
        g_serviceState.license.hasLicense = true;
        g_serviceState.license.blocked = false;
        g_serviceState.license.expired = false;
        g_serviceState.licenseMessage.clear();
    }
}

static void UpdateAuthStateLocked(const AuthSessionData& authData, TrayAppRpcStatusCode statusCode, const std::wstring& message)
{
    g_serviceState.auth = authData;
    g_serviceState.authStatus = statusCode;
    g_serviceState.authMessage = message;
}
