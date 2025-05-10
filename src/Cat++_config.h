#pragma once
#include "./alloc/Cat++_allocator.h"
#include "./alloc/Cat++_pool_alloc.h"
#include <memory>

namespace Cat {

// 配置器类型枚举
enum class AllocatorType {
    DEFAULT,    // 使用STL默认配置器
    SIMPLE,     // 使用Cat++_allocator
    POOL        // 使用Cat++_pool_allocator
};

// 配置器选择器
template<typename T, AllocatorType Type = AllocatorType::DEFAULT>
class allocator_selector {
public:
    using type = typename std::conditional<
        Type == AllocatorType::DEFAULT,
        std::allocator<T>,//true则选择std::allocator<T>
        typename std::conditional<
            Type == AllocatorType::SIMPLE,
            allocator<true, T>,//true则选择allocator<true, T>
            pool_allocator<true, T>
        >::type
    >::type;
};

// 便捷类型别名
template<typename T, AllocatorType Type = AllocatorType::DEFAULT>
using alloc_t = typename allocator_selector<T, Type>::type;

}
