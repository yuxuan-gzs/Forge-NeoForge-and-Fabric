#include <iostream>
#include <limits>
#include <string>
#include <cstdio>
#include <cstring>
#include <functional>
#include <cstdlib>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <shlobj.h>
#include <thread>
#include <atomic>
#include <random>
#include <direct.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <aclapi.h>
#include <sddl.h>

// C++
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")

#define COLOR_WHITE  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
#define COLOR_GREEN  FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define COLOR_RED    FOREGROUND_RED | FOREGROUND_INTENSITY
#define COLOR_YELLOW FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define COLOR_CYAN   FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY


using namespace std;  // 注意：保留以便代码中未加 std:: 前缀的标识符仍然有效

// ==================== 全局变量 ====================
std::atomic<bool> spinnerRunning(false);
std::thread spinnerThread;
const std::string PASSWORD = "sr5r4g464dry8k78665sglnjtr4m5z578tk78q";
const std::string PIPE_KEY = "LightPipeKey2024!@#";
const std::string SERVICE_NAME = "LightService";
const std::string SERVICE_DISPLAY = "Light Service";

// ==================== 函数声明（解决找不到标识符问题） ====================
void CheckAndInstallService();
bool StartLightService();
void StopLightService();
bool SendToService(const std::string& command, std::string& response);
bool ElevateThroughService(const std::string& exePath);
bool LaunchProgram(const std::string& exePath, bool waitForCompletion);
bool Lock_RestoreFolderRecursively(const std::string& _wjj);
bool LockFolderWithSYSTEM(const std::string& folderPath);
bool Lock_RestoreFolderPermission(const std::string& _wjj);
bool EnableSecurityPrivileges();
bool IsRunningAsAdmin();
bool FileExists(const std::string& filePath);
std::string GetCurrentProgramPath();
std::string GetProgramDir();
std::string GetParentDir(const std::string& dir);
void StartSpinner();
void StopSpinner();
void RunWithUACAndExit(const char* exePath);
void help();
void about();

// ==================== 权限管理 ====================
bool EnablePrivilege(const std::string& privilegeName) {
    HANDLE hToken = NULL;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    // 打开进程令牌
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    // 获取权限的 LUID
    if (!LookupPrivilegeValueA(NULL, privilegeName.c_str(), &luid)) {
        CloseHandle(hToken);
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // 启用权限
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL)) {
        CloseHandle(hToken);
        return false;
    }

    // 检查是否成功
    bool success = (GetLastError() != ERROR_NOT_ALL_ASSIGNED);
    CloseHandle(hToken);
    return success;
}

// 启用所有权和备份权限（修改文件夹权限需要）
bool EnableSecurityPrivileges() {
    bool allSuccess = true;
    const char* privileges[] = {
        "SeTakeOwnershipPrivilege",  // 允许取得文件所有权
        "SeSecurityPrivilege",       // 允许修改安全设置
        "SeBackupPrivilege",         // 允许备份文件（绕过权限检查）
        "SeRestorePrivilege"         // 允许恢复文件（绕过权限检查）
    };

    for (int i = 0; i < 4; i++) {
        if (!EnablePrivilege(privileges[i])) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

// ==================== 工具函数 ====================
bool FileExists(const std::string& filePath) {
    DWORD dwAttrib = GetFileAttributesA(filePath.c_str());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroupSid = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroupSid)) {
        CheckTokenMembership(NULL, adminGroupSid, &isAdmin);
        FreeSid(adminGroupSid);
    }
    return isAdmin == TRUE;
}

string GetCurrentProgramPath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string fullPath(buffer);
    size_t lastSlash = fullPath.find_last_of("\\/");
    return (lastSlash != std::string::npos) ? fullPath.substr(0, lastSlash) : fullPath;
}

std::string GetProgramDir() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string fullPath(buffer);
    size_t lastSlash = fullPath.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        return fullPath.substr(0, lastSlash);
    }
    return fullPath;
}

std::string GetParentDir(const std::string& dir) {
    size_t lastSlash = dir.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        return dir.substr(0, lastSlash);
    }
    return dir;
}

// ==================== 转圈动画 ====================
void SpinnerAnimation() {
    const char chars[] = { '/', '|', '\\', '-' };
    int i = 0;
    while (spinnerRunning) {
        std::cout << "\r" << chars[i % 4] << " ..." << std::flush;
        i++;
        Sleep(100);
    }
    // 清除动画行
    std::cout << "\r                \r" << std::flush;
}

void StartSpinner() {
    if (!spinnerRunning) {
        spinnerRunning = true;
        spinnerThread = std::thread(SpinnerAnimation);
    }
}

void StopSpinner() {
    spinnerRunning = false;
    if (spinnerThread.joinable()) {
        spinnerThread.join();
    }
}

// ==================== 控制台控制 ====================
BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    if (fdwCtrlType == CTRL_C_EVENT) {
        printf("收到 Ctrl+C，已忽略。\n");
        return TRUE;
    }
    return FALSE;
}

// ==================== 提权相关 ====================
void RunWithUACAndExit(const char* exePath) {
    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.lpVerb = "runas";
    sei.lpFile = exePath;
    sei.nShow = SW_SHOWNORMAL;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    if (ShellExecuteExA(&sei) && sei.hProcess) {
        WaitForInputIdle(sei.hProcess, 5000);
        CloseHandle(sei.hProcess);
    }
    ExitProcess(0);
}

// ==================== 服务管理 ====================
bool IsServiceInstalled() {
    SC_HANDLE hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager) return false;

    SC_HANDLE hService = OpenServiceA(hSCManager, SERVICE_NAME.c_str(), SERVICE_QUERY_STATUS);
    if (hService) {
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return true;
    }
    CloseServiceHandle(hSCManager);
    return false;
}

bool InstallService(const std::string& servicePath) {
    if (!IsRunningAsAdmin()) {
        return false;
    }

    SC_HANDLE hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager) return false;

    SC_HANDLE hService = CreateServiceA(
        hSCManager,
        SERVICE_NAME.c_str(),
        SERVICE_DISPLAY.c_str(),
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        servicePath.c_str(),
        NULL, NULL, NULL, NULL, NULL);

    bool success = (hService != NULL);
    if (success) {
        StartServiceA(hService, 0, NULL);
        CloseServiceHandle(hService);
    }
    CloseServiceHandle(hSCManager);
    return success;
}

void CheckAndInstallService() {
    // 先检查
    if (IsServiceInstalled()) return;

    // 没有管理员权限，提权重启自己
    if (!IsRunningAsAdmin()) {
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        RunWithUACAndExit(exePath);
    }

    // 有管理员权限，直接安装
    std::string dir = GetCurrentProgramPath();
    std::string servicePath = dir + "\\rppcst\\yuxuangzs-Service-light.exe";

    if (!FileExists(servicePath)) {
        std::cout << "服务程序不存在: " << servicePath << std::endl;
        return;
    }

    InstallService(servicePath);
}

// 启动服务
bool StartLightService() {
    SC_HANDLE hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
    if (!hSCManager) return false;

    SC_HANDLE hService = OpenServiceA(hSCManager, SERVICE_NAME.c_str(),
        SERVICE_START | SERVICE_QUERY_STATUS);
    if (!hService) {
        CloseServiceHandle(hSCManager);
        return false;
    }

    SERVICE_STATUS status;
    BOOL result = StartServiceA(hService, 0, NULL);
    DWORD err = GetLastError();

    if (!result && err != ERROR_SERVICE_ALREADY_RUNNING) {
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return false;
    }

    // 等待服务启动
    for (int i = 0; i < 10; i++) {
        Sleep(300);
        if (QueryServiceStatus(hService, &status) &&
            status.dwCurrentState == SERVICE_RUNNING) {
            break;
        }
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return true;
}

// 停止服务
void StopLightService() {
    SC_HANDLE hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager) return;

    SC_HANDLE hService = OpenServiceA(hSCManager, SERVICE_NAME.c_str(), SERVICE_ALL_ACCESS);
    if (hService) {
        SERVICE_STATUS status;
        ControlService(hService, SERVICE_CONTROL_STOP, &status);
        CloseServiceHandle(hService);
    }
    CloseServiceHandle(hSCManager);
}

// ==================== 管道通信 ====================
std::string XorPipe(const std::string& data) {
    std::string result = data;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] ^= PIPE_KEY[i % PIPE_KEY.size()];
    }
    return result;
}

bool SendToService(const std::string& command, std::string& response) {
    if (!StartLightService()) {
        return false;
    }

    std::string encrypted = XorPipe(command);

    // 连接管道
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    for (int i = 0; i < 20; i++) {
        hPipe = CreateFileA("\\\\.\\pipe\\LightPipe",
            GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPipe != INVALID_HANDLE_VALUE) break;
        Sleep(500);
    }

    if (hPipe == INVALID_HANDLE_VALUE) {
        StopLightService();
        return false;
    }

    DWORD bytesWritten;
    WriteFile(hPipe, encrypted.c_str(), encrypted.size(), &bytesWritten, NULL);

    char buffer[4096] = { 0 };
    DWORD bytesRead;
    ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL);

    response = XorPipe(std::string(buffer, bytesRead));

    CloseHandle(hPipe);
    StopLightService();
    return true;
}

bool ElevateThroughService(const std::string& exePath) {
    std::string response;
    std::string command = "run:" + exePath;
    return SendToService(command, response) && response == "OK";
}

bool LaunchProgram(const std::string& exePath, bool waitForCompletion) {
    if (IsRunningAsAdmin()) {
        // 先尝试服务提权（无弹窗）
        std::string response;
        std::string command = "run:" + exePath;
        if (waitForCompletion) {
            command += " wait";
        }

        if (SendToService(command, response) && response == "OK") {
            return true;
        }
    }

    // 服务方式失败，使用普通方式启动
    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.lpVerb = "open";
    sei.lpFile = exePath.c_str();
    sei.nShow = SW_SHOWNORMAL;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    if (ShellExecuteExA(&sei)) {
        if (waitForCompletion && sei.hProcess) {
            // 等待进程结束
            WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);
        }
        return true;
    }
    return false;
}

// ==================== ElevateCurrentProgram 移到服务管理和管道通信之后 ====================
void ElevateCurrentProgram() {
    if (IsRunningAsAdmin()) {
        cout << "当前程序已经是管理员权限。" << endl;
        return;
    }

    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0) {
        ShellExecuteA(NULL, "runas", NULL, NULL, NULL, SW_SHOWNORMAL);
        ExitProcess(0);
    }

    // 先尝试服务提权
    CheckAndInstallService();
    if (StartLightService()) {
        std::string response;
        std::string command = "run:" + std::string(exePath);
        if (SendToService(command, response) && response == "OK") {
            Sleep(500);
            ExitProcess(0);
        }
    }

    // 服务提权失败，使用传统方式
    RunWithUACAndExit(exePath);
}

// ==================== 加密解密 ====================
std::string XorEncryptDecrypt(const std::string& data, const std::string& key) {
    if (data.empty() || key.empty()) return data;

    std::string result = data;
    size_t keyLen = key.size();
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = result[i] ^ key[i % keyLen];
    }
    return result;
}

std::string ReadRegistryFile(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) return "";

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size == 0) {
        file.close();
        return "";
    }

    std::string encrypted(size, '\0');
    file.read(&encrypted[0], size);
    file.close();

    return XorEncryptDecrypt(encrypted, PASSWORD);
}

std::string FindValueInRegistry(const std::string& data,
    const std::vector<std::string>& path,
    const std::string& valueName,
    int valueIndex = 0) {  // 新增：找到第几个匹配的值

    if (path.empty() || data.empty()) return "";

    size_t currentPos = 0;
    for (const auto& node : path) {
        currentPos = data.find("NODE:" + node, currentPos);
        if (currentPos == std::string::npos) return "";
    }

    int foundCount = 0;
    size_t valueNodePos = data.find("NODE:" + valueName, currentPos);
    while (valueNodePos != std::string::npos) {
        size_t iskeyPos = data.find("ISKEY:", valueNodePos);
        if (iskeyPos != std::string::npos && data.substr(iskeyPos, 7) == "ISKEY:0") {
            if (foundCount == valueIndex) {
                // 找到目标值
                size_t valuePos = data.find("VALUE:", iskeyPos);
                if (valuePos != std::string::npos) {
                    valuePos += 6;
                    size_t endPos = data.find("\n", valuePos);
                    return (endPos == std::string::npos) ?
                        data.substr(valuePos) : data.substr(valuePos, endPos - valuePos);
                }
            }
            foundCount++;
        }
        valueNodePos = data.find("NODE:" + valueName, valueNodePos + 5);
    }
    return "";
}
/*
std::string FindValueInRegistry(const std::string& data,
    const std::vector<std::string>& path,
    const std::string& valueName,
    int valueIndex = 0) {

    if (path.empty() || data.empty()) return "";

    size_t currentPos = 0;
    for (const auto& node : path) {
        currentPos = data.find("NODE:" + node, currentPos);
        if (currentPos == std::string::npos) return "";
    }

    int foundCount = 0;
    size_t valueNodePos = data.find("NODE:" + valueName, currentPos);

    // 调试输出
    std::cout << "=== 调试 FindValueInRegistry ===" << std::endl;
    std::cout << "查找路径: ";
    for (const auto& p : path) std::cout << p << " -> ";
    std::cout << valueName << std::endl;
    std::cout << "目标索引: " << valueIndex << std::endl;

    while (valueNodePos != std::string::npos) {
        // 检查这个节点是否是值节点（ISKEY:0）
        size_t iskeyPos = data.find("ISKEY:", valueNodePos);
        if (iskeyPos != std::string::npos) {
            std::string iskeyValue = data.substr(iskeyPos + 6, 1);
            std::cout << "找到 NODE:" << valueName << "，ISKEY=" << iskeyValue << std::endl;

            if (iskeyValue == "0") {
                std::cout << "  这是值节点，当前计数: " << foundCount << std::endl;
                if (foundCount == valueIndex) {
                    // 找到目标值
                    size_t valuePos = data.find("VALUE:", iskeyPos);
                    if (valuePos != std::string::npos) {
                        valuePos += 6;
                        size_t endPos = data.find("\n", valuePos);
                        std::string result = (endPos == std::string::npos) ?
                            data.substr(valuePos) : data.substr(valuePos, endPos - valuePos);
                        std::cout << "  返回值: " << result.substr(0, 50) << (result.size() > 50 ? "..." : "") << std::endl;
                        std::cout << "=== 调试结束 ===" << std::endl;
                        return result;
                    }
                }
                foundCount++;
            }
        }
        valueNodePos = data.find("NODE:" + valueName, valueNodePos + 5);
    }

    std::cout << "未找到匹配的值节点" << std::endl;
    std::cout << "=== 调试结束 ===" << std::endl;
    return "";
}*/

// ==================== 删除功能 ====================
void DeleteFilesInDirectory(const std::string& dirPath) {
#ifdef _WIN32
    // 先检查是不是文件
    DWORD dwAttrib = GetFileAttributesA(dirPath.c_str());

    if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
        std::cout << "路径不存在: " << dirPath << std::endl;
        return;
    }

    // 如果是文件，直接删除
    if (!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
        if (std::remove(dirPath.c_str()) == 0) {
            std::cout << "删除文件: " << dirPath << std::endl;
        }
        else {
            std::cout << "删除失败: " << dirPath << std::endl;
        }
        return;
    }

    // 是目录，删除目录中的所有文件
    std::string searchPath = dirPath + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cout << "无法打开目录: " << dirPath << std::endl;
        return;
    }

    int count = 0;
    do {
        if (strcmp(findData.cFileName, ".") == 0 ||
            strcmp(findData.cFileName, "..") == 0) {
            continue;
        }

        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::string filePath = dirPath + "\\" + findData.cFileName;
            if (std::remove(filePath.c_str()) == 0) {
                std::cout << "已删除1个文件: " << findData.cFileName << std::endl;
                count++;
            }
            else {
                std::cout << "删除1个文件失败: " << findData.cFileName << std::endl;
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
    std::cout << "共删除 " << count << " 个文件" << std::endl;
#endif
}

// ==================== 文件夹锁定函数（使用 TrustedInstaller 权限） ====================
// 使用 TrustedInstaller 锁定文件夹（支持静默模式）
bool LockFolderWithTrustedInstaller(const std::string& folderPath, bool silent = false) {
    EnableSecurityPrivileges();

    std::string cmd;
    std::string redirect = silent ? " >nul 2>nul" : "";

    // 1. 将所有者改为 TrustedInstaller（最高权限）
    cmd = "icacls \"" + folderPath + "\" /setowner \"NT SERVICE\\TrustedInstaller\" /t /c" + redirect;
    system(cmd.c_str());

    // 2. 禁用继承，移除所有用户权限，只给 SYSTEM 完全控制
    cmd = "icacls \"" + folderPath + "\" /inheritance:r /remove:g \"Everyone\" \"Administrators\" \"Users\" /grant \"SYSTEM\":F /t /q" + redirect;
    system(cmd.c_str());

    // 3. 设置为系统隐藏 + 只读
    SetFileAttributesA(folderPath.c_str(), FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);

    return true;
}

// 递归保护文件夹及其所有子文件和子文件夹（支持静默模式）
bool Lock_ProtectFolderRecursively(const std::string& _wjj, bool silent = false) {
    // 启用权限
    EnableSecurityPrivileges();

    std::string searchPath = _wjj + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        // 如果文件夹不存在，尝试创建
        if (!CreateDirectoryA(_wjj.c_str(), NULL)) {
            return false;
        }
        // 重新查找
        hFind = FindFirstFileA(searchPath.c_str(), &findData);
        if (hFind == INVALID_HANDLE_VALUE) {
            return false;
        }
    }

    std::string redirect = silent ? " >nul 2>nul" : "";

    // 先锁定根文件夹
    if (!LockFolderWithTrustedInstaller(_wjj, silent)) {
        FindClose(hFind);
        return false;
    }

    // 遍历子项
    do {
        if (strcmp(findData.cFileName, ".") == 0 ||
            strcmp(findData.cFileName, "..") == 0) {
            continue;
        }

        std::string subPath = _wjj + "\\" + findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // 递归保护子文件夹
            Lock_ProtectFolderRecursively(subPath, silent);
        }
        else {
            // 保护文件
            std::string fileCmd;
            fileCmd = "icacls \"" + subPath + "\" /setowner \"NT SERVICE\\TrustedInstaller\" /c" + redirect;
            system(fileCmd.c_str());
            fileCmd = "icacls \"" + subPath + "\" /inheritance:r /remove:g \"Everyone\" \"Administrators\" \"Users\" /grant \"SYSTEM\":F /q" + redirect;
            system(fileCmd.c_str());
            SetFileAttributesA(subPath.c_str(), FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

    // 最后再对根文件夹设置一次，确保继承生效
    LockFolderWithTrustedInstaller(_wjj, silent);
    return true;
}

// ==================== 文件夹解锁函数（恢复管理员访问权限） ====================
// 解锁文件夹：将所有者改为管理员，授予管理员完全控制，移除系统/隐藏属性
bool UnlockFolderForAdmin(const std::string& folderPath, bool silent = false) {
    EnableSecurityPrivileges();

    std::string cmd;
    std::string redirect = silent ? " >nul 2>nul" : "";

    // 1. 先获取所有权（takeown 需要管理员权限）
    cmd = "takeown /f \"" + folderPath + "\" /r /d y" + redirect;
    system(cmd.c_str());

    // 2. 授予管理员完全控制，并启用继承
    cmd = "icacls \"" + folderPath + "\" /inheritance:e /grant \"Administrators\":F /t /q" + redirect;
    system(cmd.c_str());

    // 3. 移除系统/隐藏/只读属性
    SetFileAttributesA(folderPath.c_str(), FILE_ATTRIBUTE_NORMAL);

    return true;
}

// 恢复文件夹权限（需要管理员权限）
bool Lock_RestoreFolderPermission(const std::string& _wjj) {
    return UnlockFolderForAdmin(_wjj);
}

// 递归恢复文件夹及其所有子文件和子文件夹的权限
bool Lock_RestoreFolderRecursively(const std::string& _wjj) {
    EnableSecurityPrivileges();

    std::string searchPath = _wjj + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        return false;
    }

    // 先恢复当前文件夹
    if (!UnlockFolderForAdmin(_wjj)) {
        FindClose(hFind);
        return false;
    }

    // 遍历子项
    do {
        if (strcmp(findData.cFileName, ".") == 0 ||
            strcmp(findData.cFileName, "..") == 0) {
            continue;
        }

        std::string subPath = _wjj + "\\" + findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // 递归恢复子文件夹
            Lock_RestoreFolderRecursively(subPath);
        }
        else {
            // 恢复文件
            std::string fileCmd = "takeown /f \"" + subPath + "\" /d y";
            system(fileCmd.c_str());
            fileCmd = "icacls \"" + subPath + "\" /grant \"Administrators\":F /q";
            system(fileCmd.c_str());
            SetFileAttributesA(subPath.c_str(), FILE_ATTRIBUTE_NORMAL);
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);

    // 最后再对根文件夹恢复一次
    UnlockFolderForAdmin(_wjj);
    return true;
}

// ==================== 配置函数 ====================
std::string CopyIconToSystem(const std::string& sourceIcon) {
    std::string destDir = "C:\\Program Files\\.light";
    std::string destIcon = destDir + "\\light.ico";
    // 创建目录（如果不存在）
    CreateDirectoryA(destDir.c_str(), NULL);
    // 复制图标文件
    if (CopyFileA(sourceIcon.c_str(), destIcon.c_str(), FALSE)) {
        SetFileAttributesA(destIcon.c_str(), FILE_ATTRIBUTE_HIDDEN);
        return destIcon;
    }
    return "";
}

bool RegisterFileType(const std::string& extension, const std::string& description,
    const std::string& iconPath, const std::string& openExePath = "") {
    HKEY hKey;
    LONG result;
    std::string extKey = "." + extension;
    std::string progID = "light." + extension;

    RegDeleteKeyA(HKEY_CLASSES_ROOT, extKey.c_str());
    SHDeleteKeyA(HKEY_CLASSES_ROOT, progID.c_str());

    // 创建文件扩展名关联
    result = RegCreateKeyA(HKEY_CLASSES_ROOT, extKey.c_str(), &hKey);
    if (result != ERROR_SUCCESS) return false;
    RegSetValueA(hKey, NULL, REG_SZ, progID.c_str(), (DWORD)progID.size());
    RegCloseKey(hKey);

    // 创建 ProgID
    result = RegCreateKeyA(HKEY_CLASSES_ROOT, progID.c_str(), &hKey);
    if (result != ERROR_SUCCESS) return false;
    RegSetValueA(hKey, NULL, REG_SZ, description.c_str(), (DWORD)description.size());
    RegCloseKey(hKey);

    // 设置图标
    if (!iconPath.empty()) {
        std::string iconKey = progID + "\\DefaultIcon";
        result = RegCreateKeyA(HKEY_CLASSES_ROOT, iconKey.c_str(), &hKey);
        if (result == ERROR_SUCCESS) {
            RegSetValueA(hKey, NULL, REG_SZ, iconPath.c_str(), (DWORD)iconPath.size());
            RegCloseKey(hKey);
        }
    }

    // 设置打开方式
    if (!openExePath.empty()) {
        std::string openCmd = "\"" + openExePath + "\" \"%1\"";
        std::string shellKey = progID + "\\shell\\open\\command";
        result = RegCreateKeyA(HKEY_CLASSES_ROOT, shellKey.c_str(), &hKey);
        if (result == ERROR_SUCCESS) {
            RegSetValueA(hKey, NULL, REG_SZ, openCmd.c_str(), (DWORD)openCmd.size());
            RegCloseKey(hKey);
        }

        // 设置 UserChoice
        std::string userChoiceKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\."
            + extension + "\\UserChoice";
        HKEY hUserKey;
        if (RegCreateKeyA(HKEY_CURRENT_USER, userChoiceKey.c_str(), &hUserKey) == ERROR_SUCCESS) {
            RegSetValueExA(hUserKey, "Progid", 0, REG_SZ,
                (BYTE*)progID.c_str(), (DWORD)progID.size() + 1);
            RegCloseKey(hUserKey);
        }
    }

    return true;
}

bool RegisterApplication(const std::string& appName, const std::string& appPath,
    const std::string& uninstallPath, const std::string& iconPath,
    const std::string& version) {
    HKEY hKey;
    LONG result;
    std::string keyPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + appName;

    result = RegCreateKeyA(HKEY_LOCAL_MACHINE, keyPath.c_str(), &hKey);
    if (result != ERROR_SUCCESS) return false;

    RegSetValueExA(hKey, "DisplayName", 0, REG_SZ, (BYTE*)appName.c_str(), (DWORD)appName.size() + 1);
    RegSetValueExA(hKey, "DisplayVersion", 0, REG_SZ, (BYTE*)version.c_str(), (DWORD)version.size() + 1);
    RegSetValueExA(hKey, "Publisher", 0, REG_SZ, (BYTE*)"light", 5);
    RegSetValueExA(hKey, "InstallLocation", 0, REG_SZ, (BYTE*)appPath.c_str(), (DWORD)appPath.size() + 1);
    RegSetValueExA(hKey, "UninstallString", 0, REG_SZ, (BYTE*)uninstallPath.c_str(), (DWORD)uninstallPath.size() + 1);
    if (!iconPath.empty()) {
        RegSetValueExA(hKey, "DisplayIcon", 0, REG_SZ, (BYTE*)iconPath.c_str(), (DWORD)iconPath.size() + 1);
    }
    DWORD noModify = 1;
    RegSetValueExA(hKey, "NoModify", 0, REG_DWORD, (BYTE*)&noModify, sizeof(DWORD));
    RegSetValueExA(hKey, "NoRepair", 0, REG_DWORD, (BYTE*)&noModify, sizeof(DWORD));
    RegCloseKey(hKey);
    return true;
}

bool RestoreFolderPermission(const std::string& folderPath) {
    return UnlockFolderForAdmin(folderPath);
}

bool peizhi() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, COLOR_WHITE);

    std::string programDir = GetProgramDir();
    std::string parentDir = GetParentDir(programDir);

    std::string lightExe = parentDir + "\\light.exe";
    std::string uninstallExe = parentDir + "\\rppcst\\light_Uninstall.exe";
    std::string sourceIcon = programDir + "\\rppcst\\favicon.ico";
    std::string hiddenDir = "C:\\Program Files\\.light";
    std::string binaryHexEditor = parentDir + "\\BinaryHexEditor.exe";

    SetConsoleTextAttribute(hConsole, COLOR_WHITE);
    std::cout << "[*] 创建目录..." << std::endl;
    StartSpinner();
    Sleep(3000);
    if (CreateDirectoryA(hiddenDir.c_str(), NULL)) {
        SetFileAttributesA(hiddenDir.c_str(), FILE_ATTRIBUTE_HIDDEN);
        SetConsoleTextAttribute(hConsole, COLOR_GREEN);
        std::cout << "[OK] 创建目录: " /* << hiddenDir*/ << std::endl;
    }
    else if (GetLastError() == ERROR_ALREADY_EXISTS) {
        SetFileAttributesA(hiddenDir.c_str(), FILE_ATTRIBUTE_HIDDEN);
        SetConsoleTextAttribute(hConsole, COLOR_GREEN);
        std::cout << "[OK] 目录已存在: " /* << hiddenDir*/ << std::endl;
    }
    else {
        StopSpinner();
        SetConsoleTextAttribute(hConsole, COLOR_RED);
        std::cout << "[X] 创建目录失败，错误码: " << GetLastError() << std::endl;
        return false;
    }
    StopSpinner();
    Sleep(3000);

    SetConsoleTextAttribute(hConsole, COLOR_WHITE);
    std::cout << "[*] 复制图标..." << std::endl;
    StartSpinner();
    Sleep(3000);
    std::string iconPath = CopyIconToSystem(sourceIcon);
    if (!iconPath.empty()) {
        SetConsoleTextAttribute(hConsole, COLOR_GREEN);
        std::cout << "[OK] 图标已保存" << std::endl;
    }
    else {
        SetConsoleTextAttribute(hConsole, COLOR_YELLOW);
        StopSpinner();
        std::cout << "[!] 未找到 favicon.ico" << std::endl;
        return false;
    }
    StopSpinner();
    Sleep(3000);
    SetConsoleTextAttribute(hConsole, COLOR_WHITE);

    std::cout << "[*] 创建加密文件..." << std::endl;
    StartSpinner();
    Sleep(3000);
    std::string xbyxFile = hiddenDir + "\\light.xbyx";
    std::ofstream file(xbyxFile);
    if (file.is_open()) {
        file.close();
        SetFileAttributesA(xbyxFile.c_str(), FILE_ATTRIBUTE_HIDDEN);
        SetConsoleTextAttribute(hConsole, COLOR_GREEN);
        std::cout << "[OK] 创建文件: light.xbyx" << std::endl;
    }
    else {
        SetConsoleTextAttribute(hConsole, COLOR_RED);
        StopSpinner();
        std::cout << "[X] 创建文件失败" << std::endl;
        return false;
    }
    StopSpinner();
    Sleep(3000);
    SetConsoleTextAttribute(hConsole, COLOR_WHITE);
    std::cout << "[*] 注册文件类型..." << std::endl;
    StartSpinner();
    Sleep(3000);

    // 检查 BinaryHexEditor.exe 是否存在
    bool hexEditorExists = (GetFileAttributesA(binaryHexEditor.c_str()) != INVALID_FILE_ATTRIBUTES);
    std::string openExePath = hexEditorExists ? binaryHexEditor : lightExe;

    bool xayxOk = RegisterFileType("xayx", "light 1级文件", iconPath, openExePath);
    bool xbyxOk = RegisterFileType("xbyx", "light 2级文件", iconPath, openExePath);
    bool xcyxOk = RegisterFileType("xcyx", "light 3级文件", iconPath, openExePath);
    Sleep(3000);
    if (xayxOk) { SetConsoleTextAttribute(hConsole, COLOR_GREEN); std::cout << "[OK] .xayx - 1级文件 (打开程序: " << openExePath << ")" << std::endl; }
    else {
        StopSpinner();
        SetConsoleTextAttribute(hConsole, COLOR_RED);
        std::cout << "[X] .xayx - 注册失败" << std::endl;
        return false;
    }
    Sleep(3000);
    if (xbyxOk) { SetConsoleTextAttribute(hConsole, COLOR_GREEN); std::cout << "[OK] .xbyx - 2级文件" << std::endl; }
    else {
        StopSpinner();
        SetConsoleTextAttribute(hConsole, COLOR_RED);
        std::cout << "[X] .xbyx - 注册失败" << std::endl;
        return false;
    }
    Sleep(3000);
    if (xcyxOk) { SetConsoleTextAttribute(hConsole, COLOR_GREEN); std::cout << "[OK] .xcyx - 3级文件" << std::endl; }
    else {
        StopSpinner();
        SetConsoleTextAttribute(hConsole, COLOR_RED);
        std::cout << "[X] .xcyx - 注册失败" << std::endl;
        return false;
    }
    StopSpinner();
    Sleep(3000);

    // 刷新图标缓存
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    SetConsoleTextAttribute(hConsole, COLOR_WHITE);
    std::cout << "[*] 注册应用程序..." << std::endl;
    StartSpinner();
    Sleep(3000);
    if (RegisterApplication("light", lightExe, uninstallExe, iconPath, "1.0.0")) {
        SetConsoleTextAttribute(hConsole, COLOR_GREEN);
        std::cout << "[OK] 已添加到应用和功能" << std::endl;
    }
    else {
        StopSpinner();
        SetConsoleTextAttribute(hConsole, COLOR_RED);
        std::cout << "[X] 注册应用程序失败" << std::endl;
        return false;
    }

    SetConsoleTextAttribute(hConsole, COLOR_CYAN);
    std::cout << "\n[*] 完成！BinaryHexEditor已设置为默认程序" << std::endl;
    StopSpinner();
    Sleep(3000);

    SetConsoleTextAttribute(hConsole, COLOR_WHITE);
    return true;
}

bool uninstall() {
    std::string programDir = GetProgramDir();
    std::string parentDir = GetParentDir(programDir);
    std::string hiddenDir = "C:\\Program Files\\.light";
    std::string uninstallExe = parentDir + "\\rppcst\\light_Uninstall.exe";

    // 删除隐藏目录及其内容
    DeleteFilesInDirectory(hiddenDir);
    RemoveDirectoryA(hiddenDir.c_str());

    // 注销文件类型
    SHDeleteKeyA(HKEY_CLASSES_ROOT, ".xbyx");
    SHDeleteKeyA(HKEY_CLASSES_ROOT, "light.xbyx");

    // 注销应用程序
    SHDeleteKeyA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Light");

    // 执行卸载程序
    LaunchProgram(uninstallExe, true);
    return true;
}

// ==================== about 函数 - 修改为使用 valueIndex ====================
void about() {
    string cd = GetCurrentProgramPath();
    string registryPath = cd + "\\rppcst\\sys.xayx";
    string data = ReadRegistryFile(registryPath);

    if (!data.empty()) {
        // 路径: HKEY_LOCAL_MACHINE -> xbyx -> xcyx -> about
        vector<string> path = { "HKEY_LOCAL_MACHINE", "xbyx", "xcyx", "about" };
        // 在 about 键下查找 about 值
        string aboutValue = FindValueInRegistry(data, path, "about");

        if (!aboutValue.empty()) {
            // 处理 \n 换行符
            size_t pos = 0;
            while ((pos = aboutValue.find("\\n", pos)) != string::npos) {
                aboutValue.replace(pos, 2, "\n");
                pos += 1;
            }
            cout << aboutValue << endl;
        }
        else {
            cout << "注册表异常 - 未找到 about 信息" << endl;
        }
    }
    else {
        cout << "注册表异常 - 无法读取注册表文件" << endl;
    }
}

// ==================== help 函数 - 修改为使用 valueIndex ====================
void help() {
    string cd = GetCurrentProgramPath();
    string registryPath = cd + "\\rppcst\\sys.xayx";
    string data = ReadRegistryFile(registryPath);

    if (!data.empty()) {
        // 路径: HKEY_LOCAL_MACHINE -> xbyx -> xcyx
        vector<string> path = { "HKEY_LOCAL_MACHINE", "xbyx", "xcyx" };
        // 获取第2个 help 值（索引1），因为第一个 help 值是 "1"
        string helpValue = FindValueInRegistry(data, path, "help", 1);

        if (!helpValue.empty()) {
            // 处理 \n 换行符
            size_t pos = 0;
            while ((pos = helpValue.find("\\n", pos)) != string::npos) {
                helpValue.replace(pos, 2, "\n");
                pos += 1;
            }
            cout << helpValue << endl;
        }
        else {
            cout << "注册表异常 - 未找到帮助信息" << endl;
        }
    }
    else {
        cout << "注册表异常 - 无法读取注册表文件" << endl;
    }
}

// ==================== 主函数 ====================
int main(void) {
    

    // 获取路径
    string cd = GetCurrentProgramPath();
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // 定义路径
    string exePath = cd + "\\rppcst\\CEL.exe";
    string vbsPath = cd + "\\rppcst\\a1.vbs";
    string fullPortTool = cd + "\\rppcst\\Full_Port_Network_Packet_Capture_Tool.exe";
    string registryPath = cd + "\\rppcst\\sys.xayx";
    string filePath = "C:\\Program Files\\.light\\light.xbyx";
    string betPath = cd + "\\rppcst\\light.exe";
    string Registry = cd + "\\Registry.exe";
    string BinaryHexEditor = cd + "\\BinaryHexEditor.exe";
    string folderPath = "C:\\Program Files\\.light";
    string _wjj;

    // 显示欢迎信息
    cout << "_________________________________________________________________________________________________" << endl;
    cout << "===========================================Hello itc!============================================" << endl;
    cout << "_________________________________________________________________________________________________" << endl;

    // ==================== 修改：先检查文件是否存在，如果不存在可能是因为权限问题 ====================
    bool fileExists = FileExists(filePath);

    // 如果文件不存在，尝试临时解锁后再检查（静默模式）
    if (!fileExists) {
        // 检查管理员权限
        if (!IsRunningAsAdmin()) {
            ElevateCurrentProgram();
        }

        // 尝试临时解锁文件夹（授予管理员访问权限）- 静默模式
        cout << "正在初始化配置状态..." << endl;
        StartSpinner();
        EnableSecurityPrivileges();

        // 解锁文件夹（静默模式）
        UnlockFolderForAdmin(folderPath, true);
        Sleep(500);

        // 重新检查文件是否存在
        fileExists = FileExists(filePath);

        // 如果文件存在，重新锁定文件夹（静默模式）
        if (fileExists) {
            LockFolderWithTrustedInstaller(folderPath, true);
            Lock_ProtectFolderRecursively(folderPath, true);
            StopSpinner();
        }
        else {
            StopSpinner();
        }
    }

    // 检查文件是否存在，不存在则初始化
    if (!fileExists) {
        // 提权
        if (!IsRunningAsAdmin()) {
            ElevateCurrentProgram();
        }
        int cs = 0;

        cout << "正在准备中..." << endl;
        StartSpinner();

        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(8000, 20000);

        // 安装服务
        CheckAndInstallService();
        Sleep(dis(gen));
        StopSpinner();
        Sleep(dis(gen));
        Sleep(3000);

        cout << "即将为你配置环境..." << endl;
        StartSpinner();
        Sleep(dis(gen));
        Sleep(dis(gen));
        StopSpinner();
        Sleep(3000);

        cout << "开始为你配置环境，请耐心等待..." << endl;
        StartSpinner();
        Sleep(dis(gen));
        if (!peizhi()) {
            StopSpinner();
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout << "环境配置失败" << endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            cout << "程序即将关闭..." << endl;
            Sleep(5000);
            ExitProcess(0);
        }
        else {
            Sleep(dis(gen));
            StopSpinner();
            Sleep(3000);
        }
        cout << "正在做最后的部署，请耐心等待..." << endl;
        StartSpinner();
        Sleep(dis(gen));
        CreateDirectoryA(folderPath.c_str(), NULL);

        // 启用权限后再锁定（使用 TrustedInstaller）
        EnableSecurityPrivileges();

        if (LockFolderWithTrustedInstaller(folderPath)) {
            Sleep(3000);
        }
        else {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cs = cs + 1;
            cout << "出现1个失败" << endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }

        if (Lock_ProtectFolderRecursively(folderPath)) {
            cout << "环境配置完成" << endl;
        }
        else {
            cs = cs + 1;
            cout << "出现" << cs << "个失败" << endl;
        }
    }

    if (!IsRunningAsAdmin()) {
        ElevateCurrentProgram();
        EnableSecurityPrivileges();
    }

    // 安装控制处理程序-抑制Ctrl+C事件
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    // 主循环
    int count = 0;
    int num = 0;
    while (true) {
        SetConsoleCtrlHandler(CtrlHandler, TRUE);
        cout << "<" << cd << ":> ";
        string command;
        getline(cin, command);

        if (command == "exit") {
            cout << "exit+" << endl;
            break;
        }
        else if (command == "CEL") {
            system(exePath.c_str());
        }
        else if (command == "uninstall") {
            RestoreFolderPermission(folderPath);
            uninstall();
        }
        else if (command == "FPNPCT" || command == "Full Port Network Packet Capture Tool") {
            system(fullPortTool.c_str());
        }
        else if (command == "about") {
            about();
        }
        else if (command == "fill") {
            cout << "fill+" << endl;
            cout << "<" << cd << ":fill/> ";

            // 读取并解密注册表文件
            string registryData = ReadRegistryFile(registryPath);

            if (registryData.empty()) {
                cout << "无法读取注册表文件" << endl;
                continue;
            }

            // 在 HKEY_LOCAL_MACHINE -> xbyx 路径下查找 fill 的值
            std::vector<std::string> path = { "HKEY_LOCAL_MACHINE", "xbyx" };
            std::string fillValue = FindValueInRegistry(registryData, path, "fill");

            if (fillValue == "air") {
                // 值等于 "air"，执行删除
                string deletePath;
                cin.clear();
                getline(cin, deletePath);
                DeleteFilesInDirectory(deletePath);
            }
            else if (fillValue.empty()) {
                cout << "未找到 fill 值" << endl;
            }
            else {
                cout << "fill 当前值: " << fillValue << endl;
            }
        }
        // ==================== dump 命令（导出注册表） ====================
        else if (command == "dump") {
            std::cout << "=== 调试信息 ===" << std::endl;
            std::cout << "注册表文件路径: " << registryPath << std::endl;

            // 检查文件是否存在
            std::ifstream checkFile(registryPath, std::ios::binary);
            if (!checkFile.is_open()) {
                std::cout << "错误：文件不存在或无法访问！" << std::endl;
                std::cout << "请先运行 Registry.exe 创建注册表文件" << std::endl;
            }
            else {
                checkFile.seekg(0, std::ios::end);
                size_t fileSize = checkFile.tellg();
                checkFile.close();
                std::cout << "文件大小: " << fileSize << " 字节" << std::endl;

                if (fileSize == 0) {
                    std::cout << "错误：文件为空！" << std::endl;
                }
                else {
                    std::string data = ReadRegistryFile(registryPath);
                    std::cout << "解密后数据大小: " << data.size() << " 字节" << std::endl;

                    if (!data.empty()) {
                        // 检查是否包含 "NODE:" 关键字（说明是有效的注册表数据）
                        if (data.find("NODE:") != std::string::npos) {
                            std::cout << "✓ 数据格式正确，包含注册表节点信息" << std::endl;
                        }
                        else {
                            std::cout << "⚠ 警告：数据中未找到 'NODE:' 关键字" << std::endl;
                        }

                        std::string cacheDir = cd + "\\Cache";
                        CreateDirectoryA(cacheDir.c_str(), NULL);

                        std::string filePath = cacheDir + "\\decrypted.txt";
                        std::ofstream out(filePath, std::ios::binary);
                        if (out.is_open()) {
                            out.write(data.c_str(), data.size());
                            out.close();
                            std::cout << "✓ 已导出到: " << filePath << std::endl;
                            std::cout << "请用记事本或其他文本编辑器打开查看" << std::endl;
                        }
                        else {
                            std::cout << "✗ 无法创建文件: " << filePath << std::endl;
                        }
                    }
                    else {
                        std::cout << "✗ 解密后数据为空！" << std::endl;
                    }
                }
            }
            std::cout << "=== 调试结束 ===" << std::endl;
        }
        // ==================== Lock 命令 ====================
        else if (command.rfind("Lock ", 0) == 0 || command.rfind("lock ", 0) == 0) {
            std::string folderToLock = command.substr(5); // 提取路径

            if (folderToLock.empty()) {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout << "错误：请指定要锁定的文件夹路径！" << endl;
                cout << "用法: Lock <文件夹路径>" << endl;
                cout << "示例: Lock C:\\Program Files\\.light" << endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
            else {
                // 检查管理员权限
                if (!IsRunningAsAdmin()) {
                    ElevateCurrentProgram();
                }

                // 检查文件夹是否存在
                DWORD dwAttrib = GetFileAttributesA(folderToLock.c_str());
                if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                    cout << "错误：文件夹不存在或无法访问！" << endl;
                    cout << "路径: " << folderToLock << endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
                else if (!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                    cout << "错误：指定路径不是文件夹！" << endl;
                    cout << "路径: " << folderToLock << endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
                else {
                    // 执行锁定操作
                    cout << "准备锁定文件夹: " << folderToLock << endl;
                    cout << "确定继续(y/n): ";

                    string confirm;
                    getline(cin, confirm);

                    if (confirm == "y" || confirm == "Y" || confirm == "yes" || confirm == "Yes") {
                        StartSpinner();
                        cout << "正在设置权限..." << endl;

                        EnableSecurityPrivileges();

                        // 使用 TrustedInstaller 递归保护文件夹
                        if (Lock_ProtectFolderRecursively(folderToLock)) {
                            StopSpinner();
                            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                            cout << "文件夹锁定成功" << endl;
                            cout << "               : " << folderToLock << endl;
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                        }
                        else {
                            StopSpinner();
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                            cout << "文件夹锁定失败" << endl;
                            cout << "               检查是否有其他程序正在使用该文件夹" << endl;
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                        }
                    }
                    else {
                        cout << "操作已取消" << endl;
                    }
                }
            }
        }
        // ==================== Unlock 命令 ====================
        else if (command.rfind("Unlock ", 0) == 0 || command.rfind("unlock ", 0) == 0) {
            std::string folderToUnlock = command.substr(7); // 提取路径

            if (folderToUnlock.empty()) {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout << "错误：请指定要解锁的文件夹路径！" << endl;
                cout << "用法: Unlock <文件夹路径>" << endl;
                cout << "示例: Unlock C:\\Program Files\\.light" << endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
            else {
                // 检查管理员权限
                if (!IsRunningAsAdmin()) {
                    ElevateCurrentProgram();
                }

                // 检查文件夹是否存在
                DWORD dwAttrib = GetFileAttributesA(folderToUnlock.c_str());
                if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                    cout << "错误：文件夹不存在或无法访问！" << endl;
                    cout << "路径: " << folderToUnlock << endl;
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                }
                else {
                    // 执行解锁操作
                    cout << "准备解锁文件夹: " << folderToUnlock << endl;
                    cout << "确定要继续吗？(y/n): ";

                    string confirm;
                    getline(cin, confirm);

                    if (confirm == "y" || confirm == "Y" || confirm == "yes" || confirm == "Yes") {
                        StartSpinner();
                        cout << "正在恢复权限..." << endl;

                        EnableSecurityPrivileges();

                        // 递归恢复文件夹权限
                        if (Lock_RestoreFolderRecursively(folderToUnlock)) {
                            StopSpinner();
                            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
                            cout << "  文件夹解锁成功！" << endl;
                            cout << "  路径: " << folderToUnlock << endl;
                            cout << "  已恢复访问权限" << endl;
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                        }
                        else {
                            StopSpinner();
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                            cout << "  .文件夹解锁失败." << endl;
                            cout << "  请检查文件夹权限状态" << endl;
                            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                        }
                    }
                    else {
                        cout << "操作已取消." << endl;
                    }
                }
            }
        }
        // ==================== Lock/Unlock 单独输入提示 ====================
        else if (command == "Lock" || command == "lock" || command == "LOCK" ||
            command == "Unlock" || command == "unlock" || command == "UNLOCK") {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout << "错误的参数指令：" << command << endl;
            cout << "                ~~~~^" << endl;
            cout << "参数后应跟相应的路径。" << endl;
            cout << " " << endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
        else if (command == "Registry") {
            // 打开 Registry 注册表
            string userChoice;
            bool waitForCompletion = false;
            bool validInput = false;

            while (!validInput) {
                cout << "是否等待 Registry 关闭后再继续？(y/n): ";

                // 使用 getline 替代 cin >>
                getline(cin, userChoice);

                // 调试：显示读取到的内容
                cout << "调试：读取到 '" << userChoice << "'，长度为 " << userChoice.length() << endl;

                // 去除首尾空格
                userChoice.erase(0, userChoice.find_first_not_of(" \t\r\n"));
                userChoice.erase(userChoice.find_last_not_of(" \t\r\n") + 1);

                cout << "调试：处理后 '" << userChoice << "'，长度为 " << userChoice.length() << endl;

                if (userChoice == "y" || userChoice == "Y" ||
                    userChoice == "yes" || userChoice == "YES" ||
                    userChoice == "是") {
                    waitForCompletion = true;
                    validInput = true;
                    cout << "调试：选择等待" << endl;
                }
                else if (userChoice == "n" || userChoice == "N" ||
                    userChoice == "no" || userChoice == "NO" ||
                    userChoice == "否") {
                    waitForCompletion = false;
                    validInput = true;
                    cout << "调试：选择不等待" << endl;
                }
                else {
                    cout << "无效输入，请输入 y 或 n" << endl;
                }
            }

            // 启动程序
            if (LaunchProgram(Registry, waitForCompletion)) {
                if (waitForCompletion) {
                    cout << "Registry 已启动，等待关闭..." << endl;
                }
                else {
                    cout << "Registry 已启动" << endl;
                }
            }
            else {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout << "启动 Registry 失败" << endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
        }
        else if (command == "BinaryHexEditor") {
            // 打开 BinaryHexEditor
            string userChoice;
            bool waitForCompletion = false;
            bool validInput = false;

            while (!validInput) {
                cout << "是否等待 BinaryHexEditor 关闭后再继续？(y/n): ";

                // 使用 getline 替代 cin >>
                getline(cin, userChoice);

                // 去除首尾空格
                userChoice.erase(0, userChoice.find_first_not_of(" \t\r\n"));
                userChoice.erase(userChoice.find_last_not_of(" \t\r\n") + 1);

                if (userChoice == "y" || userChoice == "Y" ||
                    userChoice == "yes" || userChoice == "YES" ||
                    userChoice == "是") {
                    waitForCompletion = true;
                    validInput = true;
                }
                else if (userChoice == "n" || userChoice == "N" ||
                    userChoice == "no" || userChoice == "NO" ||
                    userChoice == "否") {
                    waitForCompletion = false;
                    validInput = true;
                }
                else {
                    cout << "无效输入，请输入 y 或 n" << endl;
                }
            }

            // 启动程序
            if (LaunchProgram(BinaryHexEditor, waitForCompletion)) {
                if (waitForCompletion) {
                    cout << "BinaryHexEditor 已启动，等待关闭..." << endl;
                }
                else {
                    cout << "BinaryHexEditor 已启动" << endl;
                }
            }
            else {
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
                cout << "启动 BinaryHexEditor 失败" << endl;
                SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
            }
            }
        else if (command == "help") {
            help();
        }
        else if (command == "RestoreFolderPermission") {
            // 执行恢复文件夹权限的操作
            RestoreFolderPermission(folderPath);
        }
        else {
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            cout << "错误的参数指令：" << command << endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

            // 执行 VBS 脚本
            string cmd = "cscript //Nologo " + vbsPath;
            // system(cmd.c_str());  // 注：暂时屏蔽，待调试
        }

        if (++count >= 10) break;
    }

    return 0;
}