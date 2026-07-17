cmake_minimum_required(VERSION 3.16)
project(NeoForgeToFabricConverter VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 如果所有实现都在头文件中，只需要编译 main.cpp
add_executable(converter main.cpp)

# 如果需要分开编译，取消下面的注释并确保有对应的 .cpp 文件
# add_executable(converter
#     main.cpp
#     Converter.cpp
#     RuleEngine.cpp
#     ASTParser.cpp
# )

# 设置输出目录
set_target_properties(converter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Windows下可能需要链接额外库
if(WIN32)
    target_link_libraries(converter)
endif()

# 打印构建信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")