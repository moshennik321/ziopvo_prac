#pragma once

#include <string>

#include "trayapp_rpc.h"

bool RequestServiceStopViaRpc();
bool GetAuthStateViaRpc(TrayAppAuthState* state);
bool LoginViaRpc(const std::wstring& email, const std::wstring& password, TrayAppAuthState* state);
bool LogoutViaRpc(TrayAppOperationResult* result);
bool GetLicenseStateViaRpc(TrayAppLicenseState* state);
bool ActivateLicenseViaRpc(const std::wstring& activationCode, TrayAppLicenseState* state);
