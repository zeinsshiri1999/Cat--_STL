#pragma once
#include "./Cat++_allocator.h"
#include "./Cat++_pool_alloc.h"
#include "../execption/allocator_exception.h"
#include <memory>

namespace Cat {

// 分配器类型标签
struct stl_tag {};      // 标准分配器
struct simple_tag {};   // 简单分配器
struct pool_tag {};     // 池分配器

// 分配器选择器：根据Tag选择不同的分配器实现
template<bool threads, typename T, typename Tag = pool_tag>
class alloc;

// STL分配器特化
template<bool threads, typename T>
class alloc<threads, T, stl_tag> : public std::allocator<T> {
    using type = std::allocator<T>;
public:
    using type::type;  // 继承构造函数

    // 继承类型
    using type::value_type;
    using type::pointer;
    using type::const_pointer;
    using type::reference;
    using type::const_reference;
    using type::size_type;
    using type::difference_type;
};

// 简单分配器特化
template<bool threads, typename T>
class alloc<threads, T, simple_tag> : public allocator<threads, T> {
    using type = allocator<threads, T>;
public:
    using type::type;  // 继承构造函数

    // 继承类型
    using type::value_type;
    using type::pointer;
    using type::const_pointer;
    using type::reference;
    using type::const_reference;
    using type::size_type;
    using type::difference_type;
};

// 池分配器特化
template<bool threads, typename T>
class alloc<threads, T, pool_tag> : public pool_allocator<threads, T> {
    using type = pool_allocator<threads, T>;
public:
    using type::type;  // 继承构造函数

    // 继承类型
    using type::value_type;
    using type::pointer;
    using type::const_pointer;
    using type::reference;
    using type::const_reference;
    using type::size_type;
    using type::difference_type;

};

} // namespace Cat
