#pragma once

#include <cstdint>
#include <istream>
#include <map>
#include <string_view>
#include <string>
#include <vector>

enum class AvObjectType : uint32_t {
    Unknown = 0,
    Pe = 1,
    PowerShell = 2
};

struct AvRecord {
    std::string recordId;
    uint8_t statusCode = 0;
    long long updatedAtEpochMillis = 0;
    uint64_t objectSignaturePrefix = 0;
    uint32_t objectSignatureLength = 0;
    std::vector<unsigned char> firstBytes;
    std::vector<unsigned char> remainderHash;
    std::vector<unsigned char> objectSignature;
    uint64_t offsetBegin = 0;
    uint64_t offsetEnd = 0;
    AvObjectType objectType = AvObjectType::Unknown;
    std::vector<unsigned char> avRecordSignature;
    std::wstring name;
};

struct AvDatabase {
    bool loaded = false;
    long long releaseUnixSeconds = 0;
    std::wstring releaseDateText;
    std::map<uint64_t, std::vector<AvRecord>> recordsByPrefix;
    size_t totalRecordCount = 0;
};

enum class AvDatabaseLoadStatus {
    Ok = 0,
    IoError,
    InvalidManifestFormat,
    InvalidManifestSignature,
    InvalidDataFormat,
    InvalidDataHash,
    EmptyDatabase
};

struct AvDatabaseLoadResult {
    AvDatabaseLoadStatus status = AvDatabaseLoadStatus::Ok;
    size_t loadedRecordCount = 0;
    size_t skippedRecordCount = 0;
    std::vector<std::string> invalidRecordIds;
    std::wstring message;
};

struct ScanOutcome {
    bool completed = false;
    bool malicious = false;
    uint32_t scannedFileCount = 0;
    uint32_t detectedFileCount = 0;
    uint64_t firstMatchOffset = 0;
    AvObjectType objectType = AvObjectType::Unknown;
    std::wstring targetPath;
    std::wstring detectedPath;
    std::wstring details;
};

void LoadEmbeddedAntivirusDatabase(AvDatabase* database);
bool WriteDefaultAntivirusDatabaseFiles(
    const std::wstring& manifestPath,
    const std::wstring& dataPath,
    std::wstring* errorMessage);
AvDatabaseLoadResult LoadAntivirusDatabaseFromFiles(
    const std::wstring& manifestPath,
    const std::wstring& dataPath,
    AvDatabase* database);
void ClearAntivirusDatabase(AvDatabase* database);

ScanOutcome ScanFilePath(const std::wstring& path, const AvDatabase& database);
ScanOutcome ScanDirectoryPath(const std::wstring& path, const AvDatabase& database);
ScanOutcome ScanFixedDrives(const AvDatabase& database);
