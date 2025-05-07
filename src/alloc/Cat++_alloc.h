#pragma once
#include<cstddef>
#include<cstdio>
#include<cstdlib>
#include"../execption/allocator_exception.h"
//allocator interface

//线程安全
//尽量兼容STL接口规范
//异常处理
//内存碎片优化


namespace Cat
{
    template<class T>
    class allocator{
    private:
        allocator() = default;
        //禁用拷贝构造
        allocator(const allocator&) = delete;
        //禁用赋值操作符，接口声明不需要写形参
        allocator& operator=(const allocator&) = delete;

    private:
        typedef void(*execption_handler)();//函数指针用于动态指定代码段的函数
        static inline execption_handler alloc_oom_handler = nullptr;//初始化为空，不指向任何函数，C++17后可在.h文件中初始化（inline）
        
        static execption_handler set_exception_handler(execption_handler f){
            execption_handler old = alloc_oom_handler;
            alloc_oom_handler = f;
            return old;
        }

        static T* oom_malloc(size_t n){ 
            T* result = malloc(n);
            if(result == nullptr){
                for(;;){
                    if(alloc_oom_handler == nullptr){
                        throw OutOfMemoryException();
                    }
                    alloc_oom_handler();
                    result = malloc(n);
                    if(result){
                        return result;
                    }
                }
            }
            return result;
        }
        static T* oom_realloc(T* ptr, size_t new_size){
            T* result = realloc(ptr, new_size);
            if(result == nullptr){
                for(;;){
                    if(alloc_oom_handler == nullptr){
                        throw OutOfMemoryException();
                    }
                    alloc_oom_handler();
                    result = realloc(ptr, new_size);
                    if(result){
                        return result;
                    }
                }
            }
        }
        
    public://向外传递类型
        typedef T         value_type;
        typedef T*        pointer;
        typedef const T*  const_pointer;
        typedef T&        reference;      //声明变量别名，其实是指针的语法糖
        typedef const T&  const_reference;//不能修改指向的值
        typedef size_t    size_type;

    public:
        static T* allocate(size_t n, const execption_handler handler = nullptr){
            set_exception_handler(handler);
            try {
                return static_cast<T*>(oom_malloc(n * sizeof(T)));
            } catch (const std::exception& e) {
                fprintf(stderr, "alloc failed: %s\n", e.what());
                return nullptr;
            }
        }

        static void deallocate(T* ptr){
            try {
                ::free(ptr);
            } catch (const std::exception& e) {
                fprintf(stderr, "deallocate failed: %s\n", e.what());
            }
        }

        static void construct(T* ptr, const T& value){
            try {
                new(ptr) T(value);
            } catch (const std::exception& e) {
                fprintf(stderr, "construct failed: %s\n", e.what());
            }
        }
        
        static void destroy(T* ptr){
            try {
                ptr->~T();
            } catch (const std::exception& e) {
                fprintf(stderr, "destroy failed: %s\n", e.what());
            }
        }

        static T* reallocate(T* ptr, size_t old_size, size_t new_size){
            try {
                return static_cast<T*>(oom_realloc(ptr, new_size*sizeof(T)));
            } catch (const std::exception& e) {
                fprintf(stderr, "reallocate failed: %s\n", e.what());
                return nullptr;
            }   
        }
    };
}