// JavaParserBridge.cpp
#define _CRT_SECURE_NO_WARNINGS
#include "JavaParserBridge.h"
#include "FileUtils.h"
#include <cstdlib>
#include <sstream>
#include <windows.h>
#include <iostream>

namespace ModMigrator {

    JavaParserBridge::JavaParserBridge() {
        m_javaHome = FindJava();
        m_classPath = GetDefaultClassPath();
        m_javaParserPath = FileUtils::GetExeDirectory();
    }

    JavaParserBridge::JavaParserBridge(const std::string& javaHome) {
        m_javaHome = javaHome;
        m_classPath = GetDefaultClassPath();
        m_javaParserPath = FileUtils::GetExeDirectory();
    }

    void JavaParserBridge::SetJavaHome(const std::string& javaHome) {
        m_javaHome = javaHome;
    }

    void JavaParserBridge::SetClassPath(const std::string& classPath) {
        m_classPath = classPath;
    }

    void JavaParserBridge::AddToClassPath(const std::string& path) {
        if (!m_classPath.empty()) {
            m_classPath += ";";
        }
        m_classPath += path;
    }

    std::string JavaParserBridge::FindJava() {
        const char* javaHome = std::getenv("JAVA_HOME");
        if (javaHome) {
            std::string javaPath = std::string(javaHome) + "\\bin\\java.exe";
            if (FileUtils::FileExists(javaPath)) {
                return javaPath;
            }
        }

        std::vector<std::string> paths = {
            "java.exe",
            "C:\\Program Files\\Java\\jdk-17\\bin\\java.exe",
            "C:\\Program Files\\Java\\jdk-11\\bin\\java.exe",
            "C:\\Program Files\\Java\\jre1.8.0\\bin\\java.exe"
        };

        for (const auto& path : paths) {
            if (FileUtils::FileExists(path)) {
                return path;
            }
        }

        char* pathEnv = std::getenv("PATH");
        if (pathEnv) {
            std::string pathStr(pathEnv);
            size_t pos = 0;
            while ((pos = pathStr.find(';')) != std::string::npos) {
                std::string dir = pathStr.substr(0, pos);
                pathStr.erase(0, pos + 1);
                std::string javaPath = dir + "\\java.exe";
                if (FileUtils::FileExists(javaPath)) {
                    return javaPath;
                }
            }
        }

        return "";
    }

    std::string JavaParserBridge::GetDefaultClassPath() {
        std::string exeDir = FileUtils::GetExeDirectory();
        std::string libDir = exeDir + "\\lib";

        std::vector<std::string> jars = {
            "javaparser-core-3.26.2.jar",
            "antlr-4.13.1-complete.jar",
            "javaparser-symbol-solver-core-3.26.2.jar",
            "gson-2.11.0.jar"
        };

        std::string classPath;
        for (const auto& jar : jars) {
            std::string jarPath = libDir + "\\" + jar;
            if (FileUtils::FileExists(jarPath)) {
                if (!classPath.empty()) {
                    classPath += ";";
                }
                classPath += jarPath;
            }
        }

        // 添加 java 目录（存放 ModParserANTLR.class）
        if (!classPath.empty()) {
            classPath += ";";
        }
        classPath += exeDir + "\\java";

        return classPath;
    }

    bool JavaParserBridge::IsJavaAvailable() {
        std::string java = FindJava();
        return !java.empty();
    }

    std::string JavaParserBridge::GetJavaVersion() {
        std::string java = FindJava();
        if (java.empty()) {
            return "";
        }

        std::string command = "\"" + java + "\" -version 2>&1";
        std::string output, error;
        ExecuteCommand(command, output, error, nullptr);

        std::string version = output.empty() ? error : output;
        size_t pos = version.find("version");
        if (pos != std::string::npos) {
            pos = version.find("\"", pos);
            if (pos != std::string::npos) {
                size_t end = version.find("\"", pos + 1);
                if (end != std::string::npos) {
                    return version.substr(pos + 1, end - pos - 1);
                }
            }
        }
        return "unknown";
    }

    std::string JavaParserBridge::BuildCommand(const std::string& sourcePath,
        const std::string& outputPath) {
        std::stringstream cmd;
        cmd << "\"" << m_javaHome << "\"";
        cmd << " -cp \"" << m_classPath << "\"";
        cmd << " ModParserANTLR";
        cmd << " \"" << sourcePath << "\"";
        cmd << " \"" << outputPath << "\"";
        return cmd.str();
    }

    bool JavaParserBridge::ExecuteCommand(const std::string& command,
        std::string& output,
        std::string& error,
        std::function<void(const std::string&)> logCb) {
        SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
        HANDLE hStdOutRead, hStdOutWrite;
        HANDLE hStdErrRead, hStdErrWrite;

        if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
            return false;
        }
        if (!CreatePipe(&hStdErrRead, &hStdErrWrite, &sa, 0)) {
            CloseHandle(hStdOutRead);
            CloseHandle(hStdOutWrite);
            return false;
        }

        PROCESS_INFORMATION pi;
        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hStdOutWrite;
        si.hStdError = hStdErrWrite;

        std::string cmdLine = command;
        BOOL result = CreateProcessA(
            NULL,
            (LPSTR)cmdLine.c_str(),
            NULL, NULL, TRUE, 0, NULL, NULL,
            &si, &pi
        );

        CloseHandle(hStdOutWrite);
        CloseHandle(hStdErrWrite);

        if (!result) {
            CloseHandle(hStdOutRead);
            CloseHandle(hStdErrRead);
            return false;
        }

        // 实时读取输出
        char buffer[4096];
        DWORD bytesRead;
        bool processRunning = true;

        while (processRunning) {
            DWORD exitCode;
            if (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                processRunning = false;
            }

            // 读取标准输出
            while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string text(buffer);
                output += text;
                if (logCb) {
                    std::stringstream ss(text);
                    std::string line;
                    while (std::getline(ss, line)) {
                        if (!line.empty()) {
                            logCb("[JAVA] " + line);
                        }
                    }
                }
            }

            // 读取标准错误
            while (ReadFile(hStdErrRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string text(buffer);
                error += text;
                if (logCb) {
                    std::stringstream ss(text);
                    std::string line;
                    while (std::getline(ss, line)) {
                        if (!line.empty()) {
                            logCb("[JAVA-ERR] " + line);
                        }
                    }
                }
            }

            if (processRunning) {
                Sleep(50);
            }
        }

        // 最后再读取一次残留数据
        while (ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
            if (logCb) {
                logCb("[JAVA] " + std::string(buffer));
            }
        }
        while (ReadFile(hStdErrRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            error += buffer;
            if (logCb) {
                logCb("[JAVA-ERR] " + std::string(buffer));
            }
        }

        CloseHandle(hStdOutRead);
        CloseHandle(hStdErrRead);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        return true;
    }

    bool JavaParserBridge::ParseMod(const std::string& sourcePath,
        const std::string& outputPath,
        std::string& output,
        std::string& error,
        std::function<void(const std::string&)> logCb) {
        if (m_javaHome.empty()) {
            error = "Java not found. Please install Java 8 or higher.";
            if (logCb) logCb("[ERROR] " + error);
            return false;
        }

        std::string command = BuildCommand(sourcePath, outputPath);

        if (logCb) {
            logCb("[INFO] Running: " + command);
        }

        bool result = ExecuteCommand(command, output, error, logCb);

        if (!result && logCb) {
            logCb("[ERROR] Java process failed");
        }

        return result;
    }

    std::string JavaParserBridge::GetExeDirectory() {
        return FileUtils::GetExeDirectory();
    }

} // namespace ModMigrator