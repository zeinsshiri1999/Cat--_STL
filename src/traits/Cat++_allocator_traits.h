#pragma once
#include "Cat++_type_traits.h" // 引入基础类型特征
#include <type_traits>  // 用于 std::is_empty_v

namespace Cat {

// 分配器类型特征：继承自基础类型特征
template<typename T, VectorMode Mode = VectorMode::Safe>
struct allocator_traits : public base_traits<T, Mode> {
    using base = base_traits<T, Mode>;
    using typename base::value_type;
    using typename base::pointer;
    using typename base::const_pointer;
    using typename base::reference;
    using typename base::const_reference;
    using typename base::size_type;
    using typename base::difference_type;
    using typename base::iterator;
    using typename base::const_iterator;
    using typename base::reverse_iterator;
    using typename base::const_reverse_iterator;

    // 使用基类的 rebind 机制
    template<typename U>
    using rebind = typename base::template rebind<U>;
    
    // 分配器特有的 rebind 别名
    template<typename U>
    using rebind_alloc = typename base::template rebind<U>::other;
    
    // 分配器的 traits 特化
    template<typename U>
    using rebind_traits = allocator_traits<rebind_alloc<U>, Mode>;

    // 获取类型大小
    static constexpr std::size_t type_size() noexcept {
        return sizeof(value_type);
    }
    
    // 获取类型对齐要求
    static constexpr std::size_t alignment() noexcept {
        return alignof(value_type);
    }
    
    // 计算最大可分配对象数
    static constexpr std::size_t max_allocation_size() noexcept {
        return std::size_t(-1) / sizeof(value_type);
    }
    
    // 提供标准的分配器特征检查
    static constexpr bool propagate_on_container_copy_assignment = false;
    static constexpr bool propagate_on_container_move_assignment = false;
    static constexpr bool propagate_on_container_swap = false;
    static constexpr bool is_always_equal = std::is_empty_v<T>;
};

} // namespace Cat 