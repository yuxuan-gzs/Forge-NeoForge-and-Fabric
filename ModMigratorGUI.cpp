// ModMigratorGUI.cpp
#include "ModMigratorGUI.h"
#include "Resource.h"
#include <shlobj.h>
#include <shellapi.h>
#include <sstream>
#include <chrono>

ModMigratorGUI* ModMigratorGUI::s_instance = nullptr;

ModMigratorGUI::ModMigratorGUI()
    : m_hDlg(nullptr), m_hInstance(nullptr), m_isConverting(false) {
    s_instance = this;
    InitializeCriticalSection(&m_logCriticalSection);
}

ModMigratorGUI::~ModMigratorGUI() {
    DeleteCriticalSection(&m_logCriticalSection);
}

bool ModMigratorGUI::Create(HINSTANCE hInstance, int nCmdShow) {
    m_hInstance = hInstance;
    return DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN_DIALOG), NULL, DialogProc) != -1;
}

void ModMigratorGUI::Run() {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

INT_PTR CALLBACK ModMigratorGUI::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (s_instance) {
        return s_instance->HandleMessage(hDlg, uMsg, wParam, lParam);
    }
    return FALSE;
}

INT_PTR ModMigratorGUI::HandleMessage(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        OnInit(hDlg);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_BROWSE_SOURCE:
            OnBrowseSource(hDlg);
            break;
        case IDC_BTN_BROWSE_OUTPUT:
            OnBrowseOutput(hDlg);
            break;
        case IDC_BTN_CONVERT:
            OnConvert(hDlg);
            break;
        case IDC_BTN_CLEAR_LOG:
            OnClearLog(hDlg);
            break;
        case IDC_BTN_OPEN_OUTPUT:
            OnOpenOutput(hDlg);
            break;
        case IDC_BTN_ABOUT:
            OnAbout(hDlg);
            break;
        case IDCANCEL:
            DestroyWindow(hDlg);
            break;
        }
        return TRUE;

    case WM_CLOSE:
        if (m_isConverting) {
            if (MessageBoxA(hDlg, "Conversion in progress. Are you sure you want to exit?", "Confirm Exit", MB_YESNO | MB_ICONQUESTION) == IDNO) {
                return TRUE;
            }
        }
        DestroyWindow(hDlg);
        return TRUE;

    case WM_DESTROY:
        PostQuitMessage(0);
        return TRUE;

    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wParam;
        char fileName[MAX_PATH];
        DragQueryFileA(hDrop, 0, fileName, MAX_PATH);
        DragFinish(hDrop);
        if (GetFileAttributesA(fileName) & FILE_ATTRIBUTE_DIRECTORY) {
            SetWindowTextA(m_hEditSource, fileName);
        }
        return TRUE;
    }
    }
    return FALSE;
}

void ModMigratorGUI::OnInit(HWND hDlg) {
    m_hDlg = hDlg;

    m_hEditSource = GetDlgItem(hDlg, IDC_EDIT_SOURCE);
    m_hEditOutput = GetDlgItem(hDlg, IDC_EDIT_OUTPUT);
    m_hEditLog = GetDlgItem(hDlg, IDC_EDIT_LOG);
    m_hProgress = GetDlgItem(hDlg, IDC_PROGRESS);
    m_hBtnConvert = GetDlgItem(hDlg, IDC_BTN_CONVERT);
    m_hCheckIncludeSource = GetDlgItem(hDlg, IDC_CHECK_INCLUDE_SOURCE);
    m_hCheckExportZip = GetDlgItem(hDlg, IDC_CHECK_EXPORT_ZIP);
    m_hCheckGenerateEvents = GetDlgItem(hDlg, IDC_CHECK_GENERATE_EVENTS);
    m_hCheckGenerateCaps = GetDlgItem(hDlg, IDC_CHECK_GENERATE_CAPS);

    // 默认勾选
    SendMessage(m_hCheckIncludeSource, BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(m_hCheckExportZip, BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(m_hCheckGenerateEvents, BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(m_hCheckGenerateCaps, BM_SETCHECK, BST_CHECKED, 0);

    // 进度条
    SendMessage(m_hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    // 支持拖放
    DragAcceptFiles(hDlg, TRUE);

    // 设置图标
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP_ICON));
    if (hIcon) {
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }

    AppendLog("[INFO] ModMigrator v1.0 started");
    AppendLog("[INFO] Select a Forge/NeoForge mod folder and output folder");
    AppendLog("[INFO] You can drag and drop a folder onto the source field");
}

void ModMigratorGUI::OnBrowseSource(HWND hDlg) {
    BROWSEINFOA bi = { 0 };
    bi.hwndOwner = hDlg;
    bi.lpszTitle = "Select Forge/NeoForge Mod Folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl) {
        char path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) {
            SetWindowTextA(m_hEditSource, path);
        }
        CoTaskMemFree(pidl);
    }
}

void ModMigratorGUI::OnBrowseOutput(HWND hDlg) {
    BROWSEINFOA bi = { 0 };
    bi.hwndOwner = hDlg;
    bi.lpszTitle = "Select Output Folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl) {
        char path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) {
            SetWindowTextA(m_hEditOutput, path);
        }
        CoTaskMemFree(pidl);
    }
}

void ModMigratorGUI::OnConvert(HWND hDlg) {
    if (m_isConverting) {
        AppendLog("[WARN] Conversion already in progress");
        return;
    }

    char sourcePath[MAX_PATH], outputPath[MAX_PATH];
    GetWindowTextA(m_hEditSource, sourcePath, MAX_PATH);
    GetWindowTextA(m_hEditOutput, outputPath, MAX_PATH);

    if (strlen(sourcePath) == 0 || strlen(outputPath) == 0) {
        MessageBoxA(hDlg, "Please select both source and output folders", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    m_currentOutputPath = outputPath;
    SetUIEnabled(false);
    SendMessage(m_hProgress, PBM_SETPOS, 0, 0);
    AppendLog("[INFO] Starting conversion...");
    AppendLog("[INFO] Source: " + std::string(sourcePath));
    AppendLog("[INFO] Output: " + std::string(outputPath));

    // 启动转换线程
    m_isConverting = true;
    m_convertThread = std::thread([this, sourcePath, outputPath]() {
        ModMigratorCore::ConvertOptions options;
        options.includeSource = (SendMessage(m_hCheckIncludeSource, BM_GETCHECK, 0, 0) == BST_CHECKED);
        options.exportZip = (SendMessage(m_hCheckExportZip, BM_GETCHECK, 0, 0) == BST_CHECKED);
        options.generateEvents = (SendMessage(m_hCheckGenerateEvents, BM_GETCHECK, 0, 0) == BST_CHECKED);
        options.generateCaps = (SendMessage(m_hCheckGenerateCaps, BM_GETCHECK, 0, 0) == BST_CHECKED);

        bool result = ModMigratorCore::Convert(sourcePath, outputPath, options,
            [this](const std::string& msg) { this->AppendLog(msg); },
            [this](int progress) { this->UpdateProgress(progress); });

        this->m_isConverting = false;
        this->SetUIEnabled(true);

        if (result) {
            this->AppendLog("[INFO] Conversion completed successfully!");
            this->AppendLog("[INFO] Output: " + std::string(outputPath));
            MessageBoxA(this->m_hDlg, "Conversion completed successfully!\n\nOpen output folder?", "Success", MB_YESNO | MB_ICONINFORMATION);
        }
        else {
            this->AppendLog("[ERROR] Conversion failed!");
            MessageBoxA(this->m_hDlg, "Conversion failed! Check the log for details.", "Error", MB_OK | MB_ICONERROR);
        }
        });
    m_convertThread.detach();
}

void ModMigratorGUI::OnClearLog(HWND hDlg) {
    SetWindowTextA(m_hEditLog, "");
}

void ModMigratorGUI::OnOpenOutput(HWND hDlg) {
    char outputPath[MAX_PATH];
    GetWindowTextA(m_hEditOutput, outputPath, MAX_PATH);

    if (strlen(outputPath) == 0) {
        MessageBoxA(hDlg, "Please select an output folder first", "Info", MB_OK | MB_ICONINFORMATION);
        return;
    }

    CreateDirectoryA(outputPath, NULL);
    ShellExecuteA(NULL, "open", outputPath, NULL, NULL, SW_SHOW);
}

void ModMigratorGUI::OnAbout(HWND hDlg) {
    MessageBoxA(hDlg,
        "ModMigrator v1.0\n\n"
        "Forge/NeoForge to MCreator Converter\n\n"
        "Features:\n"
        "  - Parse DeferredRegister, @ObjectHolder\n"
        "  - Parse @SubscribeEvent, @Mod.EventHandler\n"
        "  - Parse @CapabilityInject\n"
        "  - Parse Shaped/Shapeless/Smelting recipes\n"
        "  - Extract tags\n"
        "  - Generate .mcreator file\n\n"
        "Java 8+ required for parsing\n"
        "JavaParser 3.26.2, Gson 2.11.0\n\n"
        "License: MIT",
        "About ModMigrator", MB_OK | MB_ICONINFORMATION);
}

void ModMigratorGUI::AppendLog(const std::string& text) {
    EnterCriticalSection(&m_logCriticalSection);

    if (!m_hEditLog) {
        LeaveCriticalSection(&m_logCriticalSection);
        return;
    }

    std::string logLine = text + "\r\n";
    int len = GetWindowTextLengthA(m_hEditLog);
    SendMessageA(m_hEditLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageA(m_hEditLog, EM_REPLACESEL, FALSE, (LPARAM)logLine.c_str());
    SendMessageA(m_hEditLog, EM_SCROLLCARET, 0, 0);

    LeaveCriticalSection(&m_logCriticalSection);
}

void ModMigratorGUI::UpdateProgress(int value) {
    if (m_hProgress) {
        SendMessage(m_hProgress, PBM_SETPOS, (WPARAM)value, 0);
    }
}

void ModMigratorGUI::SetUIEnabled(bool enabled) {
    EnableWindow(m_hBtnConvert, enabled);
    EnableWindow(m_hEditSource, enabled);
    EnableWindow(m_hEditOutput, enabled);
    EnableWindow(m_hCheckIncludeSource, enabled);
    EnableWindow(m_hCheckExportZip, enabled);
    EnableWindow(m_hCheckGenerateEvents, enabled);
    EnableWindow(m_hCheckGenerateCaps, enabled);

    if (enabled) {
        SetWindowTextA(m_hBtnConvert, "Start Conversion");
    }
    else {
        SetWindowTextA(m_hBtnConvert, "Converting...");
    }
}