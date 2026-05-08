

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Tue Jan 19 06:14:07 2038
 */
/* Compiler settings for src\trayapp_rpc.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0628 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */


#ifndef __trayapp_rpc_h__
#define __trayapp_rpc_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if defined(_CONTROL_FLOW_GUARD_XFG)
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __TrayAppRpc_INTERFACE_DEFINED__
#define __TrayAppRpc_INTERFACE_DEFINED__

/* interface TrayAppRpc */
/* [unique][version][uuid] */ 

typedef 
enum TrayAppRpcStatusCode
    {
        TRAYAPP_RPC_STATUS_OK	= 0,
        TRAYAPP_RPC_STATUS_NOT_AUTHENTICATED	= 1,
        TRAYAPP_RPC_STATUS_INVALID_CREDENTIALS	= 2,
        TRAYAPP_RPC_STATUS_NETWORK_ERROR	= 3,
        TRAYAPP_RPC_STATUS_NO_LICENSE	= 4,
        TRAYAPP_RPC_STATUS_LICENSE_BLOCKED	= 5,
        TRAYAPP_RPC_STATUS_LICENSE_EXPIRED	= 6,
        TRAYAPP_RPC_STATUS_ACTIVATION_FAILED	= 7,
        TRAYAPP_RPC_STATUS_SERVER_ERROR	= 8
    } 	TrayAppRpcStatusCode;

typedef struct TrayAppOperationResult
    {
    TrayAppRpcStatusCode statusCode;
    wchar_t message[ 256 ];
    } 	TrayAppOperationResult;

typedef struct TrayAppAuthState
    {
    TrayAppRpcStatusCode statusCode;
    boolean authenticated;
    boolean antivirusEnabled;
    wchar_t email[ 256 ];
    wchar_t message[ 256 ];
    } 	TrayAppAuthState;

typedef struct TrayAppLicenseState
    {
    TrayAppRpcStatusCode statusCode;
    boolean hasLicense;
    boolean blocked;
    boolean expired;
    boolean antivirusEnabled;
    hyper expiresAtUnixSeconds;
    wchar_t expiresAtText[ 64 ];
    wchar_t message[ 256 ];
    } 	TrayAppLicenseState;

void TrayAppRpcStopService( 
    /* [in] */ handle_t hBinding);

void TrayAppRpcGetAuthState( 
    /* [in] */ handle_t hBinding,
    /* [out] */ TrayAppAuthState *state);

void TrayAppRpcLogin( 
    /* [in] */ handle_t hBinding,
    /* [string][in] */ wchar_t *email,
    /* [string][in] */ wchar_t *password,
    /* [out] */ TrayAppAuthState *state);

void TrayAppRpcLogout( 
    /* [in] */ handle_t hBinding,
    /* [out] */ TrayAppOperationResult *result);

void TrayAppRpcGetLicenseState( 
    /* [in] */ handle_t hBinding,
    /* [out] */ TrayAppLicenseState *state);

void TrayAppRpcActivateLicense( 
    /* [in] */ handle_t hBinding,
    /* [string][in] */ wchar_t *activationCode,
    /* [out] */ TrayAppLicenseState *state);



extern RPC_IF_HANDLE TrayAppRpc_v1_0_c_ifspec;
extern RPC_IF_HANDLE TrayAppRpc_v1_0_s_ifspec;
#endif /* __TrayAppRpc_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


