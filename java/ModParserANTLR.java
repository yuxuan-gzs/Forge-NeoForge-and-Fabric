// ModParserANTLR.java - 完整修复版（使用自定义序列化器控制所有字段）
import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.tree.*;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonArray;
import com.google.gson.JsonPrimitive;
import com.google.gson.JsonSerializationContext;
import com.google.gson.JsonSerializer;

import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.regex.*;
import java.util.stream.*;
import java.lang.reflect.Type;

import antlr.JavaLexer;
import antlr.JavaParser;
import antlr.JavaParserBaseListener;

public class ModParserANTLR {

    public static class ElementInfo {
        public String name;
        public String type;
        public String registryName;
        public List<String> files = new ArrayList<>();
        public Map<String, String> metadata = new HashMap<>();
        public boolean isBlock = false;
        public boolean isEvent = false;
        public boolean isCapability = false;
        public boolean isRecipe = false;
        public String sourceFile;
        public String className;
        public String packageName;
        public List<String> annotations = new ArrayList<>();
        public List<String> interfaces = new ArrayList<>();
        public String superClass;
        public String modId;
        public boolean compiles = true;
        public boolean locked_code = false;
    }

    public static class ParsedMod {
        public String modId;
        public String modName;
        public String modVersion;
        public String modDescription;
        public String modAuthor;
        public String mcVersion;
        public List<String> dependencies = new ArrayList<>();
        public String modLoader;
        public String generator;
        public List<ElementInfo> elements = new ArrayList<>();
        public List<ElementInfo> mod_elements = new ArrayList<>();
        public Map<String, List<String>> tagElements = new HashMap<>();
        public Map<String, List<String>> tabElementOrder = new HashMap<>();
        public Map<String, Map<String, String>> languageMap = new HashMap<>();
        public String sourcePath;
        public int totalFiles;
        public int errorCount;
        public long parseTimeMs;
        public List<String> errors = new ArrayList<>();
    }

    // ========== ElementInfo 序列化器 ==========
    private static class ElementInfoSerializer implements JsonSerializer<ElementInfo> {
        @Override
        public JsonElement serialize(ElementInfo src, Type typeOfSrc, JsonSerializationContext context) {
            JsonObject obj = new JsonObject();
            obj.addProperty("name", src.name);
            obj.addProperty("type", src.type);
            obj.addProperty("compiles", src.compiles);
            obj.addProperty("locked_code", src.locked_code);
            
            String registryName = src.registryName;
            if (registryName != null && registryName.contains(":")) {
                registryName = registryName.replace(":", "_");
            }
            obj.addProperty("registry_name", registryName);
            
            JsonObject metadata = new JsonObject();
            if (src.files != null && !src.files.isEmpty()) {
                JsonArray filesArray = new JsonArray();
                for (String file : src.files) {
                    filesArray.add(file.replace("\\", "/"));
                }
                metadata.add("files", filesArray);
            }
            if (src.metadata != null && !src.metadata.isEmpty()) {
                for (Map.Entry<String, String> entry : src.metadata.entrySet()) {
                    metadata.addProperty(entry.getKey(), entry.getValue());
                }
            }
            obj.add("metadata", metadata);
            
            return obj;
        }
    }

    // ========== ParsedMod 序列化器（完全控制输出） ==========
    private static class ParsedModSerializer implements JsonSerializer<ParsedMod> {
        @Override
        public JsonElement serialize(ParsedMod src, Type typeOfSrc, JsonSerializationContext context) {
            JsonObject obj = new JsonObject();
            
            // 基本字段
            obj.addProperty("modId", src.modId);
            obj.addProperty("modName", src.modName);
            obj.addProperty("modVersion", src.modVersion);
            obj.addProperty("modDescription", src.modDescription);
            obj.addProperty("modAuthor", src.modAuthor);
            obj.addProperty("mcVersion", src.mcVersion);
            obj.addProperty("modLoader", src.modLoader);
            obj.addProperty("generator", src.generator);
            obj.addProperty("sourcePath", src.sourcePath);
            obj.addProperty("totalFiles", src.totalFiles);
            obj.addProperty("errorCount", src.errorCount);
            obj.addProperty("parseTimeMs", src.parseTimeMs);
            
            // dependencies
            JsonArray depsArray = new JsonArray();
            if (src.dependencies != null) {
                for (String dep : src.dependencies) {
                    depsArray.add(dep);
                }
            }
            obj.add("dependencies", depsArray);
            
            // errors
            JsonArray errorsArray = new JsonArray();
            if (src.errors != null) {
                for (String err : src.errors) {
                    errorsArray.add(err);
                }
            }
            obj.add("errors", errorsArray);
            
            // elements（使用 ElementInfoSerializer）
            JsonArray elementsArray = new JsonArray();
            if (src.elements != null) {
                ElementInfoSerializer elemSerializer = new ElementInfoSerializer();
                for (ElementInfo elem : src.elements) {
                    elementsArray.add(elemSerializer.serialize(elem, ElementInfo.class, context));
                }
            }
            obj.add("elements", elementsArray);
            
            // mod_elements（复制 elements）
            JsonArray modElementsArray = new JsonArray();
            if (src.mod_elements != null) {
                ElementInfoSerializer elemSerializer = new ElementInfoSerializer();
                for (ElementInfo elem : src.mod_elements) {
                    modElementsArray.add(elemSerializer.serialize(elem, ElementInfo.class, context));
                }
            }
            obj.add("mod_elements", modElementsArray);
            
            // tagElements
            JsonObject tagElementsObj = new JsonObject();
            if (src.tagElements != null) {
                for (Map.Entry<String, List<String>> entry : src.tagElements.entrySet()) {
                    JsonArray arr = new JsonArray();
                    for (String val : entry.getValue()) {
                        arr.add(val);
                    }
                    tagElementsObj.add(entry.getKey(), arr);
                }
            }
            obj.add("tagElements", tagElementsObj);
            
            // ========== tabElementOrder（关键修复） ==========
            JsonObject tabOrderObj = new JsonObject();
            if (src.tabElementOrder != null && !src.tabElementOrder.isEmpty()) {
                for (Map.Entry<String, List<String>> entry : src.tabElementOrder.entrySet()) {
                    JsonArray arr = new JsonArray();
                    for (String val : entry.getValue()) {
                        arr.add(val);
                    }
                    tabOrderObj.add(entry.getKey(), arr);
                }
                System.out.println("[DEBUG] Serializing tabElementOrder: " + src.tabElementOrder.size() + " entries");
            } else {
                System.out.println("[DEBUG] tabElementOrder is empty or null");
            }
            obj.add("tabElementOrder", tabOrderObj);
            
            // ========== languageMap ==========
            JsonObject langMapObj = new JsonObject();
            if (src.languageMap != null && !src.languageMap.isEmpty()) {
                for (Map.Entry<String, Map<String, String>> langEntry : src.languageMap.entrySet()) {
                    JsonObject langObj = new JsonObject();
                    for (Map.Entry<String, String> entry : langEntry.getValue().entrySet()) {
                        langObj.addProperty(entry.getKey(), entry.getValue());
                    }
                    langMapObj.add(langEntry.getKey(), langObj);
                }
            }
            obj.add("languageMap", langMapObj);
            
            return obj;
        }
    }

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.err.println("Usage: java ModParserANTLR <sourcePath> [outputPath]");
            System.exit(1);
        }

        String sourcePath = args[0];
        String outputPath = args.length > 1 ? args[1] : "parsed_result.json";

        long startTime = System.currentTimeMillis();

        System.out.println("[INFO] Parsing mod at: " + sourcePath);
        ParsedMod result = parseMod(sourcePath);
        result.parseTimeMs = System.currentTimeMillis() - startTime;

        // 使用自定义序列化器
        Gson gson = new GsonBuilder()
            .setPrettyPrinting()
            .registerTypeAdapter(ElementInfo.class, new ElementInfoSerializer())
            .registerTypeAdapter(ParsedMod.class, new ParsedModSerializer())
            .create();
        String json = gson.toJson(result);

        try (FileWriter writer = new FileWriter(outputPath)) {
            writer.write(json);
        }

        System.out.println("[INFO] ========================================");
        System.out.println("[INFO] Total files: " + result.totalFiles);
        System.out.println("[INFO] Elements found: " + result.elements.size());
        System.out.println("[INFO] Mod ID: " + result.modId);
        System.out.println("[INFO] Mod Name: " + result.modName);
        System.out.println("[INFO] Time: " + result.parseTimeMs + " ms");
        System.out.println("[INFO] Output: " + outputPath);
        System.out.println("[INFO] Errors: " + result.errorCount);
        
        Map<String, Integer> typeCount = new HashMap<>();
        for (ElementInfo elem : result.elements) {
            typeCount.put(elem.type, typeCount.getOrDefault(elem.type, 0) + 1);
        }
        System.out.println("[INFO] Element types: " + typeCount);
        System.out.println("[INFO] Tab order elements: " + 
            (result.tabElementOrder.containsKey(result.modId) ? 
             result.tabElementOrder.get(result.modId).size() : 0));
    }

    private static ParsedMod parseMod(String sourcePath) throws IOException {
        ParsedMod result = new ParsedMod();
        result.sourcePath = sourcePath;

        // ========== 搜索并解析 mods.toml ==========
        Path foundToml = null;
        try (Stream<Path> walk = Files.walk(Paths.get(sourcePath))) {
            Optional<Path> toml = walk
                .filter(p -> !Files.isDirectory(p))
                .filter(p -> p.getFileName().toString().equals("mods.toml") || 
                             p.getFileName().toString().equals("neoforge.mods.toml"))
                .findFirst();
            if (toml.isPresent()) {
                foundToml = toml.get();
            }
        } catch (IOException e) {
            System.err.println("[WARN] Failed to search for mods.toml: " + e.getMessage());
        }

        String modId = "converted_mod";
        String modName = "converted_mod";
        
        if (foundToml != null) {
            try {
                String toml = new String(Files.readAllBytes(foundToml));
                System.out.println("[INFO] Found mods.toml: " + foundToml);
                
                modId = extractTomlValue(toml, "modId");
                if (modId.isEmpty()) {
                    modId = extractTomlValue(toml, "mod_id");
                }
                if (modId.isEmpty()) {
                    modId = extractModIdFromModsBlock(toml);
                }
                
                modName = extractTomlValue(toml, "displayName");
                if (modName.isEmpty()) modName = extractTomlValue(toml, "name");
                if (modName.isEmpty()) modName = modId;
                
                result.modVersion = extractTomlValue(toml, "version");
                if (result.modVersion.isEmpty()) result.modVersion = "1.0.0";
                
                result.modDescription = extractTomlValue(toml, "description");
                result.modAuthor = extractTomlValue(toml, "authors");
                if (result.modAuthor.isEmpty()) result.modAuthor = extractTomlValue(toml, "author");
                
                result.mcVersion = extractTomlValue(toml, "mcversion");
                if (result.mcVersion.isEmpty()) result.mcVersion = "1.20.1";
                
                result.modLoader = extractTomlValue(toml, "modLoader");
                if (result.modLoader.isEmpty()) {
                    if (toml.contains("neoforge") || toml.contains("lowcodefml")) {
                        result.modLoader = "neoforge";
                    } else {
                        result.modLoader = "forge";
                    }
                } else if (result.modLoader.contains("lowcodefml")) {
                    result.modLoader = "neoforge";
                } else if (result.modLoader.contains("javafml") || result.modLoader.contains("forge")) {
                    result.modLoader = "forge";
                } else {
                    result.modLoader = "forge";
                }
                
                System.out.println("[INFO] Mod ID from toml: " + modId);
                System.out.println("[INFO] Mod Name from toml: " + modName);
                System.out.println("[INFO] Mod Loader: " + result.modLoader);
            } catch (Exception e) {
                System.err.println("[ERROR] Failed to read mods.toml: " + e.getMessage());
                e.printStackTrace();
            }
        } else {
            System.out.println("[WARN] mods.toml not found");
        }

        result.modId = modId;
        result.modName = modName;
        result.generator = result.modLoader + "-" + result.mcVersion;

        // ========== 扫描 Java 文件 ==========
        Path srcJavaDir = Paths.get(sourcePath, "src", "main", "java");
        if (!Files.exists(srcJavaDir)) {
            srcJavaDir = Paths.get(sourcePath);
        }

        List<Path> javaFiles;
        try (Stream<Path> walk = Files.walk(srcJavaDir)) {
            javaFiles = walk.filter(p -> p.toString().endsWith(".java"))
                            .collect(Collectors.toList());
        } catch (IOException e) {
            System.err.println("[ERROR] Failed to scan Java files: " + e.getMessage());
            return result;
        }

        result.totalFiles = javaFiles.size();
        System.out.println("[INFO] Found " + javaFiles.size() + " Java files");

        // ========== 解析 Java 文件 ==========
        List<ElementInfo> allElements = new ArrayList<>();
        Set<String> seenRegistryNames = new HashSet<>();

        int processed = 0;
        for (Path javaFile : javaFiles) {
            try {
                String content = new String(Files.readAllBytes(javaFile));
                String filePath = javaFile.toString();
                
                extractRealRegistryElements(content, filePath, modId, seenRegistryNames, allElements);

            } catch (Exception e) {
                result.errorCount++;
                if (result.errorCount <= 20) {
                    System.err.println("[ERROR] Failed to parse " + javaFile.getFileName() + ": " + e.getMessage());
                }
            }
            processed++;
            if (processed % 50 == 0) {
                System.out.println("[INFO] Processed " + processed + "/" + javaFiles.size() + " files");
            }
        }

        // 去重合并
        Map<String, ElementInfo> uniqueElements = new LinkedHashMap<>();
        for (ElementInfo elem : allElements) {
            String key = elem.registryName;
            if (!uniqueElements.containsKey(key)) {
                uniqueElements.put(key, elem);
            } else {
                ElementInfo existing = uniqueElements.get(key);
                for (String file : elem.files) {
                    if (!existing.files.contains(file)) {
                        existing.files.add(file);
                    }
                }
                for (Map.Entry<String, String> entry : elem.metadata.entrySet()) {
                    if (!existing.metadata.containsKey(entry.getKey())) {
                        existing.metadata.put(entry.getKey(), entry.getValue());
                    }
                }
            }
        }
        result.elements = new ArrayList<>(uniqueElements.values());

        // ========== 构建 language_map ==========
        Map<String, Map<String, String>> langMap = new HashMap<>();
        Map<String, String> enUs = new HashMap<>();
        String displayName = modName;
        if (displayName.contains("&")) {
            displayName = displayName.replace("&", "\\u0026");
        }
        enUs.put("item_group." + modId + "." + modId, displayName);
        langMap.put("en_us", enUs);
        result.languageMap = langMap;

        // ========== 构建 tab_element_order（使用下划线格式） ==========
        Map<String, List<String>> tabOrder = new LinkedHashMap<>();
        List<String> orderList = new ArrayList<>();
        for (ElementInfo elem : result.elements) {
            if (elem.type.equals("item") || elem.type.equals("block")) {
                String regName = elem.registryName.replace(":", "_");
                orderList.add(regName);
            }
        }
        if (!orderList.isEmpty()) {
            tabOrder.put(modId, orderList);
        }
        result.tabElementOrder = tabOrder;

        // 复制 elements 到 mod_elements
        result.mod_elements = new ArrayList<>(result.elements);

        System.out.println("[INFO] Elements found: " + result.elements.size());
        System.out.println("[INFO] Tab order elements: " + orderList.size());
        return result;
    }

    // ========== 只提取真正的注册元素 ==========
    private static void extractRealRegistryElements(String content, String filePath, String modId, 
                                                    Set<String> seenRegistryNames, List<ElementInfo> elements) {
        String packageName = "";
        Pattern packagePattern = Pattern.compile("package\\s+([\\w.]+);");
        Matcher packageMatcher = packagePattern.matcher(content);
        if (packageMatcher.find()) {
            packageName = packageMatcher.group(1);
        }

        // 1. @ObjectHolder
        Pattern objectHolderPattern = Pattern.compile(
            "@ObjectHolder\\s*\\(\\s*[\"']([^\"']+)[\"']\\s*\\)\\s*(?:public|private|protected)?\\s*(?:static)?\\s*(?:final)?\\s*\\w+\\s+(\\w+)\\s*;"
        );
        Matcher objectHolderMatcher = objectHolderPattern.matcher(content);
        while (objectHolderMatcher.find()) {
            String regName = objectHolderMatcher.group(1);
            String fieldName = objectHolderMatcher.group(2);
            if (!regName.contains(":")) {
                regName = modId + ":" + regName;
            }
            ElementInfo elem = new ElementInfo();
            elem.name = fieldName;
            elem.className = fieldName;
            elem.type = determineType(regName, content);
            elem.registryName = regName;
            elem.packageName = packageName;
            elem.sourceFile = filePath;
            elem.files.add(filePath);
            elem.isBlock = elem.type.equals("block");
            elem.modId = modId;
            String key = elem.registryName;
            if (!seenRegistryNames.contains(key)) {
                seenRegistryNames.add(key);
                elements.add(elem);
            }
        }

        // 2. DeferredRegister
        Pattern deferredRegPattern = Pattern.compile(
            "DeferredRegister\\s*<([^>]+)>\\s+(\\w+)\\s*=\\s*DeferredRegister\\.create\\s*\\([^,]+,\\s*[\"']([^\"']+)[\"']\\s*\\)"
        );
        Matcher deferredMatcher = deferredRegPattern.matcher(content);
        while (deferredMatcher.find()) {
            String typeName = deferredMatcher.group(1);
            String fieldName = deferredMatcher.group(2);
            boolean isBlock = typeName.toLowerCase().contains("block") || 
                             typeName.toLowerCase().contains("blockentity");
            ElementInfo elem = new ElementInfo();
            elem.name = fieldName;
            elem.className = fieldName;
            elem.type = isBlock ? "block" : "item";
            elem.registryName = modId + ":" + fieldName.toLowerCase();
            elem.packageName = packageName;
            elem.sourceFile = filePath;
            elem.files.add(filePath);
            elem.isBlock = isBlock;
            elem.modId = modId;
            String key = elem.registryName;
            if (!seenRegistryNames.contains(key)) {
                seenRegistryNames.add(key);
                elements.add(elem);
            }
        }

        // 3. DeferredRegister register() 调用
        Pattern deferredRegisterCallPattern = Pattern.compile(
            "(\\w+)\\.register\\s*\\(\\s*[\"']([^\"']+)[\"']\\s*,\\s*\\(\\)\\s*->\\s*new\\s+(\\w+)\\s*\\("
        );
        Matcher deferredCallMatcher = deferredRegisterCallPattern.matcher(content);
        while (deferredCallMatcher.find()) {
            String regName = deferredCallMatcher.group(2);
            String typeName = deferredCallMatcher.group(3);
            boolean isBlock = typeName.toLowerCase().contains("block");
            ElementInfo elem = new ElementInfo();
            elem.name = regName;
            elem.className = typeName;
            elem.type = isBlock ? "block" : "item";
            elem.registryName = modId + ":" + regName;
            elem.packageName = packageName;
            elem.sourceFile = filePath;
            elem.files.add(filePath);
            elem.isBlock = isBlock;
            elem.modId = modId;
            String key = elem.registryName;
            if (!seenRegistryNames.contains(key)) {
                seenRegistryNames.add(key);
                elements.add(elem);
            }
        }

        // 4. RegistryObject
        Pattern registryObjectPattern = Pattern.compile(
            "RegistryObject\\s*<[^>]+>\\s+(\\w+)\\s*=\\s*\\w+\\.register\\s*\\(\\s*[\"']([^\"']+)[\"']\\s*,"
        );
        Matcher registryObjectMatcher = registryObjectPattern.matcher(content);
        while (registryObjectMatcher.find()) {
            String fieldName = registryObjectMatcher.group(1);
            String regName = registryObjectMatcher.group(2);
            ElementInfo elem = new ElementInfo();
            elem.name = fieldName;
            elem.className = fieldName;
            elem.type = determineType(regName, content);
            elem.registryName = modId + ":" + regName;
            elem.packageName = packageName;
            elem.sourceFile = filePath;
            elem.files.add(filePath);
            elem.isBlock = elem.type.equals("block");
            elem.modId = modId;
            String key = elem.registryName;
            if (!seenRegistryNames.contains(key)) {
                seenRegistryNames.add(key);
                elements.add(elem);
            }
        }

        // 5. @SubscribeEvent
        Pattern subscribeEventPattern = Pattern.compile(
            "@SubscribeEvent\\s+(?:public|private|protected)?\\s*(?:static)?\\s*void\\s+(\\w+)\\s*\\(\\s*(\\w+)\\s+\\w+\\s*\\)"
        );
        Matcher subscribeMatcher = subscribeEventPattern.matcher(content);
        while (subscribeMatcher.find()) {
            String methodName = subscribeMatcher.group(1);
            String eventClass = subscribeMatcher.group(2);
            ElementInfo elem = new ElementInfo();
            elem.name = methodName;
            elem.type = "event";
            elem.isEvent = true;
            elem.registryName = methodName + "_event";
            elem.packageName = packageName;
            elem.sourceFile = filePath;
            elem.files.add(filePath);
            elem.modId = modId;
            elem.metadata.put("eventClass", eventClass);
            String key = elem.registryName;
            if (!seenRegistryNames.contains(key)) {
                seenRegistryNames.add(key);
                elements.add(elem);
            }
        }

        // 6. @Mod.EventHandler
        Pattern eventHandlerPattern = Pattern.compile(
            "@Mod\\.EventHandler\\s+(?:public|private|protected)?\\s*void\\s+(\\w+)\\s*\\(\\s*\\w+\\s+\\w+\\s*\\)"
        );
        Matcher eventHandlerMatcher = eventHandlerPattern.matcher(content);
        while (eventHandlerMatcher.find()) {
            String methodName = eventHandlerMatcher.group(1);
            ElementInfo elem = new ElementInfo();
            elem.name = methodName;
            elem.type = "event";
            elem.isEvent = true;
            elem.registryName = methodName + "_event_fml";
            elem.packageName = packageName;
            elem.sourceFile = filePath;
            elem.files.add(filePath);
            elem.modId = modId;
            elem.metadata.put("eventType", "FML");
            String key = elem.registryName;
            if (!seenRegistryNames.contains(key)) {
                seenRegistryNames.add(key);
                elements.add(elem);
            }
        }

        // 7. @CapabilityInject
        Pattern capabilityPattern = Pattern.compile(
            "@CapabilityInject\\s*\\(\\s*(\\w+)\\.class\\s*\\)\\s*(?:public|private|protected)?\\s*(?:static)?\\s*\\w+\\s+(\\w+)\\s*;"
        );
        Matcher capabilityMatcher = capabilityPattern.matcher(content);
        while (capabilityMatcher.find()) {
            String capClass = capabilityMatcher.group(1);
            String fieldName = capabilityMatcher.group(2);
            ElementInfo elem = new ElementInfo();
            elem.name = fieldName;
            elem.type = "capability";
            elem.isCapability = true;
            elem.registryName = "capability_" + fieldName.toLowerCase();
            elem.packageName = packageName;
            elem.sourceFile = filePath;
            elem.files.add(filePath);
            elem.modId = modId;
            elem.metadata.put("capabilityClass", capClass);
            String key = elem.registryName;
            if (!seenRegistryNames.contains(key)) {
                seenRegistryNames.add(key);
                elements.add(elem);
            }
        }
    }

    // ========== 辅助方法 ==========
    private static String determineType(String name, String content) {
        String lower = name.toLowerCase();
        if (lower.contains("block") || lower.contains("blockentity")) {
            return "block";
        }
        if (content.contains("extends Block") || content.contains("implements Block")) {
            return "block";
        }
        return "item";
    }

    // ========== TOML 解析 ==========
    private static String extractTomlValue(String toml, String key) {
        Pattern pattern = Pattern.compile(
            "(?:^|\\n)\\s*" + key + "\\s*=\\s*[\"']([^\"']+)[\"']",
            Pattern.CASE_INSENSITIVE
        );
        Matcher matcher = pattern.matcher(toml);
        if (matcher.find()) {
            return matcher.group(1).trim();
        }
        
        pattern = Pattern.compile(
            "(?:^|\\n)\\s*" + key + "\\s*=\\s*([^\\n#]+)",
            Pattern.CASE_INSENSITIVE
        );
        matcher = pattern.matcher(toml);
        if (matcher.find()) {
            String value = matcher.group(1).trim();
            int commentIdx = value.indexOf('#');
            if (commentIdx > 0) {
                value = value.substring(0, commentIdx).trim();
            }
            return value;
        }
        
        return "";
    }

    // ========== 从 [[mods]] 块提取 modId ==========
    private static String extractModIdFromModsBlock(String toml) {
        Pattern modsBlockPattern = Pattern.compile(
            "\\[\\[mods\\]\\]\\s*\\n([^\\[]+)",
            Pattern.DOTALL
        );
        Matcher modsMatcher = modsBlockPattern.matcher(toml);
        if (modsMatcher.find()) {
            String blockContent = modsMatcher.group(1);
            Pattern idPattern = Pattern.compile(
                "modId\\s*=\\s*[\"']([^\"']+)[\"']"
            );
            Matcher idMatcher = idPattern.matcher(blockContent);
            if (idMatcher.find()) {
                return idMatcher.group(1).trim();
            }
        }
        return "";
    }

    // ========== ANTLR Listener ==========
    private static class ModExtractor extends JavaParserBaseListener {
        private final String sourceFile;
        private final String modId;
        private final Set<String> seenRegistryNames;
        private final List<ElementInfo> elements;
        private final List<String> errors;
        
        private String currentPackage = "";
        private String currentClassName = "";
        private List<String> currentAnnotations = new ArrayList<>();
        private boolean inClass = false;

        public ModExtractor(String sourceFile, String modId, Set<String> seenRegistryNames, 
                           List<ElementInfo> elements, List<String> errors) {
            this.sourceFile = sourceFile;
            this.modId = modId;
            this.seenRegistryNames = seenRegistryNames;
            this.elements = elements;
            this.errors = errors;
        }

        @Override
        public void enterPackageDeclaration(JavaParser.PackageDeclarationContext ctx) {
            if (ctx.qualifiedName() != null) {
                currentPackage = ctx.qualifiedName().getText();
            }
        }

        @Override
        public void enterClassDeclaration(JavaParser.ClassDeclarationContext ctx) {
            inClass = true;
            String fullText = ctx.getText();
            Pattern p = Pattern.compile(
                "(?:public\\s+|private\\s+|protected\\s+|abstract\\s+|final\\s+|static\\s+)*class\\s+(\\w+)"
            );
            Matcher m = p.matcher(fullText);
            if (m.find()) {
                currentClassName = m.group(1);
            } else {
                currentClassName = "UnknownClass";
            }
            currentAnnotations.clear();
        }

        @Override
        public void exitClassDeclaration(JavaParser.ClassDeclarationContext ctx) {
            inClass = false;
        }

        @Override
        public void enterAnnotation(JavaParser.AnnotationContext ctx) {
            if (ctx.qualifiedName() != null) {
                String annName = ctx.qualifiedName().getText();
                currentAnnotations.add(annName);
            }
        }
    }
}