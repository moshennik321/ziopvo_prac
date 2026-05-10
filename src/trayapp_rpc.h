

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

typedef 
enum TrayAppScanObjectType
    {
        TRAYAPP_SCAN_OBJECT_UNKNOWN	= 0,
        TRAYAPP_SCAN_OBJECT_PE	= 1,
        TRAYAPP_SCAN_OBJECT_POWERSHELL	= 2
    } 	TrayAppScanObjectType;

typedef struct TrayAppAvDatabaseInfo
    {
    TrayAppRpcStatusCode statusCode;
    boolean loaded;
    hyper releaseUnixSeconds;
    unsigned long recordCount;
    wchar_t releaseDateText[ 64 ];
    wchar_t message[ 256 ];
    } 	TrayAppAvDatabaseInfo;

typedef struct TrayAppScanResult
    {
    TrayAppRpcStatusCode statusCode;
    boolean completed;
    boolean malicious;
    unsigned long scannedFileCount;
    unsigned long detectedFileCount;
    hyper firstMatchOffset;
    TrayAppScanObjectType objectType;
    wchar_t targetPath[ 512 ];
    wchar_t detectedPath[ 512 ];
    wchar_t details[ 512 ];
    } 	TrayAppScanResult;

typedef struct TrayAppScheduledScanState
    {
    TrayAppRpcStatusCode statusCode;
    boolean enabled;
    unsigned long intervalMinutes;
    hyper nextRunUnixSeconds;
    wchar_t nextRunText[ 64 ];
    TrayAppScanResult lastResult;
    wchar_t message[ 256 ];
    } 	TrayAppScheduledScanState;

typedef struct TrayAppMonitoringState
    {
    TrayAppRpcStatusCode statusCode;
    unsigned long directoryCount;
    wchar_t directories[ 1024 ];
    TrayAppScanResult lastResult;
    wchar_t message[ 256 ];
    } 	TrayAppMonitoringState;

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

void TrayAppRpcGetAvDatabaseInfo( 
    /* [in] */ handle_t hBinding,
    /* [out] */ TrayAppAvDatabaseInfo *info);

void TrayAppRpcScanFile( 
    /* [in] */ handle_t hBinding,
    /* [string][in] */ wchar_t *path,
    /* [out] */ TrayAppScanResult *result);

void TrayAppRpcScanDirectory( 
    /* [in] */ handle_t hBinding,
    /* [string][in] */ wchar_t *path,
    /* [out] */ TrayAppScanResult *result);

void TrayAppRpcScanFixedDisks( 
    /* [in] */ handle_t hBinding,
    /* [out] */ TrayAppScanResult *result);

void TrayAppRpcConfigureScheduledScan( 
    /* [in] */ handle_t hBinding,
    /* [in] */ unsigned long intervalMinutes,
    /* [in] */ boolean enabled,
    /* [out] */ TrayAppOperationResult *result);

void TrayAppRpcGetScheduledScanState( 
    /* [in] */ handle_t hBinding,
    /* [out] */ TrayAppScheduledScanState *state);

void TrayAppRpcAddMonitoredDirectory( 
    /* [in] */ handle_t hBinding,
    /* [string][in] */ wchar_t *path,
    /* [out] */ TrayAppOperationResult *result);

void TrayAppRpcRemoveMonitoredDirectory( 
    /* [in] */ handle_t hBinding,
    /* [string][in] */ wchar_t *path,
    /* [out] */ TrayAppOperationResult *result);

void TrayAppRpcGetMonitoringState( 
    /* [in] */ handle_t hBinding,
    /* [out] */ TrayAppMonitoringState *state);



extern RPC_IF_HANDLE TrayAppRpc_v1_0_c_ifspec;
extern RPC_IF_HANDLE TrayAppRpc_v1_0_s_ifspec;
#endif /* __TrayAppRpc_INTERFACE_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


