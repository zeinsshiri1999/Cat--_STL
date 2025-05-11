#pragma once

#include <cstddef>      // 用于size_t
#include <type_traits>  // 用于类型特征
#include <functional>   // 用于std::function
#include "../alloc/Cat++_alloc_selector.h"  // 自定义分配器
#include "../execption/allocator_exception.h"  // 自定义异常

namespace Cat {

/*
 * vector实现思路
 * 
 * 一、数据模型（实体类）
 * 1. vector_base
 *    - 数据：三个指针（start_, finish_, end_of_storage_）
 *    - 原因：连续内存布局需要知道内存块的起始、结束和容量边界
 *    - 构造：分配内存并初始化指针
 *    - 析构：释放内存
 * 
 * 2. vector
 *    - 数据：继承vector_base
 *    - 原因：复用内存布局数据
 * 
 * 二、功能接口（方法类）
 * 1. 构造与析构
 *    - 默认构造：初始化空容器
 *    - 填充构造：用n个value初始化
 *    - 范围构造：用迭代器范围初始化
 *    - 拷贝构造：复制另一个vector
 *    - 移动构造：移动另一个vector
 *    - 析构：清理资源
 * 
 * 2. 元素访问
 *    - 迭代器：begin/end/rbegin/rend
 *    - 下标：operator[]
 *    - 边界检查：at
 *    - 首尾元素：front/back
 * 
 * 3. 容量操作
 *    - 大小：size/empty
 *    - 容量：capacity/reserve
 *    - 调整：resize
 * 
 * 4. 修改操作
 *    - 尾部：push_back/pop_back
 *    - 插入：insert
 *    - 删除：erase/clear
 *    - 交换：swap
 */

// 数据层：vector_base - 定义内存布局
template<typename T, typename Allocator = alloc<false, T>>
class vector_base {
public:
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using pointer = typename allocator_type::pointer;
    using const_pointer = typename allocator_type::const_pointer;
    using reference = value_type&;
    using const_reference = const value_type&;

    // 迭代器类型定义
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

protected:
    pointer start_ = nullptr;           // 指向第一个元素
    pointer finish_ = nullptr;          // 指向最后一个元素的下一个位置
    pointer end_of_storage_ = nullptr;  // 指向分配内存的末尾
    allocator_type alloc_;              // 分配器实例
    size_type growth_factor_ = 2;       // 扩容因子
    size_type initial_capacity_ = 16;   // 初始容量

    // 计算属性
    std::function<size_type()> size = [this]() { return finish_ - start_; };
    std::function<size_type()> capacity = [this]() { return end_of_storage_ - start_; };

protected:
    // 默认容量0初始化
    // 指定容量0初始化
    // 指定容量和值初始化
    // 范围初始化
    // 拷贝构造
    // 移动构造
    // 析构
    vector_base() {
        try {
            start_ = alloc_.allocate(initial_capacity_);
            finish_ = start_;
            end_of_storage_ = start_ + initial_capacity_;
        } catch (const std::exception& e) {
            throw OutOfMemoryException();
        }
    }
    
    // 分配n个元素的内存
    vector_base(size_type n, const allocator_type& a) : alloc_(a) {
        try {
            start_ = alloc_.allocate(n);
            finish_ = start_;
            end_of_storage_ = start_ + n;
        } catch (const std::exception& e) {
            throw OutOfMemoryException();
        }
    }

    ~vector_base() {
        if (start_) {
            alloc_.deallocate(start_, end_of_storage_ - start_);
            start_ = finish_ = end_of_storage_ = nullptr;
        }
    }

public:
    // 指针相关属性
    pointer get_start() const { return start_; }
    void set_start(pointer ptr) { start_ = ptr; }
    pointer get_finish() const { return finish_; }
    void set_finish(pointer ptr) { finish_ = ptr; }
    pointer get_end_of_storage() const { return end_of_storage_; }
    void set_end_of_storage(pointer ptr) { end_of_storage_ = ptr; }

    // 分配器相关属性
    allocator_type& get_allocator() { return alloc_; }
    const allocator_type& get_allocator() const { return alloc_; }

    // 计算属性（只读）
    size_type get_size() const { return size(); }
    size_type get_capacity() const { return capacity(); }

    // 配置属性
    size_type get_growth_factor() const { return growth_factor_; }
    void set_growth_factor(size_type factor) { growth_factor_ = factor; }
    size_type get_initial_capacity() const { return initial_capacity_; }
    void set_initial_capacity(size_type capacity) { initial_capacity_ = capacity; }

    // 计算新容量
    size_type calculate_new_capacity(size_type min_capacity) const {
        if (min_capacity <= get_capacity()) {
            return get_capacity();
        }
        return std::max(get_capacity() * get_growth_factor(), min_capacity);
    }
};

// 操作层：vector - 定义功能接口
template<typename T, typename Allocator = alloc<false, T>>
class vector : private vector_base<T, Allocator> {
    using base = vector_base<T, Allocator>;
    using typename base::value_type;
    using typename base::allocator_type;
    using typename base::size_type;
    using typename base::difference_type;
    using typename base::pointer;
    using typename base::const_pointer;
    using typename base::reference;
    using typename base::const_reference;
    using typename base::iterator;
    using typename base::const_iterator;
    using typename base::reverse_iterator;
    using typename base::const_reverse_iterator;

public:
    // 构造函数
    vector() : base() {}
    explicit vector(size_type n) : base(n, allocator_type()) { fill_initialize(n, value_type()); }
    vector(size_type n, const value_type& value) : base(n, allocator_type()) { fill_initialize(n, value); }
    template<typename InputIterator>
    vector(InputIterator first, InputIterator last) : base() {
        range_initialize(first, last);
    }
    vector(const vector& x) : base() { range_initialize(x.begin(), x.end()); }
    vector(vector&& x) noexcept : base() { swap(x); }
    ~vector() { destroy(begin(), end()); }

    // 赋值操作
    vector& operator=(const vector& x) {
        if (this != &x) {
            assign(x.begin(), x.end());
        }
        return *this;
    }
    vector& operator=(vector&& x) noexcept {
        if (this != &x) {
            clear();
            swap(x);
        }
        return *this;
    }

    // 迭代器操作
    iterator begin() { return this->get_start(); }
    const_iterator begin() const { return this->get_start(); }
    iterator end() { return this->get_finish(); }
    const_iterator end() const { return this->get_finish(); }
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    // 容量操作
    size_type size() const { return size_type(end() - begin()); }
    size_type capacity() const { return size_type(this->get_end_of_storage() - begin()); }
    bool empty() const { return begin() == end(); }
    void reserve(size_type n);
    void resize(size_type new_size, const value_type& x = value_type());

    // 元素访问
    reference operator[](size_type n) { return *(begin() + n); }
    const_reference operator[](size_type n) const { return *(begin() + n); }
    reference at(size_type n);
    const_reference at(size_type n) const;
    reference front() { return *begin(); }
    const_reference front() const { return *begin(); }
    reference back() { return *(end() - 1); }
    const_reference back() const { return *(end() - 1); }

    // 修改操作
    void push_back(const value_type& x);
    void pop_back();
    iterator insert(iterator position, const value_type& x);
    void insert(iterator position, size_type n, const value_type& x);
    template<typename InputIterator>
    void insert(iterator position, InputIterator first, InputIterator last);
    iterator erase(iterator position);
    iterator erase(iterator first, iterator last);
    void clear() { erase(begin(), end()); }
    void swap(vector& x) noexcept {
        std::swap(this->start_, x.start_);
        std::swap(this->finish_, x.finish_);
        std::swap(this->end_of_storage_, x.end_of_storage_);
    }

private:
    // 辅助函数
    void fill_initialize(size_type n, const value_type& value);
    template<typename InputIterator>
    void range_initialize(InputIterator first, InputIterator last);
    void destroy(iterator first, iterator last);
    void insert_aux(iterator position, const value_type& x);

    // 内存管理
    void reallocate(size_type new_capacity) {
        if (new_capacity <= capacity()) {
            return;
        }

        pointer new_start = this->get_allocator().allocate(new_capacity);
        if (!new_start) {
            throw OutOfMemoryException();
        }

        // 保存旧数据
        pointer old_start = this->get_start();
        pointer old_finish = this->get_finish();
        size_type old_size = size();

        // 更新指针
        this->set_start(new_start);
        this->set_finish(new_start + old_size);
        this->set_end_of_storage(new_start + new_capacity);

        // 释放旧内存
        if (old_start) {
            this->get_allocator().deallocate(old_start, this->get_end_of_storage() - old_start);
        }
    }
};

} // namespace Cat