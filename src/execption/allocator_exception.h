#pragma once
#include <exception>
#include <string>

namespace Cat {
    // 基础异常类
    class allocator_exception : public std::exception {
    protected:
        std::string message_;
        std::string details_;

    public:
        allocator_exception(const char* msg, const char* details = "") 
            : message_(msg), details_(details) {}
        
        const char* what() const noexcept override {
            return message_.c_str();
        }

        const char* details() const noexcept {
            return details_.c_str();
        }

        virtual ~allocator_exception() = default;
    };

    // 内存不足异常
    class OutOfMemoryException : public allocator_exception {
    public:
        OutOfMemoryException(const char* details = "") 
            : allocator_exception("Out of memory", details) {}
    };

    // 分配器配置错误异常
    class AllocatorConfigException : public allocator_exception {
    public:
        AllocatorConfigException(const char* details = "") 
            : allocator_exception("Allocator configuration error", details) {}
    };

    // 内存对齐错误异常
    class AlignmentException : public allocator_exception {
    public:
        AlignmentException(const char* details = "") 
            : allocator_exception("Memory alignment error", details) {}
    };

    // 内存池耗尽异常
    class PoolExhaustedException : public allocator_exception {
    public:
        PoolExhaustedException(const char* details = "") 
            : allocator_exception("Memory pool exhausted", details) {}
    };

    // 无效操作异常
    class InvalidOperationException : public allocator_exception {
    public:
        InvalidOperationException(const char* details = "") 
            : allocator_exception("Invalid operation", details) {}
    };
}