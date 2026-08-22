// JavaParserBridge.h
#pragma once

#include <string>
#include <vector>
#include <functional>

#pragma comment(lib,"comctl32.lib")


namespace ModMigrator {

    // Java 解析器桥接
    class JavaParserBridge {
    public:
        // 构造函数
        JavaParserBridge();
        explicit JavaParserBridge(const std::string& javaHome);

        // 设置 Java 路径
        void SetJavaHome(const std::string& javaHome);

        // 设置类路径
        void SetClassPath(const std::string& classPath);
        void AddToClassPath(const std::string& path);

        // 运行解析器
        bool ParseMod(const std::string& sourcePath,
            const std::string& outputPath,
            std::string& output,
            std::string& error,
            std::function<void(const std::string&)> logCb = nullptr);

        // 检查 Java 是否可用
        static bool IsJavaAvailable();

        // 获取 Java 版本
        std::string GetJavaVersion();

        // 获取可执行文件目录
        static std::string GetExeDirectory();

        // 获取默认类路径
        static std::string GetDefaultClassPath();

    private:
        std::string m_javaHome;
        std::string m_classPath;
        std::string m_javaParserPath;

        // 构建 Java 命令
        std::string BuildCommand(const std::string& sourcePath,
            const std::string& outputPath);

        // 执行命令
        bool ExecuteCommand(const std::string& command,
            std::string& output,
            std::string& error,
            std::function<void(const std::string&)> logCb = nullptr);

        // 查找 Java
        static std::string FindJava();


        // 查找 jar 文件
        static std::string FindJar(const std::string& name);
    };

} // namespace ModMigrator
