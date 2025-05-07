# 指定最低 CMake 版本  
cmake_minimum_required(VERSION 3.11)        

# 格式：项目名，版本号，描述，语言，${CMAKE_PROJECT_NAME}引用名称
project(Cat++STL VERSION 1.0.0
    DESCRIPTION "Very nice project"
    LANGUAGES CXX)                         

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)#不设或设为False，当编译器不支持对应标准，尝试用上一版标准编译
#set(CMAKE_CXX_EXTENSIONS ON) # 允许用C++扩展特性，否则编译失败

# 构建可执行文件
file(GLOB SRC_FILES 
  "${PROJECT_SOURCE_DIR}/src/*.h"
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/*.c"
  "${PROJECT_SOURCE_DIR}/src/*.cc"
)
add_executable(${CMAKE_PROJECT_NAME} ${SRC_FILES})# 构建可执行文件

# 配置编译选项
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")#变量gcc_like_cxx如果用CXX且用下列任一编译器，值为true
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE 
  "$<${gcc_like_cxx}:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>"
  "$<${msvc_cxx}:-W3>"
  "$<$<CONFIG:Debug>:-g;-O0>"  
  "$<$<CONFIG:Release>:-O3;-DNDEBUG>"  
)

# 条件添加本地依赖
option(USE_UTIL1 "注释" OFF)                            #提供一个布尔变量,生成时用用-D指定值：cmake . -DUSE_Module1=OFF
if(USE_UTIL1)
    add_subdirectory(./util/util1)              #添加本地依赖目录，要有CMakeLists.txt
    list(APPEND EXTRA_LIBS UTIL1)               #把本地依赖库添加到库变量;./util/util1的库UTIL1
elseif()

else()
    
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${EXTRA_LIBS})#指定EXTRA_LIBS变量中链接库

# 添加第三方依赖
# find_package(fmt CONFIG REQUIRED)#用配置模式查找。这种模式下，CMake 会尝试找到名为 oatppConfig.cmake 或 oatpp-config.cmake 的文件来配置包的相关信息

# target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE fmt::fmt)#链接库

# c++导入此头文件可访问set变量
set(CMAKE_CONFIG_IN_FILE "${PROJECT_SOURCE_DIR}/CmakeConfig.h.in")
set(CMAKE_CONFIG_H_FILE "${PROJECT_BINARY_DIR}/CmakeConfig.h")
# ** 检查 CmakeConfig.h.in 是否存在，如果不存在则自动创建默认文件 **
if(NOT EXISTS ${CMAKE_CONFIG_IN_FILE})
    file(WRITE ${CMAKE_CONFIG_IN_FILE} "")
endif()

configure_file(${CMAKE_CONFIG_IN_FILE} ${CMAKE_CONFIG_H_FILE})#将.in中set变量替换为值，再输出到.h文件
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${PROJECT_BINARY_DIR})#让C++可搜索build文件夹下的头文件CmakeConfig.h

