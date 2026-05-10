#pragma once

#include <string>

#include "trayapp_rpc.h"

bool RequestServiceStopViaRpc();
bool GetAuthStateViaRpc(TrayAppAuthState* state);
bool LoginViaRpc(const std::wstring& email, const std::wstring& password, TrayAppAuthState* state);
bool LogoutViaRpc(TrayAppOperationResult* result);
bool GetLicenseStateViaRpc(TrayAppLicenseState* state);
bool ActivateLicenseViaRpc(const std::wstring& activationCode, TrayAppLicenseState* state);
bool GetAvDatabaseInfoViaRpc(TrayAppAvDatabaseInfo* info);
bool ScanFileViaRpc(const std::wstring& path, TrayAppScanResult* result);
bool ScanDirectoryViaRpc(const std::wstring& path, TrayAppScanResult* result);
bool ScanFixedDisksViaRpc(TrayAppScanResult* result);
bool ConfigureScheduledScanViaRpc(unsigned long intervalMinutes, bool enabled, TrayAppOperationResult* result);
bool GetScheduledScanStateViaRpc(TrayAppScheduledScanState* state);
bool AddMonitoredDirectoryViaRpc(const std::wstring& path, TrayAppOperationResult* result);
bool RemoveMonitoredDirectoryViaRpc(const std::wstring& path, TrayAppOperationResult* result);
bool GetMonitoringStateViaRpc(TrayAppMonitoringState* state);
