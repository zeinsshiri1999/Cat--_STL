#=============================================================================
# 项目基础配置
#=============================================================================
cmake_minimum_required(VERSION 3.11)
project(Cat++STL 
    VERSION 1.0.0
    DESCRIPTION "A custom STL implementation inspired by SGI STL"
    LANGUAGES CXX
)

#=============================================================================
# C++标准配置
#=============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # 强制要求C++20支持，否则编译失败
set(CMAKE_CXX_EXTENSIONS OFF)        # 禁用编译器扩展，确保标准兼容性

#=============================================================================
# 编译器选项配置
#=============================================================================
# 识别编译器类型
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

# 通用编译选项
set(COMMON_COMPILE_OPTIONS
    "$<${gcc_like_cxx}:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>"  # GCC类编译器选项
    "$<${msvc_cxx}:-W3>"                                            # MSVC编译器选项
)

# 调试模式编译选项
set(DEBUG_COMPILE_OPTIONS
    "$<$<CONFIG:Debug>:-g;-O0>"  # -g: 生成调试信息, -O0: 禁用优化
)

# 发布模式编译选项
set(RELEASE_COMPILE_OPTIONS
    "$<$<CONFIG:Release>:-O3;-DNDEBUG>"  # -O3: 最高优化级别, -DNDEBUG: 禁用断言
)

#=============================================================================
# 接口(头文件)收集 & 源文件收集
#=============================================================================
# 递归收集所有头文件
file(GLOB_RECURSE HEAD_FILES 
    "${PROJECT_SOURCE_DIR}/src/**/*.h"
)

# 递归收集所有源文件
file(GLOB_RECURSE SRC_FILES 
    "${PROJECT_SOURCE_DIR}/src/**/*.cpp"
    "${PROJECT_SOURCE_DIR}/src/**/*.c"
    "${PROJECT_SOURCE_DIR}/src/**/*.cc"
)

#=============================================================================
# 主库目标
#=============================================================================
add_library(${CMAKE_PROJECT_NAME} INTERFACE)
target_include_directories(${CMAKE_PROJECT_NAME} INTERFACE ${PROJECT_SOURCE_DIR}/src)
target_compile_options(${CMAKE_PROJECT_NAME} INTERFACE 
    ${COMMON_COMPILE_OPTIONS}
    ${DEBUG_COMPILE_OPTIONS}
    ${RELEASE_COMPILE_OPTIONS}
)

#=============================================================================
# 本地依赖(util)管理
#=============================================================================
# 如果util下有本地依赖库，可以用add_subdirectory(util/xxx)
# 例如:
add_subdirectory(util/dev_dependency/Cat++_test)
# target_link_libraries(CatppSTL PUBLIC my_local_lib)

#=============================================================================
# 第三方依赖(module)管理
#=============================================================================
# 如果module下有第三方依赖库，可以用add_subdirectory(module/xxx)
# find_package(fmt CONFIG REQUIRED)#用配置模式查找。这种模式下，CMake 会尝试找到名为 oatppConfig.cmake 或 oatpp-config.cmake 的文件来配置包的相关信息
# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE fmt::fmt)#链接库

#=============================================================================
# 测试目标
#=============================================================================
file(GLOB_RECURSE TEST_SOURCES "${PROJECT_SOURCE_DIR}/test/*.cpp")
add_executable(test_alloc ${TEST_SOURCES})

target_link_libraries(test_alloc PRIVATE ${CMAKE_PROJECT_NAME} Cat++_test)
target_include_directories(test_alloc PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/util
    ${PROJECT_SOURCE_DIR}/util/dev_dependency/Cat++_test/src
)

target_compile_options(test_alloc PRIVATE
    ${COMMON_COMPILE_OPTIONS}
    ${DEBUG_COMPILE_OPTIONS}
    ${RELEASE_COMPILE_OPTIONS}
)

#=============================================================================
# 开发/测试工具依赖
#=============================================================================
# add_library(CatppTestFramework INTERFACE)
# target_include_directories(CatppTestFramework INTERFACE ${PROJECT_SOURCE_DIR}/util/Cat++_test)
# target_link_libraries(test_alloc PRIVATE CatppTestFramework)

#=============================================================================
# 配置头文件生成，c++导入此头文件可访问set变量
#=============================================================================
# 配置头文件路径
set(CMAKE_CONFIG_IN_FILE "${PROJECT_SOURCE_DIR}/CmakeConfig.h.in")
set(CMAKE_CONFIG_H_FILE "${PROJECT_BINARY_DIR}/CmakeConfig.h")
# 确保配置头文件模板存在
if(NOT EXISTS ${CMAKE_CONFIG_IN_FILE})
    file(WRITE ${CMAKE_CONFIG_IN_FILE} "")
endif()

# 生成配置头文件
# 将.in文件中的变量替换为值，输出到.h文件
configure_file(${CMAKE_CONFIG_IN_FILE} ${CMAKE_CONFIG_H_FILE})
# 添加配置头文件目录到包含路径，让C++可搜索build文件夹下的头文件CmakeConfig.h
target_include_directories(${CMAKE_PROJECT_NAME} INTERFACE ${PROJECT_BINARY_DIR})

#=============================================================================
# 备注
#=============================================================================
# - 只将src/和util/暴露为头文件包含路径，module/一般由第三方库自己管理
# - 本地依赖和第三方依赖建议用add_subdirectory方式集成，便于维护
# - 测试/开发依赖只链接到测试目标，避免污染主库