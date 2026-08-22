// JsonParser.h
#pragma once

#include <string>
#include <vector>
#include <map>
#include "ModMigrator.h"

namespace ModMigrator {

    class JsonParser {
    public:
        static std::vector<ElementInfo> ParseElements(const std::string& json);
        static ModInfo ParseModInfo(const std::string& json);
        static std::string ExtractString(const std::string& json, const std::string& key);
        static std::vector<std::string> ExtractStringArray(const std::string& json, const std::string& key);
        static std::map<std::string, std::string> ExtractObject(const std::string& json, const std::string& key);

        // ✅ 新增：解析 tabElementOrder
        static std::map<std::string, std::vector<std::string>> ParseTabElementOrder(const std::string& json);

        // ✅ 新增：解析 languageMap
        static std::map<std::string, std::map<std::string, std::string>> ParseLanguageMap(const std::string& json);

        // ✅ 新增：解析 tagElements
        static std::map<std::string, std::vector<std::string>> ParseTagElements(const std::string& json);

    private:
        static size_t FindKey(const std::string& json, const std::string& key, size_t start = 0);
        static std::string ExtractStringValue(const std::string& json, size_t pos);
        static std::vector<std::string> ExtractArrayValue(const std::string& json, size_t pos);
        static std::map<std::string, std::string> ExtractObjectValue(const std::string& json, size_t pos);
        static size_t SkipWhitespace(const std::string& json, size_t pos);
        static size_t FindMatchingBrace(const std::string& json, size_t pos);
        static size_t FindMatchingBracket(const std::string& json, size_t pos);
        static void DebugLog(const std::string& msg);
    };

} // namespace ModMigrator