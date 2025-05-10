#pragma once
#include "Cat++_allocator.h"
#include <cstdlib>
#include <cstring>
#include <type_traits>
//线程安全
//尽量兼容STL接口规范
//异常处理
/*
内存池化:
1）减少内存碎片
2）避免频繁分配时的系统调用开销
3）提高内存利用率：malloc头部有一块小内存，用于管理分配的内存，在大量分配小内存时，会造成内存浪费
*/

//直觉思路是模8+1：(bytes % __ALIGN == 0) ? bytes : (bytes / __ALIGN + 1) * __ALIGN
//(bytes + __ALIGN - 1) & ~(__ALIGN - 1)
//__ALIGN - 1低位都是1，~(__ALIGN - 1)为低位清零掩码
//需求：bytes 未对齐，输入bytes = k * __ALIGN + remainder；输出(k+1) * __ALIGN
//需求：bytes 对齐，输入bytes = k * __ALIGN；输出k * __ALIGN

namespace Cat {

// 内存池节点
union free_list_node {
    free_list_node* block;      // 当块空闲时，作为指针类型指向下一个空闲块
    char client_data[1];        // 当块分配出去时，作为一个灵活数组类型供用户使用
};

template<bool threads>
class alloc_pool final {
private:
    // 禁止实例化、拷贝和移动
    alloc_pool() = delete;
    ~alloc_pool() = delete;
    alloc_pool(const alloc_pool&) = delete;
    alloc_pool& operator=(const alloc_pool&) = delete;
    alloc_pool(alloc_pool&&) = delete;
    alloc_pool& operator=(alloc_pool&&) = delete;

    // 内存池配置
    static constexpr size_t ALIGN = 8;                       // 最小分配单元
    static constexpr size_t MAX_BYTES = 128;                 // 小块大小上限
    static constexpr size_t NUM_OF_NODES = MAX_BYTES / ALIGN;// 空闲数组节点数量(128/8)

    // 内存池状态
    static inline char* start = nullptr;                     // 内存池起始位置
    static inline char* end = nullptr;                       // 内存池结束位置
    static inline size_t size = 0;                           // 内存池大小
    static inline free_list_node* free_serial[NUM_OF_NODES] = {nullptr}; // 空闲链表数组

public:
    // 配置访问接口
    static constexpr size_t get_align() { return ALIGN; }
    static constexpr size_t get_max_bytes() { return MAX_BYTES; }
    static constexpr size_t get_num_of_nodes() { return NUM_OF_NODES; }

    // 内存池状态访问接口
    static char* get_start() { return start; }
    static void set_start(char* ptr) { start = ptr; }
    static char* get_end() { return end; }
    static void set_end(char* ptr) { end = ptr; }
    static size_t get_size() { return size; }
    static void set_size(size_t size) { size = size; }
    
    // 空闲链表操作接口
    static free_list_node* get_free_list(size_t index) { return free_serial[index]; }
    static void set_free_list(size_t index, free_list_node* node) { free_serial[index] = node; }
};

// 定义具体的别名
using pool_t = alloc_pool<true>;   // 线程安全版本
using pool_f = alloc_pool<false>;  // 非线程安全版本

// 内存池分配器类
template<bool threads, typename T>
class pool_allocator : public allocator<threads, T> {
private:
    // 内存池管理方法
    using pool = typename std::conditional<threads, pool_t, pool_f>::type;
    
    /*
     * 内存对齐计算：
     * 1. 未对齐情况：bytes = k * ALIGN + remainder
     *    计算：(bytes + ALIGN - 1) / ALIGN - 1
     *    结果：k
     * 2. 已对齐情况：bytes = k * ALIGN
     *    计算：(bytes + ALIGN - 1) / ALIGN - 1
     *    结果：k - 1
     */
    size_t get_free_serial_index(size_t bytes) const {
        return (bytes + pool::get_align() - 1) / pool::get_align() - 1;
    }

    /*
     * 内存对齐计算：
     * 1. 未对齐情况：bytes = k * ALIGN + remainder
     *    计算：(bytes + ALIGN - 1) & ~(ALIGN - 1)
     *    结果：(k + 1) * ALIGN
     * 2. 已对齐情况：bytes = k * ALIGN
     *    计算：(bytes + ALIGN - 1) & ~(ALIGN - 1)
     *    结果：k * ALIGN
     * 
     * 原理：
     * - ALIGN - 1 的低位都是1
     * - ~(ALIGN - 1) 是低位清零掩码
     * - 通过位运算实现向上取整到ALIGN的倍数
     */
    size_t round_up(size_t bytes) const {
        return (bytes + pool::get_align() - 1) & ~(pool::get_align() - 1);
    }

    // 内存池管理
    void* refill(size_t node_size, const size_t& nodes = 20);
    char* chunk_alloc(size_t node_size, unsigned int& nodes);

public:
    // 模板构造函数，允许从其他类型的allocator构造
    template<typename U>
    struct rebind {
        using other = pool_allocator<threads, U>;
    };

    // 构造函数和析构函数
    pool_allocator() noexcept = default;
    template<typename U>
    pool_allocator(const pool_allocator<threads, U>&) noexcept {}
    ~pool_allocator() noexcept override = default;

public:
    // 内存分配：优先使用内存池，大块内存直接使用malloc
    T* allocate(size_t n) override {
        if(n * sizeof(T) > pool::get_max_bytes()) {
            return static_cast<T*>(malloc(n * sizeof(T)));
        }

        free_list_node* block = pool::get_free_list(get_free_serial_index(n * sizeof(T)));
        if(block) {
            pool::set_free_list(get_free_serial_index(n * sizeof(T)), block->block);
            return static_cast<T*>(block);
        }

        return static_cast<T*>(refill(round_up(n * sizeof(T))));
    }

    // 内存释放：优先使用内存池，大块内存直接使用free
    void deallocate(T* ptr, size_t n) noexcept override {
        if(n * sizeof(T) > pool::get_max_bytes()) {
            free(ptr);
            return;
        }
        
        if(ptr) {
            free_list_node* my_free_list = pool::get_free_list(get_free_serial_index(n * sizeof(T)));
            ((free_list_node*)ptr)->block = my_free_list;
            pool::set_free_list(get_free_serial_index(n * sizeof(T)), (free_list_node*)ptr);
        }
    }

    // 内存重分配：优先使用内存池，大块内存直接使用realloc
    T* reallocate(T* ptr, size_t old_size, size_t new_size) override {
        if(old_size * sizeof(T) > pool::get_max_bytes() && new_size * sizeof(T) > pool::get_max_bytes()) {
            return static_cast<T*>(realloc(ptr, new_size * sizeof(T)));
        }
        if(round_up(old_size * sizeof(T)) == round_up(new_size * sizeof(T))) {
            return ptr;
        }

        T* result = allocate(new_size);
        size_t copy_sz = new_size > old_size ? old_size : new_size;
        memcpy(result, ptr, copy_sz * sizeof(T));
        deallocate(ptr, old_size);
        return result;
    }

    size_t max_size() const noexcept override {
        return size_t(-1) / sizeof(T);
    }
};

// 实现refill和chunk_alloc方法
template<bool threads, typename T>
void* pool_allocator<threads, T>::refill(size_t node_size, const size_t& nodes) {
    unsigned int nobjs = nodes;
    char* chunk = chunk_alloc(node_size, nobjs);
    if(nobjs == 1)
        return chunk;

    free_list_node* my_free_list = pool::get_free_list(get_free_serial_index(node_size));
    my_free_list->block = (free_list_node*)chunk;

    free_list_node* current_node = (free_list_node*)((char*)chunk + node_size);
    free_list_node* next_node = (free_list_node*)((char*)current_node + node_size);
    if(next_node > (free_list_node*)((char*)pool::get_end()-node_size)) {
        return chunk;
    }

    for(int i = 1; i < nobjs - 1; i++) {
        current_node->block = next_node;
        if(i == nobjs - 2) {
            next_node->block = nullptr;
            break;
        }

        if((free_list_node*)((char*)next_node + node_size) > (free_list_node*)((char*)pool::get_end()-node_size)) {
            next_node->block = nullptr;
            break;
        }

        current_node = next_node;
        next_node = (free_list_node*)((char*)next_node + node_size);
    }
    return chunk;
}

template<bool threads, typename T>
char* pool_allocator<threads, T>::chunk_alloc(size_t node_size, unsigned int& nodes) {
    char* result;
    size_t need_bytes = node_size * nodes;
    size_t bytes_left = pool::get_end() - pool::get_start();

    if(bytes_left >= need_bytes) {
        result = pool::get_start();
        pool::set_start(pool::get_start() + need_bytes);
        return result;
    }
    else if(bytes_left >= node_size) {
        nodes = bytes_left / node_size;
        result = pool::get_start();
        pool::set_start(pool::get_start() + nodes * node_size);
        return result;
    }
    else {
        size_t bytes_to_get = 2 * need_bytes + round_up(pool::get_size()>>4);

        if(bytes_left > 0) {
            free_list_node* my_free_list = pool::get_free_list(get_free_serial_index(bytes_left));
            ((free_list_node*)pool::get_start())->block = my_free_list;
            pool::set_free_list(get_free_serial_index(bytes_left), (free_list_node*)pool::get_start());
        }

        pool::set_start((char*)malloc(bytes_to_get));
        if(!pool::get_start()) {
            for(int i = node_size; i < pool::get_num_of_nodes(); i++) {
                if(pool::get_free_list(i)) {
                    pool::set_free_list(i, pool::get_free_list(i)->block);
                    pool::set_start((char*)pool::get_free_list(i));
                    pool::set_end(pool::get_start() + node_size);
                    return chunk_alloc(node_size, nodes);
                }
            }
            return nullptr;
        }

        pool::set_size(pool::get_size() + bytes_to_get);
        pool::set_end(pool::get_start() + bytes_to_get);
        return chunk_alloc(node_size, nodes);
    }
}

} // namespace Cat

