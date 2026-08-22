// ModMigratorCore.h
#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace ModMigratorCore {

    // 元素信息
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

    // 模组信息
    struct ModInfo {
        std::string modId;
        std::string modName;
        std::string modVersion;
        std::string modDescription;
        std::string modAuthor;
        std::string mcVersion;
        std::string modLoader;
        std::vector<std::string> dependencies;
        std::vector<ElementInfo> elements;
        std::map<std::string, std::vector<std::string>> tags;
    };

    // 转换选项
    struct ConvertOptions {
        bool includeSource = true;
        bool exportZip = true;
        bool generateEvents = true;
        bool generateCaps = true;
        bool verbose = true;
    };

    // 回调函数类型
    using LogCallback = std::function<void(const std::string&)>;
    using ProgressCallback = std::function<void(int)>;

    // ========== 工具函数 ==========
    std::string EscapeJSON(const std::string& s);

    // ========== 主转换函数 ==========
    bool Convert(const std::string& sourcePath,
        const std::string& outputPath,
        const ConvertOptions& options,
        LogCallback logCb,
        ProgressCallback progressCb);

    // ========== 解析 mods.toml ==========
    ModInfo ParseModsToml(const std::string& tomlPath);

    // ========== 生成 .mcreator 文件 ==========
    bool GenerateMCreatorFile(const std::string& outputPath,
        const ModInfo& modInfo,
        LogCallback logCb);

    // ========== 复制文件 ==========
    bool CopySourceFiles(const std::string& sourcePath,
        const std::string& outputPath,
        LogCallback logCb);

    bool CopyResourceFiles(const std::string& sourcePath,
        const std::string& outputPath,
        LogCallback logCb);

    // ========== 生成 ZIP ==========
    bool GenerateZip(const std::string& outputPath,
        const std::string& zipName,
        LogCallback logCb);

} // namespace ModMigratorCore