#pragma once
#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include "../execption/allocator_exception.h"
#include "../traits/Cat++_type_traits.h"
#include "../traits/Cat++_allocator_traits.h"
//allocator interface

// 线程安全由外部保证，分配器本身不保证线程安全
// 尽量兼容STL接口规范
// 异常处理
namespace Cat {

template<class Derived, class T, VectorMode Mode = VectorMode::Safe>
class allocator_interface 
: public allocator_traits<T, Mode> {
public:
    // 只using受模式影响的类型
    using typename allocator_traits<T, Mode>::pointer;
    using typename allocator_traits<T, Mode>::const_pointer;
    using typename allocator_traits<T, Mode>::reference;
    using typename allocator_traits<T, Mode>::const_reference;
    using typename allocator_traits<T, Mode>::value_type;
    using typename allocator_traits<T, Mode>::size_type;
    using typename allocator_traits<T, Mode>::difference_type;
    using typename allocator_traits<T, Mode>::iterator;
    using typename allocator_traits<T, Mode>::const_iterator;
    using typename allocator_traits<T, Mode>::reverse_iterator;
    using typename allocator_traits<T, Mode>::const_reverse_iterator;

    // 异常处理函数类型
    typedef void(*exception_handler)();

    // 使用 traits 中的 rebind 机制实现重绑定
    // 允许容器在内部使用不同类型的分配器，如 vector<T> 要为迭代器分配内存时，需要 allocator<iterator> 而不是 allocator<T>
    template<typename U>
    struct rebind {
        using other = typename Derived::template rebind<U>::other;
    };
    
    // 标准库风格的 rebind 类型别名
    template<typename U>
    using rebind_alloc = typename rebind<U>::other;
    
    // 静态断言确保 rebind 后的分配器也继承自适当的 allocator_interface
    template<typename U>
    struct rebind_check {
        static_assert(
            std::is_base_of_v<
                allocator_interface<
                    typename Derived::template rebind<U>::other, 
                    U, 
                    Mode
                >, 
                typename Derived::template rebind<U>::other
            >,
            "Rebound allocator must inherit from allocator_interface"
        );
    };

    // 默认实现：地址操作
    [[deprecated("address is deprecated in C++17 and removed in C++20")]]
    [[nodiscard]] pointer address(reference x) noexcept {
        if constexpr (std::is_same_v<pointer, T*>) {
            return &x; // 编译期计算出函数结果
        } else {
            return pointer(&x); // 只能运行时返回智能指针
        }
    }

    [[deprecated("address is deprecated in C++17 and removed in C++20")]]
    [[nodiscard]] const_pointer address(const_reference x) noexcept {
        if constexpr (std::is_same_v<const_pointer, const T*>) {
            return &x; // 编译期计算出函数结果
        } else {
            return const_pointer(&x); // 只能运行时返回智能指针
        }
    }

    // 使用CRTP调用派生类实现，确保没有运行期绑定实现allocator_interface* ptr = new Derived();只有编译期绑定实现Derived.interface()
    [[nodiscard]] pointer allocate(std::size_t n, const void* = 0) {

        static_assert(
            std::is_same_v<
                decltype(std::declval<Derived>().allocate(n)), 
                pointer
            >,
            "Derived::allocate must return pointer type"
        );
        
        return static_cast<Derived*>(this)->allocate(n);
    }

    void deallocate(pointer ptr, std::size_t n) noexcept {

        static_assert(
            noexcept(std::declval<Derived>().deallocate(ptr, n)),
            "Derived::deallocate must be noexcept"
        );
        
        static_cast<Derived*>(this)->deallocate(ptr, n);
    }

    [[nodiscard]] pointer reallocate(pointer ptr, std::size_t old_size, std::size_t new_size) {

        static_assert(
            std::is_same_v<
                decltype(std::declval<Derived>().reallocate(ptr, old_size, new_size)), 
                pointer
            >,
            "Derived::reallocate must return pointer type"
        );
        
        return static_cast<Derived*>(this)->reallocate(ptr, old_size, new_size);
    }

    // 最大分配大小； const 接口表明：这是一个只读操作，调用这个函数不改变任何指向对象状态
    size_type max_size() const noexcept {

        static_assert(
            noexcept(std::declval<const Derived>().max_size()),
            "Derived::max_size must be noexcept"
        );
        
        return static_cast<const Derived*>(this)->max_size();
    }

    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {
        if (p == nullptr) return;
        try {
            ::new((void*)p) U(std::forward<Args>(args)...);
        } catch (const std::exception& e) {
            fprintf(stderr, "construct failed: %s\n", e.what());
            throw;
        }
    }

    template<typename U>
    void destroy(U* p) noexcept {
        if (p != nullptr) {
            try {
                p->~U();
            } catch (const std::exception& e) {
                fprintf(stderr, "destroy failed: %s\n", e.what());
            }
        }
    }

    // 获取类型对齐要求
    static constexpr std::size_t alignment() noexcept {
        return alignof(T);
    }
    
    // 计算最大可分配对象数
    static constexpr std::size_t max_allocation_size() noexcept {
        return std::size_t(-1) / sizeof(T);
    }

    // 增加接口完整性检查
    // 检查派生类是否实现了必要的接口
    template<typename D = Derived>
    static constexpr bool check_derived_implementation() {
        return 

            std::is_same_v<
                decltype(std::declval<D>().allocate(std::size_t{})), 
                pointer
            > &&
            

            noexcept(
                std::declval<D>().deallocate(
                    std::declval<pointer>(), 
                    std::size_t{}
                )
            ) &&
            

            std::is_same_v<
                decltype(std::declval<D>().reallocate(
                    std::declval<pointer>(), 
                    std::size_t{}, 
                    std::size_t{}
                )), 
                pointer
            > &&
            

            noexcept(std::declval<D>().max_size());
    }
    
    // 全局接口检查
    static_assert(
        check_derived_implementation(), 
        "Derived class must implement all required interfaces with correct signatures"
    );

protected:
    // 接口类禁用拷贝和移动：禁用实例化；CRTP基类防止出现任何动态绑定allocator_interface* ptr = new Derived();
    allocator_interface() = delete;
    allocator_interface(const allocator_interface&) = delete;
    allocator_interface(allocator_interface&&) = delete;
    allocator_interface& operator=(const allocator_interface&) = delete;
    allocator_interface& operator=(allocator_interface&&) = delete;
    ~allocator_interface() = delete;
};

// 基础实现
template<class T, VectorMode Mode = VectorMode::Safe>
class allocator final: public allocator_interface<allocator<T, Mode>, T, Mode> {
public:
    using Base = allocator_interface<allocator<T, Mode>, T, Mode>;
    using typename Base::pointer;
    using typename Base::const_pointer;
    using typename Base::reference;
    using typename Base::const_reference;
    using typename Base::value_type;
    using typename Base::size_type;
    using typename Base::difference_type;
    using typename Base::iterator;
    using typename Base::const_iterator;
    using typename Base::exception_handler;

    // 实现 allocator 的 rebind 机制，遵循 allocator_traits 接口规范
    template<typename U>
    struct rebind {
        using other = allocator<U, Mode>;
    };
    
    // 标准库风格的 rebind 类型别名
    template<typename U>
    using rebind_alloc = allocator<U, Mode>;

    allocator() noexcept {}
    template<typename U>
    allocator(const allocator<U, Mode>&) noexcept {}
    ~allocator() noexcept {}

    pointer address(reference x) const noexcept { return &x; }
    const_pointer address(const_reference x) const noexcept { return &x; }

    size_type max_size() const noexcept {
        return std::size_t(-1) / sizeof(T);
    }

    // 异常处理
    static exception_handler set_exception_handler(exception_handler f) noexcept {
        exception_handler old = alloc_oom_handler;
        alloc_oom_handler = f;
        return old;
    }

private:
    // 线程局部存储：每个线程独立的异常处理器
    static thread_local exception_handler alloc_oom_handler;
    
    // 内存分配失败处理版本
    [[nodiscard]] static pointer oom_malloc(std::size_t bytes) { 
        pointer result = static_cast<pointer>(malloc(bytes));
        if(result == nullptr) {
            for(;;) {
                if(alloc_oom_handler == nullptr) {
                    throw OutOfMemoryException();
                }
                alloc_oom_handler();
                result = static_cast<pointer>(malloc(bytes));
                if(result) {
                    return result;
                }
            }
        }
        return result;
    }
    
    // 内存重分配失败处理版本
    [[nodiscard]] static pointer oom_realloc(pointer ptr, std::size_t new_size) {
        pointer result = static_cast<pointer>(realloc(ptr, new_size));
        if(result == nullptr) {
            for(;;) {
                if(alloc_oom_handler == nullptr) {
                    throw OutOfMemoryException();
                }
                alloc_oom_handler();
                result = static_cast<pointer>(realloc(ptr, new_size));
                if(result) {
                    return result;
                }
            }
        }
        return result;
    }
    
public:
    // 内存分配
    [[nodiscard]] pointer allocate(std::size_t n) {
        if (n == 0) return nullptr;
        if (n > this->max_allocation_size()) {
            throw std::bad_alloc();
        }
        try {
            return oom_malloc(n * this->type_size());
        } catch (const std::exception& e) {
            fprintf(stderr, "alloc failed: %s\n", e.what());
            return nullptr;
        }
    }

    void deallocate(pointer ptr, std::size_t n) noexcept {//显式声明不抛异常，可安全调用
        if (ptr != nullptr) {
            ::free(ptr);
        }
    }

    // 内存重分配
    [[nodiscard]] pointer reallocate(pointer ptr, std::size_t old_size, std::size_t new_size) {
        if (new_size == 0) {
            deallocate(ptr, old_size);
            return nullptr;
        }
        if (new_size > this->max_allocation_size()) {
            throw std::bad_alloc();
        }
        try {
            return oom_realloc(ptr, new_size * this->type_size());
        } catch (const std::exception& e) {
            fprintf(stderr, "reallocate failed: %s\n", e.what());
            return nullptr;
        }   
    }

    // 虽然是按T类型分配的空间，但构造时用U指定类型更灵活，允许在容器内构造不同类型的对象（如迭代器）
    // typename... Args支持构造时参数指定任意类型，事实上泛型函数可根据入参自动推导泛型，所以调用时不用显式指定
    // 变量传递时会发生值拷贝，但左右值引用都是0拷贝的，const var&声明则用指针读取值，不拷贝；声明var&&传递右值则直接用右值
    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {//&&接受右值，或右值引用变量，但为了避免再写一个左值引用的版本，&&语法实际允许左值和左值引用输入
        if (p == nullptr) return;
        try {
            ::new((void*)p) U(std::forward<Args>(args)...);//Args&&... args声明了函数内部形参args值传递是0拷贝的，但形参args又变成了左值，std::forward<Args>(args)用于恢复值原始类型
        } catch (const std::exception& e) {
            fprintf(stderr, "construct failed: %s\n", e.what());
            throw;
        }
    }
    
    template<typename U>
    void destroy(U* p) noexcept {
        if (p != nullptr) {
            try {
                p->~U();
            } catch (const std::exception& e) {
                fprintf(stderr, "destroy failed: %s\n", e.what());
            }
        }
    }

    //allocator无状态，都相等；无状态分配器不需要右值版本
    template<typename T1, typename T2>
    friend bool operator==(const allocator<T1, Mode>&, const allocator<T2, Mode>&) noexcept {
        return true;
    }

    template<typename T1, typename T2>
    friend bool operator!=(const allocator<T1, Mode>&, const allocator<T2, Mode>&) noexcept {
        return false;
    }
};

// 初始化线程局部存储
template<class T, VectorMode Mode>
thread_local typename allocator<T, Mode>::exception_handler allocator<T, Mode>::alloc_oom_handler = nullptr;
}