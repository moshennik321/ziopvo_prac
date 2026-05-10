#pragma once

#include <cstdint>
#include <istream>
#include <map>
#include <string>
#include <vector>

enum class AvObjectType : uint32_t {
    Unknown = 0,
    Pe = 1,
    PowerShell = 2
};

struct AvRecord {
    uint64_t objectSignaturePrefix = 0;
    uint32_t objectSignatureLength = 0;
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
void ClearAntivirusDatabase(AvDatabase* database);

ScanOutcome ScanFilePath(const std::wstring& path, const AvDatabase& database);
ScanOutcome ScanDirectoryPath(const std::wstring& path, const AvDatabase& database);
ScanOutcome ScanFixedDrives(const AvDatabase& database);
