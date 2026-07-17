#ifndef AST_PARSER_H
#define AST_PARSER_H

#include "utils.h"
#include "SymbolTable.h"
#include <stack>

struct ASTNode {
    enum Type {
        COMPILATION_UNIT,
        PACKAGE_DECL,
        IMPORT_DECL,
        CLASS_DECL,
        METHOD_DECL,
        FIELD_DECL,
        ANNOTATION,
        BLOCK,
        STATEMENT,
        EXPRESSION,
        UNKNOWN
    };

    Type type;
    std::string value;
    std::vector<ASTNode*> children;
    int lineNumber;
    int columnNumber;

    ASTNode(Type t, const std::string& v = "", int line = 0, int col = 0)
        : type(t), value(v), lineNumber(line), columnNumber(col) {
    }

    ~ASTNode() {
        for (auto child : children) {
            delete child;
        }
    }

    void addChild(ASTNode* child) {
        children.push_back(child);
    }
};

class ASTParser {
private:
    std::string content;
    size_t position;
    int currentLine;
    int currentColumn;
    SymbolTable* symbolTable;

    // 在 ASTParser.h 中修改

    char peek() {
        if (position >= content.length()) return '\0';
        unsigned char c = static_cast<unsigned char>(content[position]);
        return static_cast<char>(c);
    }

    char consume() {
        if (position >= content.length()) return '\0';
        unsigned char c = static_cast<unsigned char>(content[position++]);
        if (c == '\n') {
            currentLine++;
            currentColumn = 1;
        }
        else {
            currentColumn++;
        }
        return static_cast<char>(c);
    }

    void skipWhitespace() {
        while (position < content.length()) {
            unsigned char c = static_cast<unsigned char>(content[position]);
            // 只跳过标准的空白字符，避免处理中文字符
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                consume();
            }
            else {
                break;
            }
        }
    }

    bool match(const std::string& str) {
        if (position + str.length() > content.length()) return false;
        if (content.substr(position, str.length()) == str) {
            position += str.length();
            currentColumn += str.length();
            return true;
        }
        return false;
    }

    std::string parseIdentifier() {
        skipWhitespace();
        std::string identifier;
        while (position < content.length() && (std::isalnum(content[position]) || content[position] == '_')) {
            identifier += consume();
        }
        return identifier;
    }

    ASTNode* parseAnnotation() {
        skipWhitespace();
        if (!match("@")) return nullptr;

        ASTNode* annotation = new ASTNode(ASTNode::ANNOTATION, "@", currentLine, currentColumn);
        std::string name = parseIdentifier();
        annotation->value = "@" + name;

        // 解析注解参数（简化版）
        if (peek() == '(') {
            consume(); // '('
            annotation->value += "(";
            int parenDepth = 1;
            while (position < content.length() && parenDepth > 0) {
                char c = consume();
                annotation->value += c;
                if (c == '(') parenDepth++;
                if (c == ')') parenDepth--;
            }
        }

        return annotation;
    }

    ASTNode* parseMethodDeclaration() {
        skipWhitespace();
        if (!match("public") && !match("private") && !match("protected")) {
            return nullptr;
        }

        ASTNode* method = new ASTNode(ASTNode::METHOD_DECL, "", currentLine, currentColumn);

        // 解析注解
        while (peek() == '@') {
            method->addChild(parseAnnotation());
            skipWhitespace();
        }

        // 解析修饰符
        while (match("static") || match("final") || match("synchronized")) {
            // 存储修饰符
        }

        std::string returnType = parseIdentifier();
        std::string methodName = parseIdentifier();

        if (peek() != '(') return nullptr;

        method->value = methodName;

        // 解析参数列表
        if (match("(")) {
            std::string params;
            int parenDepth = 1;
            while (position < content.length() && parenDepth > 0) {
                char c = consume();
                params += c;
                if (c == '(') parenDepth++;
                if (c == ')') parenDepth--;
            }
        }

        // 解析方法体
        if (peek() == '{') {
            consume(); // '{'
            int braceDepth = 1;
            std::string body;
            while (position < content.length() && braceDepth > 0) {
                char c = consume();
                body += c;
                if (c == '{') braceDepth++;
                if (c == '}') braceDepth--;
            }
            method->value = methodName + "(" + ")" + " : " + returnType;
        }

        return method;
    }

    ASTNode* parseFieldDeclaration() {
        skipWhitespace();
        if (!match("public") && !match("private") && !match("protected")) {
            return nullptr;
        }

        ASTNode* field = new ASTNode(ASTNode::FIELD_DECL, "", currentLine, currentColumn);

        // 解析注解
        while (peek() == '@') {
            field->addChild(parseAnnotation());
            skipWhitespace();
        }

        std::string type = parseIdentifier();
        std::string name = parseIdentifier();

        field->value = name + " : " + type;

        // 处理初始化
        if (peek() == '=') {
            consume(); // '='
            std::string init;
            while (peek() != ';' && peek() != ',' && peek() != '\0') {
                init += consume();
            }
            field->value += " = " + Utils::trim(init);
        }

        if (peek() == ';') consume();

        return field;
    }

    ASTNode* parseClassDeclaration() {
        skipWhitespace();
        if (!match("public") && !match("class")) {
            return nullptr;
        }

        // 如果没有匹配到public class，尝试只匹配class
        if (!match("class")) {
            // 回退到之前的position
            return nullptr;
        }

        ASTNode* classNode = new ASTNode(ASTNode::CLASS_DECL, "", currentLine, currentColumn);
        std::string className = parseIdentifier();
        classNode->value = className;

        // 解析extends
        if (match("extends")) {
            std::string superClass = parseIdentifier();
            // 记录到符号表
        }

        // 解析implements
        if (match("implements")) {
            while (true) {
                std::string interface = parseIdentifier();
                // 记录到符号表
                if (peek() != ',') break;
                consume(); // ','
            }
        }

        // 解析类体
        if (match("{")) {
            while (position < content.length() && peek() != '}') {
                skipWhitespace();
                if (peek() == '@') {
                    classNode->addChild(parseAnnotation());
                }
                else if (peek() == 'p' || peek() == 'u' || peek() == 'c') {
                    // 尝试解析方法或字段
                    int savedPos = position;
                    int savedLine = currentLine;
                    int savedCol = currentColumn;

                    ASTNode* method = parseMethodDeclaration();
                    if (method) {
                        classNode->addChild(method);
                    }
                    else {
                        position = savedPos;
                        currentLine = savedLine;
                        currentColumn = savedCol;

                        ASTNode* field = parseFieldDeclaration();
                        if (field) {
                            classNode->addChild(field);
                        }
                    }
                }
                else {
                    consume();
                }
            }
            if (peek() == '}') consume();
        }

        return classNode;
    }

public:
    ASTParser(SymbolTable* st) : position(0), currentLine(1), currentColumn(1), symbolTable(st) {}

    ASTNode* parse(const std::string& source) {
        content = Utils::removeComments(source);
        position = 0;
        currentLine = 1;
        currentColumn = 1;

        ASTNode* root = new ASTNode(ASTNode::COMPILATION_UNIT);

        skipWhitespace();

        // 解析package
        if (match("package")) {
            ASTNode* packageNode = new ASTNode(ASTNode::PACKAGE_DECL);
            std::string packageName;
            while (peek() != ';' && peek() != '\0') {
                packageName += consume();
            }
            if (peek() == ';') consume();
            packageNode->value = Utils::trim(packageName);
            root->addChild(packageNode);
            skipWhitespace();
        }

        // 解析imports
        while (match("import")) {
            ASTNode* importNode = new ASTNode(ASTNode::IMPORT_DECL);
            std::string importPath;
            while (peek() != ';' && peek() != '\0') {
                importPath += consume();
            }
            if (peek() == ';') consume();
            importNode->value = Utils::trim(importPath);
            root->addChild(importNode);
            skipWhitespace();
        }

        // 解析类声明
        while (position < content.length()) {
            skipWhitespace();
            if (peek() == 'p' || peek() == 'c') {
                ASTNode* classNode = parseClassDeclaration();
                if (classNode) {
                    root->addChild(classNode);
                }
                else {
                    consume(); // 跳过未知内容
                }
            }
            else {
                consume();
            }
        }

        return root;
    }

    // 遍历AST并收集信息
    void collectClassInfo(ASTNode* node, ClassInfo& info) {
        if (!node) return;

        if (node->type == ASTNode::CLASS_DECL) {
            info.name = node->value;
        }

        for (auto child : node->children) {
            if (child->type == ASTNode::METHOD_DECL) {
                // 检查是否是事件处理方法
                for (auto annotationChild : child->children) {
                    if (annotationChild->type == ASTNode::ANNOTATION) {
                        if (annotationChild->value.find("@SubscribeEvent") != std::string::npos) {
                            info.isEventSubscriber = true;
                            info.eventHandlers.push_back(child->value);
                        }
                    }
                }
                info.methods[child->value] = "method";
            }
            else if (child->type == ASTNode::FIELD_DECL) {
                info.fields[child->value] = "field";
            }

            collectClassInfo(child, info);
        }
    }
};

#endif
