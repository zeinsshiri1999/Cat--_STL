#include "../src/alloc/Cat++_alloc_selector.h"
#include "../util/dev_dependency/Cat++_test/src/Cat++_PerformanceTest.h"
#include <string>
#include <cstdio>
#include <vector>
#include <stdexcept>

namespace Cat {

// 基本分配/释放测试
// 需求：测试分配器在分配和释放中等规模内存时的性能表现
// 实现思路：分配一块1000个int的内存，写入数据，释放
// 以此衡量分配器在常规场景下的效率
// --------------------------------------------------
template<typename Alloc>
void basicAllocTest() {
    Alloc alloc;
    int* ptr = alloc.allocate(1000);
    if (ptr == nullptr) {
        throw std::runtime_error("Basic allocation failed");
    }
    for (int i = 0; i < 1000; ++i) {
        ptr[i] = i;
    }
    alloc.deallocate(ptr, 1000);
}

// 小内存分配测试
// 需求：测试分配器在频繁分配/释放小块内存时的性能
// 实现思路：循环分配1个int，写入数据，释放
// 反映分配器在高碎片化场景下的表现
// --------------------------------------------------
template<typename Alloc>
void smallAllocTest() {
    Alloc alloc;
    std::vector<int*> ptrs;
    ptrs.reserve(1000);
    
    try {
        // 分配阶段
        for (int i = 0; i < 1000; ++i) {
            int* ptr = alloc.allocate(1);
            if (ptr == nullptr) {
                printf("Allocation failed at iteration %d\n", i);
                // 清理已分配的内存
                for (int* p : ptrs) {
                    if (p != nullptr) {
                        alloc.deallocate(p, 1);
                    }
                }
                throw std::runtime_error("Small allocation failed");
            }
            *ptr = i;
            ptrs.push_back(ptr);
            
            // 验证写入的数据
            if (*ptr != i) {
                printf("Data verification failed at iteration %d\n", i);
                throw std::runtime_error("Data verification failed");
            }
        }
        
        // 释放阶段
        for (int* ptr : ptrs) {
            if (ptr != nullptr) {
                alloc.deallocate(ptr, 1);
            }
        }
    } catch (const std::exception& e) {
        printf("Error in smallAllocTest: %s\n", e.what());
        // 确保清理所有已分配的内存
        for (int* ptr : ptrs) {
            if (ptr != nullptr) {
                try {
                    alloc.deallocate(ptr, 1);
                } catch (...) {
                    // 忽略清理时的错误
                }
            }
        }
        throw;
    }
}

// 大内存分配测试
// 需求：测试分配器在分配/释放大块内存时的性能
// 实现思路：分配1000000个int，写入数据，释放
// 评估分配器在极端大内存场景下的效率
// --------------------------------------------------
template<typename Alloc>
void largeAllocTest() {
    Alloc alloc;
    int* ptr = alloc.allocate(1000000);
    if (ptr == nullptr) {
        throw std::runtime_error("Large allocation failed");
    }
    for (int i = 0; i < 1000000; ++i) {
        ptr[i] = i;
    }
    alloc.deallocate(ptr, 1000000);
}

// 内存重分配测试
// 需求：测试分配器在重分配（realloc）时的性能
// 实现思路：分配1000个int，写入数据，分配2000个int，拷贝数据，释放
// 反映分配器在动态扩容场景下的效率
// --------------------------------------------------
template<typename Alloc>
void reallocTest() {
    Alloc alloc;
    int* ptr = alloc.allocate(1000);
    if (ptr == nullptr) {
        throw std::runtime_error("Initial allocation failed");
    }
    
    // 写入初始数据
    for (int i = 0; i < 1000; ++i) {
        ptr[i] = i;
    }
    
    // 重新分配
    int* new_ptr = alloc.allocate(2000);
    if (new_ptr == nullptr) {
        alloc.deallocate(ptr, 1000);
        throw std::runtime_error("Reallocation failed");
    }
    
    // 拷贝数据
    for (int i = 0; i < 1000; ++i) {
        new_ptr[i] = ptr[i];
    }
    
    // 释放旧内存
    alloc.deallocate(ptr, 1000);
    
    // 写入新数据
    for (int i = 1000; i < 2000; ++i) {
        new_ptr[i] = i;
    }
    
    // 释放新内存
    alloc.deallocate(new_ptr, 2000);
}

// 运行指定的测试
template<typename Alloc>
void runTest(const std::string& allocatorName, int testChoice, int iterations) {
    printf("\n=== %s ===\n", allocatorName.c_str());
    
    try {
        switch (testChoice) {
            case 1:
                FunctionTest([&]() { basicAllocTest<Alloc>(); })
                    .runBenchmark("Basic Allocation/Deallocation", iterations)
                    .print();
                break;
            case 2:
                FunctionTest([&]() { smallAllocTest<Alloc>(); })
                    .runBenchmark("Small Memory Allocations", iterations)
                    .print();
                break;
            case 3:
                FunctionTest([&]() { largeAllocTest<Alloc>(); })
                    .runBenchmark("Large Memory Allocation", iterations)
                    .print();
                break;
            case 4:
                FunctionTest([&]() { reallocTest<Alloc>(); })
                    .runBenchmark("Memory Reallocation", iterations)
                    .print();
                break;
            default:
                printf("Invalid test choice\n");
        }
    } catch (const std::exception& e) {
        printf("Test failed: %s\n", e.what());
    } catch (...) {
        printf("Test failed with unknown error\n");
    }
}

} // namespace Cat

int main() {
    constexpr int DEFAULT_ITERATIONS = 100;
    
    printf("\n=== Memory Allocator Performance Test ===\n");
    
    // 选择分配器
    printf("\nSelect allocator type:\n");
    printf("1. STL Default Allocator\n");
    printf("2. Cat++ Simple Allocator\n");
    printf("3. Cat++ Pool Allocator\n");
    printf("Enter your choice (1-3): ");
    
    int allocChoice;
    scanf("%d", &allocChoice);
    
    // 选择测试类型
    printf("\nSelect test type:\n");
    printf("1. Basic Allocation/Deallocation\n");
    printf("2. Small Memory Allocations\n");
    printf("3. Large Memory Allocation\n");
    printf("4. Memory Reallocation\n");
    printf("Enter your choice (1-4): ");
    
    int testChoice;
    scanf("%d", &testChoice);
    
    // 运行测试
    switch (allocChoice) {
        case 1:
            Cat::runTest<Cat::alloc<true, int, Cat::stl_tag>>(
                "STL Default Allocator", testChoice, DEFAULT_ITERATIONS);
            break;
        case 2:
            Cat::runTest<Cat::alloc<true, int, Cat::simple_tag>>(
                "Cat++ Simple Allocator", testChoice, DEFAULT_ITERATIONS);
            break;
        case 3:
            Cat::runTest<Cat::alloc<true, int, Cat::pool_tag>>(
                "Cat++ Pool Allocator", testChoice, DEFAULT_ITERATIONS);
            break;
        default:
            printf("Invalid allocator choice\n");
    }
    
    return 0;
}