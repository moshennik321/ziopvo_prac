#pragma once

#include <rpc.h>

extern "C" {
void* __RPC_USER midl_user_allocate(size_t size);
void __RPC_USER midl_user_free(void* pointer);
}

RPC_STATUS CreateTrayAppRpcBinding(RPC_BINDING_HANDLE* bindingHandle);
void FreeTrayAppRpcBinding(RPC_BINDING_HANDLE* bindingHandle);
