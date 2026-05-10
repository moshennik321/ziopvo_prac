#define WIN32_LEAN_AND_MEAN

#include "antivirus_engine.h"

#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>

#include <algorithm>
#include <ctime>
#include <fstream>
#include <sstream>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")

namespace {
constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ULL;
constexpr uint64_t kFnvPrime = 1099511628211ULL;
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

uint64_t ComputeFnv1a64(const unsigned char* data, size_t size)
{
    uint64_t hash = kFnvOffsetBasis;
    for (size_t index = 0; index < size; ++index) {
        hash ^= static_cast<uint64_t>(data[index]);
        hash *= kFnvPrime;
    }
    return hash;
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

    if (BCryptGetProperty(
            algorithmHandle,
            BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(&objectLength),
            sizeof(objectLength),
            &dataLength,
            0) != 0 ||
        BCryptGetProperty(
            algorithmHandle,
            BCRYPT_HASH_LENGTH,
            reinterpret_cast<PUCHAR>(&hashLength),
            sizeof(hashLength),
            &dataLength,
            0) != 0) {
        BCryptCloseAlgorithmProvider(algorithmHandle, 0);
        return {};
    }

    hashObject.resize(objectLength);
    hashBytes.resize(hashLength);

    if (BCryptCreateHash(
            algorithmHandle,
            &hashHandle,
            hashObject.data(),
            static_cast<ULONG>(hashObject.size()),
            nullptr,
            0,
            0) != 0 ||
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

std::vector<unsigned char> UInt64ToBytes(uint64_t value)
{
    std::vector<unsigned char> bytes(8, 0);
    for (size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<unsigned char>((value >> (index * 8)) & 0xFFU);
    }
    return bytes;
}

std::vector<unsigned char> UInt32ToBytes(uint32_t value)
{
    std::vector<unsigned char> bytes(4, 0);
    for (size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<unsigned char>((value >> (index * 8)) & 0xFFU);
    }
    return bytes;
}

uint64_t BytesToUInt64LittleEndian(const unsigned char* bytes)
{
    uint64_t value = 0;
    for (int index = 0; index < 8; ++index) {
        value |= (static_cast<uint64_t>(bytes[index]) << (index * 8));
    }
    return value;
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

std::vector<unsigned char> BuildSignableRecordBytes(const AvRecord& record)
{
    std::vector<unsigned char> bytes;
    const auto appendBytes = [&bytes](const std::vector<unsigned char>& part) {
        bytes.insert(bytes.end(), part.begin(), part.end());
    };

    appendBytes(UInt64ToBytes(record.objectSignaturePrefix));
    appendBytes(UInt32ToBytes(record.objectSignatureLength));
    appendBytes(record.objectSignature);
    appendBytes(UInt64ToBytes(record.offsetBegin));
    appendBytes(UInt64ToBytes(record.offsetEnd));
    appendBytes(UInt32ToBytes(static_cast<uint32_t>(record.objectType)));
    return bytes;
}

std::vector<unsigned char> BuildRecordSignature(
    uint64_t prefix,
    uint32_t length,
    const std::vector<unsigned char>& objectSignature,
    uint64_t offsetBegin,
    uint64_t offsetEnd,
    AvObjectType objectType)
{
    std::vector<unsigned char> serialized;
    const auto appendUInt64 = [&serialized](uint64_t value) {
        const std::vector<unsigned char> bytes = UInt64ToBytes(value);
        serialized.insert(serialized.end(), bytes.begin(), bytes.end());
    };
    const auto appendUInt32 = [&serialized](uint32_t value) {
        for (int index = 0; index < 4; ++index) {
            serialized.push_back(static_cast<unsigned char>((value >> (index * 8)) & 0xFFU));
        }
    };

    appendUInt64(prefix);
    appendUInt32(length);
    serialized.insert(serialized.end(), objectSignature.begin(), objectSignature.end());
    appendUInt64(offsetBegin);
    appendUInt64(offsetEnd);
    appendUInt32(static_cast<uint32_t>(objectType));

    const uint64_t signatureHash = ComputeFnv1a64(serialized.data(), serialized.size());
    return UInt64ToBytes(signatureHash);
}

AvRecord BuildRecord(
    const std::vector<unsigned char>& signatureBytes,
    uint64_t offsetBegin,
    uint64_t offsetEnd,
    AvObjectType objectType,
    const std::wstring& name)
{
    AvRecord record = {};
    record.objectSignatureLength = static_cast<uint32_t>(signatureBytes.size());
    record.objectSignaturePrefix = BytesToUInt64LittleEndian(signatureBytes.data());
    record.objectSignature = UInt64ToBytes(ComputeFnv1a64(signatureBytes.data(), signatureBytes.size()));
    record.offsetBegin = offsetBegin;
    record.offsetEnd = offsetEnd;
    record.objectType = objectType;
    record.name = name;
    return record;
}

bool VerifyRecordSignature(const AvRecord& record)
{
    const std::vector<unsigned char> certificateBytes = DecodeBase64OrPem(kSigningCertificatePem);
    if (certificateBytes.empty() || record.avRecordSignature.empty()) {
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

    const std::vector<unsigned char> signableBytes = BuildSignableRecordBytes(record);
    const std::vector<unsigned char> hashBytes = ComputeSha256(signableBytes.data(), signableBytes.size());
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
        const_cast<PUCHAR>(record.avRecordSignature.data()),
        static_cast<ULONG>(record.avRecordSignature.size()),
        BCRYPT_PAD_PKCS1);

    BCryptDestroyKey(keyHandle);
    CertFreeCertificateContext(certContext);
    return verifyStatus == 0;
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
            if (record.objectSignatureLength < 8) {
                continue;
            }

            const uint32_t extraByteCount = record.objectSignatureLength - 8;
            std::vector<unsigned char> signatureBytes = prefixBytes;
            if (extraByteCount > 0) {
                std::vector<unsigned char> extraBytes(extraByteCount, 0);
                stream.read(reinterpret_cast<char*>(extraBytes.data()), extraByteCount);
                if (static_cast<uint32_t>(stream.gcount()) != extraByteCount) {
                    continue;
                }
                signatureBytes.insert(signatureBytes.end(), extraBytes.begin(), extraBytes.end());
            }

            const std::vector<unsigned char> computedSignature = UInt64ToBytes(
                ComputeFnv1a64(signatureBytes.data(), signatureBytes.size()));
            if (computedSignature != record.objectSignature) {
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

    ClearAntivirusDatabase(database);

    const std::vector<unsigned char> peSignature = {
        'M', 'Z', 0x90, 0x00, 0x03, 0x00, 0x00, 0x00,
        'T', 'R', 'A', 'Y', 'P', 'E', '!', '!'
    };
    const std::vector<unsigned char> powerShellSignature = {
        'I', 'n', 'v', 'o', 'k', 'e', '-', 'M',
        'i', 'm', 'i', 'k', 'a', 't', 'z'
    };

    AvRecord peRecord = BuildRecord(peSignature, 0, 0, AvObjectType::Pe, L"Test.PE.Traffic");
    peRecord.avRecordSignature = DecodeBase64OrPem(
        "UxvNvzphQmEQUZLD0159dRgOUobnyxpPhqGccbSv+TLLilzcC6SDQ/wkEAQhs+ZFZDRBPZG6/CLOLZMLfgz2gJ8cQz0+HQEHI73RlfzSvZ/SqZcTZoJS1k+vzFZnGAnsS7A36eaisTZj+aK/567gKIlioarjRV9odTJmHp6qRfcYCF1AX72dkhN6dv7W/SSMQreDjmtF7EYBVTufPNot+K9ricCqc9X/GismCEhp7p8gy5Xi7Ndv2UjVFvwNOaJaL3nSvCyj88icRfma4GiartVzcjKhBjg4Ek/J7ho9y1JlU7ts6MqMJ4estz0CP6nAFe3Ag00ENYp4WR5Jpw9pag==");

    AvRecord powerShellRecord = BuildRecord(powerShellSignature, 0, 4096, AvObjectType::PowerShell, L"Test.PS.Mimikatz");
    powerShellRecord.avRecordSignature = DecodeBase64OrPem(
        "lnejOluCfObTccb+5nezt1yZMuWo0Ejt5cBluPmQ+17B3HhCBDBoT5avUbI3gY8EECZ5/Ly62zcke9TfA06/XVM4813EhnApGjiaM5878pL68rn3QaAEyxyvCyLz1sQeeB7RLVcqRf1m4xwNAD769w2nEVE7e+IW/ouc0oCgxRPdZVrrKISxT8QW4AETAltSVhu55oaRqpl25bz6nMEVeocDZ0kj/cLfvJQUlkLcXq+PRoyTKtRZdaSfq5QYDwU5MzYdMsNdTRTbEfzuGFMwz85zwKbslg1tD5mYjnTenRe9qbBS4zpFhBZ+l2Nb8gwiIPP7Q7vE5CZbb+u4Z2KhEw==");

    const std::vector<AvRecord> records = { peRecord, powerShellRecord };

    for (const AvRecord& record : records) {
        if (VerifyRecordSignature(record)) {
            database->recordsByPrefix[record.objectSignaturePrefix].push_back(record);
        }
    }

    database->loaded = true;
    database->totalRecordCount = 0;
    for (const auto& bucket : database->recordsByPrefix) {
        database->totalRecordCount += bucket.second.size();
    }
    database->releaseUnixSeconds = static_cast<long long>(time(nullptr));
    database->releaseDateText = FormatLocalDateTime(database->releaseUnixSeconds);
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

    DWORD attributes = GetFileAttributesW(path.c_str());
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
