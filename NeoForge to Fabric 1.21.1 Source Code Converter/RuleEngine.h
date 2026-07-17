#ifndef RULE_ENGINE_H
#define RULE_ENGINE_H

#include "utils.h"
#include "SymbolTable.h"
#include <functional>
#include <queue>
#include <regex>
#include <string>
#include <iostream>
#include <unordered_set>
#include <unordered_map>

struct ConversionRule {
    std::string name;
    std::regex pattern;
    std::function<std::string(const std::string&, SymbolTable*, const std::string&)> transform;
    int priority;
    std::string sourceContext;
    bool appliedOnce;

    ConversionRule(const std::string& n, const std::regex& p,
        std::function<std::string(const std::string&, SymbolTable*, const std::string&)> t,
        int prio = 100, const std::string& ctx = "", bool once = false)
        : name(n), pattern(p), transform(t), priority(prio), sourceContext(ctx), appliedOnce(once) {
    }

    bool operator<(const ConversionRule& other) const {
        return priority > other.priority;
    }
};

class RuleEngine {
private:
    std::priority_queue<ConversionRule> rules;
    SymbolTable* symbolTable;
    std::unordered_map<std::string, std::string> temporaryCache;
    std::unordered_set<std::string> appliedRules;

public:
    RuleEngine(SymbolTable* st) : symbolTable(st) {
        initializeRules();
    }

    void initializeRules() {
        while (!rules.empty()) rules.pop();
        appliedRules.clear();

        // ========== 1. 删除所有 NeoForge 导入 ==========
        rules.push(ConversionRule("DeleteNeoForgeImports",
            std::regex(R"(import net\.neoforged\.[^\n]+;\n)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string { return ""; },
            1, "", true));

        // ========== 2. 删除 @EventBusSubscriber ==========
        rules.push(ConversionRule("DeleteEventBusSubscriber",
            std::regex(R"(@EventBusSubscriber\s*\n)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string { return ""; },
            2, "", true));

        // ========== 3. 删除 @SubscribeEvent ==========
        rules.push(ConversionRule("DeleteSubscribeEvent",
            std::regex(R"(@SubscribeEvent\s*\n)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string { return ""; },
            3, "", true));

        // ========== 4. 删除被注释的 REGISTRY 声明 ==========
        rules.push(ConversionRule("DeleteCommentedRegistry",
            std::regex(R"(//\s*public static final DeferredRegister\.[^\n]+;\n)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string { return ""; },
            4, "", true));

        // ========== 5. 删除 Capabilities 注释块 ==========
        rules.push(ConversionRule("DeleteCapabilitiesBlock",
            std::regex(R"(// Fabric: Capabilities are replaced by Component API[^\n]*\n// Register components in ModInitializer:[^\n]*\n(?:// [^\n]*\n)*)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string { return ""; },
            5, "", true));

        // ========== 6. 转换 DeferredItem 为 Item ==========
        rules.push(ConversionRule("ConvertDeferredItem",
            std::regex(R"(public static final DeferredItem<Item>\s+(\w+);)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                std::smatch match;
                if (std::regex_search(code, match, std::regex(R"(public static final DeferredItem<Item>\s+(\w+);)"))) {
                    return "public static Item " + match[1].str() + ";";
                }
                return code;
            }, 10, "", true));

        // ========== 7. 转换静态块中的注册调用 ==========
        rules.push(ConversionRule("ConvertRegistryRegister",
            std::regex(R"((\w+)\s*=\s*REGISTRY\.register\s*\(\s*\"([^\"]+)\"\s*,\s*([^:;]+)(?:::new)?\s*\)\s*;)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                std::smatch match;
                if (std::regex_search(code, match, std::regex(R"((\w+)\s*=\s*REGISTRY\.register\s*\(\s*\"([^\"]+)\"\s*,\s*([^:;]+)(?:::new)?\s*\)\s*;)"))) {
                    return match[1].str() + " = Registry.register(Registries.ITEM, Identifier.of(MOD_ID, \"" + match[2].str() + "\"), new " + match[3].str() + "(new FabricItemSettings()));";
                }
                return code;
            }, 11, "", true));

        // ========== 8. 转换 block() 辅助方法 ==========
        rules.push(ConversionRule("ConvertBlockHelper",
            std::regex(R"(private static DeferredItem<Item> block\s*\([^)]+\)\s*\{[^}]*\})"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                return "private static Item blockItem(Block block, Item.Settings settings) {\n    return new BlockItem(block, settings);\n}";
            }, 20, "", true));

        // ========== 9. 转换 block() 调用 ==========
        rules.push(ConversionRule("ConvertBlockCall",
            std::regex(R"(=\s*block\s*\(([^,)]+)(?:,\s*([^)]+))?\s*\))"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                std::smatch match;
                if (std::regex_search(code, match, std::regex(R"(=\s*block\s*\(([^,)]+)(?:,\s*([^)]+))?\s*\))"))) {
                    if (match.size() > 2 && match[2].matched) {
                        return "= blockItem(" + match[1].str() + ".get(), " + match[2].str() + ")";
                    }
                    else {
                        return "= blockItem(" + match[1].str() + ".get(), new Item.Settings())";
                    }
                }
                return code;
            }, 21, "", false));

        // ========== 10. 修复重复的 public static final ==========
        rules.push(ConversionRule("FixDuplicatePublicStatic",
            std::regex(R"(public static final\s+public static final)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                return "public static final";
            }, 30, "", false));

        // ========== 11. 修复双分号 ==========
        rules.push(ConversionRule("FixDoubleSemicolon",
            std::regex(R"(;;)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                return ";";
            }, 31, "", false));

        // ========== 12. 清理奇怪的空格和缩进 ==========
        rules.push(ConversionRule("CleanupIndent",
            std::regex(R"(^\s+static \{)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                return "static {";
            }, 40, "", false));

        // ========== 13. 添加 Fabric 导入 ==========
        rules.push(ConversionRule("AddFabricImports",
            std::regex(R"(package\s+[a-zA-Z0-9_.]+;)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                std::string imports = "\n\nimport net.minecraft.registry.Registry;\n"
                    "import net.minecraft.registry.Registries;\n"
                    "import net.minecraft.util.Identifier;\n"
                    "import net.fabricmc.fabric.api.item.v1.FabricItemSettings;\n"
                    "import net.minecraft.world.item.Item;\n"
                    "import net.minecraft.world.item.BlockItem;\n"
                    "import net.minecraft.world.level.block.Block;\n";
                if (code.find("import net.minecraft.registry.Registry") != std::string::npos) {
                    return code;
                }
                size_t insertPos = code.find(";") + 1;
                return code.substr(0, insertPos) + imports + code.substr(insertPos);
            }, 100, "", true));

        // ========== 14. 最终清理空行 ==========
        rules.push(ConversionRule("FinalCleanup",
            std::regex(R"(\n{3,})"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                return "\n\n";
            }, 200, "", false));

        // ========== 15. 转换 REGISTRY.register 为 Fabric Registry.register ==========
        rules.push(ConversionRule(
            "ConvertRegistryRegisterCalls",
            std::regex(R"((\w+)\s*=\s*REGISTRY\.register\s*\(\s*\"([^\"]+)\"\s*,\s*(\w+)::new\s*\)\s*;)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                std::smatch match;
                if (std::regex_search(code, match, std::regex(R"((\w+)\s*=\s*REGISTRY\.register\s*\(\s*\"([^\"]+)\"\s*,\s*(\w+)::new\s*\)\s*;)"))) {
                    return match[1].str() + " = Registry.register(Registries.ITEM, Identifier.of(MOD_ID, \"" + match[2].str() + "\"), new " + match[3].str() + "(new FabricItemSettings()));";
                }
                return code;
            },
            12, "", true
        ));

        // ========== 16. 转换 blockItem 调用中的 REGISTRY.register 格式 ==========
        rules.push(ConversionRule(
            "ConvertBlockItemRegistryCall",
            std::regex(R"(ASPHALT_MIXING_PLANT\s*=\s*blockItem\([^,]+,\s*new Item\.Settings\(\)\))"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                return "ASPHALT_MIXING_PLANT = blockItem(ImmersionInTrafficContextModBlocks.ASPHALT_MIXING_PLANT.get(), new Item.Settings())";
            },
            12, "", true
        ));

        // ========== 17. 删除重复的 blockItem 方法 ==========
        rules.push(ConversionRule(
            "RemoveDuplicateBlockItem",
            std::regex(R"(private static Item blockItem\(Block block, Item\.Settings settings\) \{\n    return new BlockItem\(block, settings\);\n\}\n\nprivate static Item blockItem\(Block block, Item\.Settings settings\) \{\n    return new BlockItem\(block, settings\);\n\})"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                return "private static Item blockItem(Block block, Item.Settings settings) {\n    return new BlockItem(block, settings);\n}";
            },
            25, "", true
        ));

        // ========== 18. 将 registerCapabilities 方法转换为注释 ==========
        rules.push(ConversionRule(
            "ConvertCapabilitiesMethodToComment",
            std::regex(R"(@SubscribeEvent\s*\n\s*public static void registerCapabilities\([^)]+\)\s*\{[^{]*\{[^}]*\}[^}]*\})"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                return "/*\n"
                    " * @SubscribeEvent - Capabilities registration converted to Fabric Component API\n"
                    " * The following capabilities need manual conversion:\n"
                    " *   - RoadEngineeringInventoryCapability\n"
                    " *   - IndustrialFurnaceUserManualInventoryCapability\n"
                    " *   - FluidBucketWrapper (for fluid buckets)\n"
                    " * \n"
                    " * In Fabric, use Component API instead:\n"
                    " *   public static final ComponentType<MyComponent> MY_COMPONENT = \n"
                    " *       ComponentType.<MyComponent>builder()\n"
                    " *           .persistent(MyComponent.CODEC)\n"
                    " *           .build(Identifier.of(MOD_ID, \"my_component\"));\n"
                    " */\n"
                    "// Original capabilities registration code has been removed.\n"
                    "// Please reimplement using Fabric Component API.";
            },
            13, "", true
        ));

        // ========== 19. 删除 DeferredRegister.Items REGISTRY 声明 ==========
        rules.push(ConversionRule(
            "DeleteRegistryDeclaration",
            std::regex(R"(public static final DeferredRegister\.Items REGISTRY = DeferredRegister\.createItems\([^;]+\);\n)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                return "// Fabric: Removed DeferredRegister. Use direct Registry.register calls.\n";
            },
            6, "", true
        ));

        // ========== 20. 删除多余的导入（FluidBucketWrapper, Capabilities 等）==========
        rules.push(ConversionRule(
            "DeleteUnusedImports",
            std::regex(R"(import net\.neoforged\.neoforge\.fluids\.capability\.wrappers\.FluidBucketWrapper;\n)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                return "";
            },
            1, "", true
        ));

        // ========== 21. 修复 Item.Settings 构造器 ==========
        rules.push(ConversionRule(
            "FixItemSettings",
            std::regex(R"(new Item\.Settings\(\)(?!\.))"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                return "new Item.Settings()";
            },
            50, "", false
        ));

        // ========== 22. 确保 Fabric 导入存在 ==========
        rules.push(ConversionRule(
            "EnsureAllFabricImports",
            std::regex(R"(package\s+[a-zA-Z0-9_.]+;)"),
            [](const std::string& code, SymbolTable* st, const std::string& context) -> std::string {
                std::string neededImports = "";

                if (code.find("Registry.register") != std::string::npos) {
                    neededImports += "import net.minecraft.registry.Registry;\n";
                    neededImports += "import net.minecraft.registry.Registries;\n";
                }
                if (code.find("Identifier.of") != std::string::npos) {
                    neededImports += "import net.minecraft.util.Identifier;\n";
                }
                if (code.find("FabricItemSettings") != std::string::npos) {
                    neededImports += "import net.fabricmc.fabric.api.item.v1.FabricItemSettings;\n";
                }
                if (code.find("BlockItem") != std::string::npos) {
                    neededImports += "import net.minecraft.world.item.BlockItem;\n";
                }

                if (neededImports.empty()) return code;

                size_t insertPos = code.find(";") + 1;
                if (code.find("import net.minecraft.registry.Registry") != std::string::npos) {
                    return code;
                }

                return code.substr(0, insertPos) + "\n" + neededImports + code.substr(insertPos);
            },
            100, "", true
        ));
    }

    std::string applyAllRules(const std::string& code, const std::string& contextClass = "", const std::string& contextMethod = "") {
        std::string result = code;

        std::vector<ConversionRule> rulesList;
        std::priority_queue<ConversionRule> tempQueue = rules;
        while (!tempQueue.empty()) {
            rulesList.push_back(tempQueue.top());
            tempQueue.pop();
        }

        for (const auto& rule : rulesList) {
            if (rule.appliedOnce && appliedRules.find(rule.name) != appliedRules.end()) {
                continue;
            }

            if (!rule.sourceContext.empty()) {
                if (!contextClass.empty() && rule.sourceContext != contextClass) {
                    continue;
                }
            }

            try {
                std::string newResult;
                std::string remaining = result;
                std::smatch match;
                bool ruleApplied = false;

                while (std::regex_search(remaining, match, rule.pattern)) {
                    newResult += match.prefix().str();
                    newResult += rule.transform(match.str(), symbolTable, contextClass);
                    remaining = match.suffix().str();
                    ruleApplied = true;
                }
                newResult += remaining;

                if (ruleApplied) {
                    result = newResult;
                    if (rule.appliedOnce) {
                        appliedRules.insert(rule.name);
                    }
                }
            }
            catch (const std::exception& e) {
                // 忽略错误
            }
        }

        result = std::regex_replace(result, std::regex("\n{3,}"), "\n\n");
        return result;
    }

    std::string getCached(const std::string& key) {
        if (temporaryCache.find(key) != temporaryCache.end()) {
            return temporaryCache[key];
        }
        return "";
    }
};

#endif // RULE_ENGINE_H