#pragma once

namespace Cat {

// 性能模式枚举：通过模板参数控制泛型类在编译期选择不同的实现
enum class Mode {
    Safe,      // 安全模式：使用智能指针、边界检查等安全特性
    Fast       // 高性能模式：使用裸指针、无边界检查等性能特性
};

} // namespace Cat 