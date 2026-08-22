# ModMigrator

**Forge/NeoForge → MCreator Mod Migration Tool**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Windows-blue?style=for-the-badge)](https://github.com)
[![Java](https://img.shields.io/badge/Java-8%2B-orange?style=for-the-badge)](https://adoptium.net/)
[![MCreator](https://img.shields.io/badge/MCreator-2024.3%2B-green?style=for-the-badge)](https://mcreator.net/)

---

## 📖 Table of Contents

- [Introduction](#-introduction)
- [Features](#-features)
- [Quick Start](#-quick-start)
- [Requirements](#-requirements)
- [Usage](#-usage)
- [Directory Structure](#-directory-structure)
- [How It Works](#-how-it-works)
- [Tech Stack](#-tech-stack)
- [Roadmap](#-roadmap)
- [Contributing](#-contributing)
- [License](#-license)

---

## 💡 Introduction

**ModMigrator** is an automated tool that converts existing Forge/NeoForge mods to MCreator workspace format (`.mcreator` files). It parses Java source code and `mods.toml` files to automatically extract mod elements such as items, blocks, events, and capabilities — generating workspace files ready for import into MCreator.

### Use Cases

- Migrating existing Forge mods to MCreator for further development
- Learning and studying mod structure
- Quickly generating MCreator workspace templates
- Reverse engineering mods for educational purposes

---

## ✨ Features

| Feature | Description |
|---------|-------------|
| ✅ **mods.toml Parsing** | Extracts modId, name, version, author, dependencies |
| ✅ **Java Source Analysis** | Uses ANTLR4 to parse Java files and extract registered elements |
| ✅ **Element Detection** | Automatically identifies Items, Blocks, Events, Capabilities, etc. |
| ✅ **MCreator Output** | Generates `.mcreator` workspace files ready for import |
| ✅ **Source Preservation** | Retains original Java source code structure |
| ✅ **ZIP Packaging** | Automatically packages the workspace as a ZIP archive |

---

## 🚀 Quick Start

```bash
# Clone the repository
git clone https://github.com/yourusername/ModMigrator.git
cd ModMigrator

# Run the application
ModMigrator.exe
