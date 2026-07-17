#ifndef FILE_SCANNER_H
#define FILE_SCANNER_H

#include "utils.h"
#include "SymbolTable.h"
#include <regex>
#include <set>

class FileScanner {
private:
    SymbolTable* symbolTable;
    std::string sourceRoot;
    std::string modid;
    std::string mainClass;

    // 从mods.toml读取modid
    std::string findModIdFromToml(const fs::path& tomlPath) {
        if (!fs::exists(tomlPath)) return "";

        std::string content = Utils::readFile(tomlPath);
        std::regex modIdRegex(R"(modId\s*=\s*\"([^\"]+)\")");
        std::smatch match;
        if (std::regex_search(content, match, modIdRegex)) {
            return match[1].str();
        }
        return "";
    }

    // 从@Mod注解查找modid
    std::string findModIdFromAnnotation(const fs::path& javaFile) {
        std::string content = Utils::readFile(javaFile);
        std::regex modAnnotationRegex(R"(@Mod\s*\(\s*\"([^\"]+)\"\s*\))");
        std::smatch match;
        if (std::regex_search(content, match, modAnnotationRegex)) {
            return match[1].str();
        }
        return "";
    }

    // 查找主类（带@Mod注解的类）
    std::string findMainClass(const fs::path& javaFile) {
        std::string content = Utils::readFile(javaFile);
        if (content.find("@Mod") != std::string::npos) {
            std::regex classRegex(R"(public\s+class\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*[\{])");
            std::smatch match;
            if (std::regex_search(content, match, classRegex)) {
                return match[1].str();
            }
        }
        return "";
    }

public:
    FileScanner(SymbolTable* st, const std::string& root)
        : symbolTable(st), sourceRoot(root), modid(""), mainClass("") {
    }

    void scan() {
        // 1. 在src/main/java和src/main/resources中搜索
        std::vector<fs::path> tomlPaths;
        std::vector<fs::path> javaPaths;

        // 递归搜索
        std::function<void(const fs::path&)> searchDir = [&](const fs::path& dir) {
            if (!fs::exists(dir)) return;
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    if (entry.path().extension() == ".toml" &&
                        entry.path().filename().string().find("mods.toml") != std::string::npos) {
                        tomlPaths.push_back(entry.path());
                    }
                    else if (entry.path().extension() == ".java") {
                        javaPaths.push_back(entry.path());
                    }
                }
            }
            };

        // 搜索标准Maven目录结构
        searchDir(sourceRoot + "/src/main/resources");
        searchDir(sourceRoot + "/src/main/java");

        // 从toml查找
        for (const auto& tomlPath : tomlPaths) {
            modid = findModIdFromToml(tomlPath);
            if (!modid.empty()) break;
        }

        // 从注解查找
        if (modid.empty()) {
            for (const auto& javaPath : javaPaths) {
                modid = findModIdFromAnnotation(javaPath);
                if (!modid.empty()) {
                    mainClass = findMainClass(javaPath);
                    break;
                }
            }
        }

        // 如果还没找到，使用文件夹名作为fallback
        if (modid.empty()) {
            modid = fs::path(sourceRoot).filename().string();
            // 清理modid（只保留字母数字和下划线）
            std::regex invalidChars(R"([^a-zA-Z0-9_])");
            modid = std::regex_replace(modid, invalidChars, "_");
        }

        // 记录到符号表
        ModInfo info;
        info.modid = modid;
        info.mainClass = mainClass;
        symbolTable->registerMod(modid, info);
    }

    std::string getModId() const { return modid; }
    std::string getMainClass() const { return mainClass; }

    // 获取所有需要转换的Java文件
    std::vector<fs::path> getJavaFiles() {
        std::vector<fs::path> javaFiles;
        std::function<void(const fs::path&)> collect = [&](const fs::path& dir) {
            if (!fs::exists(dir)) return;
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file() && entry.path().extension() == ".java") {
                    javaFiles.push_back(entry.path());
                }
            }
            };
        collect(sourceRoot + "/src/main/java");
        return javaFiles;
    }

    // 获取资源文件
    std::vector<fs::path> getResourceFiles() {
        std::vector<fs::path> resourceFiles;
        std::function<void(const fs::path&)> collect = [&](const fs::path& dir) {
            if (!fs::exists(dir)) return;
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    std::string ext = entry.path().extension().string();
                    if (ext == ".json" || ext == ".mcmeta" || ext == ".png" || ext == ".ogg") {
                        resourceFiles.push_back(entry.path());
                    }
                }
            }
            };
        collect(sourceRoot + "/src/main/resources");
        return resourceFiles;
    }
};

#endif
