#ifndef TEMPLATE_GENERATOR_H
#define TEMPLATE_GENERATOR_H

#include "utils.h"
#include <string>

class TemplateGenerator {
private:
    std::string modid;
    std::string mainClass;
    std::string packageName;

public:
    TemplateGenerator(const std::string& mid, const std::string& mClass, const std::string& pkg)
        : modid(mid), mainClass(mClass), packageName(pkg) {
    }

    // 生成fabric.mod.json
    std::string generateFabricModJson() {
        return R"({
  "schemaVersion": 1,
  "id": ")" + modid + R"(",
  "version": "${version}",
  "name": "Converted Mod",
  "description": "Automatically converted from NeoForge to Fabric",
  "authors": ["Conversion Tool"],
  "contact": {},
  "license": "All Rights Reserved",
  "icon": "assets/)" + modid + R"(/icon.png",
  "environment": "*",
  "entrypoints": {
    "main": [
      ")" + packageName + "." + mainClass + R"("
    ],
    "client": [
      ")" + packageName + "." + (mainClass.empty() ? "ClientModInitializerImpl" : mainClass + "Client") + R"("
    ]
  },
  "mixins": [
    ")" + modid + R"(/mixins.json"
  ],
  "depends": {
    "fabricloader": ">=0.15.0",
    "minecraft": "1.21.1",
    "java": ">=21",
    "fabric-api": "*"
  }
})";
    }

    // 生成mixins.json
    std::string generateMixinsJson() {
        return R"({
  "required": true,
  "minVersion": "0.8",
  "package": ")" + packageName + R"(/mixin",
  "compatibilityLevel": "JAVA_17",
  "mixins": [],
  "client": [],
  "server": []
})";
    }

    // 生成主类模板（如果原项目没有合适的主类）
    std::string generateMainClassTemplate() {
        std::string className = mainClass.empty() ? "ConvertedMod" : mainClass;
        return R"(package )" + packageName + R"(;

import net.fabricmc.api.ModInitializer;
import net.fabricmc.fabric.api.event.lifecycle.v1.ServerLifecycleEvents;
import net.fabricmc.fabric.api.event.player.PlayerBlockBreakEvents;
import net.minecraft.util.Identifier;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class )" + className + R"( implements ModInitializer {
    
    public static final String MOD_ID = ")" + modid + R"(";
    public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);
    
    @Override
    public void onInitialize() {
        LOGGER.info("Initializing converted mod: )" + modid + R"(");
        
        // TODO: Migrate your initialization code here
        
        // Example: Register events
        ServerLifecycleEvents.SERVER_STARTED.register(server -> {
            LOGGER.info("Server started");
        });
        
        PlayerBlockBreakEvents.BEFORE.register((world, pos, state, player) -> {
            // Handle block break events
            return true;
        });
    }
    
    public static Identifier id(String path) {
        return new Identifier(MOD_ID, path);
    }
})";
    }

    // 生成build.gradle
    std::string generateBuildGradle() {
        return R"(plugins {
    id 'fabric-loom' version '1.7-SNAPSHOT'
    id 'maven-publish'
}

version = project.mod_version
group = project.maven_group

base {
    archivesName = project.archives_base_name
}

repositories {
    mavenCentral()
    maven {
        name = "Fabric"
        url = "https://maven.fabricmc.net/"
    }
}

dependencies {
    minecraft "com.mojang:minecraft:${project.minecraft_version}"
    mappings "net.fabricmc:yarn:${project.yarn_mappings}:v2"
    modImplementation "net.fabricmc:fabric-loader:${project.loader_version}"
    modImplementation "net.fabricmc.fabric-api:fabric-api:${project.fabric_version}"
}

processResources {
    inputs.property "version", project.version
    inputs.property "minecraft_version", project.minecraft_version
    inputs.property "loader_version", project.loader_version
    filteringCharset "UTF-8"
    
    filesMatching("fabric.mod.json") {
        expand "version": project.version,
               "minecraft_version": project.minecraft_version,
               "loader_version": project.loader_version
    }
}

tasks.withType(JavaCompile).configureEach {
    it.options.release = 21
}

java {
    withSourcesJar()
    sourceCompatibility = JavaVersion.VERSION_21
    targetCompatibility = JavaVersion.VERSION_21
}

jar {
    from("LICENSE") {
        rename { "${it}_${project.archivesBaseName}"}
    }
}

publishing {
    publications {
        create("mavenJava", MavenPublication) {
            artifactId = project.archives_base_name
            from components.java
        }
    }
    repositories {
    }
})";
    }

    // 生成gradle.properties
    std::string generateGradleProperties() {
        return R"(# Fabric Properties
    mod_version=1.0.0
    maven_group=com.example
    archives_base_name=)" + modid + R"(
    
    # Dependencies
    minecraft_version=1.21.1
    yarn_mappings=1.21.1+build.1
    loader_version=0.15.11
    fabric_version=0.100.0+1.21.1
    
    # Java
    java_version=21)";
    }

    // 生成settings.gradle
    std::string generateSettingsGradle() {
        return R"(pluginManagement {
    repositories {
        maven {
            name = 'Fabric'
            url = 'https://maven.fabricmc.net/'
        }
        mavenCentral()
        gradlePluginPortal()
    }
})";
    }
};

#endif
