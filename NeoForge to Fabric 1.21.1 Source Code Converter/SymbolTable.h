#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "utils.h"
#include <unordered_map>
#include <unordered_set>
#include <variant>

struct ClassInfo {
    std::string package;
    std::string name;
    std::string superClass;
    std::vector<std::string> interfaces;
    std::unordered_map<std::string, std::string> fields; // fieldName -> type
    std::unordered_map<std::string, std::string> methods; // methodSignature -> returnType
    bool isEventSubscriber = false;
    std::vector<std::string> eventHandlers; // 事件处理方法名
};

struct ModInfo {
    std::string modid;
    std::string mainClass;
    std::string name;
    std::string version;
    std::vector<std::string> dependencies;
};

class SymbolTable {
private:
    std::unordered_map<std::string, ClassInfo> classes; // 全限定名 -> ClassInfo
    std::unordered_map<std::string, ModInfo> mods;
    std::unordered_map<std::string, std::string> aliasToFullName; // 简单类名 -> 全限定名
    std::unordered_set<std::string> knownNeoForgeClasses;
    std::unordered_set<std::string> knownFabricClasses;

public:
    SymbolTable() {
        // 初始化已知的NeoForge类映射
        initNeoForgeClasses();
        initFabricClasses();
    }

    void initNeoForgeClasses() {
        knownNeoForgeClasses = {
            "net.neoforged.neoforge.common.NeoForge",
            "net.neoforged.neoforge.event.entity.living.LivingEvent",
            "net.neoforged.neoforge.event.entity.living.LivingHurtEvent",
            "net.neoforged.neoforge.event.entity.player.PlayerEvent",
            "net.neoforged.bus.api.SubscribeEvent",
            "net.neoforged.fml.common.Mod",
            "net.neoforged.fml.javafmlmod.FMLJavaModLoadingContext",
            "net.neoforged.fml.event.lifecycle.FMLCommonSetupEvent",
            "net.neoforged.fml.event.lifecycle.FMLClientSetupEvent",
            "net.neoforged.neoforge.registries.DeferredRegister",
            "net.neoforged.neoforge.common.ModConfigSpec",
            "net.neoforged.neoforge.network.NetworkRegistry",
            // 更多...
        };
    }

    void initFabricClasses() {
        knownFabricClasses = {
            "net.fabricmc.fabric.api.event.lifecycle.v1.ServerLifecycleEvents",
            "net.fabricmc.fabric.api.event.player.PlayerBlockBreakEvents",
            "net.fabricmc.fabric.api.networking.v1.ServerPlayNetworking",
            "net.fabricmc.fabric.api.itemgroup.v1.ItemGroupEvents",
            "net.fabricmc.fabric.api.object.builder.v1.entity.FabricDefaultAttributeRegistry",
            "net.fabricmc.api.ModInitializer",
            "net.fabricmc.fabric.api.client.rendering.v1.EntityRendererRegistry",
            // 更多...
        };
    }

    void addClass(const std::string& fullName, const ClassInfo& info) {
        classes[fullName] = info;
        // 提取简单类名作为别名
        size_t lastDot = fullName.find_last_of('.');
        if (lastDot != std::string::npos) {
            std::string simpleName = fullName.substr(lastDot + 1);
            aliasToFullName[simpleName] = fullName;
        }
    }

    ClassInfo* getClass(const std::string& name) {
        // 尝试查找全限定名
        if (classes.find(name) != classes.end()) {
            return &classes[name];
        }
        // 尝试通过别名查找
        if (aliasToFullName.find(name) != aliasToFullName.end()) {
            return &classes[aliasToFullName[name]];
        }
        return nullptr;
    }

    std::string translateClassReference(const std::string& className) {
        // 检查是否是NeoForge类，需要转换为Fabric类
        if (isNeoForgeClass(className)) {
            return getFabricEquivalent(className);
        }
        return className;
    }

    bool isNeoForgeClass(const std::string& className) {
        for (const auto& neoClass : knownNeoForgeClasses) {
            if (className.find(neoClass) != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    std::string getFabricEquivalent(const std::string& neoForgeClass) {
        // 映射表：NeoForge类 -> Fabric类或处理方式
        static const std::unordered_map<std::string, std::string> classMapping = {
            {"net.neoforged.neoforge.common.NeoForge", "net.fabricmc.fabric.api.event.lifecycle.v1.ServerLifecycleEvents"},
            {"net.neoforged.bus.api.IEventBus", "net.fabricmc.fabric.api.event.Event"},
            {"net.neoforged.fml.common.Mod", "net.fabricmc.api.ModInitializer"},
            {"net.neoforged.neoforge.registries.DeferredRegister", "net.fabricmc.fabric.api.item.v1.FabricItemSettings"},
            {"net.neoforged.neoforge.common.ModConfigSpec", "net.fabricmc.loader.api.FabricLoader"},
        };

        auto it = classMapping.find(neoForgeClass);
        if (it != classMapping.end()) {
            return it->second;
        }

        // 默认：移除neoforge，添加fabric提示
        std::string result = neoForgeClass;
        result = Utils::replace(result, "net.neoforged.neoforge", "net.fabricmc.fabric");
        result = Utils::replace(result, "net.neoforged.fml", "net.fabricmc.fabric");
        return result + " /* NEEDS MANUAL FIX */";
    }

    void registerMod(const std::string& modid, const ModInfo& info) {
        mods[modid] = info;
    }

    ModInfo* getMod(const std::string& modid) {
        if (mods.find(modid) != mods.end()) {
            return &mods[modid];
        }
        return nullptr;
    }
};

#endif
