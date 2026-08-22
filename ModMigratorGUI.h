// ModMigratorGUI.h
#pragma once

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>

#include "ModMigratorCore.h"

class ModMigratorGUI {
public:
    ModMigratorGUI();
    ~ModMigratorGUI();

    bool Create(HINSTANCE hInstance, int nCmdShow);
    void Run();

private:
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR HandleMessage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void OnInit(HWND hDlg);
    void OnBrowseSource(HWND hDlg);
    void OnBrowseOutput(HWND hDlg);
    void OnConvert(HWND hDlg);
    void OnClearLog(HWND hDlg);
    void OnOpenOutput(HWND hDlg);
    void OnAbout(HWND hDlg);

    void AppendLog(const std::string& text);
    void UpdateProgress(int value);
    void SetUIEnabled(bool enabled);

    static void LogCallback(const std::string& msg);
    static void ProgressCallback(int progress);

    static ModMigratorGUI* s_instance;

    HWND m_hDlg;
    HINSTANCE m_hInstance;
    std::thread m_convertThread;
    std::atomic<bool> m_isConverting;
    std::string m_currentOutputPath;

    // Control handles
    HWND m_hEditSource;
    HWND m_hEditOutput;
    HWND m_hEditLog;
    HWND m_hProgress;
    HWND m_hBtnConvert;
    HWND m_hCheckIncludeSource;
    HWND m_hCheckExportZip;
    HWND m_hCheckGenerateEvents;
    HWND m_hCheckGenerateCaps;

    CRITICAL_SECTION m_logCriticalSection;
};
