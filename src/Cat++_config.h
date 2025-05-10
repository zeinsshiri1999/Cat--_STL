#pragma once
#include "./alloc/Cat++_allocator.h"
#include "./alloc/Cat++_pool_alloc.h"

#if defined(__GNUG__) && !defined(__clang__)
    // GCC
    #include <ext/pool_allocator.h>
    #define STL_ALLOCATOR(threads, T) __gnu_cxx::__pool_alloc<T>
#else
    // Clang 及其他编译器
    #include <memory>
    #define STL_ALLOCATOR(threads, T) std::allocator<T>
#endif

namespace Cat {

// 分配器类型标签
struct stl_tag {};
struct simple_tag {};
struct pool_tag {};

// 分配器选择器
template<bool threads, typename T, typename Tag = pool_tag>
class alloc;

// STL分配器特化
template<bool threads, typename T>
class alloc<threads, T, stl_tag> : public STL_ALLOCATOR(threads, T) {
public:
    using type = STL_ALLOCATOR(threads, T);
};

// 简单分配器特化
template<bool threads, typename T>
class alloc<threads, T, simple_tag> : public allocator<threads, T> {
public:
    using type = allocator<threads, T>;
};

// 池分配器特化
template<bool threads, typename T>
class alloc<threads, T, pool_tag> : public pool_allocator<threads, T> {
public:
    using type = pool_allocator<threads, T>;
};

} // namespace Cat

#undef STL_ALLOCATOR
