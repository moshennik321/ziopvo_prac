#include "rpc_client.h"

#include <rpc.h>

#include "trayapp_rpc_shared.h"

namespace {
template <typename Callback>
bool ExecuteRpcCall(Callback&& callback)
{
    RPC_BINDING_HANDLE bindingHandle = nullptr;
    if (CreateTrayAppRpcBinding(&bindingHandle) != RPC_S_OK) {
        return false;
    }

    bool success = true;
    RpcTryExcept {
        callback(bindingHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER) {
        success = false;
    }
    RpcEndExcept

    FreeTrayAppRpcBinding(&bindingHandle);
    return success;
}
}

bool RequestServiceStopViaRpc()
{
    return ExecuteRpcCall([](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcStopService(bindingHandle);
    });
}

bool GetAuthStateViaRpc(TrayAppAuthState* state)
{
    if (!state) {
        return false;
    }

    ZeroMemory(state, sizeof(*state));
    return ExecuteRpcCall([&](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcGetAuthState(bindingHandle, state);
    });
}

bool LoginViaRpc(const std::wstring& email, const std::wstring& password, TrayAppAuthState* state)
{
    if (!state) {
        return false;
    }

    ZeroMemory(state, sizeof(*state));
    return ExecuteRpcCall([&](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcLogin(bindingHandle, const_cast<wchar_t*>(email.c_str()), const_cast<wchar_t*>(password.c_str()), state);
    });
}

bool LogoutViaRpc(TrayAppOperationResult* result)
{
    if (!result) {
        return false;
    }

    ZeroMemory(result, sizeof(*result));
    return ExecuteRpcCall([&](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcLogout(bindingHandle, result);
    });
}

bool GetLicenseStateViaRpc(TrayAppLicenseState* state)
{
    if (!state) {
        return false;
    }

    ZeroMemory(state, sizeof(*state));
    return ExecuteRpcCall([&](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcGetLicenseState(bindingHandle, state);
    });
}

bool ActivateLicenseViaRpc(const std::wstring& activationCode, TrayAppLicenseState* state)
{
    if (!state) {
        return false;
    }

    ZeroMemory(state, sizeof(*state));
    return ExecuteRpcCall([&](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcActivateLicense(bindingHandle, const_cast<wchar_t*>(activationCode.c_str()), state);
    });
}

bool GetAvDatabaseInfoViaRpc(TrayAppAvDatabaseInfo* info)
{
    if (!info) {
        return false;
    }

    ZeroMemory(info, sizeof(*info));
    return ExecuteRpcCall([&](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcGetAvDatabaseInfo(bindingHandle, info);
    });
}

bool ScanFileViaRpc(const std::wstring& path, TrayAppScanResult* result)
{
    if (!result) {
        return false;
    }

    ZeroMemory(result, sizeof(*result));
    return ExecuteRpcCall([&](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcScanFile(bindingHandle, const_cast<wchar_t*>(path.c_str()), result);
    });
}

bool ScanDirectoryViaRpc(const std::wstring& path, TrayAppScanResult* result)
{
    if (!result) {
        return false;
    }

    ZeroMemory(result, sizeof(*result));
    return ExecuteRpcCall([&](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcScanDirectory(bindingHandle, const_cast<wchar_t*>(path.c_str()), result);
    });
}

bool ScanFixedDisksViaRpc(TrayAppScanResult* result)
{
    if (!result) {
        return false;
    }

    ZeroMemory(result, sizeof(*result));
    return ExecuteRpcCall([&](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcScanFixedDisks(bindingHandle, result);
    });
}

bool ConfigureScheduledScanViaRpc(unsigned long intervalMinutes, bool enabled, TrayAppOperationResult* result)
{
    if (!result) {
        return false;
    }

    ZeroMemory(result, sizeof(*result));
    return ExecuteRpcCall([&](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcConfigureScheduledScan(bindingHandle, intervalMinutes, enabled ? TRUE : FALSE, result);
    });
}

bool GetScheduledScanStateViaRpc(TrayAppScheduledScanState* state)
{
    if (!state) {
        return false;
    }

    ZeroMemory(state, sizeof(*state));
    return ExecuteRpcCall([&](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcGetScheduledScanState(bindingHandle, state);
    });
}

bool AddMonitoredDirectoryViaRpc(const std::wstring& path, TrayAppOperationResult* result)
{
    if (!result) {
        return false;
    }

    ZeroMemory(result, sizeof(*result));
    return ExecuteRpcCall([&](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcAddMonitoredDirectory(bindingHandle, const_cast<wchar_t*>(path.c_str()), result);
    });
}

bool RemoveMonitoredDirectoryViaRpc(const std::wstring& path, TrayAppOperationResult* result)
{
    if (!result) {
        return false;
    }

    ZeroMemory(result, sizeof(*result));
    return ExecuteRpcCall([&](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcRemoveMonitoredDirectory(bindingHandle, const_cast<wchar_t*>(path.c_str()), result);
    });
}

bool GetMonitoringStateViaRpc(TrayAppMonitoringState* state)
{
    if (!state) {
        return false;
    }

    ZeroMemory(state, sizeof(*state));
    return ExecuteRpcCall([&](RPC_BINDING_HANDLE bindingHandle) {
        TrayAppRpcGetMonitoringState(bindingHandle, state);
    });
}
