

/* this ALWAYS GENERATED file contains the RPC client stubs */


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

#if defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/

#include <string.h>

#include "trayapp_rpc.h"

#define TYPE_FORMAT_STRING_SIZE   225                               
#define PROC_FORMAT_STRING_SIZE   589                               
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _trayapp_rpc_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } trayapp_rpc_MIDL_TYPE_FORMAT_STRING;

typedef struct _trayapp_rpc_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } trayapp_rpc_MIDL_PROC_FORMAT_STRING;

typedef struct _trayapp_rpc_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } trayapp_rpc_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax_2_0 = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};

#if defined(_CONTROL_FLOW_GUARD_XFG)
#define XFG_TRAMPOLINES(ObjectType)\
NDR_SHAREABLE unsigned long ObjectType ## _UserSize_XFG(unsigned long * pFlags, unsigned long Offset, void * pObject)\
{\
return  ObjectType ## _UserSize(pFlags, Offset, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserMarshal_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserMarshal(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserUnmarshal_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserUnmarshal(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE void ObjectType ## _UserFree_XFG(unsigned long * pFlags, void * pObject)\
{\
ObjectType ## _UserFree(pFlags, (ObjectType *)pObject);\
}
#define XFG_TRAMPOLINES64(ObjectType)\
NDR_SHAREABLE unsigned long ObjectType ## _UserSize64_XFG(unsigned long * pFlags, unsigned long Offset, void * pObject)\
{\
return  ObjectType ## _UserSize64(pFlags, Offset, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserMarshal64_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserMarshal64(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserUnmarshal64_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserUnmarshal64(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE void ObjectType ## _UserFree64_XFG(unsigned long * pFlags, void * pObject)\
{\
ObjectType ## _UserFree64(pFlags, (ObjectType *)pObject);\
}
#define XFG_BIND_TRAMPOLINES(HandleType, ObjectType)\
static void* ObjectType ## _bind_XFG(HandleType pObject)\
{\
return ObjectType ## _bind((ObjectType) pObject);\
}\
static void ObjectType ## _unbind_XFG(HandleType pObject, handle_t ServerHandle)\
{\
ObjectType ## _unbind((ObjectType) pObject, ServerHandle);\
}
#define XFG_TRAMPOLINE_FPTR(Function) Function ## _XFG
#define XFG_TRAMPOLINE_FPTR_DEPENDENT_SYMBOL(Symbol) Symbol ## _XFG
#else
#define XFG_TRAMPOLINES(ObjectType)
#define XFG_TRAMPOLINES64(ObjectType)
#define XFG_BIND_TRAMPOLINES(HandleType, ObjectType)
#define XFG_TRAMPOLINE_FPTR(Function) Function
#define XFG_TRAMPOLINE_FPTR_DEPENDENT_SYMBOL(Symbol) Symbol
#endif


extern const trayapp_rpc_MIDL_TYPE_FORMAT_STRING trayapp_rpc__MIDL_TypeFormatString;
extern const trayapp_rpc_MIDL_PROC_FORMAT_STRING trayapp_rpc__MIDL_ProcFormatString;
extern const trayapp_rpc_MIDL_EXPR_FORMAT_STRING trayapp_rpc__MIDL_ExprFormatString;

#define GENERIC_BINDING_TABLE_SIZE   0            


/* Standard interface: TrayAppRpc, ver. 1.0,
   GUID={0x6B68D2E2,0x4144,0x4F88,{0x9B,0x87,0x1A,0x31,0x6A,0x68,0x7B,0x24}} */



static const RPC_CLIENT_INTERFACE TrayAppRpc___RpcClientInterface =
    {
    sizeof(RPC_CLIENT_INTERFACE),
    {{0x6B68D2E2,0x4144,0x4F88,{0x9B,0x87,0x1A,0x31,0x6A,0x68,0x7B,0x24}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    0,
    0,
    0,
    0,
    0,
    0x00000000
    };
RPC_IF_HANDLE TrayAppRpc_v1_0_c_ifspec = (RPC_IF_HANDLE)& TrayAppRpc___RpcClientInterface;
#ifdef __cplusplus
namespace {
#endif

extern const MIDL_STUB_DESC TrayAppRpc_StubDesc;
#ifdef __cplusplus
}
#endif

static RPC_BINDING_HANDLE TrayAppRpc__MIDL_AutoBindHandle;


void TrayAppRpcStopService( 
    /* [in] */ handle_t hBinding)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[0],
                  hBinding);
    
}


void TrayAppRpcGetAuthState( 
    /* [in] */ handle_t hBinding,
    /* [out] */ TrayAppAuthState *state)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[30],
                  hBinding,
                  state);
    
}


void TrayAppRpcLogin( 
    /* [in] */ handle_t hBinding,
    /* [string][in] */ wchar_t *email,
    /* [string][in] */ wchar_t *password,
    /* [out] */ TrayAppAuthState *state)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[66],
                  hBinding,
                  email,
                  password,
                  state);
    
}


void TrayAppRpcLogout( 
    /* [in] */ handle_t hBinding,
    /* [out] */ TrayAppOperationResult *result)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[114],
                  hBinding,
                  result);
    
}


void TrayAppRpcGetLicenseState( 
    /* [in] */ handle_t hBinding,
    /* [out] */ TrayAppLicenseState *state)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[150],
                  hBinding,
                  state);
    
}


void TrayAppRpcActivateLicense( 
    /* [in] */ handle_t hBinding,
    /* [string][in] */ wchar_t *activationCode,
    /* [out] */ TrayAppLicenseState *state)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[186],
                  hBinding,
                  activationCode,
                  state);
    
}


void TrayAppRpcGetAvDatabaseInfo( 
    /* [in] */ handle_t hBinding,
    /* [out] */ TrayAppAvDatabaseInfo *info)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[228],
                  hBinding,
                  info);
    
}


void TrayAppRpcScanFile( 
    /* [in] */ handle_t hBinding,
    /* [string][in] */ wchar_t *path,
    /* [out] */ TrayAppScanResult *result)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[264],
                  hBinding,
                  path,
                  result);
    
}


void TrayAppRpcScanDirectory( 
    /* [in] */ handle_t hBinding,
    /* [string][in] */ wchar_t *path,
    /* [out] */ TrayAppScanResult *result)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[306],
                  hBinding,
                  path,
                  result);
    
}


void TrayAppRpcScanFixedDisks( 
    /* [in] */ handle_t hBinding,
    /* [out] */ TrayAppScanResult *result)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[348],
                  hBinding,
                  result);
    
}


void TrayAppRpcConfigureScheduledScan( 
    /* [in] */ handle_t hBinding,
    /* [in] */ unsigned long intervalMinutes,
    /* [in] */ boolean enabled,
    /* [out] */ TrayAppOperationResult *result)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[384],
                  hBinding,
                  intervalMinutes,
                  enabled,
                  result);
    
}


void TrayAppRpcGetScheduledScanState( 
    /* [in] */ handle_t hBinding,
    /* [out] */ TrayAppScheduledScanState *state)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[432],
                  hBinding,
                  state);
    
}


void TrayAppRpcAddMonitoredDirectory( 
    /* [in] */ handle_t hBinding,
    /* [string][in] */ wchar_t *path,
    /* [out] */ TrayAppOperationResult *result)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[468],
                  hBinding,
                  path,
                  result);
    
}


void TrayAppRpcRemoveMonitoredDirectory( 
    /* [in] */ handle_t hBinding,
    /* [string][in] */ wchar_t *path,
    /* [out] */ TrayAppOperationResult *result)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[510],
                  hBinding,
                  path,
                  result);
    
}


void TrayAppRpcGetMonitoringState( 
    /* [in] */ handle_t hBinding,
    /* [out] */ TrayAppMonitoringState *state)
{

    NdrClientCall2(
                  ( PMIDL_STUB_DESC  )&TrayAppRpc_StubDesc,
                  (PFORMAT_STRING) &trayapp_rpc__MIDL_ProcFormatString.Format[552],
                  hBinding,
                  state);
    
}


#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const trayapp_rpc_MIDL_PROC_FORMAT_STRING trayapp_rpc__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure TrayAppRpcStopService */

			0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x0 ),	/* 0 */
/*  8 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 10 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 12 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 14 */	NdrFcShort( 0x0 ),	/* 0 */
/* 16 */	NdrFcShort( 0x0 ),	/* 0 */
/* 18 */	0x40,		/* Oi2 Flags:  has ext, */
			0x0,		/* 0 */
/* 20 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */
/* 26 */	NdrFcShort( 0x0 ),	/* 0 */
/* 28 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Procedure TrayAppRpcGetAuthState */

/* 30 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 32 */	NdrFcLong( 0x0 ),	/* 0 */
/* 36 */	NdrFcShort( 0x1 ),	/* 1 */
/* 38 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 40 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 42 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 44 */	NdrFcShort( 0x0 ),	/* 0 */
/* 46 */	NdrFcShort( 0x0 ),	/* 0 */
/* 48 */	0x41,		/* Oi2 Flags:  srv must size, has ext, */
			0x1,		/* 1 */
/* 50 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 52 */	NdrFcShort( 0x0 ),	/* 0 */
/* 54 */	NdrFcShort( 0x0 ),	/* 0 */
/* 56 */	NdrFcShort( 0x0 ),	/* 0 */
/* 58 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter state */

/* 60 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 62 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 64 */	NdrFcShort( 0xc ),	/* Type Offset=12 */

	/* Procedure TrayAppRpcLogin */

/* 66 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 68 */	NdrFcLong( 0x0 ),	/* 0 */
/* 72 */	NdrFcShort( 0x2 ),	/* 2 */
/* 74 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 76 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 78 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 80 */	NdrFcShort( 0x0 ),	/* 0 */
/* 82 */	NdrFcShort( 0x0 ),	/* 0 */
/* 84 */	0x43,		/* Oi2 Flags:  srv must size, clt must size, has ext, */
			0x3,		/* 3 */
/* 86 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 88 */	NdrFcShort( 0x0 ),	/* 0 */
/* 90 */	NdrFcShort( 0x0 ),	/* 0 */
/* 92 */	NdrFcShort( 0x0 ),	/* 0 */
/* 94 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter email */

/* 96 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 98 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 100 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Parameter password */

/* 102 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 104 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 106 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Parameter state */

/* 108 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 110 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 112 */	NdrFcShort( 0xc ),	/* Type Offset=12 */

	/* Procedure TrayAppRpcLogout */

/* 114 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 116 */	NdrFcLong( 0x0 ),	/* 0 */
/* 120 */	NdrFcShort( 0x3 ),	/* 3 */
/* 122 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 124 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 126 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 128 */	NdrFcShort( 0x0 ),	/* 0 */
/* 130 */	NdrFcShort( 0x0 ),	/* 0 */
/* 132 */	0x41,		/* Oi2 Flags:  srv must size, has ext, */
			0x1,		/* 1 */
/* 134 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 136 */	NdrFcShort( 0x0 ),	/* 0 */
/* 138 */	NdrFcShort( 0x0 ),	/* 0 */
/* 140 */	NdrFcShort( 0x0 ),	/* 0 */
/* 142 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter result */

/* 144 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 146 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 148 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Procedure TrayAppRpcGetLicenseState */

/* 150 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 152 */	NdrFcLong( 0x0 ),	/* 0 */
/* 156 */	NdrFcShort( 0x4 ),	/* 4 */
/* 158 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 160 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 162 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 164 */	NdrFcShort( 0x0 ),	/* 0 */
/* 166 */	NdrFcShort( 0x0 ),	/* 0 */
/* 168 */	0x41,		/* Oi2 Flags:  srv must size, has ext, */
			0x1,		/* 1 */
/* 170 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 172 */	NdrFcShort( 0x0 ),	/* 0 */
/* 174 */	NdrFcShort( 0x0 ),	/* 0 */
/* 176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 178 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter state */

/* 180 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 182 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 184 */	NdrFcShort( 0x42 ),	/* Type Offset=66 */

	/* Procedure TrayAppRpcActivateLicense */

/* 186 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 188 */	NdrFcLong( 0x0 ),	/* 0 */
/* 192 */	NdrFcShort( 0x5 ),	/* 5 */
/* 194 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 196 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 198 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 202 */	NdrFcShort( 0x0 ),	/* 0 */
/* 204 */	0x43,		/* Oi2 Flags:  srv must size, clt must size, has ext, */
			0x2,		/* 2 */
/* 206 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 208 */	NdrFcShort( 0x0 ),	/* 0 */
/* 210 */	NdrFcShort( 0x0 ),	/* 0 */
/* 212 */	NdrFcShort( 0x0 ),	/* 0 */
/* 214 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter activationCode */

/* 216 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 218 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 220 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Parameter state */

/* 222 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 224 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 226 */	NdrFcShort( 0x42 ),	/* Type Offset=66 */

	/* Procedure TrayAppRpcGetAvDatabaseInfo */

/* 228 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 230 */	NdrFcLong( 0x0 ),	/* 0 */
/* 234 */	NdrFcShort( 0x6 ),	/* 6 */
/* 236 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 238 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 240 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 242 */	NdrFcShort( 0x0 ),	/* 0 */
/* 244 */	NdrFcShort( 0x0 ),	/* 0 */
/* 246 */	0x41,		/* Oi2 Flags:  srv must size, has ext, */
			0x1,		/* 1 */
/* 248 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 250 */	NdrFcShort( 0x0 ),	/* 0 */
/* 252 */	NdrFcShort( 0x0 ),	/* 0 */
/* 254 */	NdrFcShort( 0x0 ),	/* 0 */
/* 256 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter info */

/* 258 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 260 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 262 */	NdrFcShort( 0x5e ),	/* Type Offset=94 */

	/* Procedure TrayAppRpcScanFile */

/* 264 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 266 */	NdrFcLong( 0x0 ),	/* 0 */
/* 270 */	NdrFcShort( 0x7 ),	/* 7 */
/* 272 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 274 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 276 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 278 */	NdrFcShort( 0x0 ),	/* 0 */
/* 280 */	NdrFcShort( 0x0 ),	/* 0 */
/* 282 */	0x43,		/* Oi2 Flags:  srv must size, clt must size, has ext, */
			0x2,		/* 2 */
/* 284 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 286 */	NdrFcShort( 0x0 ),	/* 0 */
/* 288 */	NdrFcShort( 0x0 ),	/* 0 */
/* 290 */	NdrFcShort( 0x0 ),	/* 0 */
/* 292 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter path */

/* 294 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 296 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 298 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Parameter result */

/* 300 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 302 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 304 */	NdrFcShort( 0x80 ),	/* Type Offset=128 */

	/* Procedure TrayAppRpcScanDirectory */

/* 306 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 308 */	NdrFcLong( 0x0 ),	/* 0 */
/* 312 */	NdrFcShort( 0x8 ),	/* 8 */
/* 314 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 316 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 318 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 320 */	NdrFcShort( 0x0 ),	/* 0 */
/* 322 */	NdrFcShort( 0x0 ),	/* 0 */
/* 324 */	0x43,		/* Oi2 Flags:  srv must size, clt must size, has ext, */
			0x2,		/* 2 */
/* 326 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 328 */	NdrFcShort( 0x0 ),	/* 0 */
/* 330 */	NdrFcShort( 0x0 ),	/* 0 */
/* 332 */	NdrFcShort( 0x0 ),	/* 0 */
/* 334 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter path */

/* 336 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 338 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 340 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Parameter result */

/* 342 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 344 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 346 */	NdrFcShort( 0x80 ),	/* Type Offset=128 */

	/* Procedure TrayAppRpcScanFixedDisks */

/* 348 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 350 */	NdrFcLong( 0x0 ),	/* 0 */
/* 354 */	NdrFcShort( 0x9 ),	/* 9 */
/* 356 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 358 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 360 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 362 */	NdrFcShort( 0x0 ),	/* 0 */
/* 364 */	NdrFcShort( 0x0 ),	/* 0 */
/* 366 */	0x41,		/* Oi2 Flags:  srv must size, has ext, */
			0x1,		/* 1 */
/* 368 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 370 */	NdrFcShort( 0x0 ),	/* 0 */
/* 372 */	NdrFcShort( 0x0 ),	/* 0 */
/* 374 */	NdrFcShort( 0x0 ),	/* 0 */
/* 376 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter result */

/* 378 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 380 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 382 */	NdrFcShort( 0x80 ),	/* Type Offset=128 */

	/* Procedure TrayAppRpcConfigureScheduledScan */

/* 384 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 386 */	NdrFcLong( 0x0 ),	/* 0 */
/* 390 */	NdrFcShort( 0xa ),	/* 10 */
/* 392 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 394 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 396 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 398 */	NdrFcShort( 0xd ),	/* 13 */
/* 400 */	NdrFcShort( 0x0 ),	/* 0 */
/* 402 */	0x41,		/* Oi2 Flags:  srv must size, has ext, */
			0x3,		/* 3 */
/* 404 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 406 */	NdrFcShort( 0x0 ),	/* 0 */
/* 408 */	NdrFcShort( 0x0 ),	/* 0 */
/* 410 */	NdrFcShort( 0x0 ),	/* 0 */
/* 412 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter intervalMinutes */

/* 414 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 416 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 418 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter enabled */

/* 420 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 422 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 424 */	0x3,		/* FC_SMALL */
			0x0,		/* 0 */

	/* Parameter result */

/* 426 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 428 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 430 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Procedure TrayAppRpcGetScheduledScanState */

/* 432 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 434 */	NdrFcLong( 0x0 ),	/* 0 */
/* 438 */	NdrFcShort( 0xb ),	/* 11 */
/* 440 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 442 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 444 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 446 */	NdrFcShort( 0x0 ),	/* 0 */
/* 448 */	NdrFcShort( 0x0 ),	/* 0 */
/* 450 */	0x41,		/* Oi2 Flags:  srv must size, has ext, */
			0x1,		/* 1 */
/* 452 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 454 */	NdrFcShort( 0x0 ),	/* 0 */
/* 456 */	NdrFcShort( 0x0 ),	/* 0 */
/* 458 */	NdrFcShort( 0x0 ),	/* 0 */
/* 460 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter state */

/* 462 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 464 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 466 */	NdrFcShort( 0xa2 ),	/* Type Offset=162 */

	/* Procedure TrayAppRpcAddMonitoredDirectory */

/* 468 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 470 */	NdrFcLong( 0x0 ),	/* 0 */
/* 474 */	NdrFcShort( 0xc ),	/* 12 */
/* 476 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 478 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 480 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 482 */	NdrFcShort( 0x0 ),	/* 0 */
/* 484 */	NdrFcShort( 0x0 ),	/* 0 */
/* 486 */	0x43,		/* Oi2 Flags:  srv must size, clt must size, has ext, */
			0x2,		/* 2 */
/* 488 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 490 */	NdrFcShort( 0x0 ),	/* 0 */
/* 492 */	NdrFcShort( 0x0 ),	/* 0 */
/* 494 */	NdrFcShort( 0x0 ),	/* 0 */
/* 496 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter path */

/* 498 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 500 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 502 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Parameter result */

/* 504 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 506 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 508 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Procedure TrayAppRpcRemoveMonitoredDirectory */

/* 510 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 512 */	NdrFcLong( 0x0 ),	/* 0 */
/* 516 */	NdrFcShort( 0xd ),	/* 13 */
/* 518 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 520 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 522 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 524 */	NdrFcShort( 0x0 ),	/* 0 */
/* 526 */	NdrFcShort( 0x0 ),	/* 0 */
/* 528 */	0x43,		/* Oi2 Flags:  srv must size, clt must size, has ext, */
			0x2,		/* 2 */
/* 530 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 532 */	NdrFcShort( 0x0 ),	/* 0 */
/* 534 */	NdrFcShort( 0x0 ),	/* 0 */
/* 536 */	NdrFcShort( 0x0 ),	/* 0 */
/* 538 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter path */

/* 540 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 542 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 544 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Parameter result */

/* 546 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 548 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 550 */	NdrFcShort( 0x2a ),	/* Type Offset=42 */

	/* Procedure TrayAppRpcGetMonitoringState */

/* 552 */	0x0,		/* 0 */
			0x48,		/* Old Flags:  */
/* 554 */	NdrFcLong( 0x0 ),	/* 0 */
/* 558 */	NdrFcShort( 0xe ),	/* 14 */
/* 560 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 562 */	0x32,		/* FC_BIND_PRIMITIVE */
			0x0,		/* 0 */
/* 564 */	NdrFcShort( 0x0 ),	/* X64 Stack size/offset = 0 */
/* 566 */	NdrFcShort( 0x0 ),	/* 0 */
/* 568 */	NdrFcShort( 0x0 ),	/* 0 */
/* 570 */	0x41,		/* Oi2 Flags:  srv must size, has ext, */
			0x1,		/* 1 */
/* 572 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 574 */	NdrFcShort( 0x0 ),	/* 0 */
/* 576 */	NdrFcShort( 0x0 ),	/* 0 */
/* 578 */	NdrFcShort( 0x0 ),	/* 0 */
/* 580 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter state */

/* 582 */	NdrFcShort( 0x113 ),	/* Flags:  must size, must free, out, simple ref, */
/* 584 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 586 */	NdrFcShort( 0xc8 ),	/* Type Offset=200 */

			0x0
        }
    };

static const trayapp_rpc_MIDL_TYPE_FORMAT_STRING trayapp_rpc__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x11, 0x0,	/* FC_RP */
/*  4 */	NdrFcShort( 0x8 ),	/* Offset= 8 (12) */
/*  6 */	
			0x1d,		/* FC_SMFARRAY */
			0x1,		/* 1 */
/*  8 */	NdrFcShort( 0x200 ),	/* 512 */
/* 10 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 12 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x1,		/* 1 */
/* 14 */	NdrFcShort( 0x408 ),	/* 1032 */
/* 16 */	NdrFcShort( 0x0 ),	/* 0 */
/* 18 */	NdrFcShort( 0x0 ),	/* Offset= 0 (18) */
/* 20 */	0xd,		/* FC_ENUM16 */
			0x3,		/* FC_SMALL */
/* 22 */	0x3,		/* FC_SMALL */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 24 */	0x0,		/* 0 */
			NdrFcShort( 0xffed ),	/* Offset= -19 (6) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 28 */	0x0,		/* 0 */
			NdrFcShort( 0xffe9 ),	/* Offset= -23 (6) */
			0x3e,		/* FC_STRUCTPAD2 */
/* 32 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 34 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 36 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 38 */	
			0x11, 0x0,	/* FC_RP */
/* 40 */	NdrFcShort( 0x2 ),	/* Offset= 2 (42) */
/* 42 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x1,		/* 1 */
/* 44 */	NdrFcShort( 0x204 ),	/* 516 */
/* 46 */	NdrFcShort( 0x0 ),	/* 0 */
/* 48 */	NdrFcShort( 0x0 ),	/* Offset= 0 (48) */
/* 50 */	0xd,		/* FC_ENUM16 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 52 */	0x0,		/* 0 */
			NdrFcShort( 0xffd1 ),	/* Offset= -47 (6) */
			0x5b,		/* FC_END */
/* 56 */	
			0x11, 0x0,	/* FC_RP */
/* 58 */	NdrFcShort( 0x8 ),	/* Offset= 8 (66) */
/* 60 */	
			0x1d,		/* FC_SMFARRAY */
			0x1,		/* 1 */
/* 62 */	NdrFcShort( 0x80 ),	/* 128 */
/* 64 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 66 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 68 */	NdrFcShort( 0x290 ),	/* 656 */
/* 70 */	NdrFcShort( 0x0 ),	/* 0 */
/* 72 */	NdrFcShort( 0x0 ),	/* Offset= 0 (72) */
/* 74 */	0xd,		/* FC_ENUM16 */
			0x3,		/* FC_SMALL */
/* 76 */	0x3,		/* FC_SMALL */
			0x3,		/* FC_SMALL */
/* 78 */	0x3,		/* FC_SMALL */
			0xb,		/* FC_HYPER */
/* 80 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 82 */	NdrFcShort( 0xffea ),	/* Offset= -22 (60) */
/* 84 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 86 */	NdrFcShort( 0xffb0 ),	/* Offset= -80 (6) */
/* 88 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 90 */	
			0x11, 0x0,	/* FC_RP */
/* 92 */	NdrFcShort( 0x2 ),	/* Offset= 2 (94) */
/* 94 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 96 */	NdrFcShort( 0x298 ),	/* 664 */
/* 98 */	NdrFcShort( 0x0 ),	/* 0 */
/* 100 */	NdrFcShort( 0x0 ),	/* Offset= 0 (100) */
/* 102 */	0xd,		/* FC_ENUM16 */
			0x3,		/* FC_SMALL */
/* 104 */	0x3f,		/* FC_STRUCTPAD3 */
			0xb,		/* FC_HYPER */
/* 106 */	0x8,		/* FC_LONG */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 108 */	0x0,		/* 0 */
			NdrFcShort( 0xffcf ),	/* Offset= -49 (60) */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 112 */	0x0,		/* 0 */
			NdrFcShort( 0xff95 ),	/* Offset= -107 (6) */
			0x40,		/* FC_STRUCTPAD4 */
/* 116 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 118 */	
			0x11, 0x0,	/* FC_RP */
/* 120 */	NdrFcShort( 0x8 ),	/* Offset= 8 (128) */
/* 122 */	
			0x1d,		/* FC_SMFARRAY */
			0x1,		/* 1 */
/* 124 */	NdrFcShort( 0x400 ),	/* 1024 */
/* 126 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 128 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 130 */	NdrFcShort( 0xc20 ),	/* 3104 */
/* 132 */	NdrFcShort( 0x0 ),	/* 0 */
/* 134 */	NdrFcShort( 0x0 ),	/* Offset= 0 (134) */
/* 136 */	0xd,		/* FC_ENUM16 */
			0x3,		/* FC_SMALL */
/* 138 */	0x3,		/* FC_SMALL */
			0x3e,		/* FC_STRUCTPAD2 */
/* 140 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 142 */	0xb,		/* FC_HYPER */
			0xd,		/* FC_ENUM16 */
/* 144 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 146 */	NdrFcShort( 0xffe8 ),	/* Offset= -24 (122) */
/* 148 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 150 */	NdrFcShort( 0xffe4 ),	/* Offset= -28 (122) */
/* 152 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 154 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (122) */
/* 156 */	0x40,		/* FC_STRUCTPAD4 */
			0x5b,		/* FC_END */
/* 158 */	
			0x11, 0x0,	/* FC_RP */
/* 160 */	NdrFcShort( 0x2 ),	/* Offset= 2 (162) */
/* 162 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 164 */	NdrFcShort( 0xeb8 ),	/* 3768 */
/* 166 */	NdrFcShort( 0x0 ),	/* 0 */
/* 168 */	NdrFcShort( 0x0 ),	/* Offset= 0 (168) */
/* 170 */	0xd,		/* FC_ENUM16 */
			0x3,		/* FC_SMALL */
/* 172 */	0x3f,		/* FC_STRUCTPAD3 */
			0x8,		/* FC_LONG */
/* 174 */	0x40,		/* FC_STRUCTPAD4 */
			0xb,		/* FC_HYPER */
/* 176 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 178 */	NdrFcShort( 0xff8a ),	/* Offset= -118 (60) */
/* 180 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 182 */	NdrFcShort( 0xffca ),	/* Offset= -54 (128) */
/* 184 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 186 */	NdrFcShort( 0xff4c ),	/* Offset= -180 (6) */
/* 188 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 190 */	
			0x11, 0x0,	/* FC_RP */
/* 192 */	NdrFcShort( 0x8 ),	/* Offset= 8 (200) */
/* 194 */	
			0x1d,		/* FC_SMFARRAY */
			0x1,		/* 1 */
/* 196 */	NdrFcShort( 0x800 ),	/* 2048 */
/* 198 */	0x5,		/* FC_WCHAR */
			0x5b,		/* FC_END */
/* 200 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 202 */	NdrFcShort( 0x1628 ),	/* 5672 */
/* 204 */	NdrFcShort( 0x0 ),	/* 0 */
/* 206 */	NdrFcShort( 0x0 ),	/* Offset= 0 (206) */
/* 208 */	0xd,		/* FC_ENUM16 */
			0x8,		/* FC_LONG */
/* 210 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 212 */	NdrFcShort( 0xffee ),	/* Offset= -18 (194) */
/* 214 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 216 */	NdrFcShort( 0xffa8 ),	/* Offset= -88 (128) */
/* 218 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 220 */	NdrFcShort( 0xff2a ),	/* Offset= -214 (6) */
/* 222 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */

			0x0
        }
    };

static const unsigned short TrayAppRpc_FormatStringOffsetTable[] =
    {
    0,
    30,
    66,
    114,
    150,
    186,
    228,
    264,
    306,
    348,
    384,
    432,
    468,
    510,
    552
    };


#ifdef __cplusplus
namespace {
#endif
static const MIDL_STUB_DESC TrayAppRpc_StubDesc = 
    {
    (void *)& TrayAppRpc___RpcClientInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    &TrayAppRpc__MIDL_AutoBindHandle,
    0,
    0,
    0,
    0,
    trayapp_rpc__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x8010274, /* MIDL Version 8.1.628 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };
#ifdef __cplusplus
}
#endif
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_AMD64)*/

