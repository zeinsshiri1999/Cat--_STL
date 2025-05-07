#pragma once
#include <exception>

namespace Cat{
    // 自定义异常类
    class allocator_exception : public std::exception {
    protected:
        const char* message;
    public:
        allocator_exception(const char* msg) : message(msg) {}
        const char* what() const noexcept override {
            return message;
        }
    };

    class OutOfMemoryException : public allocator_exception{
    public:
        OutOfMemoryException() : allocator_exception("Out of memory") {}
    };
    
}