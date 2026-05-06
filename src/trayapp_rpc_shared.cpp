#include "trayapp_rpc_shared.h"

#include <string>

namespace {
constexpr wchar_t kRpcProtocolSequence[] = L"ncalrpc";
constexpr wchar_t kRpcEndpoint[] = L"TrayAppServiceRpcEndpoint";
}

void* __RPC_USER midl_user_allocate(size_t size)
{
    return malloc(size);
}

void __RPC_USER midl_user_free(void* pointer)
{
    free(pointer);
}

RPC_STATUS CreateTrayAppRpcBinding(RPC_BINDING_HANDLE* bindingHandle)
{
    if (!bindingHandle) {
        return RPC_S_INVALID_ARG;
    }

    *bindingHandle = nullptr;

    RPC_WSTR stringBinding = nullptr;
    RPC_STATUS status = RpcStringBindingComposeW(
        nullptr,
        reinterpret_cast<RPC_WSTR>(const_cast<wchar_t*>(kRpcProtocolSequence)),
        nullptr,
        reinterpret_cast<RPC_WSTR>(const_cast<wchar_t*>(kRpcEndpoint)),
        nullptr,
        &stringBinding);
    if (status != RPC_S_OK) {
        return status;
    }

    status = RpcBindingFromStringBindingW(stringBinding, bindingHandle);
    RpcStringFreeW(&stringBinding);

    return status;
}

void FreeTrayAppRpcBinding(RPC_BINDING_HANDLE* bindingHandle)
{
    if (bindingHandle && *bindingHandle) {
        RpcBindingFree(bindingHandle);
    }
}
