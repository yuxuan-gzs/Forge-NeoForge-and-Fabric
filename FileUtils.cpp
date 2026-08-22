// FileUtils.cpp
#define _CRT_SECURE_NO_WARNINGS
#include "FileUtils.h"
#include <windows.h>
#undef GetCurrentDirectory
#undef CreateDirectory
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <filesystem>

#pragma comment(lib,"comctl32.lib")


namespace fs = std::filesystem;


namespace ModMigrator {

    std::string FileUtils::ReadFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    bool FileUtils::WriteFile(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << content;
        return true;
    }

    bool FileUtils::FileExists(const std::string& path) {
        DWORD attrs = GetFileAttributesA(path.c_str());
        return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool FileUtils::DirectoryExists(const std::string& path) {
        DWORD attrs = GetFileAttributesA(path.c_str());
        return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool FileUtils::CreateDirectory(const std::string& path) {
        return CreateDirectoryA(path.c_str(), NULL) != 0;
    }

    bool FileUtils::CreateDirectories(const std::string& path) {
        try {
            fs::create_directories(path);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool FileUtils::CopyDirectory(const std::string& src, const std::string& dst) {
        try {
            fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    bool FileUtils::DeleteDirectory(const std::string& path) {
        try {
            fs::remove_all(path);
            return true;
        }
        catch (...) {
            return false;
        }
    }

    size_t FileUtils::GetFileSize(const std::string& path) {
        try {
            return fs::file_size(path);
        }
        catch (...) {
            return 0;
        }
    }

    std::string FileUtils::GetFileName(const std::string& path) {
        return fs::path(path).filename().string();
    }

    std::string FileUtils::GetFileExtension(const std::string& path) {
        return fs::path(path).extension().string();
    }

    std::string FileUtils::GetDirectoryPath(const std::string& path) {
        return fs::path(path).parent_path().string();
    }

    // 补齐 GetCurrentDirectory - 使用 Windows API
    std::string FileUtils::GetCurrentDirectory() {
        char buffer[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, buffer);
        return std::string(buffer);
    }

    // 补齐 GetExeDirectory
    std::string FileUtils::GetExeDirectory() {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        std::string exePath(path);
        size_t pos = exePath.find_last_of("\\/");
        if (pos != std::string::npos) {
            return exePath.substr(0, pos);
        }
        return ".";
    }

    std::vector<std::string> FileUtils::ListFiles(const std::string& path, const std::string& extension) {
        std::vector<std::string> files;
        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    if (extension.empty() || entry.path().extension() == extension) {
                        files.push_back(entry.path().string());
                    }
                }
            }
        }
        catch (...) {}
        return files;
    }

    std::vector<std::string> FileUtils::ListFilesRecursive(const std::string& path, const std::string& extension) {
        std::vector<std::string> files;
        try {
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    if (extension.empty() || entry.path().extension() == extension) {
                        files.push_back(entry.path().string());
                    }
                }
            }
        }
        catch (...) {}
        return files;
    }

    std::string FileUtils::GetTempDirectory() {
        char path[MAX_PATH];
        GetTempPathA(MAX_PATH, path);
        return std::string(path);
    }

    std::string FileUtils::CreateTempFile(const std::string& prefix) {
        char path[MAX_PATH];
        UINT unique = 0;
        GetTempFileNameA(GetTempDirectory().c_str(), prefix.c_str(), unique, path);
        return std::string(path);
    }

    std::string FileUtils::CreateTempDirectory(const std::string& prefix) {
        std::string tempPath = GetTempDirectory() + prefix + "_" + std::to_string(time(nullptr));
        CreateDirectory(tempPath);
        return tempPath;
    }

} // namespace ModMigrator