// FileUtils.h
#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

namespace ModMigrator {

    // 文件操作工具
    class FileUtils {
    public:
        // 读取文件内容
        static std::string ReadFile(const std::string& path);

        // 写入文件
        static bool WriteFile(const std::string& path, const std::string& content);

        // 检查文件是否存在
        static bool FileExists(const std::string& path);

        // 检查目录是否存在
        static bool DirectoryExists(const std::string& path);

        // 创建目录
        static bool CreateDirectory(const std::string& path);

        // 创建多级目录
        static bool CreateDirectories(const std::string& path);

        // 递归复制目录
        static bool CopyDirectory(const std::string& src, const std::string& dst);

        // 递归删除目录
        static bool DeleteDirectory(const std::string& path);

        // 获取文件大小
        static size_t GetFileSize(const std::string& path);

        // 获取文件名
        static std::string GetFileName(const std::string& path);

        // 获取文件扩展名
        static std::string GetFileExtension(const std::string& path);

        // 获取目录路径
        static std::string GetDirectoryPath(const std::string& path);

        // 获取当前工作目录
        static std::string GetCurrentDirectory();

        // 获取可执行文件目录
        static std::string GetExeDirectory();

        // 列出目录下的所有文件
        static std::vector<std::string> ListFiles(const std::string& path,
            const std::string& extension = "");

        // 递归列出所有文件
        static std::vector<std::string> ListFilesRecursive(const std::string& path,
            const std::string& extension = "");

        // 获取临时目录
        static std::string GetTempDirectory();

        // 创建临时文件
        static std::string CreateTempFile(const std::string& prefix = "");

        // 创建临时目录
        static std::string CreateTempDirectory(const std::string& prefix = "");
    };

} // namespace ModMigrator
