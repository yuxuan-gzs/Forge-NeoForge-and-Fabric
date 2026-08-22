// main.cpp
#include "ModMigratorGUI.h"
#pragma comment(lib,"comctl32.lib")
#include <windows.h>

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 初始化公共控件
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_PROGRESS_CLASS | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);

    // 创建并运行图形界面
    ModMigratorGUI gui;
    if (!gui.Create(hInstance, nCmdShow)) {
        MessageBoxA(NULL, "Failed to create main window", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    gui.Run();

    return 0;
}