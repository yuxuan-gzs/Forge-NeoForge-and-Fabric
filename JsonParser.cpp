// JsonParser.cpp - 完整修复版
#include "JsonParser.h"
#include <regex>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <windows.h>

namespace ModMigrator {

    void JsonParser::DebugLog(const std::string& msg) {
        std::string fullMsg = "[JSON-PARSE] " + msg + "\n";
        OutputDebugStringA(fullMsg.c_str());
        std::cerr << fullMsg;
    }

    size_t JsonParser::SkipWhitespace(const std::string& json, size_t pos) {
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) {
            pos++;
        }
        return pos;
    }

    size_t JsonParser::FindKey(const std::string& json, const std::string& key, size_t start) {
        if (json.empty() || key.empty()) {
            DebugLog("FindKey: json or key is empty");
            return std::string::npos;
        }

        std::string pattern = "\"" + key + "\"";
        size_t pos = json.find(pattern, start);
        while (pos != std::string::npos) {
            size_t before = pos;
            while (before > 0 && (json[before - 1] == ' ' || json[before - 1] == '\t' ||
                json[before - 1] == '\n' || json[before - 1] == '\r')) {
                before--;
            }
            if (before == 0 || json[before - 1] == '{' || json[before - 1] == ',' ||
                json[before - 1] == ':' || json[before - 1] == '[') {
                size_t after = pos + pattern.length();
                after = SkipWhitespace(json, after);
                if (after < json.length() && json[after] == ':') {
                    DebugLog("FindKey: key '" + key + "' found at pos " + std::to_string(pos));
                    return after + 1;
                }
            }
            pos = json.find(pattern, pos + 1);
        }
        DebugLog("FindKey: key '" + key + "' NOT found");
        return std::string::npos;
    }

    size_t JsonParser::FindMatchingBrace(const std::string& json, size_t pos) {
        if (json.empty() || pos >= json.length()) {
            return std::string::npos;
        }
        int depth = 0;
        for (size_t i = pos; i < json.length(); i++) {
            if (json[i] == '{') depth++;
            else if (json[i] == '}') {
                depth--;
                if (depth == 0) return i;
            }
        }
        return std::string::npos;
    }

    size_t JsonParser::FindMatchingBracket(const std::string& json, size_t pos) {
        if (json.empty() || pos >= json.length()) {
            return std::string::npos;
        }
        int depth = 0;
        for (size_t i = pos; i < json.length(); i++) {
            if (json[i] == '[') depth++;
            else if (json[i] == ']') {
                depth--;
                if (depth == 0) return i;
            }
        }
        return std::string::npos;
    }

    std::string JsonParser::ExtractStringValue(const std::string& json, size_t pos) {
        if (json.empty() || pos >= json.length()) {
            DebugLog("ExtractStringValue: invalid position or empty json");
            return "";
        }

        pos = SkipWhitespace(json, pos);
        if (pos >= json.length() || json[pos] != '"') {
            DebugLog("ExtractStringValue: no opening quote at position " + std::to_string(pos));
            return "";
        }

        pos++;
        std::string result;
        result.reserve(256);

        while (pos < json.length()) {
            char c = json[pos];
            if (c == '\\') {
                if (pos + 1 >= json.length()) {
                    DebugLog("ExtractStringValue: unexpected end after escape");
                    break;
                }
                char next = json[pos + 1];
                switch (next) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case '"': result += '"'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'u': {
                    result += '?';
                    pos += 5;
                    continue;
                }
                default: result += next; break;
                }
                pos += 2;
            }
            else if (c == '"') {
                pos++;
                DebugLog("ExtractStringValue: success, len=" + std::to_string(result.length()));
                return result;
            }
            else {
                result += c;
                pos++;
            }
        }

        DebugLog("ExtractStringValue: no closing quote, returning partial result");
        return result;
    }

    std::vector<std::string> JsonParser::ExtractArrayValue(const std::string& json, size_t pos) {
        std::vector<std::string> result;
        if (json.empty() || pos >= json.length()) {
            DebugLog("ExtractArrayValue: invalid position or empty json");
            return result;
        }

        pos = SkipWhitespace(json, pos);
        if (pos >= json.length() || json[pos] != '[') {
            DebugLog("ExtractArrayValue: not an array at position " + std::to_string(pos));
            return result;
        }

        pos++;
        DebugLog("ExtractArrayValue: parsing array");

        while (pos < json.length()) {
            pos = SkipWhitespace(json, pos);
            if (pos >= json.length()) {
                DebugLog("ExtractArrayValue: unexpected end of json");
                break;
            }

            if (json[pos] == ']') {
                DebugLog("ExtractArrayValue: end of array, count=" + std::to_string(result.size()));
                break;
            }

            if (json[pos] == '"') {
                std::string value = ExtractStringValue(json, pos);
                if (!value.empty()) {
                    result.push_back(value);
                    DebugLog("ExtractArrayValue: added element, length=" + std::to_string(value.length()));
                }
                size_t endPos = pos + 1;
                while (endPos < json.length()) {
                    if (json[endPos] == '\\') {
                        endPos += 2;
                        continue;
                    }
                    else if (json[endPos] == '"') {
                        endPos++;
                        break;
                    }
                    else {
                        endPos++;
                    }
                }
                pos = endPos;
            }
            else {
                DebugLog("ExtractArrayValue: skipping non-string value at position " + std::to_string(pos));
                while (pos < json.length() && json[pos] != ',' && json[pos] != ']') {
                    pos++;
                }
            }

            pos = SkipWhitespace(json, pos);
            if (pos < json.length() && json[pos] == ',') {
                pos++;
                DebugLog("ExtractArrayValue: skipping comma");
            }
        }

        return result;
    }

    std::map<std::string, std::string> JsonParser::ExtractObjectValue(const std::string& json, size_t pos) {
        std::map<std::string, std::string> result;
        if (json.empty() || pos >= json.length()) {
            DebugLog("ExtractObjectValue: invalid position or empty json");
            return result;
        }

        pos = SkipWhitespace(json, pos);
        if (pos >= json.length() || json[pos] != '{') {
            DebugLog("ExtractObjectValue: not an object at position " + std::to_string(pos));
            return result;
        }

        pos++;

        while (pos < json.length()) {
            pos = SkipWhitespace(json, pos);
            if (pos >= json.length() || json[pos] == '}') {
                DebugLog("ExtractObjectValue: end of object, count=" + std::to_string(result.size()));
                break;
            }

            if (json[pos] == '"') {
                std::string key = ExtractStringValue(json, pos);
                pos += key.length() + 2;
                pos = SkipWhitespace(json, pos);

                if (pos < json.length() && json[pos] == ':') {
                    pos++;
                    pos = SkipWhitespace(json, pos);

                    if (pos < json.length() && json[pos] == '"') {
                        std::string value = ExtractStringValue(json, pos);
                        if (!key.empty() && !value.empty()) {
                            result[key] = value;
                            DebugLog("ExtractObjectValue: added '" + key + "' = '" + value + "'");
                        }
                        pos += value.length() + 2;
                    }
                    else if (pos < json.length() && json[pos] == '{') {
                        DebugLog("ExtractObjectValue: skipping nested object for key '" + key + "'");
                        size_t end = FindMatchingBrace(json, pos);
                        if (end != std::string::npos) {
                            pos = end + 1;
                        }
                    }
                    else if (pos < json.length() && json[pos] == '[') {
                        DebugLog("ExtractObjectValue: skipping nested array for key '" + key + "'");
                        size_t end = FindMatchingBracket(json, pos);
                        if (end != std::string::npos) {
                            pos = end + 1;
                        }
                    }
                    else {
                        while (pos < json.length() && json[pos] != ',' && json[pos] != '}') {
                            pos++;
                        }
                    }
                }
            }

            pos = SkipWhitespace(json, pos);
            if (pos < json.length() && json[pos] == ',') {
                pos++;
                DebugLog("ExtractObjectValue: skipping comma");
            }
        }

        return result;
    }

    std::string JsonParser::ExtractString(const std::string& json, const std::string& key) {
        if (json.empty() || key.empty()) {
            DebugLog("ExtractString: json or key is empty");
            return "";
        }

        DebugLog("ExtractString: key='" + key + "'");
        size_t pos = FindKey(json, key);
        if (pos == std::string::npos) {
            DebugLog("ExtractString: key '" + key + "' not found");
            return "";
        }
        return ExtractStringValue(json, pos);
    }

    std::vector<std::string> JsonParser::ExtractStringArray(const std::string& json, const std::string& key) {
        if (json.empty() || key.empty()) {
            DebugLog("ExtractStringArray: json or key is empty");
            return {};
        }

        DebugLog("ExtractStringArray: key='" + key + "'");
        size_t pos = FindKey(json, key);
        if (pos == std::string::npos) {
            DebugLog("ExtractStringArray: key '" + key + "' not found");
            return {};
        }
        return ExtractArrayValue(json, pos);
    }

    std::map<std::string, std::string> JsonParser::ExtractObject(const std::string& json, const std::string& key) {
        if (json.empty() || key.empty()) {
            DebugLog("ExtractObject: json or key is empty");
            return {};
        }

        DebugLog("ExtractObject: key='" + key + "'");
        size_t pos = FindKey(json, key);
        if (pos == std::string::npos) {
            DebugLog("ExtractObject: key '" + key + "' not found");
            return {};
        }
        return ExtractObjectValue(json, pos);
    }

    std::vector<ElementInfo> JsonParser::ParseElements(const std::string& json) {
        std::vector<ElementInfo> elements;

        if (json.empty()) {
            DebugLog("ParseElements: json is empty");
            return elements;
        }

        DebugLog("======= ParseElements START =======");
        DebugLog("JSON size: " + std::to_string(json.length()) + " bytes");

        try {
            size_t pos = json.find("\"elements\":");
            DebugLog("Searching for '\"elements\":': found at " + std::to_string(pos));

            if (pos == std::string::npos) {
                DebugLog("'\"elements\":' not found, trying '\"mod_elements\":'");
                pos = json.find("\"mod_elements\":");
                DebugLog("Searching for '\"mod_elements\":': found at " + std::to_string(pos));
            }

            if (pos == std::string::npos) {
                DebugLog("ERROR: neither 'elements' nor 'mod_elements' found!");
                return elements;
            }

            pos = json.find(':', pos);
            if (pos == std::string::npos) {
                DebugLog("ERROR: no ':' after elements key");
                return elements;
            }

            pos++;
            pos = SkipWhitespace(json, pos);

            if (pos >= json.length() || json[pos] != '[') {
                DebugLog("ERROR: elements key not followed by '['");
                return elements;
            }

            DebugLog("Found '[' at position " + std::to_string(pos));

            pos++;
            int braceDepth = 0;
            size_t elemStart = std::string::npos;
            int elementCount = 0;

            for (size_t i = pos; i < json.length(); i++) {
                char c = json[i];

                if (c == '{') {
                    if (braceDepth == 0) {
                        elemStart = i;
                        DebugLog("Element " + std::to_string(elementCount + 1) + " starts at " + std::to_string(i));
                    }
                    braceDepth++;
                }
                else if (c == '}') {
                    braceDepth--;
                    if (braceDepth == 0 && elemStart != std::string::npos) {
                        std::string elemJson = json.substr(elemStart, i - elemStart + 1);
                        DebugLog("Extracting element " + std::to_string(elementCount + 1) +
                            ", length=" + std::to_string(elemJson.length()));

                        ElementInfo elem;
                        elem.name = ExtractString(elemJson, "name");
                        elem.type = ExtractString(elemJson, "type");
                        elem.registryName = ExtractString(elemJson, "registry_name");

                        std::map<std::string, std::string> metadata = ExtractObject(elemJson, "metadata");
                        if (!metadata.empty()) {
                            auto it = metadata.find("files");
                            if (it != metadata.end()) {
                                elem.files = ExtractStringArray(elemJson, "files");
                            }
                            elem.metadata = metadata;
                        }

                        if (elem.files.empty()) {
                            elem.files = ExtractStringArray(elemJson, "files");
                        }

                        elem.isBlock = (elem.type == "block");
                        elem.isEvent = (elem.type == "event");
                        elem.isCapability = (elem.type == "capability");
                        elem.isRecipe = (elem.type == "recipe");

                        if (!elem.name.empty() && !elem.registryName.empty()) {
                            elements.push_back(elem);
                            elementCount++;
                            DebugLog("SUCCESS: added element " + std::to_string(elementCount) +
                                ": name='" + elem.name + "', registry='" + elem.registryName + "'");
                        }
                        else {
                            DebugLog("FAILED: name='" + elem.name + "', registry='" + elem.registryName + "'");
                        }

                        elemStart = std::string::npos;

                        size_t nextPos = i + 1;
                        nextPos = SkipWhitespace(json, nextPos);
                        if (nextPos < json.length() && json[nextPos] == ',') {
                            i = nextPos;
                            DebugLog("Skipped comma at position " + std::to_string(nextPos));
                        }
                    }
                }
                else if (c == '[') {
                    int arrayDepth = 1;
                    while (i < json.length() && arrayDepth > 0) {
                        i++;
                        if (i >= json.length()) break;
                        if (json[i] == '[') arrayDepth++;
                        else if (json[i] == ']') arrayDepth--;
                    }
                }
            }
        }
        catch (const std::out_of_range& e) {
            DebugLog("EXCEPTION: out_of_range in ParseElements: " + std::string(e.what()));
        }
        catch (const std::exception& e) {
            DebugLog("EXCEPTION: " + std::string(e.what()));
        }
        catch (...) {
            DebugLog("EXCEPTION: unknown exception in ParseElements");
        }

        DebugLog("======= ParseElements END: found " + std::to_string(elements.size()) + " elements =======");
        return elements;
    }

    ModInfo JsonParser::ParseModInfo(const std::string& json) {
        DebugLog("======= ParseModInfo START =======");
        ModInfo info;
        info.modId = ExtractString(json, "modId");
        DebugLog("modId: '" + info.modId + "'");
        info.modName = ExtractString(json, "modName");
        DebugLog("modName: '" + info.modName + "'");
        info.modVersion = ExtractString(json, "modVersion");
        info.modDescription = ExtractString(json, "modDescription");
        info.modAuthor = ExtractString(json, "modAuthor");
        info.mcVersion = ExtractString(json, "mcVersion");
        info.modLoader = ExtractString(json, "modLoader");
        info.generator = ExtractString(json, "generator");
        info.sourcePath = ExtractString(json, "sourcePath");
        info.dependencies = ExtractStringArray(json, "dependencies");
        info.errors = ExtractStringArray(json, "errors");
        info.elements = ParseElements(json);
        info.tabElementOrder = ParseTabElementOrder(json);
        info.languageMap = ParseLanguageMap(json);
        DebugLog("ParseModInfo: elements=" + std::to_string(info.elements.size()) +
            ", tab orders=" + std::to_string(info.tabElementOrder.size()) +
            ", languages=" + std::to_string(info.languageMap.size()));
        DebugLog("======= ParseModInfo END =======");
        return info;
    }

    // ========== 解析 tabElementOrder ==========
    std::map<std::string, std::vector<std::string>> JsonParser::ParseTabElementOrder(const std::string& json) {
        std::map<std::string, std::vector<std::string>> result;

        if (json.empty()) {
            DebugLog("ParseTabElementOrder: json is empty");
            return result;
        }

        DebugLog("ParseTabElementOrder: START");

        size_t pos = json.find("\"tabElementOrder\"");
        if (pos == std::string::npos) {
            DebugLog("ParseTabElementOrder: 'tabElementOrder' not found");
            return result;
        }

        pos = json.find(':', pos);
        if (pos == std::string::npos) {
            DebugLog("ParseTabElementOrder: no ':' after tabElementOrder");
            return result;
        }
        pos++;
        pos = SkipWhitespace(json, pos);
        if (pos >= json.length() || json[pos] != '{') {
            DebugLog("ParseTabElementOrder: tabElementOrder not followed by '{'");
            return result;
        }

        pos++;
        DebugLog("ParseTabElementOrder: parsing tabElementOrder object");

        while (pos < json.length()) {
            pos = SkipWhitespace(json, pos);
            if (pos >= json.length() || json[pos] == '}') {
                DebugLog("ParseTabElementOrder: end of tabElementOrder");
                break;
            }

            if (json[pos] == '"') {
                std::string key = ExtractStringValue(json, pos);
                DebugLog("ParseTabElementOrder: found key '" + key + "'");

                pos += key.length() + 2;
                pos = SkipWhitespace(json, pos);

                if (pos < json.length() && json[pos] == ':') {
                    pos++;
                    pos = SkipWhitespace(json, pos);

                    if (pos < json.length() && json[pos] == '[') {
                        size_t end = FindMatchingBracket(json, pos);
                        if (end != std::string::npos) {
                            std::vector<std::string> values = ExtractArrayValue(json, pos);
                            if (!key.empty() && !values.empty()) {
                                result[key] = values;
                                DebugLog("ParseTabElementOrder: added " + std::to_string(values.size()) + " elements for key '" + key + "'");
                            }
                            pos = end + 1;
                        }
                        else {
                            DebugLog("ParseTabElementOrder: cannot find matching bracket for key '" + key + "'");
                            while (pos < json.length() && json[pos] != ',' && json[pos] != '}') {
                                pos++;
                            }
                        }
                    }
                }
            }

            pos = SkipWhitespace(json, pos);
            if (pos < json.length() && json[pos] == ',') {
                pos++;
            }
        }

        DebugLog("ParseTabElementOrder: END, found " + std::to_string(result.size()) + " entries");
        return result;
    }

    // ========== 解析 languageMap ==========
    std::map<std::string, std::map<std::string, std::string>> JsonParser::ParseLanguageMap(const std::string& json) {
        std::map<std::string, std::map<std::string, std::string>> result;

        if (json.empty()) {
            DebugLog("ParseLanguageMap: json is empty");
            return result;
        }

        DebugLog("ParseLanguageMap: START");

        size_t pos = json.find("\"languageMap\"");
        if (pos == std::string::npos) {
            DebugLog("ParseLanguageMap: 'languageMap' not found");
            return result;
        }

        pos = json.find(':', pos);
        if (pos == std::string::npos) {
            DebugLog("ParseLanguageMap: no ':' after languageMap");
            return result;
        }
        pos++;
        pos = SkipWhitespace(json, pos);
        if (pos >= json.length() || json[pos] != '{') {
            DebugLog("ParseLanguageMap: languageMap not followed by '{'");
            return result;
        }

        pos++;
        DebugLog("ParseLanguageMap: parsing languageMap object");

        while (pos < json.length()) {
            pos = SkipWhitespace(json, pos);
            if (pos >= json.length() || json[pos] == '}') {
                DebugLog("ParseLanguageMap: end of languageMap");
                break;
            }

            if (json[pos] == '"') {
                std::string langKey = ExtractStringValue(json, pos);
                DebugLog("ParseLanguageMap: found language '" + langKey + "'");

                pos += langKey.length() + 2;
                pos = SkipWhitespace(json, pos);

                if (pos < json.length() && json[pos] == ':') {
                    pos++;
                    pos = SkipWhitespace(json, pos);

                    if (pos < json.length() && json[pos] == '{') {
                        std::map<std::string, std::string> langMap = ExtractObjectValue(json, pos);
                        if (!langKey.empty() && !langMap.empty()) {
                            result[langKey] = langMap;
                            DebugLog("ParseLanguageMap: added " + std::to_string(langMap.size()) + " entries for language '" + langKey + "'");
                        }
                        size_t end = FindMatchingBrace(json, pos);
                        if (end != std::string::npos) {
                            pos = end + 1;
                        }
                    }
                }
            }

            pos = SkipWhitespace(json, pos);
            if (pos < json.length() && json[pos] == ',') {
                pos++;
            }
        }

        DebugLog("ParseLanguageMap: END, found " + std::to_string(result.size()) + " languages");
        return result;
    }

} // namespace ModMigrator