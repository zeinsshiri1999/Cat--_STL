#pragma once
#include <cstddef>
#include <iterator>
#include <utility>  // for std::pair
#include <memory>   // for smart pointers
#include "../Cat++_config.h"  // for VectorMode

namespace Cat {


// 安全模式：使用智能指针和引用包装器，提供内存安全和边界检查
// 高性能模式：使用原始指针和引用，提供最大性能
template<VectorMode Mode, typename T>
struct mode_traits {
    // 基础类型：所有模式通用
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    // 高性能模式类型：直接使用原始指针和引用
    using raw_pointer = T*;
    using raw_const_pointer = const T*;
    using raw_reference = T&;
    using raw_const_reference = const T&;

    // 安全模式类型：使用智能指针和引用包装器
    using safe_pointer = std::shared_ptr<T>;
    using safe_const_pointer = std::shared_ptr<const T>;
    using safe_reference = std::reference_wrapper<T>;
    using safe_const_reference = std::reference_wrapper<const T>;

    // 根据模式选择最终使用的类型
    // Safe模式：使用安全类型，提供内存安全和边界检查
    // Fast模式：使用高性能类型，提供最大性能
    using pointer = std::conditional_t<Mode == VectorMode::Safe, safe_pointer, raw_pointer>;
    using const_pointer = std::conditional_t<Mode == VectorMode::Safe, safe_const_pointer, raw_const_pointer>;
    using reference = std::conditional_t<Mode == VectorMode::Safe, safe_reference, raw_reference>;
    using const_reference = std::conditional_t<Mode == VectorMode::Safe, safe_const_reference, raw_const_reference>;

    // 迭代器类型：根据模式选择不同的迭代器实现
    // Safe模式：使用智能指针作为迭代器，提供内存安全
    // Fast模式：使用原始指针作为迭代器，提供最大性能
    using iterator = std::conditional_t<Mode == VectorMode::Safe,
        std::shared_ptr<T>,
        T*>;
    using const_iterator = std::conditional_t<Mode == VectorMode::Safe,
        std::shared_ptr<const T>,
        const T*>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
};

// 基础类型特征：继承自模式特征
template<typename T, VectorMode Mode = VectorMode::Safe>
struct base_traits : public mode_traits<Mode, T> {
    using base = mode_traits<Mode, T>;
    using typename base::value_type;
    using typename base::size_type;
    using typename base::difference_type;
    using typename base::pointer;
    using typename base::const_pointer;
    using typename base::reference;
    using typename base::const_reference;
    using typename base::iterator;
    using typename base::const_iterator;
    using typename base::reverse_iterator;
    using typename base::const_reverse_iterator;
};

// 分配器类型特征：继承自基础类型特征
template<typename T, VectorMode Mode = VectorMode::Safe>
struct allocator_traits : public base_traits<T, Mode> {
    using base = base_traits<T, Mode>;
    using base::value_type;
    using base::pointer;
    using base::const_pointer;
    using base::reference;
    using base::const_reference;
    using base::size_type;
    using base::difference_type;
    using base::iterator;
    using base::const_iterator;
    using base::reverse_iterator;
    using base::const_reverse_iterator;
};

// 序列容器类型特征：继承自基础类型特征
template<typename T, typename Allocator, VectorMode Mode = VectorMode::Safe>
struct sequence_traits : public base_traits<T, Mode> {
    using base = base_traits<T, Mode>;
    using base::value_type;
    using allocator_type = Allocator;  // 序列容器特有的分配器类型
    using base::size_type;
    using base::difference_type;
    using base::pointer;
    using base::const_pointer;
    using base::reference;
    using base::const_reference;
    using base::iterator;
    using base::const_iterator;
    using base::reverse_iterator;
    using base::const_reverse_iterator;
};

// 关联容器类型特征
template<typename Key, typename Value, typename Allocator, VectorMode Mode = VectorMode::Safe>
struct associative_traits : public base_traits<std::pair<const Key, Value>, Mode> {
    using base = base_traits<std::pair<const Key, Value>, Mode>;
    using key_type = Key;  // 关联容器特有的键类型
    using base::value_type;
    using allocator_type = Allocator;
    using base::size_type;
    using base::difference_type;
    using base::pointer;
    using base::const_pointer;
    using base::reference;
    using base::const_reference;
    using base::iterator;
    using base::const_iterator;
    using base::reverse_iterator;
    using base::const_reverse_iterator;
};

// CRTP类型继承基类：显式继承类型特征
template<typename Derived, typename Traits>
struct type_inherit : public Traits {
    // 只做类型别名继承
};

// 类型别名：简化类型继承的使用
template<typename T, VectorMode Mode = VectorMode::Safe>
using base_type = type_inherit<T, base_traits<T, Mode>>;

template<typename T, VectorMode Mode = VectorMode::Safe>
using allocator_type = type_inherit<T, allocator_traits<T, Mode>>;

template<typename T, typename Allocator, VectorMode Mode = VectorMode::Safe>
using sequence_type = type_inherit<T, sequence_traits<T, Allocator, Mode>>;

template<typename Key, typename Value, typename Allocator, VectorMode Mode = VectorMode::Safe>
using associative_type = type_inherit<std::pair<const Key, Value>, associative_traits<Key, Value, Allocator, Mode>>;

} // namespace Cat 