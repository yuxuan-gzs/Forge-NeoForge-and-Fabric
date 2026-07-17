#include "Converter.h"
#include <iostream>
#include <string>
#include <filesystem>

void printBanner() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║     NeoForge to Fabric 1.21.1 Source Code Converter          ║
║     Version 1.0 - Automated Translation Tool                 ║
║     ⚠️  WARNING: Experimental - Requires Manual Review       ║
╚══════════════════════════════════════════════════════════════╝
)" << std::endl;
}

void printUsage() {
    std::cout << "Usage: converter.exe <input_neoforge_project_path> <output_fabric_project_path>" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  converter.exe ./MyNeoForgeMod ./MyFabricMod" << std::endl;
    std::cout << std::endl;
    std::cout << "Requirements:" << std::endl;
    std::cout << "  - Input must be a valid NeoForge 1.21.1 mod source directory" << std::endl;
    std::cout << "  - Standard Maven project structure (src/main/java, src/main/resources)" << std::endl;
    std::cout << "  - C++17 compatible compiler" << std::endl;
}

int main(int argc, char* argv[]) {
    printBanner();

    if (argc != 3) {
        printUsage();
        return 1;
    }

    std::string inputPath = argv[1];
    std::string outputPath = argv[2];

    // 检查输入路径是否存在
    if (!std::filesystem::exists(inputPath)) {
        std::cerr << "Error: Input path does not exist: " << inputPath << std::endl;
        return 1;
    }

    // 检查是否是目录
    if (!std::filesystem::is_directory(inputPath)) {
        std::cerr << "Error: Input path is not a directory: " << inputPath << std::endl;
        return 1;
    }

    std::cout << "Starting conversion..." << std::endl;
    std::cout << "Input:  " << inputPath << std::endl;
    std::cout << "Output: " << outputPath << std::endl;
    std::cout << std::endl;

    try {
        Converter converter(inputPath, outputPath);
        converter.convert();

        std::cout << std::endl;
        std::cout << "Conversion completed!" << std::endl;
        std::cout << "Please review the generated code carefully before building." << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}