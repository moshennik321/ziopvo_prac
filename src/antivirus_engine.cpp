#define WIN32_LEAN_AND_MEAN

#include "antivirus_engine.h"

#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>

#include <algorithm>
#include <array>
#include <ctime>
#include <fstream>
#include <sstream>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")

namespace {
constexpr wchar_t kPowerShellExtension[] = L".ps1";
constexpr char kSigningCertificatePem[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDYzCCAkugAwIBAgIISWZVn+XKxNgwDQYJKoZIhvcNAQELBQAwYDELMAkGA1UE\n"
    "BhMCUlUxDjAMBgNVBAgTBVN0YXRlMQ0wCwYDVQQHEwRDaXR5MRAwDgYDVQQKEwdF\n"
    "eGFtcGxlMQwwCgYDVQQLEwNEZXYxEjAQBgNVBAMTCUxhYlNlcnZlcjAeFw0yNjA0\n"
    "MDYxNzM0MTVaFw0zNjA0MDMxNzM0MTVaMGAxCzAJBgNVBAYTAlJVMQ4wDAYDVQQI\n"
    "EwVTdGF0ZTENMAsGA1UEBxMEQ2l0eTEQMA4GA1UEChMHRXhhbXBsZTEMMAoGA1UE\n"
    "CxMDRGV2MRIwEAYDVQQDEwlMYWJTZXJ2ZXIwggEiMA0GCSqGSIb3DQEBAQUAA4IB\n"
    "DwAwggEKAoIBAQDDd6IuEO/BsgoaLXEMMDJ8d7JjL49hdwh1zz4avTrDcPG42Xya\n"
    "ui2Kc+XJxhPgwu2LLoq3cofhS8qk7w54O/kfwJyGg6FvdQSvSLwkRo/yikVJugbc\n"
    "mg50ih1VH1qgOdPQDGQMRBMPs4ehHKIyMTVSMhpFyMT0WB+XZIgOhs81nCQxfocR\n"
    "VmMFJOynu1MFrLQXC71UNJuZViyNJynjGH1BURIhLEhOirtRspImzX1gUMo6PrWy\n"
    "6GPW08LXG9v7VFpbLIGsO9ZVsB8g+KJ0k6QqXKQCI3JiaFbvS//FL4HyJVhlT8lv\n"
    "fAMB0eV8NxUM3njozaURnB5IfP4MohRMEJhXAgMBAAGjITAfMB0GA1UdDgQWBBTB\n"
    "0/2KZ6Wb+YIyAhIGLLmZ63sH5TANBgkqhkiG9w0BAQsFAAOCAQEAQRTL0UCM07/X\n"
    "HFQE3M9YjZUQPCu0mWr6r25e5d3CSDds3Kz3VaXOly3Qe5xT9+RUImJTnq4mwy6f\n"
    "M0CC05hCxZSRKzaelihA6TOX+8ibDL7Od0s81bRWB5XxVMkUOwrOt5mrXjGVif7U\n"
    "Oml8n7bil5GT03c5u78EoDT6YtFZeIgGS94X9L/WrVKRv4pptrfyM3CivEQJDBgZ\n"
    "DwWjpYaIxkEr36u/CdcfwqHprlwUDZzHTT2c6lmioDP+OPGWYU/klWgTe3LXSZ0M\n"
    "AiiRiZH+AFcXeIzfoMGVlFHB+HDMhH4IU1ZuvWLiY0du2UbZWDOFk/zxMLHUDsTq\n"
    "I7SMLW3GVw==\n"
    "-----END CERTIFICATE-----\n";
constexpr char kDefaultDataBase64[] =
    "REItS0xJTU9WAAEAAAACAAAAD1Rlc3QuUEUuVHJhZmZpYwAAAAhNWpAAAwAAAAAAACBa+PSx0WN0ZsxjJMAqxT4sO1jUxpdMal7m0U+AWlUxVQAAAAAAAAAIAAAAAlBFAAAAAAAAAAAAAAAAAAAAAAAAABBUZXN0LlBTLk1pbWlrYXR6AAAACEludm9rZS1NAAAAID0/ey92wc+S4b9STOy/RmjR9t8ekvdIEf1/f3Z9XflHAAAAAAAAAAcAAAAKUE9XRVJTSEVMTAAAAAAAAAAAAAAAAAAAEAA=";
constexpr char kDefaultManifestBase64[] =
    "TUYtS0xJTU9WAAEBAAABjrQjXAD//////////wAAAAJpJWu6/aPlZYJgSa15grxkjMdGA81Lihn3NnIKP85BEBEREREREREREREREREREREBAAABjrQjXAAAAAAAAAAAAAAAAGEAAAEAUqGmf7RsbIZ8y9Loo/J6azL9AK7b7IVRCcUcyJHB4Wt2DbRS0+0CUVCSVKYPjuwf+kC30f7i0MlSIL4PbXdDifYPpJdBl3YUgmdvtzKQdysTp+kERRRi9C6MVBuL3LcgTvPWiCjvUZ0kcfiQFPDhoG6HHZrG+ZkJm531FIG8B7DXIfc2ccTbx4NbNHIE/TLqGKffhHO+sfYcpdJO6pLZ+a94LaYDqUecXYEmlPdun/thcYMsvK4npPJvrXUlU6x4LugVEbfyXm19EKMgC3j/kuyWijJqAqQXA2sv8t/7j5sbSofce9vQOvy6XXHVcoP/euUjz68dVr7sabvdcQff8CIiIiIiIiIiIiIiIiIiIiIBAAABjrQjXAAAAAAAAAAAYQAAAGoAAAEAmSXaWYpKsluuj10SXbpTzBilNQjVWxlR7tVinI5jZQXrE9wad60ULuYyLdC8DO9Vyn8QSMusQ7nWgmUCi5uYULj4doBZOZ0m4eLb8gvfthB5rLioaRMJurA35l/0ZWUpdOchrWXf1av1sRT0fGNjuayCWAHOlxrbZ5yjHq9G3mDsX6hX9JFsULj7lyqJ0MdluivV2Y4sRJ9S21hnCVtlH/H1ryU6Wvxj8apVOQeg8bN9pHjoOpgHWK7nYzUWwQvu7hK3yPuh9mfR49+IgDVTgavhvb4+L25vCBN7NMAPVDmLNKkmKzYGpyz/e59BNSBJjpxjZ4khOQSgaO3VbpwXZQAAAQAgPptCkDH6VPki1c6YU9Ls70EJ9sLxX4YUgNatWFIDr8py2OLb9weFgndWTtGFI9CUf6ycSnnTqQRbQxXY+fVcBNzApIH9F502Y9kZx7Us5Eox4CsIzvSKKmpC8/lpRBlYqsjxBW2nsG5dLKxqYaGTbedshqNqN8s25Z6oNPB+HQj4uUilzEu+T1iIzFB0kXHmdIRcmMMhCN+lPv5FG3NFaScu4MEliYh5Al/gkga8XGShfSjQtN0E1U25pI7Moc+7WgYTZsTbi/7OjLbNKjECnse5GZbomexMFpY2S523w2/8H+8VxuUvrMo4WlRFyEgJFAKycReZpK2qKoqzoN9s";

constexpr char kDataMagic[] = "DB-KLIMOV";
constexpr char kManifestMagic[] = "MF-KLIMOV";
constexpr uint16_t kDataVersion = 1;
constexpr uint16_t kManifestVersion = 1;

struct ManifestEntry {
    std::string id;
    uint8_t statusCode = 0;
    long long updatedAtEpochMillis = 0;
    uint64_t dataOffset = 0;
    uint32_t dataLength = 0;
    std::vector<unsigned char> recordSignatureBytes;
};

struct ManifestInfo {
    uint8_t exportType = 0;
    long long generatedAtEpochMillis = 0;
    long long sinceEpochMillis = 0;
    std::array<unsigned char, 32> dataSha256 = {};
    std::vector<ManifestEntry> entries;
};

uint64_t BytesToUInt64LittleEndian(const unsigned char* bytes)
{
    uint64_t value = 0;
    for (int index = 0; index < 8; ++index) {
        value |= (static_cast<uint64_t>(bytes[index]) << (index * 8));
    }
    return value;
}

std::vector<unsigned char> ComputeSha256(const unsigned char* data, size_t size)
{
    BCRYPT_ALG_HANDLE algorithmHandle = nullptr;
    BCRYPT_HASH_HANDLE hashHandle = nullptr;
    DWORD objectLength = 0;
    DWORD dataLength = 0;
    DWORD hashLength = 0;
    std::vector<unsigned char> hashObject;
    std::vector<unsigned char> hashBytes;

    if (BCryptOpenAlgorithmProvider(&algorithmHandle, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) {
        return {};
    }

    if (BCryptGetProperty(algorithmHandle, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength), &dataLength, 0) != 0 ||
        BCryptGetProperty(algorithmHandle, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashLength), sizeof(hashLength), &dataLength, 0) != 0) {
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);
        return {};
    }

    hashObject.resize(objectLength);
    hashBytes.resize(hashLength);

    if (BCryptCreateHash(algorithmHandle, &hashHandle, hashObject.data(), static_cast<ULONG>(hashObject.size()), nullptr, 0, 0) != 0 ||
        BCryptHashData(hashHandle, const_cast<PUCHAR>(data), static_cast<ULONG>(size), 0) != 0 ||
        BCryptFinishHash(hashHandle, hashBytes.data(), static_cast<ULONG>(hashBytes.size()), 0) != 0) {
        if (hashHandle) {
            BCryptDestroyHash(hashHandle);
        }
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);
        return {};
    }

    BCryptDestroyHash(hashHandle);
    BCryptCloseAlgorithmProvider(algorithmHandle, 0);
    return hashBytes;
}

std::wstring FormatLocalDateTime(long long unixSeconds)
{
    if (unixSeconds <= 0) {
        return L"";
    }

    const time_t rawTime = static_cast<time_t>(unixSeconds);
    tm localTime = {};
    localtime_s(&localTime, &rawTime);

    wchar_t buffer[64] = {};
    wcsftime(buffer, ARRAYSIZE(buffer), L"%Y-%m-%d %H:%M:%S", &localTime);
    return buffer;
}

std::wstring ToLowerString(std::wstring value)
{
    std::transform(value.begin(), value.end(), value.begin(), towlower);
    return value;
}

std::wstring GetFileExtensionLower(const std::wstring& path)
{
    const size_t dotPos = path.find_last_of(L'.');
    if (dotPos == std::wstring::npos) {
        return L"";
    }
    return ToLowerString(path.substr(dotPos));
}

AvObjectType DetectObjectType(const std::wstring& path, std::istream& stream)
{
    stream.clear();
    stream.seekg(0, std::ios::beg);

    unsigned char header[2] = {};
    stream.read(reinterpret_cast<char*>(header), 2);
    if (stream.gcount() == 2 && header[0] == 'M' && header[1] == 'Z') {
        return AvObjectType::Pe;
    }

    if (GetFileExtensionLower(path) == kPowerShellExtension) {
        return AvObjectType::PowerShell;
    }

    return AvObjectType::Unknown;
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

std::wstring BytesToHex(const std::vector<unsigned char>& bytes)
{
    static const wchar_t* alphabet = L"0123456789abcdef";
    std::wstring output;
    output.reserve(bytes.size() * 2);
    for (unsigned char value : bytes) {
        output.push_back(alphabet[(value >> 4) & 0x0F]);
        output.push_back(alphabet[value & 0x0F]);
    }
    return output;
}

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
        case L'\b':
            escaped += L"\\b";
            break;
        case L'\f':
            escaped += L"\\f";
            break;
        case L'\n':
            escaped += L"\\n";
            break;
        case L'\r':
            escaped += L"\\r";
            break;
        case L'\t':
            escaped += L"\\t";
            break;
        default:
            if (ch <= 0x1F) {
                wchar_t buffer[7] = {};
                swprintf_s(buffer, L"\\u%04x", static_cast<unsigned int>(ch));
                escaped += buffer;
            } else {
                escaped += ch;
            }
            break;
        }
    }
    return escaped;
}

std::wstring StatusCodeToName(uint8_t statusCode)
{
    return statusCode == 2 ? L"DELETED" : L"ACTUAL";
}

std::wstring ObjectTypeToBackendName(AvObjectType objectType)
{
    switch (objectType) {
    case AvObjectType::Pe:
        return L"PE";
    case AvObjectType::PowerShell:
        return L"POWERSHELL";
    default:
        return L"UNKNOWN";
    }
}

AvObjectType BackendNameToObjectType(const std::wstring& fileType)
{
    const std::wstring normalized = ToLowerString(fileType);
    if (normalized == L"pe") {
        return AvObjectType::Pe;
    }
    if (normalized == L"powershell" || normalized == L"ps1") {
        return AvObjectType::PowerShell;
    }
    return AvObjectType::Unknown;
}

std::wstring BuildCanonicalRecordJson(const AvRecord& record)
{
    const std::wstring firstBytesHex = BytesToHex(record.firstBytes);
    const std::wstring remainderHashHex = BytesToHex(record.remainderHash);
    const std::wstring fileType = ObjectTypeToBackendName(record.objectType);
    const std::wstring status = StatusCodeToName(record.statusCode);
    const long long remainderLength = static_cast<long long>(record.objectSignatureLength) - static_cast<long long>(record.firstBytes.size());

    return
        L"{\"fileType\":\"" + JsonEscape(fileType) +
        L"\",\"firstBytesHex\":\"" + JsonEscape(firstBytesHex) +
        L"\",\"offsetEnd\":" + std::to_wstring(record.offsetEnd) +
        L",\"offsetStart\":" + std::to_wstring(record.offsetBegin) +
        L",\"remainderHashHex\":\"" + JsonEscape(remainderHashHex) +
        L"\",\"remainderLength\":" + std::to_wstring(remainderLength) +
        L",\"status\":\"" + JsonEscape(status) +
        L"\",\"threatName\":\"" + JsonEscape(record.name) + L"\"}";
}

std::vector<unsigned char> DecodeBase64OrPem(const std::string& encoded)
{
    DWORD requiredSize = 0;
    if (!CryptStringToBinaryA(
            encoded.c_str(),
            static_cast<DWORD>(encoded.size()),
            CRYPT_STRING_ANY,
            nullptr,
            &requiredSize,
            nullptr,
            nullptr)) {
        return {};
    }

    std::vector<unsigned char> decoded(requiredSize);
    if (!CryptStringToBinaryA(
            encoded.c_str(),
            static_cast<DWORD>(encoded.size()),
            CRYPT_STRING_ANY,
            decoded.data(),
            &requiredSize,
            nullptr,
            nullptr)) {
        return {};
    }
    decoded.resize(requiredSize);
    return decoded;
}

bool VerifyRsaSignature(const std::vector<unsigned char>& payload, const std::vector<unsigned char>& signatureBytes)
{
    const std::vector<unsigned char> certificateBytes = DecodeBase64OrPem(kSigningCertificatePem);
    if (certificateBytes.empty() || signatureBytes.empty()) {
        return false;
    }

    PCCERT_CONTEXT certContext = CertCreateCertificateContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        certificateBytes.data(),
        static_cast<DWORD>(certificateBytes.size()));
    if (!certContext) {
        return false;
    }

    BCRYPT_KEY_HANDLE keyHandle = nullptr;
    if (!CryptImportPublicKeyInfoEx2(
            X509_ASN_ENCODING,
            &certContext->pCertInfo->SubjectPublicKeyInfo,
            0,
            nullptr,
            &keyHandle)) {
        CertFreeCertificateContext(certContext);
        return false;
    }

    const std::vector<unsigned char> hashBytes = ComputeSha256(payload.data(), payload.size());
    if (hashBytes.empty()) {
        BCryptDestroyKey(keyHandle);
        CertFreeCertificateContext(certContext);
        return false;
    }

    BCRYPT_PKCS1_PADDING_INFO paddingInfo = {};
    paddingInfo.pszAlgId = const_cast<LPWSTR>(BCRYPT_SHA256_ALGORITHM);

    const NTSTATUS verifyStatus = BCryptVerifySignature(
        keyHandle,
        &paddingInfo,
        const_cast<PUCHAR>(hashBytes.data()),
        static_cast<ULONG>(hashBytes.size()),
        const_cast<PUCHAR>(signatureBytes.data()),
        static_cast<ULONG>(signatureBytes.size()),
        BCRYPT_PAD_PKCS1);

    BCryptDestroyKey(keyHandle);
    CertFreeCertificateContext(certContext);
    return verifyStatus == 0;
}

struct BigEndianReader {
    const std::vector<unsigned char>& bytes;
    size_t offset = 0;

    bool ReadUInt8(uint8_t* value)
    {
        if (offset + 1 > bytes.size()) {
            return false;
        }
        *value = bytes[offset++];
        return true;
    }

    bool ReadUInt16(uint16_t* value)
    {
        if (offset + 2 > bytes.size()) {
            return false;
        }
        *value = (static_cast<uint16_t>(bytes[offset]) << 8) |
            static_cast<uint16_t>(bytes[offset + 1]);
        offset += 2;
        return true;
    }

    bool ReadUInt32(uint32_t* value)
    {
        if (offset + 4 > bytes.size()) {
            return false;
        }
        *value =
            (static_cast<uint32_t>(bytes[offset]) << 24) |
            (static_cast<uint32_t>(bytes[offset + 1]) << 16) |
            (static_cast<uint32_t>(bytes[offset + 2]) << 8) |
            static_cast<uint32_t>(bytes[offset + 3]);
        offset += 4;
        return true;
    }

    bool ReadUInt64(uint64_t* value)
    {
        if (offset + 8 > bytes.size()) {
            return false;
        }
        *value = 0;
        for (int index = 0; index < 8; ++index) {
            *value = (*value << 8) | bytes[offset + index];
        }
        offset += 8;
        return true;
    }

    bool ReadInt64(long long* value)
    {
        uint64_t unsignedValue = 0;
        if (!ReadUInt64(&unsignedValue)) {
            return false;
        }
        *value = static_cast<long long>(unsignedValue);
        return true;
    }

    bool ReadBytes(size_t count, std::vector<unsigned char>* value)
    {
        if (offset + count > bytes.size()) {
            return false;
        }
        value->assign(bytes.begin() + static_cast<long long>(offset), bytes.begin() + static_cast<long long>(offset + count));
        offset += count;
        return true;
    }

    bool ReadLengthPrefixedBytes(std::vector<unsigned char>* value)
    {
        uint32_t length = 0;
        if (!ReadUInt32(&length)) {
            return false;
        }
        return ReadBytes(length, value);
    }

    bool ReadUtf8(std::wstring* value)
    {
        std::vector<unsigned char> bytesValue;
        if (!ReadLengthPrefixedBytes(&bytesValue)) {
            return false;
        }
        *value = NarrowToWide(std::string(bytesValue.begin(), bytesValue.end()));
        return true;
    }
};

std::string UuidToString(uint64_t msb, uint64_t lsb)
{
    std::ostringstream stream;
    stream.setf(std::ios::hex, std::ios::basefield);
    stream.fill('0');
    stream.width(8);
    stream << static_cast<uint32_t>(msb >> 32);
    stream << '-';
    stream.width(4);
    stream << static_cast<uint16_t>((msb >> 16) & 0xFFFF);
    stream << '-';
    stream.width(4);
    stream << static_cast<uint16_t>(msb & 0xFFFF);
    stream << '-';
    stream.width(4);
    stream << static_cast<uint16_t>(lsb >> 48);
    stream << '-';
    stream.width(12);
    stream << (lsb & 0x0000FFFFFFFFFFFFULL);
    return stream.str();
}

bool ParseManifestBytes(
    const std::vector<unsigned char>& manifestBytes,
    ManifestInfo* manifestInfo,
    std::vector<unsigned char>* unsignedManifestBytes,
    std::vector<unsigned char>* manifestSignature,
    std::wstring* errorMessage)
{
    BigEndianReader reader{ manifestBytes };
    std::vector<unsigned char> magicBytes;
    if (!reader.ReadBytes(strlen(kManifestMagic), &magicBytes) ||
        std::string(magicBytes.begin(), magicBytes.end()) != kManifestMagic) {
        if (errorMessage) {
            *errorMessage = L"Неверный формат манифеста";
        }
        return false;
    }

    uint16_t version = 0;
    uint8_t exportType = 0;
    long long generatedAt = 0;
    long long sinceEpoch = 0;
    uint32_t entryCount = 0;
    std::vector<unsigned char> shaBytes;

    if (!reader.ReadUInt16(&version) ||
        version != kManifestVersion ||
        !reader.ReadUInt8(&exportType) ||
        !reader.ReadInt64(&generatedAt) ||
        !reader.ReadInt64(&sinceEpoch) ||
        !reader.ReadUInt32(&entryCount) ||
        !reader.ReadBytes(32, &shaBytes)) {
        if (errorMessage) {
            *errorMessage = L"Манифест поврежден";
        }
        return false;
    }

    manifestInfo->exportType = exportType;
    manifestInfo->generatedAtEpochMillis = generatedAt;
    manifestInfo->sinceEpochMillis = sinceEpoch;
    std::copy(shaBytes.begin(), shaBytes.end(), manifestInfo->dataSha256.begin());
    manifestInfo->entries.clear();

    for (uint32_t index = 0; index < entryCount; ++index) {
        uint64_t msb = 0;
        uint64_t lsb = 0;
        ManifestEntry entry;
        if (!reader.ReadUInt64(&msb) ||
            !reader.ReadUInt64(&lsb) ||
            !reader.ReadUInt8(&entry.statusCode) ||
            !reader.ReadInt64(&entry.updatedAtEpochMillis) ||
            !reader.ReadUInt64(&entry.dataOffset) ||
            !reader.ReadUInt32(&entry.dataLength) ||
            !reader.ReadLengthPrefixedBytes(&entry.recordSignatureBytes)) {
            if (errorMessage) {
                *errorMessage = L"Манифест содержит неполную запись";
            }
            return false;
        }
        entry.id = UuidToString(msb, lsb);
        manifestInfo->entries.push_back(entry);
    }

    const size_t unsignedLength = reader.offset;
    if (!reader.ReadLengthPrefixedBytes(manifestSignature) || reader.offset != manifestBytes.size()) {
        if (errorMessage) {
            *errorMessage = L"Подпись манифеста отсутствует";
        }
        return false;
    }

    unsignedManifestBytes->assign(manifestBytes.begin(), manifestBytes.begin() + static_cast<long long>(unsignedLength));
    return true;
}

bool ParseDataRecord(
    const std::vector<unsigned char>& bytes,
    const ManifestEntry& manifestEntry,
    AvRecord* record,
    std::wstring* errorMessage)
{
    BigEndianReader reader{ bytes };
    std::wstring threatName;
    std::wstring fileType;
    std::vector<unsigned char> firstBytes;
    std::vector<unsigned char> remainderHash;
    long long remainderLength = 0;
    long long offsetStart = 0;
    long long offsetEnd = 0;

    if (!reader.ReadUtf8(&threatName) ||
        !reader.ReadLengthPrefixedBytes(&firstBytes) ||
        !reader.ReadLengthPrefixedBytes(&remainderHash) ||
        !reader.ReadInt64(&remainderLength) ||
        !reader.ReadUtf8(&fileType) ||
        !reader.ReadInt64(&offsetStart) ||
        !reader.ReadInt64(&offsetEnd) ||
        reader.offset != bytes.size()) {
        if (errorMessage) {
            *errorMessage = L"Повреждена запись базы";
        }
        return false;
    }

    if (firstBytes.size() != 8 || remainderLength < 0 || offsetEnd < offsetStart) {
        if (errorMessage) {
            *errorMessage = L"Запись базы содержит некорректные поля";
        }
        return false;
    }

    record->recordId = manifestEntry.id;
    record->statusCode = manifestEntry.statusCode;
    record->updatedAtEpochMillis = manifestEntry.updatedAtEpochMillis;
    record->firstBytes = firstBytes;
    record->remainderHash = remainderHash;
    record->objectSignaturePrefix = BytesToUInt64LittleEndian(firstBytes.data());
    record->objectSignatureLength = static_cast<uint32_t>(firstBytes.size() + remainderLength);
    record->objectSignature = remainderHash;
    record->offsetBegin = static_cast<uint64_t>(offsetStart);
    record->offsetEnd = static_cast<uint64_t>(offsetEnd);
    record->objectType = BackendNameToObjectType(fileType);
    record->avRecordSignature = manifestEntry.recordSignatureBytes;
    record->name = threatName;
    return true;
}

bool VerifyRecordSignature(const AvRecord& record)
{
    const std::wstring canonicalJson = BuildCanonicalRecordJson(record);
    const std::string canonicalUtf8 = WideToUtf8(canonicalJson);
    const std::vector<unsigned char> payload(canonicalUtf8.begin(), canonicalUtf8.end());
    return VerifyRsaSignature(payload, record.avRecordSignature);
}

bool ReadFileBytes(const std::wstring& path, std::vector<unsigned char>* bytes)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        return false;
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff length = stream.tellg();
    if (length < 0) {
        return false;
    }
    stream.seekg(0, std::ios::beg);
    bytes->resize(static_cast<size_t>(length));
    if (!bytes->empty()) {
        stream.read(reinterpret_cast<char*>(bytes->data()), length);
    }
    return stream.good() || stream.eof();
}

bool WriteFileBytes(const std::wstring& path, const std::vector<unsigned char>& bytes)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream.is_open()) {
        return false;
    }
    if (!bytes.empty()) {
        stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    return stream.good();
}

AvDatabaseLoadResult LoadAntivirusDatabaseFromBytes(
    const std::vector<unsigned char>& manifestBytes,
    const std::vector<unsigned char>& dataBytes,
    AvDatabase* database)
{
    AvDatabaseLoadResult result = {};
    if (!database) {
        result.status = AvDatabaseLoadStatus::IoError;
        result.message = L"Не задан объект базы";
        return result;
    }

    ClearAntivirusDatabase(database);

    ManifestInfo manifestInfo;
    std::vector<unsigned char> unsignedManifest;
    std::vector<unsigned char> manifestSignature;
    if (!ParseManifestBytes(manifestBytes, &manifestInfo, &unsignedManifest, &manifestSignature, &result.message)) {
        result.status = AvDatabaseLoadStatus::InvalidManifestFormat;
        return result;
    }

    if (!VerifyRsaSignature(unsignedManifest, manifestSignature)) {
        result.status = AvDatabaseLoadStatus::InvalidManifestSignature;
        result.message = L"ЭЦП манифеста недействительна";
        return result;
    }

    const std::vector<unsigned char> dataSha256 = ComputeSha256(dataBytes.data(), dataBytes.size());
    if (dataSha256.size() != manifestInfo.dataSha256.size() ||
        !std::equal(dataSha256.begin(), dataSha256.end(), manifestInfo.dataSha256.begin())) {
        result.status = AvDatabaseLoadStatus::InvalidDataHash;
        result.message = L"Контрольная сумма data.bin не совпадает";
        return result;
    }

    BigEndianReader dataReader{ dataBytes };
    std::vector<unsigned char> dataMagic;
    uint16_t dataVersion = 0;
    uint32_t recordCount = 0;
    if (!dataReader.ReadBytes(strlen(kDataMagic), &dataMagic) ||
        std::string(dataMagic.begin(), dataMagic.end()) != kDataMagic ||
        !dataReader.ReadUInt16(&dataVersion) ||
        dataVersion != kDataVersion ||
        !dataReader.ReadUInt32(&recordCount)) {
        result.status = AvDatabaseLoadStatus::InvalidDataFormat;
        result.message = L"Формат data.bin поврежден";
        return result;
    }

    const size_t recordsBaseOffset = dataReader.offset;
    for (const ManifestEntry& entry : manifestInfo.entries) {
        const size_t start = recordsBaseOffset + static_cast<size_t>(entry.dataOffset);
        const size_t end = start + static_cast<size_t>(entry.dataLength);
        if (end > dataBytes.size() || start > end) {
            result.status = AvDatabaseLoadStatus::InvalidDataFormat;
            result.message = L"Манифест содержит некорректное смещение записи";
            return result;
        }

        std::vector<unsigned char> rawRecord(dataBytes.begin() + static_cast<long long>(start), dataBytes.begin() + static_cast<long long>(end));
        AvRecord record = {};
        std::wstring recordError;
        if (!ParseDataRecord(rawRecord, entry, &record, &recordError)) {
            ++result.skippedRecordCount;
            result.invalidRecordIds.push_back(entry.id);
            continue;
        }

        if (!VerifyRecordSignature(record)) {
            ++result.skippedRecordCount;
            result.invalidRecordIds.push_back(entry.id);
            continue;
        }

        if (record.statusCode != 1 || record.objectType == AvObjectType::Unknown) {
            ++result.skippedRecordCount;
            continue;
        }

        database->recordsByPrefix[record.objectSignaturePrefix].push_back(record);
        ++result.loadedRecordCount;
    }

    database->loaded = true;
    database->releaseUnixSeconds = manifestInfo.generatedAtEpochMillis / 1000LL;
    database->releaseDateText = FormatLocalDateTime(database->releaseUnixSeconds);
    database->totalRecordCount = result.loadedRecordCount;
    result.status = AvDatabaseLoadStatus::Ok;
    if (result.loadedRecordCount == 0) {
        result.message = L"Не загружено ни одной корректной записи";
    }
    return result;
}

ScanOutcome ScanByteStream(std::istream& stream, const std::wstring& path, const AvDatabase& database)
{
    ScanOutcome outcome = {};
    outcome.completed = true;
    outcome.targetPath = path;
    outcome.scannedFileCount = 1;

    if (!database.loaded || database.recordsByPrefix.empty()) {
        outcome.details = L"Антивирусные базы не загружены";
        return outcome;
    }

    stream.clear();
    stream.seekg(0, std::ios::end);
    const std::streamoff streamLength = stream.tellg();
    if (streamLength < 0) {
        outcome.completed = false;
        outcome.details = L"Не удалось определить размер файла";
        return outcome;
    }

    if (streamLength < 8) {
        outcome.details = L"Сигнатуры не найдены";
        return outcome;
    }

    const AvObjectType objectType = DetectObjectType(path, stream);
    outcome.objectType = objectType;

    std::vector<unsigned char> prefixBytes(8, 0);
    for (uint64_t offset = 0; offset + 8 <= static_cast<uint64_t>(streamLength); ++offset) {
        stream.clear();
        stream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
        stream.read(reinterpret_cast<char*>(prefixBytes.data()), 8);
        if (stream.gcount() != 8) {
            break;
        }

        const uint64_t prefix = BytesToUInt64LittleEndian(prefixBytes.data());
        const auto bucketIt = database.recordsByPrefix.find(prefix);
        if (bucketIt == database.recordsByPrefix.end()) {
            continue;
        }

        for (const AvRecord& record : bucketIt->second) {
            if (record.objectType != objectType) {
                continue;
            }
            if (offset < record.offsetBegin || offset > record.offsetEnd) {
                continue;
            }
            if (record.firstBytes.size() != 8 || record.objectSignatureLength < 8) {
                continue;
            }

            const uint32_t extraByteCount = record.objectSignatureLength - static_cast<uint32_t>(record.firstBytes.size());
            std::vector<unsigned char> extraBytes(extraByteCount, 0);
            if (extraByteCount > 0) {
                stream.read(reinterpret_cast<char*>(extraBytes.data()), extraByteCount);
                if (static_cast<uint32_t>(stream.gcount()) != extraByteCount) {
                    continue;
                }
            }

            const std::vector<unsigned char> computedHash = ComputeSha256(extraBytes.data(), extraBytes.size());
            if (computedHash != record.objectSignature) {
                continue;
            }

            outcome.malicious = true;
            outcome.detectedFileCount = 1;
            outcome.firstMatchOffset = offset;
            outcome.detectedPath = path;
            outcome.details = L"Обнаружена сигнатура: " + record.name;
            return outcome;
        }
    }

    outcome.details = L"Сигнатуры не найдены";
    return outcome;
}

void EnumerateFilesRecursive(const std::wstring& directoryPath, std::vector<std::wstring>* files)
{
    if (!files) {
        return;
    }

    WIN32_FIND_DATAW findData = {};
    HANDLE findHandle = FindFirstFileW((directoryPath + L"\\*").c_str(), &findData);
    if (findHandle == INVALID_HANDLE_VALUE) {
        return;
    }

    do {
        const std::wstring entryName = findData.cFileName;
        if (entryName == L"." || entryName == L"..") {
            continue;
        }

        const std::wstring fullPath = directoryPath + L"\\" + entryName;
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
            EnumerateFilesRecursive(fullPath, files);
        } else {
            files->push_back(fullPath);
        }
    } while (FindNextFileW(findHandle, &findData));

    FindClose(findHandle);
}
}

void LoadEmbeddedAntivirusDatabase(AvDatabase* database)
{
    if (!database) {
        return;
    }

    const std::vector<unsigned char> manifestBytes = DecodeBase64OrPem(kDefaultManifestBase64);
    const std::vector<unsigned char> dataBytes = DecodeBase64OrPem(kDefaultDataBase64);
    const AvDatabaseLoadResult result = LoadAntivirusDatabaseFromBytes(manifestBytes, dataBytes, database);
    if (result.status != AvDatabaseLoadStatus::Ok) {
        ClearAntivirusDatabase(database);
    }
}

bool WriteDefaultAntivirusDatabaseFiles(
    const std::wstring& manifestPath,
    const std::wstring& dataPath,
    std::wstring* errorMessage)
{
    const std::vector<unsigned char> manifestBytes = DecodeBase64OrPem(kDefaultManifestBase64);
    const std::vector<unsigned char> dataBytes = DecodeBase64OrPem(kDefaultDataBase64);
    if (manifestBytes.empty() || dataBytes.empty()) {
        if (errorMessage) {
            *errorMessage = L"Не удалось декодировать встроенные базы";
        }
        return false;
    }

    if (!WriteFileBytes(manifestPath, manifestBytes) || !WriteFileBytes(dataPath, dataBytes)) {
        if (errorMessage) {
            *errorMessage = L"Не удалось записать встроенные базы на диск";
        }
        return false;
    }

    return true;
}

AvDatabaseLoadResult LoadAntivirusDatabaseFromFiles(
    const std::wstring& manifestPath,
    const std::wstring& dataPath,
    AvDatabase* database)
{
    AvDatabaseLoadResult result = {};
    std::vector<unsigned char> manifestBytes;
    std::vector<unsigned char> dataBytes;

    if (!ReadFileBytes(manifestPath, &manifestBytes) || !ReadFileBytes(dataPath, &dataBytes)) {
        result.status = AvDatabaseLoadStatus::IoError;
        result.message = L"Не удалось прочитать файлы антивирусных баз";
        return result;
    }

    return LoadAntivirusDatabaseFromBytes(manifestBytes, dataBytes, database);
}

void ClearAntivirusDatabase(AvDatabase* database)
{
    if (!database) {
        return;
    }

    database->loaded = false;
    database->releaseUnixSeconds = 0;
    database->releaseDateText.clear();
    database->recordsByPrefix.clear();
    database->totalRecordCount = 0;
}

ScanOutcome ScanFilePath(const std::wstring& path, const AvDatabase& database)
{
    std::ifstream fileStream(path, std::ios::binary);
    if (!fileStream.is_open()) {
        ScanOutcome outcome = {};
        outcome.completed = false;
        outcome.targetPath = path;
        outcome.details = L"Не удалось открыть файл";
        return outcome;
    }

    return ScanByteStream(fileStream, path, database);
}

ScanOutcome ScanDirectoryPath(const std::wstring& path, const AvDatabase& database)
{
    ScanOutcome outcome = {};
    outcome.completed = true;
    outcome.targetPath = path;

    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        outcome.completed = false;
        outcome.details = L"Не удалось открыть директорию";
        return outcome;
    }

    std::vector<std::wstring> files;
    EnumerateFilesRecursive(path, &files);

    outcome.scannedFileCount = static_cast<uint32_t>(files.size());
    for (const std::wstring& filePath : files) {
        const ScanOutcome fileOutcome = ScanFilePath(filePath, database);
        if (!fileOutcome.completed) {
            continue;
        }
        if (fileOutcome.malicious) {
            ++outcome.detectedFileCount;
            if (!outcome.malicious) {
                outcome.malicious = true;
                outcome.detectedPath = filePath;
                outcome.firstMatchOffset = fileOutcome.firstMatchOffset;
                outcome.objectType = fileOutcome.objectType;
                outcome.details = fileOutcome.details;
            }
        }
    }

    if (!outcome.malicious) {
        outcome.details = L"В выбранной директории угроз не обнаружено";
    }

    return outcome;
}

ScanOutcome ScanFixedDrives(const AvDatabase& database)
{
    ScanOutcome outcome = {};
    outcome.completed = true;
    outcome.targetPath = L"Fixed drives";

    wchar_t drives[512] = {};
    const DWORD driveLength = GetLogicalDriveStringsW(ARRAYSIZE(drives), drives);
    if (driveLength == 0 || driveLength >= ARRAYSIZE(drives)) {
        outcome.completed = false;
        outcome.details = L"Не удалось получить список логических дисков";
        return outcome;
    }

    for (const wchar_t* currentDrive = drives; *currentDrive != L'\0'; currentDrive += lstrlenW(currentDrive) + 1) {
        if (GetDriveTypeW(currentDrive) != DRIVE_FIXED) {
            continue;
        }

        const ScanOutcome driveOutcome = ScanDirectoryPath(currentDrive, database);
        outcome.scannedFileCount += driveOutcome.scannedFileCount;
        outcome.detectedFileCount += driveOutcome.detectedFileCount;
        if (driveOutcome.malicious && !outcome.malicious) {
            outcome.malicious = true;
            outcome.detectedPath = driveOutcome.detectedPath;
            outcome.firstMatchOffset = driveOutcome.firstMatchOffset;
            outcome.objectType = driveOutcome.objectType;
            outcome.details = driveOutcome.details;
        }
    }

    if (!outcome.malicious && outcome.details.empty()) {
        outcome.details = L"На несъемных дисках угроз не обнаружено";
    }

    return outcome;
}
