#pragma once

#include <cstddef>      // 用于size_t
#include <type_traits>  // 用于类型特征
#include <functional>   // 用于std::function
#include "../alloc/Cat++_alloc_selector.h"  // 自定义分配器
#include "../execption/allocator_exception.h"  // 自定义异常
#include "../traits/Cat++_type_traits.h"  // 容器类型特征

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
template<typename T, typename Allocator>
class vector_entity : public sequence_traits<T, Allocator> {
    using traits = sequence_traits<T, Allocator>;
    IMPORT_SEQUENCE_TYPES(traits);
protected:
    pointer start_ = nullptr;           // 指向第一个元素
    pointer finish_ = nullptr;          // 指向最后一个元素的下一个位置
    pointer end_of_storage_ = nullptr;  // 指向分配内存的末尾
    allocator_type alloc_;              // 分配器实例
    static inline size_type growth_factor_ = 2;       // 扩容因子
    static inline size_type initial_capacity_ = 16;   // 初始容量

    // 计算属性
    size_type size() const { return finish_ - start_; }
    size_type capacity() const { return end_of_storage_ - start_; }

protected:
    // 默认容量0初始化
    vector_entity() : alloc_(typename traits::allocator_type()) {// explicit 修饰的但参数构造函数不能用于隐式转换，多参数构造函数本身就不会发生隐式转换，char*常作为连续内存的指针类型，也不希望显式转换
        try {
            start_ = alloc_.allocate(initial_capacity_);
            finish_ = start_;
            end_of_storage_ = start_ + initial_capacity_;
        } catch (const std::exception& e) {
            throw allocator_exception(e.what());
        }
    }
    // 指定容量0初始化
    explicit vector_entity(size_type capacity) : alloc_(typename traits::allocator_type()) {
        capacity = (capacity <= initial_capacity_) ? initial_capacity_ : capacity;
        
        try {
            start_ = alloc_.allocate(capacity);
            finish_ = start_;
            end_of_storage_ = start_ + capacity;
        } catch (const std::exception& e) {
            throw allocator_exception(e.what());
        }
    }
    // 指定容量和值初始化
    template<typename... Args>
    vector_entity(size_type capacity, Args&&... args) : alloc_(typename traits::allocator_type()) {
        capacity = (capacity <= initial_capacity_) ? initial_capacity_ : capacity;
        int argsnum = sizeof...(args);
        try {
            if(capacity >= argsnum){//正常分配内存
                start_ = static_cast<typename traits::pointer>(alloc_.allocate(capacity));
                // 用args构造对象
                for (size_type i = 0; i < argsnum; ++i) {
                    alloc_.construct(start_ + i, std::forward<Args>(args)...);
                }
                finish_ = start_ + argsnum*(sizeof(typename traits::value_type));
                end_of_storage_ = start_ + capacity*(sizeof(typename traits::value_type));
            }
            else{//按照args数量分配内存，或自动计算一个合适的容量分配
                start_ = static_cast<typename traits::pointer>(alloc_.allocate(argsnum));
                // 用args构造对象
                for (size_type i = 0; i < argsnum; ++i) {
                    alloc_.construct(start_ + i, std::forward<Args>(args)...);
                }
                finish_ = start_ + argsnum*(sizeof(typename traits::value_type));
                end_of_storage_ = start_ + capacity*(sizeof(typename traits::value_type));
            }
        } catch (const std::exception& e) {
            // 如果构造过程中发生异常，需要清理已构造的对象
            if (start_) {
                for (typename traits::pointer p = start_; p != finish_; ++p) {
                    alloc_.destroy(p);
                }
                alloc_.deallocate(start_, (capacity>argsnum)?capacity:argsnum);
            }
            throw allocator_exception(e.what());
        }
    }
    // 范围初始化?
    template<typename InputIterator>
    vector_entity(InputIterator first, InputIterator last) : alloc_(typename traits::allocator_type()) {
        try {
            start_ = static_cast<typename traits::pointer>(alloc_.allocate(last - first));
            finish_ = start_;
            end_of_storage_ = start_ + (last - first);
            for (InputIterator it = first; it != last; ++it) {
                alloc_.construct(finish_, *it);
                ++finish_;
            }
        } catch (const std::exception& e) {
            throw allocator_exception(e.what());
        }
    }
    // 拷贝构造
    vector_entity(const vector_entity& x) : alloc_(x.alloc_) {
        try {
            start_ = static_cast<typename traits::pointer>(alloc_.allocate(x.capacity()));
            finish_ = start_;
            
            for (typename traits::const_iterator it = x.start_; it != x.finish_; ++it) {
                alloc_.construct(finish_++, *it);  // 调用拷贝构造函数?
            }
            end_of_storage_ = start_ + (x.end_of_storage_ - x.start_);
        } catch (const std::exception& e) {
            throw allocator_exception(e.what());
        }
    }
    // 移动构造
    vector_entity(vector_entity&& x) noexcept 
        : start_(x.start_)
        , finish_(x.finish_)
        , end_of_storage_(x.end_of_storage_)
        , alloc_(std::move(x.alloc_)) {
        x.start_ = x.finish_ = x.end_of_storage_ = nullptr;
    }
    // 析构
    ~vector_entity() {
        if (start_) {
            alloc_.deallocate(start_, end_of_storage_ - start_);
            start_ = finish_ = end_of_storage_ = alloc_ = nullptr;
        }
    }

public:
    // 指针相关属性
    typename traits::pointer get_start() const { return start_; }
    void set_start(typename traits::pointer ptr) { start_ = ptr; }
    typename traits::pointer get_finish() const { return finish_; }
    void set_finish(typename traits::pointer ptr) { finish_ = ptr; }
    typename traits::pointer get_end_of_storage() const { return end_of_storage_; }
    void set_end_of_storage(typename traits::pointer ptr) { end_of_storage_ = ptr; }

    // 分配器相关属性
    typename traits::allocator_type& get_allocator() { return alloc_; }
    const typename traits::allocator_type& get_allocator() const { return alloc_; }

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

// 接口层：vector_interface - 定义标准接口
template<typename T, typename Allocator>
class vector_interface : public sequence_traits<T, Allocator> {
    using traits = sequence_traits<T, Allocator>;
    IMPORT_SEQUENCE_TYPES(traits);
public:
    // 二、赋值操作
    virtual vector_interface& operator=(const vector_interface& x) = 0;
    virtual vector_interface& operator=(vector_interface&& x) noexcept = 0;
    virtual void assign(size_type n, const value_type& val) = 0;
    template<typename InputIterator>
    void assign(InputIterator first, InputIterator last);

    // 三、迭代器操作
    virtual iterator begin() noexcept = 0;
    virtual const_iterator begin() const noexcept = 0;
    virtual iterator end() noexcept = 0;
    virtual const_iterator end() const noexcept = 0;
    virtual reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    virtual const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    virtual reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    virtual const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

    // 四、容量操作
    virtual size_type size() const noexcept = 0;
    virtual size_type capacity() const noexcept = 0;
    virtual bool empty() const noexcept { return size() == 0; }
    virtual void reserve(size_type n) = 0;
    virtual void resize(size_type new_size, const value_type& x = value_type()) = 0;
    virtual void shrink_to_fit() = 0;

    // 五、元素访问
    virtual reference operator[](size_type n) = 0;
    virtual const_reference operator[](size_type n) const = 0;
    virtual reference at(size_type n) = 0;
    virtual const_reference at(size_type n) const = 0;
    virtual reference front() { return *begin(); }
    virtual const_reference front() const { return *begin(); }
    virtual reference back() { return *(end() - 1); }
    virtual const_reference back() const { return *(end() - 1); }
    virtual pointer data() noexcept { return begin(); }
    virtual const_pointer data() const noexcept { return begin(); }

    // 六、修改操作
    virtual void push_back(const value_type& x) = 0;
    virtual void push_back(value_type&& x) = 0;
    virtual void pop_back() = 0;
    
    virtual iterator insert(const_iterator position, const value_type& x) = 0;
    virtual iterator insert(const_iterator position, value_type&& x) = 0;
    virtual iterator insert(const_iterator position, size_type n, const value_type& x) = 0;
    template<typename InputIterator>
    iterator insert(const_iterator position, InputIterator first, InputIterator last);
    
    virtual iterator erase(const_iterator position) = 0;
    virtual iterator erase(const_iterator first, const_iterator last) = 0;
    virtual void clear() noexcept = 0;
    
    virtual void swap(vector_interface& x) noexcept = 0;
};

// 实现类：vector - 继承实体类并实现接口
template<typename T, typename Allocator = alloc<true, T, pool_tag>>
class vector : private vector_entity<T, Allocator>, public vector_interface<T, Allocator> {
    using entity = vector_entity<T, Allocator>;
    using interface = vector_interface<T, Allocator>;
    using traits = sequence_traits<T, Allocator>;
    IMPORT_SEQUENCE_TYPES(traits);
public:
    // 一、构造与析构
    vector() : entity() {}
    explicit vector(size_type n) : entity(n, allocator_type()) { fill_initialize(n, value_type()); }
    vector(size_type n, const value_type& value) : entity(n, allocator_type()) { fill_initialize(n, value); }
    template<typename InputIterator>
    vector(InputIterator first, InputIterator last) : entity() { range_initialize(first, last); }
    vector(const vector& x) : entity() { range_initialize(x.begin(), x.end()); }
    vector(vector&& x) noexcept : entity() { swap(x); }
    ~vector() { destroy(begin(), end()); }

    // 二、赋值操作
    vector& operator=(const vector& x) override {
        if (this != &x) {
            clear();
            range_initialize(x.begin(), x.end());
        }
        return *this;
    }
    vector& operator=(vector&& x) noexcept override {
        if (this != &x) {
            clear();
            swap(x);
        }
        return *this;
    }
    void assign(size_type n, const value_type& val) override;
    template<typename InputIterator>
    void assign(InputIterator first, InputIterator last);

    // 三、迭代器操作
    iterator begin() noexcept override { return entity::get_start(); }
    const_iterator begin() const noexcept override { return entity::get_start(); }
    iterator end() noexcept override { return entity::get_finish(); }
    const_iterator end() const noexcept override { return entity::get_finish(); }
    reverse_iterator rbegin() noexcept override { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept override { return const_reverse_iterator(end()); }
    reverse_iterator rend() noexcept override { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept override { return const_reverse_iterator(begin()); }

    // 四、容量操作
    size_type size() const noexcept override { return entity::get_size(); }
    size_type capacity() const noexcept override { return entity::get_capacity(); }
    bool empty() const noexcept override { return begin() == end(); }
    void reserve(size_type n) override;
    void resize(size_type new_size, const value_type& x = value_type()) override;
    void shrink_to_fit() override;

    // 五、元素访问
    reference operator[](size_type n) override { return *(begin() + n); }
    const_reference operator[](size_type n) const override { return *(begin() + n); }
    reference at(size_type n) override;
    const_reference at(size_type n) const override;
    reference front() override { return *begin(); }
    const_reference front() const override { return *begin(); }
    reference back() override { return *(end() - 1); }
    const_reference back() const override { return *(end() - 1); }
    pointer data() noexcept override { return begin(); }
    const_pointer data() const noexcept override { return begin(); }

    // 六、修改操作
    void push_back(const value_type& x) override;
    void push_back(value_type&& x) override;
    void pop_back() override;
    
    iterator insert(const_iterator position, const value_type& x) override;
    iterator insert(const_iterator position, value_type&& x) override;
    iterator insert(const_iterator position, size_type n, const value_type& x) override;
    template<typename InputIterator>
    iterator insert(const_iterator position, InputIterator first, InputIterator last);
    
    iterator erase(const_iterator position) override;
    iterator erase(const_iterator first, const_iterator last) override;
    void clear() noexcept override;
    
    void swap(vector& x) noexcept override;

private:
    // 辅助函数
    void fill_initialize(size_type n, const value_type& value);
    template<typename InputIterator>
    void range_initialize(InputIterator first, InputIterator last);
    void destroy(iterator first, iterator last);
    void insert_aux(iterator position, const value_type& x);
};

} // namespace Cat