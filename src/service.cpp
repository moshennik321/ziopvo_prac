#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NTDDI_VERSION   NTDDI_VISTA
#define _WIN32_WINNT    _WIN32_WINNT_VISTA

#include <windows.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <rpc.h>

#include <algorithm>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>

#include "antivirus_engine.h"
#include "backend_client.h"
#include "trayapp_rpc.h"

namespace {
constexpr wchar_t kServiceName[] = L"TrayAppService";
constexpr wchar_t kServiceDisplayName[] = L"TrayApp Background Service";
constexpr wchar_t kServiceDescription[] = L"Launches TrayApp in user sessions and keeps auth/license state in memory.";
constexpr wchar_t kTrayAppBinaryName[] = L"TrayApp.exe";
constexpr wchar_t kRpcProtocolSequence[] = L"ncalrpc";
constexpr wchar_t kRpcEndpoint[] = L"TrayAppServiceRpcEndpoint";
constexpr wchar_t kAvDbDirectory[] = L"avdb";
constexpr wchar_t kAvDbDefaultDirectory[] = L"avdb\\default";
constexpr wchar_t kAvDbBackupDirectory[] = L"avdb\\backup";
constexpr wchar_t kAvDbManifestFile[] = L"avdb\\manifest.bin";
constexpr wchar_t kAvDbDataFile[] = L"avdb\\data.bin";
constexpr wchar_t kAvDbDownloadManifestFile[] = L"avdb\\download_manifest.bin";
constexpr wchar_t kAvDbDownloadDataFile[] = L"avdb\\download_data.bin";
constexpr wchar_t kAvDbRecoveredManifestFile[] = L"avdb\\recovered_manifest.bin";
constexpr wchar_t kAvDbRecoveredDataFile[] = L"avdb\\recovered_data.bin";
constexpr wchar_t kAvDbDefaultManifestFile[] = L"avdb\\default\\manifest.bin";
constexpr wchar_t kAvDbDefaultDataFile[] = L"avdb\\default\\data.bin";
constexpr wchar_t kAvDbBackupManifestFile[] = L"avdb\\backup\\manifest.bin";
constexpr wchar_t kAvDbBackupDataFile[] = L"avdb\\backup\\data.bin";
constexpr long long kAvDbUpdateIntervalSeconds = 30LL * 60LL;

struct LaunchedProcess {
    DWORD sessionId;
    DWORD processId;
    HANDLE processHandle;
};

struct ServiceState {
    AuthSessionData auth;
    LicenseTicketData license;
    AvDatabase avDatabase;
    struct ScheduledScanStateData {
        bool enabled = false;
        unsigned long intervalMinutes = 0;
        long long nextRunUnixSeconds = 0;
        ScanOutcome lastResult = {};
        TrayAppRpcStatusCode lastStatus = TRAYAPP_RPC_STATUS_OK;
        std::wstring message;
    } scheduledScan;
    struct MonitoringStateData {
        ScanOutcome lastResult = {};
        TrayAppRpcStatusCode lastStatus = TRAYAPP_RPC_STATUS_OK;
        std::wstring message;
    } monitoring;
    TrayAppRpcStatusCode authStatus = TRAYAPP_RPC_STATUS_NOT_AUTHENTICATED;
    TrayAppRpcStatusCode licenseStatus = TRAYAPP_RPC_STATUS_NO_LICENSE;
    std::wstring authMessage = L"Войдите в учетную запись";
    std::wstring licenseMessage = L"Лицензия отсутствует";
};

struct MonitoredDirectory {
    std::wstring path;
    HANDLE changeHandle = nullptr;
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
static std::vector<MonitoredDirectory> g_monitoredDirectories;
static ServiceState g_serviceState = {};
static long long g_nextAvDatabaseUpdateUnixSeconds = 0;

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
static bool EnsureDirectoryExists(const std::wstring& path);
static bool EnsureDefaultAvDatabaseFiles(std::wstring* errorMessage);
static bool CopyDatabaseFiles(const std::wstring& sourceManifest, const std::wstring& sourceData, const std::wstring& targetManifest, const std::wstring& targetData);
static bool FileExists(const std::wstring& path);
static bool WriteBytesToFile(const std::wstring& path, const std::vector<unsigned char>& bytes);
static void MergeDatabaseRecords(AvDatabase* target, const AvDatabase& source);
static bool RestoreInvalidRecordsFromServerLocked(const std::vector<std::string>& invalidRecordIds);
static bool TryUpdateAvBasesFromServerLocked(bool createBackup, bool rollbackOnFailure, std::wstring* errorMessage);
static int InstallService();
static int UninstallService();
static long long GetNowUnixSeconds();
static void CopyTextToRpcBuffer(const std::wstring& text, wchar_t* buffer, size_t bufferCount);
static void FillAuthState(TrayAppAuthState* state);
static void FillLicenseState(TrayAppLicenseState* state);
static void FillAvDatabaseInfo(TrayAppAvDatabaseInfo* info);
static void FillScanResult(const ScanOutcome& outcome, TrayAppRpcStatusCode statusCode, TrayAppScanResult* result);
static void FillScheduledScanState(TrayAppScheduledScanState* state);
static void FillMonitoringState(TrayAppMonitoringState* state);
static void FillOperationResult(TrayAppOperationResult* result, TrayAppRpcStatusCode statusCode, const std::wstring& message);
static void ClearLicenseStateLocked(TrayAppRpcStatusCode statusCode, const std::wstring& message);
static void ClearAuthStateLocked(TrayAppRpcStatusCode statusCode, const std::wstring& message);
static TrayAppRpcStatusCode ToRpcStatusCode(int statusCode);
static bool IsLicenseUsableLocked();
static void UpdateLicenseStateLocked(const LicenseTicketData& licenseData, TrayAppRpcStatusCode statusCode, const std::wstring& message);
static void UpdateAuthStateLocked(const AuthSessionData& authData, TrayAppRpcStatusCode statusCode, const std::wstring& message);
static void LoadAvBasesLocked();
static void UnloadAvBasesLocked();
static TrayAppScanObjectType ToRpcScanObjectType(AvObjectType objectType);
static bool TryCopyDatabaseForScan(AvDatabase* databaseSnapshot);
static std::wstring FormatUnixSeconds(long long unixSeconds);
static void StopAllMonitoringLocked();
static std::wstring NormalizeDirectoryPath(const std::wstring& path);
static bool HasMonitoredDirectoryLocked(const std::wstring& normalizedPath);
static bool AddMonitoredDirectoryLocked(const std::wstring& normalizedPath, std::wstring* errorMessage);
static bool RemoveMonitoredDirectoryLocked(const std::wstring& normalizedPath, std::wstring* errorMessage);
static std::wstring BuildMonitoredDirectoriesTextLocked();

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
    LicenseTicketData licenseData = {};
    BackendResult licenseResult = {};
    if (result.statusCode == kStatusOk) {
        licenseResult = BackendCheckLicense(authData, &licenseData);
    }

    EnterCriticalSection(&g_stateLock);
    if (result.statusCode == kStatusOk) {
        UpdateAuthStateLocked(authData, TRAYAPP_RPC_STATUS_OK, L"");
        if (licenseResult.statusCode == kStatusOk) {
            UpdateLicenseStateLocked(licenseData, TRAYAPP_RPC_STATUS_OK, L"");
        } else {
            ClearLicenseStateLocked(ToRpcStatusCode(licenseResult.statusCode), licenseResult.message);
        }
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

extern "C" void TrayAppRpcGetAvDatabaseInfo(handle_t, TrayAppAvDatabaseInfo* info)
{
    FillAvDatabaseInfo(info);
}

extern "C" void TrayAppRpcScanFile(handle_t, wchar_t* path, TrayAppScanResult* result)
{
    if (!path || !result) {
        return;
    }

    AvDatabase databaseSnapshot = {};
    EnterCriticalSection(&g_stateLock);
    const bool canScan = IsLicenseUsableLocked() && g_serviceState.avDatabase.loaded;
    if (canScan) {
        databaseSnapshot = g_serviceState.avDatabase;
    }
    LeaveCriticalSection(&g_stateLock);

    if (!canScan) {
        ScanOutcome outcome = {};
        outcome.completed = false;
        outcome.targetPath = path;
        outcome.details = L"Антивирусные базы недоступны";
        FillScanResult(outcome, TRAYAPP_RPC_STATUS_NO_LICENSE, result);
        return;
    }

    FillScanResult(ScanFilePath(path, databaseSnapshot), TRAYAPP_RPC_STATUS_OK, result);
}

extern "C" void TrayAppRpcScanDirectory(handle_t, wchar_t* path, TrayAppScanResult* result)
{
    if (!path || !result) {
        return;
    }

    AvDatabase databaseSnapshot = {};
    EnterCriticalSection(&g_stateLock);
    const bool canScan = IsLicenseUsableLocked() && g_serviceState.avDatabase.loaded;
    if (canScan) {
        databaseSnapshot = g_serviceState.avDatabase;
    }
    LeaveCriticalSection(&g_stateLock);

    if (!canScan) {
        ScanOutcome outcome = {};
        outcome.completed = false;
        outcome.targetPath = path;
        outcome.details = L"Антивирусные базы недоступны";
        FillScanResult(outcome, TRAYAPP_RPC_STATUS_NO_LICENSE, result);
        return;
    }

    FillScanResult(ScanDirectoryPath(path, databaseSnapshot), TRAYAPP_RPC_STATUS_OK, result);
}

extern "C" void TrayAppRpcScanFixedDisks(handle_t, TrayAppScanResult* result)
{
    if (!result) {
        return;
    }

    AvDatabase databaseSnapshot = {};
    if (!TryCopyDatabaseForScan(&databaseSnapshot)) {
        ScanOutcome outcome = {};
        outcome.completed = false;
        outcome.targetPath = L"Fixed drives";
        outcome.details = L"Антивирусные базы недоступны";
        FillScanResult(outcome, TRAYAPP_RPC_STATUS_NO_LICENSE, result);
        return;
    }

    FillScanResult(ScanFixedDrives(databaseSnapshot), TRAYAPP_RPC_STATUS_OK, result);
}

extern "C" void TrayAppRpcConfigureScheduledScan(handle_t, unsigned long intervalMinutes, boolean enabled, TrayAppOperationResult* result)
{
    EnterCriticalSection(&g_stateLock);
    g_serviceState.scheduledScan.enabled = (enabled != FALSE);
    g_serviceState.scheduledScan.intervalMinutes = intervalMinutes;
    g_serviceState.scheduledScan.nextRunUnixSeconds =
        (enabled != FALSE && intervalMinutes > 0) ? (GetNowUnixSeconds() + static_cast<long long>(intervalMinutes) * 60LL) : 0;
    g_serviceState.scheduledScan.message =
        (enabled != FALSE && intervalMinutes > 0) ? L"Сканирование по расписанию включено" : L"Сканирование по расписанию отключено";
    LeaveCriticalSection(&g_stateLock);

    FillOperationResult(result, TRAYAPP_RPC_STATUS_OK, L"");
}

extern "C" void TrayAppRpcGetScheduledScanState(handle_t, TrayAppScheduledScanState* state)
{
    FillScheduledScanState(state);
}

extern "C" void TrayAppRpcAddMonitoredDirectory(handle_t, wchar_t* path, TrayAppOperationResult* result)
{
    if (!path || !result) {
        return;
    }

    const std::wstring normalizedPath = NormalizeDirectoryPath(path);
    std::wstring errorMessage;
    bool added = false;

    EnterCriticalSection(&g_stateLock);
    added = AddMonitoredDirectoryLocked(normalizedPath, &errorMessage);
    LeaveCriticalSection(&g_stateLock);

    FillOperationResult(result, added ? TRAYAPP_RPC_STATUS_OK : TRAYAPP_RPC_STATUS_SERVER_ERROR, errorMessage);
}

extern "C" void TrayAppRpcRemoveMonitoredDirectory(handle_t, wchar_t* path, TrayAppOperationResult* result)
{
    if (!path || !result) {
        return;
    }

    const std::wstring normalizedPath = NormalizeDirectoryPath(path);
    std::wstring errorMessage;
    bool removed = false;

    EnterCriticalSection(&g_stateLock);
    removed = RemoveMonitoredDirectoryLocked(normalizedPath, &errorMessage);
    LeaveCriticalSection(&g_stateLock);

    FillOperationResult(result, removed ? TRAYAPP_RPC_STATUS_OK : TRAYAPP_RPC_STATUS_SERVER_ERROR, errorMessage);
}

extern "C" void TrayAppRpcGetMonitoringState(handle_t, TrayAppMonitoringState* state)
{
    FillMonitoringState(state);
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
    LoadAvBasesLocked();
    g_nextAvDatabaseUpdateUnixSeconds = GetNowUnixSeconds() + kAvDbUpdateIntervalSeconds;
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
        EnterCriticalSection(&g_stateLock);
        StopAllMonitoringLocked();
        LeaveCriticalSection(&g_stateLock);
    }

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
        bool shouldRunScheduledScan = false;
        bool shouldUpdateAvDatabase = false;
        unsigned long scheduledIntervalMinutes = 0;
        std::vector<std::wstring> directoriesToScan;
        AvDatabase databaseSnapshot = {};

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
        shouldUpdateAvDatabase =
            g_nextAvDatabaseUpdateUnixSeconds != 0 &&
            now >= g_nextAvDatabaseUpdateUnixSeconds;

        if (IsLicenseUsableLocked() && g_serviceState.avDatabase.loaded) {
            if (g_serviceState.scheduledScan.enabled &&
                g_serviceState.scheduledScan.intervalMinutes > 0 &&
                g_serviceState.scheduledScan.nextRunUnixSeconds != 0 &&
                now >= g_serviceState.scheduledScan.nextRunUnixSeconds) {
                shouldRunScheduledScan = true;
                scheduledIntervalMinutes = g_serviceState.scheduledScan.intervalMinutes;
                databaseSnapshot = g_serviceState.avDatabase;
            }

            for (const MonitoredDirectory& directory : g_monitoredDirectories) {
                if (directory.changeHandle && WaitForSingleObject(directory.changeHandle, 0) == WAIT_OBJECT_0) {
                    directoriesToScan.push_back(directory.path);
                    FindNextChangeNotification(directory.changeHandle);
                }
            }

            if (!directoriesToScan.empty() && !databaseSnapshot.loaded) {
                databaseSnapshot = g_serviceState.avDatabase;
            }
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

        if (shouldUpdateAvDatabase) {
            EnterCriticalSection(&g_stateLock);
            std::wstring updateError;
            const bool updated = TryUpdateAvBasesFromServerLocked(true, true, &updateError);
            g_nextAvDatabaseUpdateUnixSeconds = GetNowUnixSeconds() + kAvDbUpdateIntervalSeconds;
            LeaveCriticalSection(&g_stateLock);
            if (!updated && !updateError.empty()) {
                AppendDebugLog(L"AV DB scheduled update failed: " + updateError);
            }
        }

        if (shouldRunScheduledScan) {
            const ScanOutcome scheduledResult = ScanFixedDrives(databaseSnapshot);
            EnterCriticalSection(&g_stateLock);
            g_serviceState.scheduledScan.lastResult = scheduledResult;
            g_serviceState.scheduledScan.lastStatus = TRAYAPP_RPC_STATUS_OK;
            g_serviceState.scheduledScan.message = scheduledResult.details;
            g_serviceState.scheduledScan.nextRunUnixSeconds =
                GetNowUnixSeconds() + static_cast<long long>(scheduledIntervalMinutes) * 60LL;
            LeaveCriticalSection(&g_stateLock);
        }

        for (const std::wstring& directoryPath : directoriesToScan) {
            const ScanOutcome monitorResult = ScanDirectoryPath(directoryPath, databaseSnapshot);
            EnterCriticalSection(&g_stateLock);
            g_serviceState.monitoring.lastResult = monitorResult;
            g_serviceState.monitoring.lastStatus = TRAYAPP_RPC_STATUS_OK;
            g_serviceState.monitoring.message = monitorResult.details;
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

    if (separator == 2 && path.size() >= 3 && path[1] == L':') {
        return path.substr(0, 3);
    }

    return path.substr(0, separator);
}

static bool FileExists(const std::wstring& path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static bool EnsureDirectoryExists(const std::wstring& path)
{
    if (path.empty()) {
        return false;
    }

    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        return (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    const std::wstring parent = GetDirectoryName(path);
    if (!parent.empty() && parent != path && !EnsureDirectoryExists(parent)) {
        return false;
    }

    if (CreateDirectoryW(path.c_str(), nullptr)) {
        return true;
    }

    return GetLastError() == ERROR_ALREADY_EXISTS;
}

static bool CopyDatabaseFiles(const std::wstring& sourceManifest, const std::wstring& sourceData, const std::wstring& targetManifest, const std::wstring& targetData)
{
    const std::wstring targetManifestDirectory = GetDirectoryName(targetManifest);
    const std::wstring targetDataDirectory = GetDirectoryName(targetData);
    if ((!targetManifestDirectory.empty() && !EnsureDirectoryExists(targetManifestDirectory)) ||
        (!targetDataDirectory.empty() && !EnsureDirectoryExists(targetDataDirectory))) {
        return false;
    }

    return CopyFileW(sourceManifest.c_str(), targetManifest.c_str(), FALSE) == TRUE &&
        CopyFileW(sourceData.c_str(), targetData.c_str(), FALSE) == TRUE;
}

static bool EnsureDefaultAvDatabaseFiles(std::wstring* errorMessage)
{
    const std::wstring defaultDirectory = GetSiblingPath(kAvDbDefaultDirectory);
    if (!EnsureDirectoryExists(defaultDirectory)) {
        if (errorMessage) {
            *errorMessage = L"Не удалось создать директорию баз по умолчанию";
        }
        return false;
    }

    const std::wstring defaultManifestPath = GetSiblingPath(kAvDbDefaultManifestFile);
    const std::wstring defaultDataPath = GetSiblingPath(kAvDbDefaultDataFile);
    if (FileExists(defaultManifestPath) && FileExists(defaultDataPath)) {
        return true;
    }

    return WriteDefaultAntivirusDatabaseFiles(defaultManifestPath, defaultDataPath, errorMessage);
}

static bool WriteBytesToFile(const std::wstring& path, const std::vector<unsigned char>& bytes)
{
    const std::wstring directory = GetDirectoryName(path);
    if (!directory.empty() && !EnsureDirectoryExists(directory)) {
        return false;
    }

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream.is_open()) {
        return false;
    }

    if (!bytes.empty()) {
        stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    return stream.good();
}

static void MergeDatabaseRecords(AvDatabase* target, const AvDatabase& source)
{
    if (!target) {
        return;
    }

    for (const auto& sourcePair : source.recordsByPrefix) {
        std::vector<AvRecord>& targetRecords = target->recordsByPrefix[sourcePair.first];
        for (const AvRecord& sourceRecord : sourcePair.second) {
            const auto existing = std::find_if(
                targetRecords.begin(),
                targetRecords.end(),
                [&](const AvRecord& targetRecord) {
                    return targetRecord.recordId == sourceRecord.recordId;
                });
            if (existing == targetRecords.end()) {
                targetRecords.push_back(sourceRecord);
                ++target->totalRecordCount;
            } else {
                *existing = sourceRecord;
            }
        }
    }

    if (source.releaseUnixSeconds > target->releaseUnixSeconds) {
        target->releaseUnixSeconds = source.releaseUnixSeconds;
        target->releaseDateText = source.releaseDateText;
    }
}

static bool RestoreInvalidRecordsFromServerLocked(const std::vector<std::string>& invalidRecordIds)
{
    if (invalidRecordIds.empty()) {
        return true;
    }

    BinarySignaturePackage package = {};
    const BackendResult downloadResult = BackendDownloadAvRecordsByIds(invalidRecordIds, &package);
    if (downloadResult.statusCode != kStatusOk) {
        AppendDebugLog(L"AV DB record recovery failed: " + downloadResult.message);
        return false;
    }

    const std::wstring recoveredManifestPath = GetSiblingPath(kAvDbRecoveredManifestFile);
    const std::wstring recoveredDataPath = GetSiblingPath(kAvDbRecoveredDataFile);
    if (!WriteBytesToFile(recoveredManifestPath, package.manifestBytes) ||
        !WriteBytesToFile(recoveredDataPath, package.dataBytes)) {
        AppendDebugLog(L"AV DB record recovery failed: cannot write temporary recovery files");
        DeleteFileW(recoveredManifestPath.c_str());
        DeleteFileW(recoveredDataPath.c_str());
        return false;
    }

    AvDatabase recoveredDatabase = {};
    const AvDatabaseLoadResult recoveredResult =
        LoadAntivirusDatabaseFromFiles(recoveredManifestPath, recoveredDataPath, &recoveredDatabase);
    DeleteFileW(recoveredManifestPath.c_str());
    DeleteFileW(recoveredDataPath.c_str());
    if (recoveredResult.status != AvDatabaseLoadStatus::Ok) {
        AppendDebugLog(L"AV DB record recovery failed: " + recoveredResult.message);
        return false;
    }

    MergeDatabaseRecords(&g_serviceState.avDatabase, recoveredDatabase);
    AppendDebugLog(L"AV DB recovered invalid records from server: " + std::to_wstring(recoveredResult.loadedRecordCount));
    return true;
}

static bool TryUpdateAvBasesFromServerLocked(bool createBackup, bool rollbackOnFailure, std::wstring* errorMessage)
{
    const std::wstring currentManifestPath = GetSiblingPath(kAvDbManifestFile);
    const std::wstring currentDataPath = GetSiblingPath(kAvDbDataFile);
    const std::wstring backupManifestPath = GetSiblingPath(kAvDbBackupManifestFile);
    const std::wstring backupDataPath = GetSiblingPath(kAvDbBackupDataFile);
    const std::wstring downloadManifestPath = GetSiblingPath(kAvDbDownloadManifestFile);
    const std::wstring downloadDataPath = GetSiblingPath(kAvDbDownloadDataFile);

    if (createBackup && FileExists(currentManifestPath) && FileExists(currentDataPath)) {
        if (!CopyDatabaseFiles(currentManifestPath, currentDataPath, backupManifestPath, backupDataPath)) {
            if (errorMessage) {
                *errorMessage = L"Не удалось создать резервную копию антивирусных баз";
            }
            return false;
        }
    }

    BinarySignaturePackage package = {};
    const BackendResult downloadResult = BackendDownloadFullAvDatabase(&package);
    if (downloadResult.statusCode != kStatusOk) {
        if (errorMessage) {
            *errorMessage = downloadResult.message;
        }
        return false;
    }

    if (!WriteBytesToFile(downloadManifestPath, package.manifestBytes) ||
        !WriteBytesToFile(downloadDataPath, package.dataBytes)) {
        if (errorMessage) {
            *errorMessage = L"Не удалось сохранить обновленные антивирусные базы";
        }
        DeleteFileW(downloadManifestPath.c_str());
        DeleteFileW(downloadDataPath.c_str());
        return false;
    }

    AvDatabase updatedDatabase = {};
    const AvDatabaseLoadResult loadResult =
        LoadAntivirusDatabaseFromFiles(downloadManifestPath, downloadDataPath, &updatedDatabase);
    if (loadResult.status != AvDatabaseLoadStatus::Ok) {
        DeleteFileW(downloadManifestPath.c_str());
        DeleteFileW(downloadDataPath.c_str());
        if (rollbackOnFailure && FileExists(backupManifestPath) && FileExists(backupDataPath)) {
            CopyDatabaseFiles(backupManifestPath, backupDataPath, currentManifestPath, currentDataPath);
        }
        if (errorMessage) {
            *errorMessage = loadResult.message;
        }
        return false;
    }

    if (!CopyDatabaseFiles(downloadManifestPath, downloadDataPath, currentManifestPath, currentDataPath)) {
        DeleteFileW(downloadManifestPath.c_str());
        DeleteFileW(downloadDataPath.c_str());
        if (rollbackOnFailure && FileExists(backupManifestPath) && FileExists(backupDataPath)) {
            CopyDatabaseFiles(backupManifestPath, backupDataPath, currentManifestPath, currentDataPath);
        }
        if (errorMessage) {
            *errorMessage = L"Не удалось применить обновленные антивирусные базы";
        }
        return false;
    }

    DeleteFileW(downloadManifestPath.c_str());
    DeleteFileW(downloadDataPath.c_str());
    g_serviceState.avDatabase = updatedDatabase;
    if (!loadResult.invalidRecordIds.empty()) {
        RestoreInvalidRecordsFromServerLocked(loadResult.invalidRecordIds);
    }

    AppendDebugLog(
        L"AV DB updated from server, records=" + std::to_wstring(loadResult.loadedRecordCount) +
        L", skipped=" + std::to_wstring(loadResult.skippedRecordCount));
    return true;
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

static void FillAvDatabaseInfo(TrayAppAvDatabaseInfo* info)
{
    if (!info) {
        return;
    }

    ZeroMemory(info, sizeof(*info));
    EnterCriticalSection(&g_stateLock);
    info->statusCode = IsLicenseUsableLocked() ? TRAYAPP_RPC_STATUS_OK : TRAYAPP_RPC_STATUS_NO_LICENSE;
    info->loaded = g_serviceState.avDatabase.loaded ? TRUE : FALSE;
    info->releaseUnixSeconds = g_serviceState.avDatabase.releaseUnixSeconds;
    info->recordCount = static_cast<unsigned long>(g_serviceState.avDatabase.totalRecordCount);
    CopyTextToRpcBuffer(g_serviceState.avDatabase.releaseDateText, info->releaseDateText, ARRAYSIZE(info->releaseDateText));
    CopyTextToRpcBuffer(
        g_serviceState.avDatabase.loaded ? L"" : L"Антивирусные базы не загружены",
        info->message,
        ARRAYSIZE(info->message));
    LeaveCriticalSection(&g_stateLock);
}

static void FillScanResult(const ScanOutcome& outcome, TrayAppRpcStatusCode statusCode, TrayAppScanResult* result)
{
    if (!result) {
        return;
    }

    ZeroMemory(result, sizeof(*result));
    result->statusCode = statusCode;
    result->completed = outcome.completed ? TRUE : FALSE;
    result->malicious = outcome.malicious ? TRUE : FALSE;
    result->scannedFileCount = outcome.scannedFileCount;
    result->detectedFileCount = outcome.detectedFileCount;
    result->firstMatchOffset = outcome.firstMatchOffset;
    result->objectType = ToRpcScanObjectType(outcome.objectType);
    CopyTextToRpcBuffer(outcome.targetPath, result->targetPath, ARRAYSIZE(result->targetPath));
    CopyTextToRpcBuffer(outcome.detectedPath, result->detectedPath, ARRAYSIZE(result->detectedPath));
    CopyTextToRpcBuffer(outcome.details, result->details, ARRAYSIZE(result->details));
}

static void FillScheduledScanState(TrayAppScheduledScanState* state)
{
    if (!state) {
        return;
    }

    ZeroMemory(state, sizeof(*state));
    EnterCriticalSection(&g_stateLock);
    state->statusCode = g_serviceState.scheduledScan.lastStatus;
    state->enabled = g_serviceState.scheduledScan.enabled ? TRUE : FALSE;
    state->intervalMinutes = g_serviceState.scheduledScan.intervalMinutes;
    state->nextRunUnixSeconds = g_serviceState.scheduledScan.nextRunUnixSeconds;
    CopyTextToRpcBuffer(FormatUnixSeconds(g_serviceState.scheduledScan.nextRunUnixSeconds), state->nextRunText, ARRAYSIZE(state->nextRunText));
    FillScanResult(g_serviceState.scheduledScan.lastResult, g_serviceState.scheduledScan.lastStatus, &state->lastResult);
    CopyTextToRpcBuffer(g_serviceState.scheduledScan.message, state->message, ARRAYSIZE(state->message));
    LeaveCriticalSection(&g_stateLock);
}

static void FillMonitoringState(TrayAppMonitoringState* state)
{
    if (!state) {
        return;
    }

    ZeroMemory(state, sizeof(*state));
    EnterCriticalSection(&g_stateLock);
    state->statusCode = g_serviceState.monitoring.lastStatus;
    state->directoryCount = static_cast<unsigned long>(g_monitoredDirectories.size());
    CopyTextToRpcBuffer(BuildMonitoredDirectoriesTextLocked(), state->directories, ARRAYSIZE(state->directories));
    FillScanResult(g_serviceState.monitoring.lastResult, g_serviceState.monitoring.lastStatus, &state->lastResult);
    CopyTextToRpcBuffer(g_serviceState.monitoring.message, state->message, ARRAYSIZE(state->message));
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

static void LoadAvBasesLocked()
{
    const std::wstring currentDirectory = GetSiblingPath(kAvDbDirectory);
    const std::wstring backupDirectory = GetSiblingPath(kAvDbBackupDirectory);
    const std::wstring defaultDirectory = GetSiblingPath(kAvDbDefaultDirectory);
    const std::wstring currentManifestPath = GetSiblingPath(kAvDbManifestFile);
    const std::wstring currentDataPath = GetSiblingPath(kAvDbDataFile);
    const std::wstring backupManifestPath = GetSiblingPath(kAvDbBackupManifestFile);
    const std::wstring backupDataPath = GetSiblingPath(kAvDbBackupDataFile);
    const std::wstring defaultManifestPath = GetSiblingPath(kAvDbDefaultManifestFile);
    const std::wstring defaultDataPath = GetSiblingPath(kAvDbDefaultDataFile);
    bool forceNetworkUpdate = false;

    ClearAntivirusDatabase(&g_serviceState.avDatabase);

    if (!EnsureDirectoryExists(currentDirectory) ||
        !EnsureDirectoryExists(backupDirectory) ||
        !EnsureDirectoryExists(defaultDirectory)) {
        g_serviceState.avDatabase.loaded = false;
        AppendDebugLog(L"AV DB load failed: cannot create storage directories");
        return;
    }

    std::wstring defaultError;
    if (!EnsureDefaultAvDatabaseFiles(&defaultError)) {
        g_serviceState.avDatabase.loaded = false;
        AppendDebugLog(L"AV DB load failed: cannot prepare default database: " + defaultError);
        return;
    }

    if (!FileExists(currentManifestPath) || !FileExists(currentDataPath)) {
        if (!CopyDatabaseFiles(defaultManifestPath, defaultDataPath, currentManifestPath, currentDataPath)) {
            AppendDebugLog(L"AV DB load warning: failed to initialize current database from defaults");
        }
    }

    AvDatabase loadedDatabase = {};
    AvDatabaseLoadResult loadResult = LoadAntivirusDatabaseFromFiles(currentManifestPath, currentDataPath, &loadedDatabase);
    if (loadResult.status == AvDatabaseLoadStatus::Ok) {
        g_serviceState.avDatabase = loadedDatabase;
        CopyDatabaseFiles(currentManifestPath, currentDataPath, backupManifestPath, backupDataPath);
        if (!loadResult.invalidRecordIds.empty()) {
            RestoreInvalidRecordsFromServerLocked(loadResult.invalidRecordIds);
        }
        AppendDebugLog(
            L"AV DB loaded from current files, records=" + std::to_wstring(loadResult.loadedRecordCount) +
            L", skipped=" + std::to_wstring(loadResult.skippedRecordCount));
        return;
    }

    AppendDebugLog(L"AV DB current load failed: " + loadResult.message);
    forceNetworkUpdate =
        loadResult.status == AvDatabaseLoadStatus::InvalidManifestSignature ||
        loadResult.status == AvDatabaseLoadStatus::InvalidManifestFormat ||
        loadResult.status == AvDatabaseLoadStatus::InvalidDataHash ||
        loadResult.status == AvDatabaseLoadStatus::InvalidDataFormat;

    if (FileExists(backupManifestPath) && FileExists(backupDataPath)) {
        if (CopyDatabaseFiles(backupManifestPath, backupDataPath, currentManifestPath, currentDataPath)) {
            loadedDatabase = {};
            loadResult = LoadAntivirusDatabaseFromFiles(currentManifestPath, currentDataPath, &loadedDatabase);
            if (loadResult.status == AvDatabaseLoadStatus::Ok) {
                g_serviceState.avDatabase = loadedDatabase;
                if (!loadResult.invalidRecordIds.empty()) {
                    RestoreInvalidRecordsFromServerLocked(loadResult.invalidRecordIds);
                }
                if (forceNetworkUpdate) {
                    std::wstring updateError;
                    TryUpdateAvBasesFromServerLocked(true, true, &updateError);
                }
                AppendDebugLog(
                    L"AV DB restored from backup, records=" + std::to_wstring(loadResult.loadedRecordCount) +
                    L", skipped=" + std::to_wstring(loadResult.skippedRecordCount));
                return;
            }
            AppendDebugLog(L"AV DB backup load failed: " + loadResult.message);
        } else {
            AppendDebugLog(L"AV DB backup copy failed");
        }
    }

    if (CopyDatabaseFiles(defaultManifestPath, defaultDataPath, currentManifestPath, currentDataPath)) {
        loadedDatabase = {};
        loadResult = LoadAntivirusDatabaseFromFiles(currentManifestPath, currentDataPath, &loadedDatabase);
        if (loadResult.status == AvDatabaseLoadStatus::Ok) {
            g_serviceState.avDatabase = loadedDatabase;
            CopyDatabaseFiles(currentManifestPath, currentDataPath, backupManifestPath, backupDataPath);
            if (!loadResult.invalidRecordIds.empty()) {
                RestoreInvalidRecordsFromServerLocked(loadResult.invalidRecordIds);
            }
            if (forceNetworkUpdate) {
                std::wstring updateError;
                TryUpdateAvBasesFromServerLocked(true, true, &updateError);
            }
            AppendDebugLog(
                L"AV DB loaded from default bundle, records=" + std::to_wstring(loadResult.loadedRecordCount) +
                L", skipped=" + std::to_wstring(loadResult.skippedRecordCount));
            return;
        }
        AppendDebugLog(L"AV DB default load failed: " + loadResult.message);
    } else {
        AppendDebugLog(L"AV DB default copy failed");
    }
}

static void UnloadAvBasesLocked()
{
    ClearAntivirusDatabase(&g_serviceState.avDatabase);
}

static bool TryCopyDatabaseForScan(AvDatabase* databaseSnapshot)
{
    if (!databaseSnapshot) {
        return false;
    }

    EnterCriticalSection(&g_stateLock);
    const bool canScan = IsLicenseUsableLocked() && g_serviceState.avDatabase.loaded;
    if (canScan) {
        *databaseSnapshot = g_serviceState.avDatabase;
    }
    LeaveCriticalSection(&g_stateLock);
    return canScan;
}

static std::wstring FormatUnixSeconds(long long unixSeconds)
{
    if (unixSeconds <= 0) {
        return L"";
    }

    const time_t rawTime = static_cast<time_t>(unixSeconds);
    tm localTime = {};
    localtime_s(&localTime, &rawTime);

    wchar_t buffer[64] = {};
    wcsftime(buffer, ARRAYSIZE(buffer), L"%Y-%m-%d %H:%M:%S", &localTime);
    return buffer;
}

static void StopAllMonitoringLocked()
{
    for (MonitoredDirectory& directory : g_monitoredDirectories) {
        if (directory.changeHandle) {
            FindCloseChangeNotification(directory.changeHandle);
            directory.changeHandle = nullptr;
        }
    }
    g_monitoredDirectories.clear();
}

static std::wstring NormalizeDirectoryPath(const std::wstring& path)
{
    std::wstring normalized = path;
    while (!normalized.empty() && (normalized.back() == L'\\' || normalized.back() == L'/')) {
        normalized.pop_back();
    }
    return normalized;
}

static bool HasMonitoredDirectoryLocked(const std::wstring& normalizedPath)
{
    return std::any_of(
        g_monitoredDirectories.begin(),
        g_monitoredDirectories.end(),
        [&](const MonitoredDirectory& directory) {
            return _wcsicmp(directory.path.c_str(), normalizedPath.c_str()) == 0;
        });
}

static bool AddMonitoredDirectoryLocked(const std::wstring& normalizedPath, std::wstring* errorMessage)
{
    if (normalizedPath.empty()) {
        if (errorMessage) {
            *errorMessage = L"Пустой путь директории";
        }
        return false;
    }

    const DWORD attributes = GetFileAttributesW(normalizedPath.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        if (errorMessage) {
            *errorMessage = L"Директория не найдена";
        }
        return false;
    }

    if (HasMonitoredDirectoryLocked(normalizedPath)) {
        if (errorMessage) {
            *errorMessage = L"";
        }
        return true;
    }

    HANDLE changeHandle = FindFirstChangeNotificationW(
        normalizedPath.c_str(),
        TRUE,
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
    if (changeHandle == INVALID_HANDLE_VALUE || !changeHandle) {
        if (errorMessage) {
            *errorMessage = L"Не удалось запустить мониторинг директории";
        }
        return false;
    }

    g_monitoredDirectories.push_back({ normalizedPath, changeHandle });
    g_serviceState.monitoring.message = L"Мониторинг настроен";
    return true;
}

static bool RemoveMonitoredDirectoryLocked(const std::wstring& normalizedPath, std::wstring* errorMessage)
{
    for (auto it = g_monitoredDirectories.begin(); it != g_monitoredDirectories.end(); ++it) {
        if (_wcsicmp(it->path.c_str(), normalizedPath.c_str()) == 0) {
            if (it->changeHandle) {
                FindCloseChangeNotification(it->changeHandle);
            }
            g_monitoredDirectories.erase(it);
            g_serviceState.monitoring.message = L"Мониторинг обновлен";
            return true;
        }
    }

    if (errorMessage) {
        *errorMessage = L"Директория не найдена в списке мониторинга";
    }
    return false;
}

static std::wstring BuildMonitoredDirectoriesTextLocked()
{
    std::wstring text;
    for (size_t index = 0; index < g_monitoredDirectories.size(); ++index) {
        if (index != 0) {
            text += L"; ";
        }
        text += g_monitoredDirectories[index].path;
    }
    return text;
}

static TrayAppScanObjectType ToRpcScanObjectType(AvObjectType objectType)
{
    switch (objectType) {
    case AvObjectType::Pe:
        return TRAYAPP_SCAN_OBJECT_PE;
    case AvObjectType::PowerShell:
        return TRAYAPP_SCAN_OBJECT_POWERSHELL;
    default:
        return TRAYAPP_SCAN_OBJECT_UNKNOWN;
    }
}


