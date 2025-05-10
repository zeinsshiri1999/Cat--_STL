set(MODULE_NAME "Cat++_test")

set(MODULE_SRC_FILES "${MODULE_NAME}_SRC_FILES")
file(GLOB ${MODULE_SRC_FILES}  
  "./src/*.h"
  "./src/*.cpp"
  "./src/*.c"
  "./src/*.cc"  
)

#set(MODULE_INC_FILES "${MODULE_NAME}_INC_FILES")
#file(GLOB ${MODULE_INC_FILES} 
#  "./inc/*.h"
#)


# 1. 纯头文件库不需要编译为 .so 或 .a 文件，只需提供头文件路径
# 2. INTERFACE 库表示这是一个纯头文件库，CMake 不会尝试编译它
# 3. 用户只需 #include 头文件，无需链接库文件，简化集成
# 4. 后续如果需要编译为库，再改为 SHARED 或 STATIC
add_library(${MODULE_NAME} INTERFACE)

# 头文件路径必须对外公开，否则主工程和其他依赖无法找到本库的头文件
# PUBLIC 表示本库和所有依赖本库的目标都能访问该路径
# ${CMAKE_CURRENT_SOURCE_DIR}/src 是头文件实际存放路径
# 这样主工程 target_link_libraries 后可直接 #include "Cat++_PerformanceTest.h"
target_include_directories(${MODULE_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 如果有依赖项，取消注释并添加
# target_link_libraries(${MODULE_NAME} math_utils)