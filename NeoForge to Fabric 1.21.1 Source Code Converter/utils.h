#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <regex>
#include <iostream> 
#include <locale>
#include <codecvt>

namespace fs = std::filesystem;

class Utils {
public:
    // 字符串trim
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, last - first + 1);
    }

    // 读取文件全部内容
    static std::string readFile(const fs::path& path) {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    // 写入文件
    static void writeFile(const fs::path& path, const std::string& content) {
        fs::create_directories(path.parent_path());
        std::ofstream file(path);
        file << content;
    }

    // 替换字符串
    static std::string replace(const std::string& str, const std::string& from, const std::string& to) {
        std::string result = str;
        size_t pos = 0;
        while ((pos = result.find(from, pos)) != std::string::npos) {
            result.replace(pos, from.length(), to);
            pos += to.length();
        }
        return result;
    }

    // 正则替换所有匹配
    static std::string regexReplace(const std::string& str, const std::regex& pattern, const std::string& replacement) {
        return std::regex_replace(str, pattern, replacement);
    }

    // 检查是否以某字符串开头
    static bool startsWith(const std::string& str, const std::string& prefix) {
        return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
    }

    // 提取包名
    static std::string extractPackage(const std::string& javaContent) {
        std::regex packageRegex(R"(package\s+([a-zA-Z_][a-zA-Z0-9_.]*)\s*;)");
        std::smatch match;
        if (std::regex_search(javaContent, match, packageRegex)) {
            return match[1].str();
        }
        return "";
    }

    // 提取类名
    static std::string extractClassName(const std::string& javaContent) {
        std::regex classRegex(R"(public\s+class\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*[{\s])");
        std::smatch match;
        if (std::regex_search(javaContent, match, classRegex)) {
            return match[1].str();
        }
        return "";
    }

    // 移除注释
    static std::string removeComments(const std::string& code) {
        std::string result;
        bool inLineComment = false;
        bool inBlockComment = false;
        bool inString = false;

        for (size_t i = 0; i < code.length(); i++) {
            unsigned char c = static_cast<unsigned char>(code[i]);  // 使用 unsigned char 避免断言

            if (!inString && !inBlockComment && !inLineComment && c == '"') {
                inString = true;
                result += code[i];
            }
            else if (inString && c == '"' && (i == 0 || code[i - 1] != '\\')) {
                inString = false;
                result += code[i];
            }
            else if (!inString && !inBlockComment && !inLineComment && c == '/' && i + 1 < code.length()) {
                unsigned char next = static_cast<unsigned char>(code[i + 1]);
                if (next == '/') {
                    inLineComment = true;
                    i++;
                }
                else if (next == '*') {
                    inBlockComment = true;
                    i++;
                }
                else {
                    result += code[i];
                }
            }
            else if (inLineComment && c == '\n') {
                inLineComment = false;
                result += '\n';
            }
            else if (inBlockComment && c == '*' && i + 1 < code.length()) {
                unsigned char next = static_cast<unsigned char>(code[i + 1]);
                if (next == '/') {
                    inBlockComment = false;
                    i++;
                }
            }
            else if (!inLineComment && !inBlockComment) {
                result += code[i];
            }
        }
        return result;
    }

    static bool isSafeSpace(unsigned char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    // 获取Java文件中的所有导入语句
    static std::vector<std::string> extractImports(const std::string& javaContent) {
        std::vector<std::string> imports;
        std::regex importRegex(R"(import\s+([a-zA-Z0-9_.*]+)\s*;)");
        std::smatch match;
        std::string::const_iterator searchStart(javaContent.cbegin());
        while (std::regex_search(searchStart, javaContent.cend(), match, importRegex)) {
            imports.push_back(match[1].str());
            searchStart = match.suffix().first;
        }
        return imports;
    }

    // 替换导入语句
    static std::string replaceImports(const std::string& javaContent, const std::unordered_map<std::string, std::string>& importMap) {
        std::string result = javaContent;
        for (const auto& [oldImport, newImport] : importMap) {
            std::regex importPattern(R"(import\s+)" + std::regex_replace(oldImport, std::regex(R"(\.)"), R"(\\.)") + R"(\s*;)");
            result = regexReplace(result, importPattern, "import " + newImport + ";");
        }
        return result;
    }

    // 添加Mth类的简单实现（用于替换MCreator的Mth.floor）
    static std::string fixMcreatorMath(const std::string& code) {
        std::string result = code;
        // 替换 Mth.floor(x, y, z) 为 new BlockPos((int)Math.floor(x), (int)Math.floor(y), (int)Math.floor(z))
        std::regex mthFloorPattern(R"(Mth\.floor\(([^,]+),\s*([^,]+),\s*([^)]+)\))");
        result = std::regex_replace(result, mthFloorPattern,
            "new BlockPos((int)Math.floor($1), (int)Math.floor($2), (int)Math.floor($3))");
        return result;
    }
};

#endif