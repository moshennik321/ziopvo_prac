#define WIN32_LEAN_AND_MEAN

#include "backend_client.h"

#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cwctype>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace {
constexpr wchar_t kDefaultHost[] = L"localhost";
constexpr INTERNET_PORT kDefaultPort = 8443;
constexpr wchar_t kLoginPath[] = L"/auth/login";
constexpr wchar_t kRefreshPath[] = L"/auth/refresh";
constexpr wchar_t kCheckLicensePath[] = L"/licenses/check";
constexpr wchar_t kActivateLicensePath[] = L"/licenses/activate";
constexpr wchar_t kBinaryFullPath[] = L"/api/binary/signatures/full";
constexpr wchar_t kBinaryByIdsPath[] = L"/api/binary/signatures/by-ids";
constexpr long kDefaultProductId = 1;

struct HttpResponse {
    DWORD statusCode = 0;
    std::wstring body;
};

struct RawHttpResponse {
    DWORD statusCode = 0;
    std::wstring contentType;
    std::vector<unsigned char> bodyBytes;
};

std::wstring JsonEscape(const std::wstring& value)
{
    std::wstring escaped;
    escaped.reserve(value.size());
    for (wchar_t ch : value) {
        switch (ch) {
        case L'\\':
            escaped += L"\\\\";
            break;
        case L'"':
            escaped += L"\\\"";
            break;
        case L'\r':
            escaped += L"\\r";
            break;
        case L'\n':
            escaped += L"\\n";
            break;
        case L'\t':
            escaped += L"\\t";
            break;
        default:
            escaped += ch;
            break;
        }
    }
    return escaped;
}

std::wstring NarrowToWide(const std::string& value)
{
    if (value.empty()) {
        return L"";
    }

    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size);
    return result;
}

std::string WideToUtf8(const std::wstring& value)
{
    if (value.empty()) {
        return {};
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

bool JsonTryExtractString(const std::wstring& json, const std::wstring& key, std::wstring* value)
{
    const std::wstring pattern = L"\"" + key + L"\"";
    size_t pos = json.find(pattern);
    if (pos == std::wstring::npos) {
        return false;
    }

    pos = json.find(L':', pos + pattern.size());
    if (pos == std::wstring::npos) {
        return false;
    }

    pos = json.find(L'"', pos + 1);
    if (pos == std::wstring::npos) {
        return false;
    }

    ++pos;
    std::wstring result;
    bool escaped = false;
    while (pos < json.size()) {
        const wchar_t ch = json[pos++];
        if (escaped) {
            switch (ch) {
            case L'"':
            case L'\\':
            case L'/':
                result += ch;
                break;
            case L'b':
                result += L'\b';
                break;
            case L'f':
                result += L'\f';
                break;
            case L'n':
                result += L'\n';
                break;
            case L'r':
                result += L'\r';
                break;
            case L't':
                result += L'\t';
                break;
            default:
                result += ch;
                break;
            }
            escaped = false;
            continue;
        }

        if (ch == L'\\') {
            escaped = true;
            continue;
        }

        if (ch == L'"') {
            if (value) {
                *value = result;
            }
            return true;
        }

        result += ch;
    }

    return false;
}

bool JsonTryExtractLongLong(const std::wstring& json, const std::wstring& key, long long* value)
{
    const std::wstring pattern = L"\"" + key + L"\"";
    size_t pos = json.find(pattern);
    if (pos == std::wstring::npos) {
        return false;
    }

    pos = json.find(L':', pos + pattern.size());
    if (pos == std::wstring::npos) {
        return false;
    }

    ++pos;
    while (pos < json.size() && iswspace(json[pos])) {
        ++pos;
    }

    size_t end = pos;
    if (end < json.size() && (json[end] == L'-' || json[end] == L'+')) {
        ++end;
    }
    while (end < json.size() && iswdigit(json[end])) {
        ++end;
    }

    if (end == pos) {
        return false;
    }

    if (value) {
        *value = _wtoll(json.substr(pos, end - pos).c_str());
    }
    return true;
}

bool JsonTryExtractBool(const std::wstring& json, const std::wstring& key, bool* value)
{
    const std::wstring pattern = L"\"" + key + L"\"";
    size_t pos = json.find(pattern);
    if (pos == std::wstring::npos) {
        return false;
    }

    pos = json.find(L':', pos + pattern.size());
    if (pos == std::wstring::npos) {
        return false;
    }

    ++pos;
    while (pos < json.size() && iswspace(json[pos])) {
        ++pos;
    }

    if (json.compare(pos, 4, L"true") == 0) {
        if (value) {
            *value = true;
        }
        return true;
    }

    if (json.compare(pos, 5, L"false") == 0) {
        if (value) {
            *value = false;
        }
        return true;
    }

    return false;
}

std::wstring Base64UrlDecodeToWide(const std::wstring& input)
{
    static const wchar_t* kAlphabet = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::wstring normalized = input;
    std::replace(normalized.begin(), normalized.end(), L'-', L'+');
    std::replace(normalized.begin(), normalized.end(), L'_', L'/');
    while (normalized.size() % 4 != 0) {
        normalized += L'=';
    }

    std::vector<int> table(256, -1);
    for (int i = 0; kAlphabet[i] != 0; ++i) {
        table[static_cast<unsigned char>(kAlphabet[i])] = i;
    }

    std::string bytes;
    bytes.reserve(normalized.size() * 3 / 4);

    for (size_t i = 0; i < normalized.size(); i += 4) {
        int vals[4] = { 0, 0, 0, 0 };
        int pad = 0;

        for (int j = 0; j < 4; ++j) {
            const wchar_t ch = normalized[i + j];
            if (ch == L'=') {
                vals[j] = 0;
                ++pad;
            } else {
                const int mapped = (ch < 256) ? table[static_cast<unsigned char>(ch)] : -1;
                if (mapped < 0) {
                    return L"";
                }
                vals[j] = mapped;
            }
        }

        const unsigned char first = static_cast<unsigned char>((vals[0] << 2) | (vals[1] >> 4));
        bytes.push_back(static_cast<char>(first));

        if (pad < 2) {
            const unsigned char second = static_cast<unsigned char>(((vals[1] & 0x0F) << 4) | (vals[2] >> 2));
            bytes.push_back(static_cast<char>(second));
        }

        if (pad < 1) {
            const unsigned char third = static_cast<unsigned char>(((vals[2] & 0x03) << 6) | vals[3]);
            bytes.push_back(static_cast<char>(third));
        }
    }

    return NarrowToWide(bytes);
}

bool ParseJwtToken(const std::wstring& token, std::wstring* subject, long long* expiresAtUnixSeconds)
{
    const size_t firstDot = token.find(L'.');
    if (firstDot == std::wstring::npos) {
        return false;
    }

    const size_t secondDot = token.find(L'.', firstDot + 1);
    if (secondDot == std::wstring::npos) {
        return false;
    }

    const std::wstring payload = token.substr(firstDot + 1, secondDot - firstDot - 1);
    const std::wstring decodedPayload = Base64UrlDecodeToWide(payload);
    if (decodedPayload.empty()) {
        return false;
    }

    std::wstring parsedSubject;
    long long parsedExpiration = 0;
    if (!JsonTryExtractString(decodedPayload, L"sub", &parsedSubject) ||
        !JsonTryExtractLongLong(decodedPayload, L"exp", &parsedExpiration)) {
        return false;
    }

    if (subject) {
        *subject = parsedSubject;
    }
    if (expiresAtUnixSeconds) {
        *expiresAtUnixSeconds = parsedExpiration;
    }
    return true;
}

long long FileTimeToUnixSeconds(const FILETIME& fileTime)
{
    ULARGE_INTEGER value = {};
    value.LowPart = fileTime.dwLowDateTime;
    value.HighPart = fileTime.dwHighDateTime;
    return static_cast<long long>((value.QuadPart - 116444736000000000ULL) / 10000000ULL);
}

long long ParseIso8601ToUnixSeconds(const std::wstring& value)
{
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int offsetHours = 0;
    int offsetMinutes = 0;
    wchar_t sign = L'+';

    if (swscanf_s(value.c_str(),
                  L"%d-%d-%dT%d:%d:%d",
                  &year,
                  &month,
                  &day,
                  &hour,
                  &minute,
                  &second) < 6) {
        return 0;
    }

    size_t tzPos = value.find_last_of(L"Z+-");
    if (tzPos != std::wstring::npos && value[tzPos] != L'Z') {
        sign = value[tzPos];
        swscanf_s(value.c_str() + tzPos + 1, L"%d:%d", &offsetHours, &offsetMinutes);
    }

    SYSTEMTIME st = {};
    st.wYear = static_cast<WORD>(year);
    st.wMonth = static_cast<WORD>(month);
    st.wDay = static_cast<WORD>(day);
    st.wHour = static_cast<WORD>(hour);
    st.wMinute = static_cast<WORD>(minute);
    st.wSecond = static_cast<WORD>(second);

    FILETIME fileTime = {};
    if (!SystemTimeToFileTime(&st, &fileTime)) {
        return 0;
    }

    long long unixSeconds = FileTimeToUnixSeconds(fileTime);
    const long long offsetSeconds = static_cast<long long>(offsetHours) * 3600LL + static_cast<long long>(offsetMinutes) * 60LL;
    if (tzPos != std::wstring::npos && value[tzPos] != L'Z') {
        unixSeconds -= (sign == L'-') ? -offsetSeconds : offsetSeconds;
    }

    return unixSeconds;
}

std::wstring GetEnvironmentOrDefault(const wchar_t* name, const wchar_t* fallback)
{
    wchar_t buffer[256] = {};
    const DWORD length = GetEnvironmentVariableW(name, buffer, ARRAYSIZE(buffer));
    if (length == 0 || length >= ARRAYSIZE(buffer)) {
        return fallback;
    }
    return buffer;
}

long GetEnvironmentLongOrDefault(const wchar_t* name, long fallback)
{
    const std::wstring value = GetEnvironmentOrDefault(name, L"");
    if (value.empty()) {
        return fallback;
    }
    return _wtol(value.c_str());
}

std::wstring ExtractErrorMessage(const std::wstring& responseBody, DWORD statusCode)
{
    std::wstring message;
    if (JsonTryExtractString(responseBody, L"message", &message) && !message.empty()) {
        return message;
    }

    wchar_t fallback[128] = {};
    swprintf_s(fallback, L"HTTP error %lu", statusCode);
    return fallback;
}

bool SendJsonRequest(
    const wchar_t* method,
    const wchar_t* path,
    const std::wstring& requestBody,
    const std::wstring& bearerToken,
    HttpResponse* response);

bool SendRawRequest(
    const wchar_t* method,
    const wchar_t* path,
    const std::wstring& requestContentType,
    const std::wstring& acceptType,
    const std::wstring& bearerToken,
    const std::vector<unsigned char>& requestBodyBytes,
    RawHttpResponse* response)
{
    if (!response) {
        return false;
    }

    const std::wstring host = GetEnvironmentOrDefault(L"TRAYAPP_SERVER_HOST", kDefaultHost);
    const INTERNET_PORT port = static_cast<INTERNET_PORT>(GetEnvironmentLongOrDefault(L"TRAYAPP_SERVER_PORT", kDefaultPort));

    HINTERNET session = WinHttpOpen(L"TrayApp/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        return false;
    }

    HINTERNET connection = WinHttpConnect(session, host.c_str(), port, 0);
    if (!connection) {
        WinHttpCloseHandle(session);
        return false;
    }

    HINTERNET request = WinHttpOpenRequest(connection, method, path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request) {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD securityFlags =
        SECURITY_FLAG_IGNORE_UNKNOWN_CA |
        SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
        SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
    WinHttpSetOption(request, WINHTTP_OPTION_SECURITY_FLAGS, &securityFlags, sizeof(securityFlags));

    std::wstring headers;
    if (!requestContentType.empty()) {
        headers += L"Content-Type: " + requestContentType + L"\r\n";
    }
    if (!acceptType.empty()) {
        headers += L"Accept: " + acceptType + L"\r\n";
    }
    if (!bearerToken.empty()) {
        headers += L"Authorization: Bearer " + bearerToken + L"\r\n";
    }

    BOOL sent = WinHttpSendRequest(
        request,
        headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str(),
        headers.empty() ? 0 : static_cast<DWORD>(headers.size()),
        requestBodyBytes.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<unsigned char*>(requestBodyBytes.data()),
        static_cast<DWORD>(requestBodyBytes.size()),
        static_cast<DWORD>(requestBodyBytes.size()),
        0);

    if (!sent) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    if (!WinHttpReceiveResponse(request, nullptr)) {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);

    wchar_t contentTypeBuffer[512] = {};
    DWORD contentTypeSize = sizeof(contentTypeBuffer);
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_TYPE, WINHTTP_HEADER_NAME_BY_INDEX, contentTypeBuffer, &contentTypeSize, WINHTTP_NO_HEADER_INDEX)) {
        contentTypeBuffer[0] = L'\0';
    }

    std::vector<unsigned char> responseBytes;
    DWORD availableSize = 0;
    do {
        availableSize = 0;
        if (!WinHttpQueryDataAvailable(request, &availableSize)) {
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connection);
            WinHttpCloseHandle(session);
            return false;
        }

        if (availableSize == 0) {
            break;
        }

        std::vector<unsigned char> buffer(availableSize);
        DWORD downloaded = 0;
        if (!WinHttpReadData(request, buffer.data(), availableSize, &downloaded)) {
            WinHttpCloseHandle(request);
            WinHttpCloseHandle(connection);
            WinHttpCloseHandle(session);
            return false;
        }

        responseBytes.insert(responseBytes.end(), buffer.begin(), buffer.begin() + downloaded);
    } while (availableSize > 0);

    response->statusCode = statusCode;
    response->contentType = contentTypeBuffer;
    response->bodyBytes = std::move(responseBytes);

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return true;
}

bool SendJsonRequest(
    const wchar_t* method,
    const wchar_t* path,
    const std::wstring& requestBody,
    const std::wstring& bearerToken,
    HttpResponse* response)
{
    if (!response) {
        return false;
    }

    RawHttpResponse rawResponse = {};
    const std::string requestBodyUtf8 = WideToUtf8(requestBody);
    const std::vector<unsigned char> requestBytes(requestBodyUtf8.begin(), requestBodyUtf8.end());
    if (!SendRawRequest(
            method,
            path,
            L"application/json",
            L"application/json",
            bearerToken,
            requestBytes,
            &rawResponse)) {
        return false;
    }

    response->statusCode = rawResponse.statusCode;
    response->body = NarrowToWide(std::string(rawResponse.bodyBytes.begin(), rawResponse.bodyBytes.end()));
    return true;
}

std::wstring TrimString(const std::wstring& value)
{
    size_t begin = 0;
    while (begin < value.size() && iswspace(value[begin])) {
        ++begin;
    }
    size_t end = value.size();
    while (end > begin && iswspace(value[end - 1])) {
        --end;
    }
    return value.substr(begin, end - begin);
}

bool TryExtractBoundary(const std::wstring& contentType, std::string* boundary)
{
    if (!boundary) {
        return false;
    }

    std::wstring lower = contentType;
    std::transform(lower.begin(), lower.end(), lower.begin(), towlower);
    const std::wstring marker = L"boundary=";
    size_t pos = lower.find(marker);
    if (pos == std::wstring::npos) {
        return false;
    }

    std::wstring rawBoundary = TrimString(contentType.substr(pos + marker.size()));
    const size_t semicolonPos = rawBoundary.find(L';');
    if (semicolonPos != std::wstring::npos) {
        rawBoundary = rawBoundary.substr(0, semicolonPos);
    }
    rawBoundary = TrimString(rawBoundary);
    if (rawBoundary.size() >= 2 && rawBoundary.front() == L'"' && rawBoundary.back() == L'"') {
        rawBoundary = rawBoundary.substr(1, rawBoundary.size() - 2);
    }

    if (rawBoundary.empty()) {
        return false;
    }

    *boundary = WideToUtf8(rawBoundary);
    return !boundary->empty();
}

bool TryExtractMultipartFilePart(
    const std::vector<unsigned char>& bodyBytes,
    const std::string& boundary,
    const std::string& filename,
    std::vector<unsigned char>* partBytes)
{
    if (!partBytes) {
        return false;
    }

    const std::string body(reinterpret_cast<const char*>(bodyBytes.data()), bodyBytes.size());
    const std::string filenameMarker = "filename=\"" + filename + "\"";
    const size_t headerPos = body.find(filenameMarker);
    if (headerPos == std::string::npos) {
        return false;
    }

    const size_t contentStartPos = body.find("\r\n\r\n", headerPos);
    if (contentStartPos == std::string::npos) {
        return false;
    }

    const size_t dataStart = contentStartPos + 4;
    const std::string nextBoundaryMarker = "\r\n--" + boundary;
    const size_t dataEnd = body.find(nextBoundaryMarker, dataStart);
    if (dataEnd == std::string::npos || dataEnd < dataStart) {
        return false;
    }

    partBytes->assign(bodyBytes.begin() + static_cast<long long>(dataStart), bodyBytes.begin() + static_cast<long long>(dataEnd));
    return true;
}

BackendResult ParseBinaryPackageResponse(const RawHttpResponse& response, BinarySignaturePackage* package)
{
    if (!package) {
        return { kStatusServerError, L"Binary package target is not provided" };
    }

    std::string boundary;
    if (!TryExtractBoundary(response.contentType, &boundary)) {
        return { kStatusServerError, L"Binary package response has no multipart boundary" };
    }

    if (!TryExtractMultipartFilePart(response.bodyBytes, boundary, "manifest.bin", &package->manifestBytes) ||
        !TryExtractMultipartFilePart(response.bodyBytes, boundary, "data.bin", &package->dataBytes)) {
        return { kStatusServerError, L"Binary package is incomplete" };
    }

    return {};
}

BackendResult BuildHttpErrorResult(DWORD statusCode, const std::wstring& responseBody, bool forActivation)
{
    BackendResult result = {};
    result.message = ExtractErrorMessage(responseBody, statusCode);

    if (statusCode == 400 || statusCode == 401 || statusCode == 403) {
        result.statusCode = forActivation ? kStatusActivationFailed : kStatusInvalidCredentials;
    } else if (statusCode == 404) {
        result.statusCode = kStatusNoLicense;
    } else if (statusCode == 409) {
        result.statusCode = kStatusActivationFailed;
    } else {
        result.statusCode = kStatusServerError;
    }

    return result;
}

std::wstring GetComputerNameValue()
{
    wchar_t buffer[MAX_COMPUTERNAME_LENGTH + 1] = {};
    DWORD size = ARRAYSIZE(buffer);
    if (GetComputerNameW(buffer, &size)) {
        return buffer;
    }
    return L"TrayApp Device";
}

std::wstring GetDeviceMacAddress()
{
    return GetEnvironmentOrDefault(L"TRAYAPP_DEVICE_MAC", L"00-11-22-33-44-55");
}

long GetProductId()
{
    return GetEnvironmentLongOrDefault(L"TRAYAPP_PRODUCT_ID", kDefaultProductId);
}

BackendResult ParseAuthResponse(const HttpResponse& response, AuthSessionData* authData)
{
    std::wstring accessToken;
    std::wstring refreshToken;
    if (!JsonTryExtractString(response.body, L"accessToken", &accessToken) ||
        !JsonTryExtractString(response.body, L"refreshToken", &refreshToken)) {
        return { kStatusServerError, L"Server returned an invalid auth response" };
    }

    std::wstring subject;
    long long accessExpiration = 0;
    long long refreshExpiration = 0;
    if (!ParseJwtToken(accessToken, &subject, &accessExpiration) ||
        !ParseJwtToken(refreshToken, nullptr, &refreshExpiration)) {
        return { kStatusServerError, L"Failed to read token expiration" };
    }

    authData->authenticated = true;
    authData->email = subject;
    authData->accessToken = accessToken;
    authData->refreshToken = refreshToken;
    authData->accessTokenExpiresAtUnixSeconds = accessExpiration;
    authData->refreshTokenExpiresAtUnixSeconds = refreshExpiration;
    return {};
}

BackendResult ParseLicenseResponse(const HttpResponse& response, LicenseTicketData* licenseData)
{
    std::wstring expirationText;
    std::wstring signature;
    long long ttlSeconds = 0;
    bool blocked = false;

    if (!JsonTryExtractString(response.body, L"expirationDate", &expirationText) ||
        !JsonTryExtractLongLong(response.body, L"ttlSeconds", &ttlSeconds) ||
        !JsonTryExtractBool(response.body, L"blocked", &blocked)) {
        return { kStatusServerError, L"Server returned an invalid license response" };
    }

    JsonTryExtractString(response.body, L"signature", &signature);

    const long long nowUnix = static_cast<long long>(time(nullptr));
    const long long expirationUnix = ParseIso8601ToUnixSeconds(expirationText);
    const bool expired = (expirationUnix != 0 && expirationUnix <= nowUnix);

    licenseData->hasLicense = !blocked && !expired;
    licenseData->blocked = blocked;
    licenseData->expired = expired;
    licenseData->expiresAtUnixSeconds = expirationUnix;
    licenseData->expiresAtText = expirationText;
    licenseData->signature = signature;
    licenseData->message = blocked ? L"License is blocked" : (expired ? L"License expired" : L"");

    long long refreshAt = nowUnix + std::max<long long>(30, ttlSeconds - 30);
    if (expirationUnix != 0) {
        refreshAt = (refreshAt < expirationUnix) ? refreshAt : expirationUnix;
    }
    licenseData->nextRefreshUnixSeconds = refreshAt;

    if (blocked) {
        return { kStatusLicenseBlocked, L"License is blocked" };
    }
    if (expired) {
        return { kStatusLicenseExpired, L"License expired" };
    }

    return {};
}
}

BackendResult BackendLogin(
    const std::wstring& email,
    const std::wstring& password,
    AuthSessionData* authData)
{
    HttpResponse response = {};
    const std::wstring body =
        L"{\"email\":\"" + JsonEscape(email) +
        L"\",\"password\":\"" + JsonEscape(password) + L"\"}";

    if (!SendJsonRequest(L"POST", kLoginPath, body, L"", &response)) {
        return { kStatusNetworkError, L"Failed to contact authentication server" };
    }

    if (response.statusCode < 200 || response.statusCode >= 300) {
        return BuildHttpErrorResult(response.statusCode, response.body, false);
    }

    return ParseAuthResponse(response, authData);
}

BackendResult BackendRefreshTokens(
    const std::wstring& refreshToken,
    AuthSessionData* authData)
{
    HttpResponse response = {};
    const std::wstring body =
        L"{\"refreshToken\":\"" + JsonEscape(refreshToken) + L"\"}";

    if (!SendJsonRequest(L"POST", kRefreshPath, body, L"", &response)) {
        return { kStatusNetworkError, L"Failed to refresh tokens" };
    }

    if (response.statusCode < 200 || response.statusCode >= 300) {
        return BuildHttpErrorResult(response.statusCode, response.body, false);
    }

    return ParseAuthResponse(response, authData);
}

BackendResult BackendCheckLicense(
    const AuthSessionData& authData,
    LicenseTicketData* licenseData)
{
    HttpResponse response = {};
    wchar_t productIdBuffer[32] = {};
    swprintf_s(productIdBuffer, L"%ld", GetProductId());

    const std::wstring body =
        L"{\"productId\":" + std::wstring(productIdBuffer) +
        L",\"deviceMac\":\"" + JsonEscape(GetDeviceMacAddress()) + L"\"}";

    if (!SendJsonRequest(L"POST", kCheckLicensePath, body, authData.accessToken, &response)) {
        return { kStatusNetworkError, L"Failed to check license status" };
    }

    if (response.statusCode < 200 || response.statusCode >= 300) {
        return BuildHttpErrorResult(response.statusCode, response.body, true);
    }

    return ParseLicenseResponse(response, licenseData);
}

BackendResult BackendActivateLicense(
    const AuthSessionData& authData,
    const std::wstring& activationCode,
    LicenseTicketData* licenseData)
{
    HttpResponse response = {};
    const std::wstring body =
        L"{\"activationKey\":\"" + JsonEscape(activationCode) +
        L"\",\"deviceName\":\"" + JsonEscape(GetComputerNameValue()) +
        L"\",\"deviceMac\":\"" + JsonEscape(GetDeviceMacAddress()) + L"\"}";

    if (!SendJsonRequest(L"POST", kActivateLicensePath, body, authData.accessToken, &response)) {
        return { kStatusNetworkError, L"Failed to activate license" };
    }

    if (response.statusCode < 200 || response.statusCode >= 300) {
        return BuildHttpErrorResult(response.statusCode, response.body, true);
    }

    return ParseLicenseResponse(response, licenseData);
}

BackendResult BackendDownloadFullAvDatabase(BinarySignaturePackage* package)
{
    RawHttpResponse response = {};
    if (!SendRawRequest(
            L"GET",
            kBinaryFullPath,
            L"",
            L"multipart/mixed",
            L"",
            {},
            &response)) {
        return { kStatusNetworkError, L"Failed to download antivirus database update" };
    }

    if (response.statusCode < 200 || response.statusCode >= 300) {
        return { kStatusServerError, L"Server returned an invalid antivirus database update response" };
    }

    return ParseBinaryPackageResponse(response, package);
}

BackendResult BackendDownloadAvRecordsByIds(
    const std::vector<std::string>& ids,
    BinarySignaturePackage* package)
{
    if (ids.empty()) {
        return {};
    }

    std::wstring body = L"{\"ids\":[";
    for (size_t index = 0; index < ids.size(); ++index) {
        if (index != 0) {
            body += L",";
        }
        body += L"\"" + JsonEscape(NarrowToWide(ids[index])) + L"\"";
    }
    body += L"]}";

    RawHttpResponse response = {};
    const std::string requestBodyUtf8 = WideToUtf8(body);
    const std::vector<unsigned char> requestBytes(requestBodyUtf8.begin(), requestBodyUtf8.end());
    if (!SendRawRequest(
            L"POST",
            kBinaryByIdsPath,
            L"application/json",
            L"multipart/mixed",
            L"",
            requestBytes,
            &response)) {
        return { kStatusNetworkError, L"Failed to download antivirus records from update server" };
    }

    if (response.statusCode < 200 || response.statusCode >= 300) {
        return { kStatusServerError, L"Server returned an invalid antivirus record update response" };
    }

    return ParseBinaryPackageResponse(response, package);
}
