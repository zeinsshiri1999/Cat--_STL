#pragma once
#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include "../execption/allocator_exception.h"
//allocator interface

//线程安全
//尽量兼容STL接口规范
//异常处理


namespace Cat {
    
template<bool threads, class T>
class allocator_interface {
public:
    // 向外传递类型
    typedef T           value_type;
    typedef T*          pointer;
    typedef const T*    const_pointer;
    typedef T&          reference;
    typedef const T&    const_reference;
    typedef size_t      size_type;
    typedef ptrdiff_t   difference_type;

    typedef void(*exception_handler)();

    //容器通常会指定alloc类型，但其内部的其他数据结构并不能用这个指定的类型，这时调用内部泛型类Alloc::template rebind<iterator>::other指定内部数据结构的迭代器
    template<typename U>
    struct rebind {
        using other = allocator_interface<threads, U>;
    };

    virtual T* allocate(size_t n) = 0;
    virtual void deallocate(T* ptr, size_t n) = 0;
    virtual T* reallocate(T* ptr, size_t old_size, size_t new_size) = 0;
    virtual size_t max_size() const = 0;

    virtual ~allocator_interface() = default;
};

// 基础实现
template<bool threads, class T>
class allocator : public allocator_interface<threads, T> {
public:
    // 继承接口的类型
    using typename allocator_interface<threads, T>::value_type;//不这样外部使用时，需要写allocator<false, int>::allocator_interface<false, int>::value_type
    using typename allocator_interface<threads, T>::pointer;
    using typename allocator_interface<threads, T>::const_pointer;
    using typename allocator_interface<threads, T>::reference;
    using typename allocator_interface<threads, T>::const_reference;
    using typename allocator_interface<threads, T>::size_type;
    using typename allocator_interface<threads, T>::difference_type;
    using typename allocator_interface<threads, T>::exception_handler;

    // 异常处理
    static exception_handler set_exception_handler(exception_handler f) {
        exception_handler old = alloc_oom_handler;
        alloc_oom_handler = f;
        return old;
    }

private:
    static inline exception_handler alloc_oom_handler = nullptr;
    
    static T* oom_malloc(size_t bytes) { 
        T* result = static_cast<T*>(malloc(bytes));
        if(result == nullptr) {
            for(;;) {
                if(alloc_oom_handler == nullptr) {
                    throw OutOfMemoryException();
                }
                alloc_oom_handler();
                result = static_cast<T*>(malloc(bytes));
                if(result) {
                    return result;
                }
            }
        }
        return result;
    }
    
    static T* oom_realloc(T* ptr, size_t new_size) {
        T* result = static_cast<T*>(realloc(ptr, new_size));
        if(result == nullptr) {
            for(;;) {
                if(alloc_oom_handler == nullptr) {
                    throw OutOfMemoryException();
                }
                alloc_oom_handler();
                result = static_cast<T*>(realloc(ptr, new_size));
                if(result) {
                    return result;
                }
            }
        }
        return result;
    }
    
public:
    //虽然这些函数不绑定实例数据，但它们需要支持多态，所以不用static
    T* allocate(size_t n) override {
        try {
            return oom_malloc(n * sizeof(T));
        } catch (const std::exception& e) {
            fprintf(stderr, "alloc failed: %s\n", e.what());
            return nullptr;
        }
    }

    void deallocate(T* ptr, size_t n) noexcept override {//显式声明不抛异常，可安全调用
        ::free(ptr);
    }

    T* reallocate(T* ptr, size_t old_size, size_t new_size) override {
        try {
            return oom_realloc(ptr, new_size * sizeof(T));
        } catch (const std::exception& e) {
            fprintf(stderr, "reallocate failed: %s\n", e.what());
            return nullptr;
        }   
    }

    //可分配最大成员数：分配内存的最大值是size_t(-1)（如32位系统上是 2^32-1）；
    size_t max_size() const noexcept override {
        return size_t(-1) / sizeof(T);
    }

    //虽然是按T类型分配的空间，但构造时用U指定类型更灵活，允许在容器内构造不同类型的对象（如迭代器）
    //typename... Args支持构造时参数指定任意类型，事实上泛型函数可根据入参自动推导泛型，所以调用时不用显式指定
    //变量传递时会发生值拷贝，但左右值引用都是0拷贝的，const var&声明则用指针读取值，不拷贝；声明var&&传递右值则直接用右值
    template<typename U, typename... Args>
    void construct(U* p, Args&&... args) {//&&接受右值，或右值引用变量，但为了避免再写一个左值引用的版本，&&语法实际允许左值和左值引用输入
        try {
            ::new((void*)p) U(std::forward<Args>(args)...);//Args&&... args声明了函数内部形参args值传递是0拷贝的，但形参args又变成了左值，std::forward<Args>(args)用于恢复值原始类型
        } catch (const std::exception& e) {
            fprintf(stderr, "construct failed: %s\n", e.what());
        }
    }
    
    template<typename U>
    void destroy(U* p) {
        try {
            p->~U();
        } catch (const std::exception& e) {
            fprintf(stderr, "destroy failed: %s\n", e.what());
        }
    }

    // allocator是无状态的，所有allocator都相等
    template<typename T1, typename T2>
    friend bool operator==(const allocator<threads, T1>&, const allocator<threads, T2>&) noexcept {
        return true;
    }

    template<typename T1, typename T2>
    friend bool operator!=(const allocator<threads, T1>&, const allocator<threads, T2>&) noexcept {
        return false;
    }
};
}