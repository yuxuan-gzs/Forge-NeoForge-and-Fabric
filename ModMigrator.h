// ModMigrator.h
#pragma once

#include "Resource.h"
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace ModMigrator {

    struct Config {
        std::string javaHome;
        std::string libPath;
        bool verbose = true;
        bool keepTempFiles = false;
    };

    struct ConvertOptions {
        bool includeSource = true;
        bool includeResources = true;
        bool exportZip = true;
        bool generateEvents = true;
        bool generateCaps = true;
        bool verbose = true;
    };

    struct ElementInfo {
        std::string name;
        std::string type;
        std::string registryName;
        std::vector<std::string> files;
        std::map<std::string, std::string> metadata;
        bool isBlock = false;
        bool isEvent = false;
        bool isCapability = false;
        bool isRecipe = false;
    };

    struct ModInfo {
        std::string modId;
        std::string modName;
        std::string modVersion;
        std::string modDescription;
        std::string modAuthor;
        std::string mcVersion;
        std::string modLoader;
        std::string generator;
        std::vector<std::string> dependencies;
        std::vector<ElementInfo> elements;
        std::map<std::string, std::vector<std::string>> tags;
        std::map<std::string, std::vector<std::string>> tabElementOrder;      // ✅ 新增
        std::map<std::string, std::map<std::string, std::string>> languageMap; // ✅ 新增
        std::map<std::string, std::vector<std::string>> tagElements;
        std::string sourcePath;
        int totalFiles = 0;
        int errorCount = 0;
        long long parseTimeMs = 0;
        std::vector<std::string> errors;
    };

    using LogCallback = std::function<void(const std::string&)>;
    using ProgressCallback = std::function<void(int)>;

    bool Convert(const std::string& sourcePath,
        const std::string& outputPath,
        const ConvertOptions& options,
        LogCallback logCb,
        ProgressCallback progressCb);

    ModInfo ParseModsToml(const std::string& tomlPath);

    bool RunJavaParser(const std::string& sourcePath,
        const std::string& outputPath,
        const std::string& javaClassPath,
        LogCallback logCb);

    std::vector<ElementInfo> ParseJsonElements(const std::string& jsonPath);

    bool GenerateMCreatorFile(const std::string& outputPath,
        const ModInfo& modInfo,
        LogCallback logCb);

    bool CopySourceFiles(const std::string& sourcePath,
        const std::string& outputPath,
        LogCallback logCb);

    bool GenerateZip(const std::string& sourcePath,
        const std::string& outputPath,
        const std::string& zipName,
        LogCallback logCb);

} // namespace ModMigrator