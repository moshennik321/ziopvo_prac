

/* this ALWAYS GENERATED file contains the RPC server stubs */


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

#define TYPE_FORMAT_STRING_SIZE   91                                
#define PROC_FORMAT_STRING_SIZE   229                               
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

/* Standard interface: TrayAppRpc, ver. 1.0,
   GUID={0x6B68D2E2,0x4144,0x4F88,{0x9B,0x87,0x1A,0x31,0x6A,0x68,0x7B,0x24}} */


extern const MIDL_SERVER_INFO TrayAppRpc_ServerInfo;

extern const RPC_DISPATCH_TABLE TrayAppRpc_v1_0_DispatchTable;

static const RPC_SERVER_INTERFACE TrayAppRpc___RpcServerInterface =
    {
    sizeof(RPC_SERVER_INTERFACE),
    {{0x6B68D2E2,0x4144,0x4F88,{0x9B,0x87,0x1A,0x31,0x6A,0x68,0x7B,0x24}},{1,0}},
    {{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}},
    (RPC_DISPATCH_TABLE*)&TrayAppRpc_v1_0_DispatchTable,
    0,
    0,
    0,
    &TrayAppRpc_ServerInfo,
    0x04000000
    };
RPC_IF_HANDLE TrayAppRpc_v1_0_s_ifspec = (RPC_IF_HANDLE)& TrayAppRpc___RpcServerInterface;
#ifdef __cplusplus
namespace {
#endif

extern const MIDL_STUB_DESC TrayAppRpc_StubDesc;
#ifdef __cplusplus
}
#endif


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
    186
    };


#ifdef __cplusplus
namespace {
#endif
static const MIDL_STUB_DESC TrayAppRpc_StubDesc = 
    {
    (void *)& TrayAppRpc___RpcServerInterface,
    MIDL_user_allocate,
    MIDL_user_free,
    0,
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

static const RPC_DISPATCH_FUNCTION TrayAppRpc_table[] =
    {
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    NdrServerCall2,
    0
    };
static const RPC_DISPATCH_TABLE TrayAppRpc_v1_0_DispatchTable = 
    {
    6,
    (RPC_DISPATCH_FUNCTION*)TrayAppRpc_table
    };

static const SERVER_ROUTINE TrayAppRpc_ServerRoutineTable[] = 
    {
    (SERVER_ROUTINE)TrayAppRpcStopService,
    (SERVER_ROUTINE)TrayAppRpcGetAuthState,
    (SERVER_ROUTINE)TrayAppRpcLogin,
    (SERVER_ROUTINE)TrayAppRpcLogout,
    (SERVER_ROUTINE)TrayAppRpcGetLicenseState,
    (SERVER_ROUTINE)TrayAppRpcActivateLicense
    };

static const MIDL_SERVER_INFO TrayAppRpc_ServerInfo = 
    {
    &TrayAppRpc_StubDesc,
    TrayAppRpc_ServerRoutineTable,
    trayapp_rpc__MIDL_ProcFormatString.Format,
    TrayAppRpc_FormatStringOffsetTable,
    0,
    0,
    0,
    0};
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_AMD64)*/

