# NeoForge to Fabric Source Code Converter

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-1.0.0-green.svg)](https://github.com/yuxuan-gzs/Forge-NeoForge-and-Fabric)
[![Minecraft](https://img.shields.io/badge/Minecraft-1.21.1-orange.svg)](https://minecraft.net)

---

## 📖 Overview

**NeoForge to Fabric Source Code Converter** is an automated code migration tool designed to help Minecraft mod developers quickly convert **NeoForge 1.21.1** mod source code to **Fabric 1.21.1** project structure.

This is an experimental tool that can automatically handle approximately **20-99%** of the conversion work, with the remaining **1-20%** requiring manual adjustment by the developer.

---

## 🎯 Use Cases

| Scenario | Suitability |
|:---|:---:|
| Handwritten NeoForge mods | ⚠️ Requires minor manual fixes |
| MCreator-generated mods | ⚠️ Requires minor manual fixes |
| Small to medium-sized mods | ✅ Works well |
| Large complex mods | ⚠️ Recommend staying on NeoForge |
| Heavy reliance on third-party libraries | ❌ Not recommended |

---

## ✨ Features

### 🔧 Java Source Code Conversion

- ✅ Automatically replace NeoForge API with Fabric API
- ✅ Convert event system
- ✅ Convert registration system
- ✅ Convert capability system
- ✅ Process annotations
- ⚠️ Import statement handling (may fail to remove NeoForge imports or add Fabric imports correctly)

### 📁 Resource File Conversion

- ✅ Convert tag files
- ✅ Convert tag references in recipe files
- ✅ Convert loot tables and advancement conditions
- ✅ Process mod ID placeholders in language files
- ⚠️ Skip NeoForge-specific `neoforge/` directory

### 🏗️ Project Structure Generation

- ✅ Generate `fabric.mod.json` (auto-reads original mod information)
- ✅ Generate `build.gradle` (Fabric Loom configuration)
- ✅ Generate `mixins.json` (Mixin support reserved)
- ✅ Generate Gradle configuration files

---

## 📥 Download & Installation

### System Requirements

- Windows 10 / 11
- C++17 Runtime Library (Visual Studio-based dynamic libraries)

### Download

> [Releases · yuxuan-gzs/Forge-NeoForge-and-Fabric](https://github.com/yuxuan-gzs/Forge-NeoForge-and-Fabric/releases)

---

## 🚀 Usage

### Step 1: Prepare NeoForge Mod Source Code

Ensure your mod has the following standard Maven structure:
