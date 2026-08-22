// ModMigratorCore.cpp
#include "ModMigratorCore.h"
#include "FileUtils.h"
#include "JsonParser.h"
#include "JavaParserBridge.h"
#include <sstream>
#include <regex>
#include <cstdlib>
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <filesystem>

#pragma comment(lib,"comctl32.lib")

namespace fs = std::filesystem;

using namespace ModMigrator;

namespace ModMigratorCore {

    std::string EscapeJSON(const std::string& s) {
        std::string result;
        for (char c : s) {
            switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
            }
        }
        return result;
    }

    ModInfo ParseModsToml(const std::string& tomlPath) {
        ModInfo info;
        std::string content = FileUtils::ReadFile(tomlPath);
        if (content.empty()) return info;

        std::regex idRegex(R"(modId\s*=\s*\"([^\"]+)\")");
        std::regex nameRegex(R"(displayName\s*=\s*\"([^\"]+)\")");
        std::regex versionRegex(R"(version\s*=\s*\"([^\"]+)\")");
        std::regex descRegex(R"(description\s*=\s*\"([^\"]+)\")");
        std::regex authorRegex(R"(authors\s*=\s*\"([^\"]+)\")");
        std::regex mcRegex(R"(mcversion\s*=\s*\"([^\"]+)\")");
        std::regex loaderRegex(R"(modLoader\s*=\s*\"([^\"]+)\")");

        std::smatch match;
        if (std::regex_search(content, match, idRegex) && match.size() > 1) {
            info.modId = match[1].str();
        }
        if (std::regex_search(content, match, nameRegex) && match.size() > 1) {
            info.modName = match[1].str();
        }
        if (std::regex_search(content, match, versionRegex) && match.size() > 1) {
            info.modVersion = match[1].str();
        }
        if (std::regex_search(content, match, descRegex) && match.size() > 1) {
            info.modDescription = match[1].str();
        }
        if (std::regex_search(content, match, authorRegex) && match.size() > 1) {
            info.modAuthor = match[1].str();
        }
        if (std::regex_search(content, match, mcRegex) && match.size() > 1) {
            info.mcVersion = match[1].str();
        }
        if (std::regex_search(content, match, loaderRegex) && match.size() > 1) {
            info.modLoader = match[1].str();
            if (info.modLoader.find("lowcodefml") != std::string::npos) {
                info.modLoader = "neoforge";
            }
            else if (info.modLoader.find("javafml") != std::string::npos) {
                info.modLoader = "forge";
            }
        }

        return info;
    }

    bool GenerateMCreatorFile(const std::string& outputPath,
        const ModInfo& modInfo,
        LogCallback logCb) {
        std::stringstream json;
        json << "{\n";
        json << "  \"_fv\": 2,\n";
        json << "  \"mod_elements\": [\n";

        bool first = true;
        for (const auto& elem : modInfo.elements) {
            if (!first) json << ",\n";
            first = false;

            std::string safeName = elem.registryName;
            std::replace(safeName.begin(), safeName.end(), ':', '_');

            json << "    {\n";
            json << "      \"name\": \"" << EscapeJSON(elem.name) << "\",\n";
            json << "      \"type\": \"" << elem.type << "\",\n";
            json << "      \"compiles\": true,\n";
            json << "      \"locked_code\": false,\n";
            json << "      \"registry_name\": \"" << EscapeJSON(safeName) << "\",\n";
            json << "      \"metadata\": {\n";
            json << "        \"files\": [\n";

            for (size_t i = 0; i < elem.files.size(); i++) {
                std::string relPath = elem.files[i];
                std::replace(relPath.begin(), relPath.end(), '\\', '/');
                json << "          \"" << EscapeJSON(relPath) << "\"";
                if (i < elem.files.size() - 1) json << ",";
                json << "\n";
            }
            json << "        ]\n";
            json << "      }\n";
            json << "    }";
        }

        json << "\n  ],\n";
        json << "  \"variable_elements\": [],\n";
        json << "  \"sound_elements\": [],\n";
        json << "  \"tag_elements\": {},\n";
        json << "  \"tab_element_order\": {},\n";
        json << "  \"language_map\": {\n";
        json << "    \"en_us\": {\n";
        json << "      \"item_group." << EscapeJSON(modInfo.modId) << "." << EscapeJSON(modInfo.modId) << "\": \"" << EscapeJSON(modInfo.modName) << "\"\n";
        json << "    }\n";
        json << "  },\n";
        json << "  \"metadata\": {\n";
        json << "    \"files\": []\n";
        json << "  },\n";
        json << "  \"foldersRoot\": {\n";
        json << "    \"name\": \"~\",\n";
        json << "    \"children\": []\n";
        json << "  },\n";
        json << "  \"workspaceSettings\": {\n";
        json << "    \"modid\": \"" << EscapeJSON(modInfo.modId) << "\",\n";
        json << "    \"modName\": \"" << EscapeJSON(modInfo.modName) << "\",\n";
        json << "    \"version\": \"" << EscapeJSON(modInfo.modVersion) << "\",\n";
        json << "    \"description\": \"" << EscapeJSON(modInfo.modDescription) << "\",\n";
        json << "    \"author\": \"" << EscapeJSON(modInfo.modAuthor) << "\",\n";
        json << "    \"websiteURL\": \"\",\n";
        json << "    \"license\": \"MIT\",\n";
        json << "    \"serverSideOnly\": false,\n";
        json << "    \"modPicture\": \"\",\n";
        json << "    \"requiredMods\": [\n";

        for (size_t i = 0; i < modInfo.dependencies.size(); i++) {
            json << "      \"" << EscapeJSON(modInfo.dependencies[i]) << "\"";
            if (i < modInfo.dependencies.size() - 1) json << ",";
            json << "\n";
        }
        json << "    ],\n";
        json << "    \"dependencies\": [\n";
        for (size_t i = 0; i < modInfo.dependencies.size(); i++) {
            json << "      \"" << EscapeJSON(modInfo.dependencies[i]) << "\"";
            if (i < modInfo.dependencies.size() - 1) json << ",";
            json << "\n";
        }
        json << "    ],\n";
        json << "    \"dependants\": [],\n";
        json << "    \"mcreatorDependencies\": [],\n";
        json << "    \"currentGenerator\": \"" << EscapeJSON(modInfo.modLoader) << "-" << EscapeJSON(modInfo.mcVersion) << "\",\n";
        json << "    \"credits\": \"Converted using ModMigrator v1.0\",\n";
        json << "    \"modElementsPackage\": \"net.mcreator." << EscapeJSON(modInfo.modId) << "\"\n";
        json << "  },\n";
        json << "  \"mcreatorVersion\": 202600229418\n";
        json << "}\n";

        std::string outputFile = outputPath + "/" + modInfo.modId + ".mcreator";
        return FileUtils::WriteFile(outputFile, json.str());
    }

    bool CopySourceFiles(const std::string& sourcePath,
        const std::string& outputPath,
        LogCallback logCb) {
        std::string srcJavaDir = sourcePath + "/src/main/java";
        std::string dstJavaDir = outputPath + "/src/main/java";

        if (FileUtils::DirectoryExists(srcJavaDir)) {
            FileUtils::CreateDirectories(dstJavaDir);
            return FileUtils::CopyDirectory(srcJavaDir, dstJavaDir);
        }
        return false;
    }

    bool CopyResourceFiles(const std::string& sourcePath,
        const std::string& outputPath,
        LogCallback logCb) {
        std::string srcResDir = sourcePath + "/src/main/resources";
        std::string dstResDir = outputPath + "/src/main/resources";

        if (FileUtils::DirectoryExists(srcResDir)) {
            FileUtils::CreateDirectories(dstResDir);
            return FileUtils::CopyDirectory(srcResDir, dstResDir);
        }
        return false;
    }

    bool GenerateZip(const std::string& outputPath,
        const std::string& zipName,
        LogCallback logCb) {
        std::string zipPath = outputPath + "/" + zipName;
        std::string psCommand = "powershell -Command \"Compress-Archive -Path '" + outputPath + "\\*' -DestinationPath '" + zipPath + "' -Force\"";
        int result = std::system(psCommand.c_str());
        return result == 0;
    }

    bool Convert(const std::string& sourcePath,
        const std::string& outputPath,
        const ConvertOptions& options,
        LogCallback logCb,
        ProgressCallback progressCb) {

        if (logCb) {
            logCb("[INFO] ========================================");
            logCb("[INFO] ModMigrator v1.0");
            logCb("[INFO] ========================================");
            logCb("[INFO] Source: " + sourcePath);
            logCb("[INFO] Output: " + outputPath);
        }

        if (progressCb) progressCb(5);

        // 1. 检查 Java
        if (!JavaParserBridge::IsJavaAvailable()) {
            if (logCb) logCb("[ERROR] Java not found. Please install Java 8+");
            return false;
        }

        // 2. 读取 mods.toml
        if (logCb) logCb("[INFO] Reading mods.toml...");
        ModInfo modInfo;
        std::string tomlPath = sourcePath + "/src/main/resources/META-INF/mods.toml";
        if (FileUtils::FileExists(tomlPath)) {
            modInfo = ParseModsToml(tomlPath);
            if (logCb) {
                logCb("[INFO] Mod ID: " + modInfo.modId);
                logCb("[INFO] Mod Name: " + modInfo.modName);
            }
        }
        else {
            if (logCb) logCb("[WARN] mods.toml not found");
            modInfo.modId = "converted_mod";
            modInfo.modName = "Converted Mod";
            modInfo.modVersion = "1.0.0";
            modInfo.modLoader = "forge";
            modInfo.mcVersion = "1.20.1";
        }

        if (progressCb) progressCb(10);

        // 3. 运行 Java 解析器
        if (logCb) logCb("[INFO] Running Java parser...");
        JavaParserBridge bridge;
        std::string javaOutput, javaError;
        std::string jsonPath = outputPath + "/parsed_result.json";

        FileUtils::CreateDirectories(outputPath);

        bool success = bridge.ParseMod(sourcePath, jsonPath, javaOutput, javaError, logCb);

        if (!success) {
            if (logCb) logCb("[ERROR] Java parser failed: " + javaError);
            return false;
        }

        if (progressCb) progressCb(40);

        // 4. 解析 JSON
        if (logCb) logCb("[INFO] Parsing JSON results...");
        std::string jsonContent = FileUtils::ReadFile(jsonPath);
        if (jsonContent.empty()) {
            if (logCb) logCb("[ERROR] Failed to read JSON file");
            return false;
        }

        auto elements = JsonParser::ParseElements(jsonContent);
        if (logCb) logCb("[INFO] Found " + std::to_string(elements.size()) + " elements");

        std::string jsonModId = JsonParser::ExtractString(jsonContent, "modId");
        if (!jsonModId.empty() && jsonModId != "converted_mod") {
            modInfo.modId = jsonModId;
        }
        std::string jsonModName = JsonParser::ExtractString(jsonContent, "modName");
        if (!jsonModName.empty()) {
            modInfo.modName = jsonModName;
        }

        modInfo.elements.clear();
        for (const auto& e : elements) {
            ModMigratorCore::ElementInfo elem;
            elem.name = e.name;
            elem.type = e.type;
            elem.registryName = e.registryName;
            elem.files = e.files;
            elem.metadata = e.metadata;
            elem.isBlock = e.isBlock;
            elem.isEvent = e.isEvent;
            elem.isCapability = e.isCapability;
            elem.isRecipe = e.isRecipe;
            modInfo.elements.push_back(elem);
        }



        if (progressCb) progressCb(60);

        // 5. 生成 .mcreator
        if (logCb) logCb("[INFO] Generating .mcreator file...");
        success = GenerateMCreatorFile(outputPath, modInfo, logCb);
        if (!success) {
            if (logCb) logCb("[ERROR] Failed to generate .mcreator");
            return false;
        }

        if (progressCb) progressCb(75);

        // 6. 复制源文件
        if (options.includeSource) {
            if (logCb) logCb("[INFO] Copying source files...");
            CopySourceFiles(sourcePath, outputPath, logCb);
            CopyResourceFiles(sourcePath, outputPath, logCb);
        }

        if (progressCb) progressCb(88);

        // 7. 生成 ZIP
        if (options.exportZip) {
            if (logCb) logCb("[INFO] Generating ZIP...");
            std::string zipName = modInfo.modId + "_" + modInfo.modVersion + ".zip";
            GenerateZip(outputPath, zipName, logCb);
        }

        if (progressCb) progressCb(100);

        if (logCb) {
            logCb("[INFO] ========================================");
            logCb("[INFO] Conversion complete!");
            logCb("[INFO] Output: " + outputPath);
            logCb("[INFO] Mod ID: " + modInfo.modId);
            logCb("[INFO] Elements: " + std::to_string(modInfo.elements.size()));
        }

        return true;
    }

} // namespace ModMigratorCore