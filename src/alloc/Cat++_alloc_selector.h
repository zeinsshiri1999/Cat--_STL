#pragma once
#include "./Cat++_allocator.h"
#include "./Cat++_pool_alloc.h"
#include "../execption/allocator_exception.h"
#include <memory>
#include "../traits/Cat++_type_traits.h"

namespace Cat {

// 分配器类型标签
struct stl_tag {};      // 标准分配器
struct simple_tag {};   // 简单分配器
struct pool_tag {};     // 池分配器

// 分配器选择器：根据Tag选择不同的分配器实现
// 线程安全由外部保证，分配器本身不保证线程安全
template<typename T, typename Tag = pool_tag, VectorMode Mode = VectorMode::Safe>
class alloc;

// STL分配器特化
template<typename T>
class alloc<T, stl_tag> : private std::allocator<T> {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template<typename U>
    struct rebind {
        using other = alloc<U, stl_tag>;
    };

    using std::allocator<T>::allocate;
    using std::allocator<T>::deallocate;
    using std::allocator<T>::construct;
    using std::allocator<T>::destroy;
};

// 简单分配器特化
template<typename T, VectorMode Mode>
class alloc<T, simple_tag, Mode> : private allocator<T, Mode>, public mode_traits<Mode, T> {
public:
    using value_type = T;
    using pointer = typename mode_traits<Mode, T>::pointer;
    using const_pointer = typename mode_traits<Mode, T>::const_pointer;
    using reference = typename mode_traits<Mode, T>::reference;
    using const_reference = typename mode_traits<Mode, T>::const_reference;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = typename mode_traits<Mode, T>::iterator;
    using const_iterator = typename mode_traits<Mode, T>::const_iterator;
    using reverse_iterator = typename mode_traits<Mode, T>::reverse_iterator;
    using const_reverse_iterator = typename mode_traits<Mode, T>::const_reverse_iterator;

    template<typename U>
    struct rebind {
        using other = alloc<U, simple_tag, Mode>;
    };

    using allocator<T, Mode>::allocate;
    using allocator<T, Mode>::deallocate;
    using allocator<T, Mode>::construct;
    using allocator<T, Mode>::destroy;
};

// 池分配器特化
template<typename T, VectorMode Mode>
class alloc<T, pool_tag, Mode> : private pool_allocator<T, Mode>, public mode_traits<Mode, T> {
public:
    using value_type = T;
    using pointer = typename mode_traits<Mode, T>::pointer;
    using const_pointer = typename mode_traits<Mode, T>::const_pointer;
    using reference = typename mode_traits<Mode, T>::reference;
    using const_reference = typename mode_traits<Mode, T>::const_reference;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = typename mode_traits<Mode, T>::iterator;
    using const_iterator = typename mode_traits<Mode, T>::const_iterator;
    using reverse_iterator = typename mode_traits<Mode, T>::reverse_iterator;
    using const_reverse_iterator = typename mode_traits<Mode, T>::const_reverse_iterator;

    template<typename U>
    struct rebind {
        using other = alloc<U, pool_tag, Mode>;
    };

    using pool_allocator<T, Mode>::allocate;
    using pool_allocator<T, Mode>::deallocate;
    using pool_allocator<T, Mode>::construct;
    using pool_allocator<T, Mode>::destroy;
};

} // namespace Cat
