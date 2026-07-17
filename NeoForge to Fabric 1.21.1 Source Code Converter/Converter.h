#ifndef CONVERTER_H
#define CONVERTER_H

#include "utils.h"
#include "SymbolTable.h"
#include "ASTParser.h"
#include "RuleEngine.h"
#include "FileScanner.h"
#include "TemplateGenerator.h"
#include <memory>
#include <sstream>
#include <regex>
#include <fstream>

class Converter {
private:
    std::string inputPath;
    std::string outputPath;
    SymbolTable symbolTable;
    std::unique_ptr<RuleEngine> ruleEngine;
    std::string modid;
    std::string mainClass;
    std::string packageName;

    // 统计信息
    int filesProcessed = 0;
    int rulesApplied = 0;
    int manualFixesNeeded = 0;

    void log(const std::string& message) {
        std::cout << "[Converter] " << message << std::endl;
    }

    void logError(const std::string& message) {
        std::cerr << "[Error] " << message << std::endl;
    }

    // 转义 JSON 字符串中的特殊字符
    std::string escapeJsonString(const std::string& input) {
        std::string output;
        for (char c : input) {
            switch (c) {
            case '"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b"; break;
            case '\f': output += "\\f"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default: output += c; break;
            }
        }
        return output;
    }

    // 去除字符串首尾空格
    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    // 格式化作者列表
    std::string formatAuthorList(const std::string& authors) {
        if (authors == "Unknown" || authors.empty()) {
            return "[\"Conversion Tool\"]";
        }

        if (authors.find('"') != std::string::npos) {
            return "[" + authors + "]";
        }

        if (authors.find(',') != std::string::npos) {
            std::string result = "[";
            std::stringstream ss(authors);
            std::string author;
            bool first = true;
            while (std::getline(ss, author, ',')) {
                if (!first) result += ", ";
                result += "\"" + escapeJsonString(trim(author)) + "\"";
                first = false;
            }
            result += "]";
            return result;
        }

        return "[\"" + escapeJsonString(authors) + "\"]";
    }

    // 查找主Java文件
    fs::path findMainJavaFile() {
        fs::path javaDir = fs::path(inputPath) / "src/main/java";
        if (!fs::exists(javaDir)) return "";

        for (const auto& entry : fs::recursive_directory_iterator(javaDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".java") {
                std::string content = Utils::readFile(entry.path());
                if (content.find("@Mod") != std::string::npos) {
                    return entry.path();
                }
            }
        }

        for (const auto& entry : fs::recursive_directory_iterator(javaDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".java") {
                return entry.path();
            }
        }

        return "";
    }

    // 转换单个Java文件
    std::string convertJavaFile(const fs::path& filePath, const std::string& outputJavaDir) {
        log("Converting: " + filePath.string());

        std::string content = Utils::readFile(filePath);
        if (content.empty()) {
            logError("Empty or unreadable file: " + filePath.string());
            return "";
        }

        // 获取相对路径（保持原有目录结构）
        std::string filePathStr = filePath.string();
        std::string srcJavaPath = inputPath + "/src/main/java/";
        std::replace(srcJavaPath.begin(), srcJavaPath.end(), '\\', '/');
        std::replace(filePathStr.begin(), filePathStr.end(), '\\', '/');

        // 提取相对路径
        std::string relativePath;
        size_t pos = filePathStr.find(srcJavaPath);
        if (pos != std::string::npos) {
            relativePath = filePathStr.substr(pos + srcJavaPath.length());
        }
        else {
            relativePath = filePath.filename().string();
        }

        // 确保输出路径保持原有目录结构
        fs::path outPath = fs::path(outputJavaDir) / relativePath;

        // 先修复MCreator特有的数学函数
        content = Utils::fixMcreatorMath(content);

        // 步骤1: 解析AST获取类信息
        ASTParser parser(&symbolTable);
        ASTNode* ast = parser.parse(content);

        ClassInfo classInfo;
        parser.collectClassInfo(ast, classInfo);

        // 步骤2: 更新符号表
        if (!packageName.empty() && !classInfo.name.empty()) {
            std::string fullClassName = packageName + "." + classInfo.name;
            symbolTable.addClass(fullClassName, classInfo);
        }

        // 步骤3: 应用规则引擎
        std::string converted = ruleEngine->applyAllRules(content, classInfo.name);

        // 步骤4: 替换导入语句
        std::unordered_map<std::string, std::string> importMap = {
            {"net.neoforged.neoforge.items.IItemHandlerModifiable", "net.fabricmc.fabric.api.transfer.v1.item.ItemStorage"},
            {"net.neoforged.neoforge.items.IItemHandler", "net.fabricmc.fabric.api.transfer.v1.storage.Storage"},
            {"net.neoforged.neoforge.common.extensions.ILevelExtension", "net.minecraft.world.level.LevelAccessor"},
            {"net.neoforged.neoforge.capabilities.Capabilities", "net.fabricmc.fabric.api.transfer.v1.item.ItemStorage"},
            {"net.neoforged.fml.common.Mod", "net.fabricmc.api.ModInitializer"},
        };
        converted = Utils::replaceImports(converted, importMap);

        // 步骤5: 处理NeoForge特有语法
        converted = handleSpecialNeoForgeSyntax(converted);

        // 步骤6: 添加警告注释
        if (converted.find("neoforged") != std::string::npos ||
            converted.find("IItemHandler") != std::string::npos) {
            std::string warning = "/*\n"
                " * WARNING: This file was automatically converted from NeoForge to Fabric.\n"
                " * Manual review and fixes are REQUIRED!\n"
                " * The following APIs need manual conversion:\n"
                " *    - Item capabilities -> Fabric Transfer API\n"
                " *    - BlockEntity data -> Fabric Component API\n"
                " *    - Event handlers -> Fabric Event API\n"
                " */\n\n";
            converted = warning + converted;
            manualFixesNeeded++;
        }

        delete ast;

        // 返回转换后的内容和输出路径
        return converted;
    }

    // 处理NeoForge特有的语法结构
    std::string handleSpecialNeoForgeSyntax(const std::string& code) {
        std::string result = code;

        result = Utils::regexReplace(result,
            std::regex(R"(@Mod\.EventBusSubscriber[^{]*\{)"),
            "// @Mod.EventBusSubscriber converted to Fabric initialization");

        result = Utils::regexReplace(result,
            std::regex(R"(@OnlyIn\s*\(\s*Dist\.(\w+)\s*\))"),
            "@Environment(EnvType.$1)");

        if (result.find("@Environment") != std::string::npos) {
            if (result.find("import net.fabricmc.api.Environment") == std::string::npos) {
                result = "import net.fabricmc.api.Environment;\nimport net.fabricmc.api.EnvType;\n" + result;
            }
        }

        return result;
    }

    // 生成 fabric.mod.json
    void generateFabricModJson(const std::string& outputResourceDir) {
        std::string fabricModJsonPath = outputResourceDir + "fabric.mod.json";

        std::string modName = modid;
        std::string modVersion = "1.0.0";
        std::string modDescription = "Converted from NeoForge to Fabric";
        std::string modAuthors = "Unknown";
        std::string modLicense = "All Rights Reserved";
        std::string modIcon = "assets/" + modid + "/icon.png";

        // 1. 从 mods.toml 读取信息
        fs::path modsTomlPath = fs::path(inputPath) / "src/main/resources/META-INF/mods.toml";
        if (fs::exists(modsTomlPath)) {
            std::string tomlContent = Utils::readFile(modsTomlPath);

            std::regex nameRegex(R"(displayName\s*=\s*\"([^\"]+)\")");
            std::smatch nameMatch;
            if (std::regex_search(tomlContent, nameMatch, nameRegex)) {
                modName = nameMatch[1].str();
            }

            std::regex versionRegex(R"(version\s*=\s*\"([^\"]+)\")");
            std::smatch versionMatch;
            if (std::regex_search(tomlContent, versionMatch, versionRegex)) {
                modVersion = versionMatch[1].str();
            }

            std::regex descRegex(R"(description\s*=\s*\"([^\"]+)\")");
            std::smatch descMatch;
            if (std::regex_search(tomlContent, descMatch, descRegex)) {
                modDescription = descMatch[1].str();
            }

            std::regex licenseRegex(R"(license\s*=\s*\"([^\"]+)\")");
            std::smatch licenseMatch;
            if (std::regex_search(tomlContent, licenseMatch, licenseRegex)) {
                modLicense = licenseMatch[1].str();
            }

            log("Read mod info from mods.toml: name=" + modName + ", version=" + modVersion);
        }

        // 2. 从主类文件读取 @Mod 注解
        fs::path mainJavaPath = findMainJavaFile();
        if (!mainJavaPath.empty() && fs::exists(mainJavaPath)) {
            std::string javaContent = Utils::readFile(mainJavaPath);

            std::regex modAnnotationRegex(R"(@Mod\s*\(\s*\"([^\"]+)\"\s*(?:,\s*name\s*=\s*\"([^\"]+)\"\s*)?\))");
            std::smatch modMatch;
            if (std::regex_search(javaContent, modMatch, modAnnotationRegex)) {
                if (modMatch.size() > 1 && !modMatch[1].str().empty()) {
                    modid = modMatch[1].str();
                }
                if (modMatch.size() > 2 && !modMatch[2].str().empty()) {
                    modName = modMatch[2].str();
                }
            }

            log("Read mod info from Java: modid=" + modid + ", name=" + modName);
        }

        // 3. 从 pack.mcmeta 读取描述
        fs::path packMcmetaPath = fs::path(inputPath) / "src/main/resources/pack.mcmeta";
        if (fs::exists(packMcmetaPath)) {
            std::string packContent = Utils::readFile(packMcmetaPath);
            std::regex descriptionRegex("\"description\"\\s*:\\s*\"([^\"]+)\"");
            std::smatch packDescMatch;
            if (std::regex_search(packContent, packDescMatch, descriptionRegex)) {
                if (modDescription == "Converted from NeoForge to Fabric") {
                    modDescription = packDescMatch[1].str() + " (Converted)";
                }
            }
        }

        // 4. 从 gradle.properties 读取版本
        fs::path gradlePropsPath = fs::path(inputPath) / "gradle.properties";
        if (fs::exists(gradlePropsPath)) {
            std::string gradleContent = Utils::readFile(gradlePropsPath);
            std::regex versionRegex(R"(mod_version\s*=\s*([^\s]+))");
            std::smatch gradleVersionMatch;
            if (std::regex_search(gradleContent, gradleVersionMatch, versionRegex)) {
                modVersion = gradleVersionMatch[1].str();
            }
        }

        // 5. 清理包名
        std::string cleanPackage = packageName;
        size_t blockPos = cleanPackage.find(".block");
        if (blockPos != std::string::npos) {
            cleanPackage = cleanPackage.substr(0, blockPos);
        }
        size_t proceduresPos = cleanPackage.find(".procedures");
        if (proceduresPos != std::string::npos) {
            cleanPackage = cleanPackage.substr(0, proceduresPos);
        }

        // 6. 确定主类名
        std::string mainClassName = mainClass.empty() ? "ConvertedMod" : mainClass;
        size_t lastDot = mainClassName.find_last_of('.');
        if (lastDot != std::string::npos) {
            mainClassName = mainClassName.substr(lastDot + 1);
        }

        // 7. 生成 fabric.mod.json
        std::string content = std::string() +
            "{\n" +
            "  \"schemaVersion\": 1,\n" +
            "  \"id\": \"" + modid + "\",\n" +
            "  \"version\": \"" + modVersion + "\",\n" +
            "  \"name\": \"" + escapeJsonString(modName) + "\",\n" +
            "  \"description\": \"" + escapeJsonString(modDescription) + "\",\n" +
            "  \"authors\": " + formatAuthorList(modAuthors) + ",\n" +
            "  \"contact\": {},\n" +
            "  \"license\": \"" + escapeJsonString(modLicense) + "\",\n" +
            "  \"icon\": \"" + modIcon + "\",\n" +
            "  \"environment\": \"*\",\n" +
            "  \"entrypoints\": {\n" +
            "    \"main\": [\n" +
            "      \"" + cleanPackage + "." + mainClassName + "\"\n" +
            "    ]\n" +
            "  },\n" +
            "  \"depends\": {\n" +
            "    \"fabricloader\": \">=0.15.0\",\n" +
            "    \"minecraft\": \"~1.21.1\",\n" +
            "    \"java\": \">=21\",\n" +
            "    \"fabric-api\": \"*\"\n" +
            "  },\n" +
            "  \"suggests\": {\n" +
            "    \"modmenu\": \"*\"\n" +
            "  }\n" +
            "}\n";

        std::ofstream outFile(fabricModJsonPath);
        if (!outFile.is_open()) {
            logError("Failed to create fabric.mod.json at: " + fabricModJsonPath);
            return;
        }
        outFile << content;
        outFile.close();

        log("Generated fabric.mod.json at: " + fabricModJsonPath);
        log("  Mod ID: " + modid);
        log("  Version: " + modVersion);
        log("  Name: " + modName);
        log("  Main Class: " + cleanPackage + "." + mainClassName);
    }

    // 复制并转换资源文件
    void convertResources(const std::vector<fs::path>& resources, const std::string& outputResourceDir) {

        for (const auto& resPath : resources) {
            std::string resPathStr = resPath.string();
            std::replace(resPathStr.begin(), resPathStr.end(), '\\', '/');

            std::string relativePath;
            size_t pos = resPathStr.find("src/main/resources/");
            if (pos != std::string::npos) {
                relativePath = resPathStr.substr(pos + 18);
            }
            else {
                relativePath = resPath.filename().string();
            }

            if (!relativePath.empty() && (relativePath[0] == '/' || relativePath[0] == '\\')) {
                relativePath = relativePath.substr(1);
            }

            // 跳过NeoForge特有文件
            if (relativePath.find("neoforge/") != std::string::npos) {
                log("Skipping NeoForge-specific: " + relativePath);
                continue;
            }

            if (relativePath == "META-INF/mods.toml") {
                log("Skipping mods.toml");
                continue;
            }

            std::string outputPathStr = outputResourceDir;
            if (!outputPathStr.empty() && outputPathStr.back() != '/' && outputPathStr.back() != '\\') {
                outputPathStr += "/";
            }
            outputPathStr += relativePath;
            fs::path outPath = fs::path(outputPathStr);

            try {
                fs::create_directories(outPath.parent_path());

                std::string content = Utils::readFile(resPath);
                bool contentChanged = false;

                // 处理标签文件
                if (relativePath.find("tags/") != std::string::npos ||
                    relativePath.find("data/minecraft/tags/") != std::string::npos) {

                    std::string newContent = Utils::regexReplace(content, std::regex("\"block/\""), "\"blocks/\"");
                    if (newContent != content) { content = newContent; contentChanged = true; }

                    newContent = Utils::regexReplace(content, std::regex("\"item/\""), "\"items/\"");
                    if (newContent != content) { content = newContent; contentChanged = true; }

                    newContent = Utils::regexReplace(content, std::regex("#block:"), "#blocks:");
                    if (newContent != content) { content = newContent; contentChanged = true; }

                    newContent = Utils::regexReplace(content, std::regex("#item:"), "#items:");
                    if (newContent != content) { content = newContent; contentChanged = true; }

                    if (contentChanged) log("Converted tags: " + relativePath);
                }

                // 处理配方文件
                if (relativePath.find("recipe/") != std::string::npos && relativePath.find(".json") != std::string::npos) {
                    std::string newContent = Utils::regexReplace(content, std::regex("\"tag\":\\s*\"block/\""), "\"tag\": \"blocks/");
                    if (newContent != content) { content = newContent; contentChanged = true; }

                    newContent = Utils::regexReplace(content, std::regex("\"tag\":\\s*\"item/\""), "\"tag\": \"items/");
                    if (newContent != content) { content = newContent; contentChanged = true; }

                    if (contentChanged) log("Converted recipe: " + relativePath);
                }

                // 处理战利品表和进度
                if ((relativePath.find("loot_table/") != std::string::npos ||
                    relativePath.find("advancement/") != std::string::npos) &&
                    relativePath.find(".json") != std::string::npos) {

                    std::string newContent = Utils::regexReplace(content, std::regex("\"condition\":\\s*\"neoforge:\""), "\"condition\": \"fabric:");
                    if (newContent != content) { content = newContent; contentChanged = true; }

                    newContent = Utils::regexReplace(content, std::regex("\"trigger\":\\s*\"neoforge:\""), "\"trigger\": \"fabric:");
                    if (newContent != content) { content = newContent; contentChanged = true; }

                    if (contentChanged) log("Converted loot/advancement: " + relativePath);
                }

                // 处理语言文件
                if (relativePath.find("lang/") != std::string::npos && relativePath.find(".json") != std::string::npos) {
                    if (!modid.empty()) {
                        std::string newContent = Utils::replace(content, "examplemod", modid);
                        if (newContent != content) { content = newContent; contentChanged = true; }
                        newContent = Utils::replace(content, "your_mod_id", modid);
                        if (newContent != content) { content = newContent; contentChanged = true; }
                        if (contentChanged) log("Updated language: " + relativePath);
                    }
                }

                if (contentChanged) {
                    std::ofstream outFile(outPath);
                    outFile << content;
                    outFile.close();
                    log("Written (converted): " + relativePath);
                }
                else {
                    fs::copy_file(resPath, outPath, fs::copy_options::overwrite_existing);
                    log("Copied: " + relativePath);
                }

            }
            catch (const std::exception& e) {
                logError("Failed to process: " + relativePath + " - " + e.what());
            }
        }
    }

    // 生成Fabric项目结构
    void generateFabricProject(const std::string& fabricResourceDir) {
        // 生成 fabric.mod.json
        generateFabricModJson(fabricResourceDir);

        // 生成 mixins.json
        std::string mixinsPath = outputPath + "mixins.json";
        std::string mixinsContent = std::string() +
            "{\n" +
            "  \"required\": true,\n" +
            "  \"minVersion\": \"0.8\",\n" +
            "  \"package\": \"" + packageName + "/mixin\",\n" +
            "  \"compatibilityLevel\": \"JAVA_17\",\n" +
            "  \"mixins\": [],\n" +
            "  \"client\": [],\n" +
            "  \"server\": []\n" +
            "}\n";
        Utils::writeFile(mixinsPath, mixinsContent);

        // 生成 build.gradle
        std::string gradlePath = outputPath + "build.gradle";
        std::string gradleContent = std::string() +
            "plugins {\n" +
            "    id 'fabric-loom' version '1.7-SNAPSHOT'\n" +
            "    id 'maven-publish'\n" +
            "}\n" +
            "\n" +
            "version = project.mod_version\n" +
            "group = project.maven_group\n" +
            "\n" +
            "repositories {\n" +
            "    mavenCentral()\n" +
            "    maven {\n" +
            "        name = \"Fabric\"\n" +
            "        url = \"https://maven.fabricmc.net/\"\n" +
            "    }\n" +
            "}\n" +
            "\n" +
            "dependencies {\n" +
            "    minecraft \"com.mojang:minecraft:${project.minecraft_version}\"\n" +
            "    mappings \"net.fabricmc:yarn:${project.yarn_mappings}:v2\"\n" +
            "    modImplementation \"net.fabricmc:fabric-loader:${project.loader_version}\"\n" +
            "    modImplementation \"net.fabricmc.fabric-api:fabric-api:${project.fabric_version}\"\n" +
            "}\n" +
            "\n" +
            "processResources {\n" +
            "    inputs.property \"version\", project.version\n" +
            "    filteringCharset \"UTF-8\"\n" +
            "    filesMatching(\"fabric.mod.json\") {\n" +
            "        expand \"version\": project.version\n" +
            "    }\n" +
            "}\n" +
            "\n" +
            "tasks.withType(JavaCompile).configureEach {\n" +
            "    it.options.release = 21\n" +
            "}\n" +
            "\n" +
            "java {\n" +
            "    withSourcesJar()\n" +
            "    sourceCompatibility = JavaVersion.VERSION_21\n" +
            "    targetCompatibility = JavaVersion.VERSION_21\n" +
            "}\n";
        Utils::writeFile(gradlePath, gradleContent);

        // 生成 gradle.properties
        std::string gradlePropsPath = outputPath + "gradle.properties";
        std::string gradlePropsContent = std::string() +
            "mod_version=1.0.0\n" +
            "maven_group=com.example\n" +
            "minecraft_version=1.21.1\n" +
            "yarn_mappings=1.21.1+build.1\n" +
            "loader_version=0.15.11\n" +
            "fabric_version=0.100.0+1.21.1\n";
        Utils::writeFile(gradlePropsPath, gradlePropsContent);

        // 生成 settings.gradle
        std::string settingsPath = outputPath + "settings.gradle";
        std::string settingsContent = std::string() +
            "pluginManagement {\n" +
            "    repositories {\n" +
            "        maven {\n" +
            "            name = 'Fabric'\n" +
            "            url = 'https://maven.fabricmc.net/'\n" +
            "        }\n" +
            "        mavenCentral()\n" +
            "        gradlePluginPortal()\n" +
            "    }\n" +
            "}\n";
        Utils::writeFile(settingsPath, settingsContent);

        log("Fabric project structure generated at: " + outputPath);
    }

public:
    Converter(const std::string& input, const std::string& output)
        : inputPath(input), outputPath(output) {

        if (outputPath.back() != '/' && outputPath.back() != '\\') {
            outputPath += "/";
        }

        ruleEngine = std::make_unique<RuleEngine>(&symbolTable);
    }

    void convert() {
        log("=== NeoForge to Fabric Converter ===");
        log("Input: " + inputPath);
        log("Output: " + outputPath);

        // 步骤1: 扫描并提取mod信息
        FileScanner scanner(&symbolTable, inputPath);
        scanner.scan();
        modid = scanner.getModId();
        mainClass = scanner.getMainClass();

        log("Detected Mod ID: " + modid);
        log("Detected Main Class: " + (mainClass.empty() ? "(auto-detect)" : mainClass));

        // 步骤2: 获取包名
        auto javaFiles = scanner.getJavaFiles();
        if (!javaFiles.empty()) {
            std::string firstJavaContent = Utils::readFile(javaFiles[0]);
            packageName = Utils::extractPackage(firstJavaContent);
            if (packageName.empty()) {
                packageName = "com.converted.mod";
            }
        }
        else {
            packageName = "com.converted.mod";
        }

        log("Package Name: " + packageName);

        // 步骤3: 设置输出目录
        std::string fabricJavaDir = outputPath + "src/main/java/" + Utils::replace(packageName, ".", "/");
        std::string fabricResourceDir = outputPath + "src/main/resources/";

        fs::create_directories(fabricJavaDir);
        fs::create_directories(fabricResourceDir);

        // 步骤4: 转换Java文件
        for (const auto& javaFile : javaFiles) {
            std::string convertedContent = convertJavaFile(javaFile);
            if (!convertedContent.empty()) {
                std::string fileName = javaFile.filename().string();
                fs::path outPath = fabricJavaDir + "/" + fileName;
                Utils::writeFile(outPath, convertedContent);
                filesProcessed++;
                log("Written: " + outPath.string());
            }
        }

        // 步骤5: 转换资源文件
        auto resources = scanner.getResourceFiles();
        convertResources(resources, fabricResourceDir);

        // 步骤6: 生成Fabric项目文件
        generateFabricProject(fabricResourceDir);

        // 步骤7: 输出报告
        printReport();
    }

    void printReport() {
        log("\n=== Conversion Report ===");
        log("Files processed: " + std::to_string(filesProcessed));
        log("Manual fixes needed: " + std::to_string(manualFixesNeeded));
        log("\nWARNING: This is an automated conversion.");
        log("Please review all generated code, especially marked with 'TODO' or 'NEEDS MANUAL FIX'.");
        log("Check the Fabric API documentation for proper event and registry usage.");
        log("\nNext steps:");
        log("1. cd " + outputPath);
        log("2. Review and fix all TODO comments");
        log("3. Run './gradlew build' to compile");
        log("4. Test thoroughly in-game");
    }
};

#endif // CONVERTER_H