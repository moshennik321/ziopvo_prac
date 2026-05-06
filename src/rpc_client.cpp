#include "rpc_client.h"

#include <rpc.h>

#include "trayapp_rpc.h"
#include "trayapp_rpc_shared.h"

bool RequestServiceStopViaRpc()
{
    RPC_BINDING_HANDLE bindingHandle = nullptr;
    if (CreateTrayAppRpcBinding(&bindingHandle) != RPC_S_OK) {
        return false;
    }

    bool success = true;

    RpcTryExcept {
        TrayAppRpcStopService(bindingHandle);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER) {
        success = false;
    }
    RpcEndExcept

    FreeTrayAppRpcBinding(&bindingHandle);
    return success;
}
