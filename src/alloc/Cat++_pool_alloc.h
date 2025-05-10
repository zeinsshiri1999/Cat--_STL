#pragma once
#include"Cat++_alloc.h"

union free_list_node {
    free_list_node* block;      // 当块空闲时，作为指针类型指向下一个空闲块
    char client_data[1];        // 当块分配出去时，作为一个灵活数组类型供用户使用
};

//内存池配置
enum {ALIGN = 8};                       // 最小分配单元
enum {MAX_BYTES = 128};                 // 小块大小上限
enum {NUM_OF_NODES = MAX_BYTES / ALIGN};// 空闲数组节点数量(128/8)

namespace Cat {

template<bool threads, typename T>
class pool_allocator : public allocator<threads, T>{
public:
    //类型定义
    typedef T        value_type;
    typedef T*       pointer;
    typedef const T* const_pointer;
    typedef T&       reference;
    typedef const T& const_reference;
    //这样才能使用基类定义的类型
    using execption_handler = typename allocator<threads, T>::execption_handler;
    //构造函数
    pool_allocator() = delete;
    //禁止拷贝和赋值
    pool_allocator(const pool_allocator& other) = delete;
    pool_allocator(pool_allocator&& other) = delete;
    pool_allocator& operator=(const pool_allocator& other) = delete;
    pool_allocator& operator=(pool_allocator&& other) = delete;

private:
    //内存池状态
    static inline char* start = nullptr;
    static inline char* end = nullptr;
    static inline size_t pool_size = 0;

    static inline free_list_node* free_serial[NUM_OF_NODES] = {nullptr};

private:
    /*需求：获取对应free_serial_index
    bytes 未对齐，输入bytes = k * __ALIGN + remainder；输出(k+1) -1
    bytes 对齐，输入bytes = k * __ALIGN；输出k -1
    */
    static size_t get_free_serial_index(size_t bytes){
        return (bytes + ALIGN - 1) / ALIGN - 1;
    }

    /*需求：向上取整
    bytes 未对齐，输入bytes = k * __ALIGN + remainder；输出(k+1) * __ALIGN
    bytes 对齐，输入bytes = k * __ALIGN；输出k * __ALIGN
    */
    static size_t round_up(size_t bytes){
        return (bytes + ALIGN - 1) & ~(ALIGN - 1);
    }
    //内存池管理方法
    static void* refill(size_t node_size, const size_t& nodes = 20, const execption_handler handler = nullptr);
    static char* chunk_alloc(size_t node_size, unsigned int& nodes, const execption_handler handler = nullptr);

public:
    static void* allocate(size_t n, const execption_handler handler = nullptr);
    static void* reallocate(T* ptr, size_t old_size, size_t new_size);
    static void deallocate(T* ptr);
    static void construct(T* ptr, const T& value);
    static void destroy(T* ptr);
};

/*  从内存池分出一批块的连续内存
1)先消费现有内存池
2)若不足，把零碎余量挂到对应链表，然后向系统 malloc 大块来扩容
3)若系统 malloc 失败，尝试从稍大等级的 free_list "借"一块；若依然失败，则退回一级配置器
*/
template<bool threads, typename T>
char* pool_allocator<threads, T>::chunk_alloc(size_t node_size, unsigned int& nodes, const execption_handler handler){
    char* result;
    size_t need_bytes = node_size * nodes;
    size_t bytes_left = end - start;//计算相差几个char

    if(bytes_left >= need_bytes){
        result = start;
        start += need_bytes;//偏移了几个元素
        return result;
    }
    else if(bytes_left >= node_size){//内存池不足以分配nodes个node_size的节点，但剩余空间至少足够分配一个节点，那就将剩余零碎余量返回给refill挂到对应链表
        size_t left_nodes = bytes_left / node_size;
        result = start;//直接返回连续内存就行了，refill会判断有多少个node
        start += left_nodes * node_size;
        return result;
    }
    else{//一个node都无法分配，扩容后再分nodes个node_size的连续内存
        size_t bytes_to_get = 2 * need_bytes + round_up(pool_size>>4);//经验值:2倍申请大小+1/16池子大小

        if(bytes_left > 0){//剩余空间挂到对应空闲链表
            free_list_node* my_free_list = free_serial[get_free_serial_index(bytes_left)];//定位对应的空闲链表
            free_list_node* new_node = (free_list_node*)start;
            new_node->block = my_free_list;  // 新节点指向当前链表头
            free_serial[get_free_serial_index(bytes_left)] = new_node;  // 更新链表头
        }

        //扩容
        start = (char*)malloc(bytes_to_get);
        if(!start){//从更大的空闲链表中回收空间到内存池
            for(int i = node_size; i < NUM_OF_NODES; i++){
                if(free_serial[i]){
                    free_serial[i] = free_serial[i]->block;
                    start = (char*)free_serial[i];
                    end = start + node_size;
                    return chunk_alloc(node_size, nodes, handler);//补充了内存池后递归调用，至少能分配一个node的连续内存
                }
            }
        }

        //扩容失败，调用自己定义的带异常处理机制的malloc扩容
        start = (char*)allocator<threads, T>::allocate(bytes_to_get/sizeof(T), handler);
        if(!start){//还失败，那没辙
            throw OutOfMemoryException();
        }
        //或者成功扩容，则更新内存池状态，再尝试分nodes个node_size的连续内存
        pool_size += bytes_to_get;
        end = start + bytes_to_get;
        return chunk_alloc(node_size, nodes, handler);
    }
}

/* 从内存池分出一批块(20次小分配触发一次syscall)，插入链表(O(1))，然后返回其中一块 
1）分层设计：调用 chunk_alloc 从内存池分出一批块的连续内存
2）分层设计：refill负责将chunk_alloc返回的内存组织成链表，挂回对应空闲链表，只返回第一块
*/
template<bool threads, typename T>
void* pool_allocator<threads, T>::refill(size_t node_size, const size_t& nodes, const execption_handler handler){
    char* chunk = chunk_alloc(node_size, nodes, handler);
    if(nodes == 1)
        return chunk;

    //将chunk组织成链表，挂回对应空闲链表
    free_list_node* my_free_list = free_serial[get_free_serial_index(node_size)];
    my_free_list->block = (free_list_node*)chunk;

    free_list_node* current_node = (free_list_node*)((char*)chunk + node_size);
    free_list_node* next_node = (free_list_node*)((char*)current_node + node_size);
    if(next_node > (free_list_node*)((char*)end-node_size)){//内存池不足时返回不足nodes个，需要判断next_node是否越界
        return chunk; //内存池只分配了一个node，直接返回
    }

    //将chunk组织成链表，挂回对应空闲链表
    for(int i = 1; i < nodes - 1; i++){
        current_node->block = next_node;
        if(i == nodes - 2){
            next_node->block = nullptr;
            break;
        }

        if((free_list_node*)((char*)next_node + node_size) > (free_list_node*)((char*)end-node_size)){//内存池不足时返回不足nodes个，需要判断next_node+1是否越界
            next_node->block = nullptr;
            break;
        }

        current_node = next_node;
        next_node = (free_list_node*)((char*)next_node + node_size);
    }
    //返回第一块
    return chunk;
}

/* 
1)大于 128 字节：直接调用第一级配置器。
2)小于等于 128：
2.1)从空闲链表分配,如果链表非空，摘下头节点，O(1) 返回；
2.2)否则调用 refill，从内存池分出一批块(20次小分配触发一次syscall)，插入链表(O(1))，然后返回其中一块
 */
template<bool threads, typename T>
void* pool_allocator<threads, T>::allocate(size_t n, const execption_handler handler){
    if(n > MAX_BYTES){//大于池子的最大值，走系统调用
        return allocator<threads, T>::allocate(n, handler);
    }

    //先从空闲链表分配
    free_list_node* block = free_serial[get_free_serial_index(n*sizeof(T))];
    if(block){//头快非空，取出头块返回；O(1)
        free_serial[get_free_serial_index(n*sizeof(T))] = block->block;
        return block;
    }

    //如果空闲链表为空，则从内存池批量分配块
    return refill(round_up(n*sizeof(T)), size_t(20), handler);
}

/*  */
template<bool threads, typename T>
void* pool_allocator<threads, T>::reallocate(T* ptr, size_t old_size, size_t new_size){
    void* result;
    size_t copy_size;
    
}


/*  */
template<bool threads, typename T>
void pool_allocator<threads, T>::deallocate(T* ptr){
    if(ptr){
        free_serial[get_free_serial_index(ptr)]->block = ptr;
    }
}

}

