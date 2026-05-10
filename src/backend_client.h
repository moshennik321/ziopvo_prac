#pragma once

#include <string>
#include <vector>

enum TrayAppStatusCode {
    kStatusOk = 0,
    kStatusNotAuthenticated = 1,
    kStatusInvalidCredentials = 2,
    kStatusNetworkError = 3,
    kStatusNoLicense = 4,
    kStatusLicenseBlocked = 5,
    kStatusLicenseExpired = 6,
    kStatusActivationFailed = 7,
    kStatusServerError = 8
};

struct AuthSessionData {
    bool authenticated = false;
    std::wstring email;
    std::wstring accessToken;
    std::wstring refreshToken;
    long long accessTokenExpiresAtUnixSeconds = 0;
    long long refreshTokenExpiresAtUnixSeconds = 0;
};

struct LicenseTicketData {
    bool hasLicense = false;
    bool blocked = false;
    bool expired = false;
    long long expiresAtUnixSeconds = 0;
    long long nextRefreshUnixSeconds = 0;
    std::wstring expiresAtText;
    std::wstring signature;
    std::wstring message;
};

struct BackendResult {
    int statusCode = kStatusOk;
    std::wstring message;
};

struct BinarySignaturePackage {
    std::vector<unsigned char> manifestBytes;
    std::vector<unsigned char> dataBytes;
};

BackendResult BackendLogin(
    const std::wstring& email,
    const std::wstring& password,
    AuthSessionData* authData);

BackendResult BackendRefreshTokens(
    const std::wstring& refreshToken,
    AuthSessionData* authData);

BackendResult BackendCheckLicense(
    const AuthSessionData& authData,
    LicenseTicketData* licenseData);

BackendResult BackendActivateLicense(
    const AuthSessionData& authData,
    const std::wstring& activationCode,
    LicenseTicketData* licenseData);

BackendResult BackendDownloadFullAvDatabase(BinarySignaturePackage* package);
BackendResult BackendDownloadAvRecordsByIds(
    const std::vector<std::string>& ids,
    BinarySignaturePackage* package);
