#pragma once

#include <cstddef>      // 用于size_t
#include <type_traits>  // 用于类型特征
#include <functional>   // 用于std::function
#include "../alloc/Cat++_alloc_selector.h"  // 自定义分配器
#include "../execption/allocator_exception.h"  // 自定义异常
#include "../traits/Cat++_type_traits.h"  // 容器类型特征
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <initializer_list>
#include "../Cat++_config.h"  // for VectorMode

namespace Cat {

// 性能统计结构体
struct vector_stats {
    size_t allocations = 0;        // 内存分配次数
    size_t deallocations = 0;      // 内存释放次数
    size_t total_allocated = 0;    // 总分配内存大小
    size_t max_allocated = 0;      // 最大分配内存大小
    size_t reallocations = 0;      // 内存重分配次数
    size_t copies = 0;             // 元素拷贝次数
    size_t moves = 0;              // 元素移动次数
    size_t comparisons = 0;        // 元素比较次数
    size_t resizes = 0;            // 容器大小调整次数
    size_t insertions = 0;         // 元素插入次数
    size_t erasures = 0;           // 元素删除次数

    void reset() noexcept {
        *this = vector_stats{};
    }
};

// 数据层：vector_base - 定义内存布局
template<typename T, typename Allocator, VectorMode Mode = VectorMode::Safe>
class vector_entity : public type_inherit<vector_entity<T, Allocator, Mode>, sequence_traits<T, Allocator, Mode>> {
protected:
    using traits = sequence_traits<T, Allocator, Mode>;
    // 按需提升为当前作用域的类型别名
    using value_ptr = T*;                    // 值指针类型
    using allocator_type = Allocator;        // 分配器类型
    using size_type = typename traits::size_type;
    using pointer = typename traits::pointer;
    using const_pointer = typename traits::const_pointer;
    using reference = typename traits::reference;
    using const_reference = typename traits::const_reference;
    using value_type = typename traits::value_type;

    // 根据模式选择存储类型
    value_ptr start_ = nullptr;           // 指向第一个元素
    value_ptr finish_ = nullptr;          // 指向最后一个元素的下一个位置
    value_ptr end_of_storage_ = nullptr;  // 指向分配内存的末尾
    allocator_type alloc_;                // 分配器实例

    // 性能优化：使用static constexpr在编译期确定常量
    static constexpr size_t default_growth_factor_ = 2;       // 默认扩容因子
    static constexpr size_t default_initial_capacity_ = 16;   // 默认初始容量
    
    // 运行时可配置的容量参数
    size_t growth_factor_ = default_growth_factor_;
    size_t initial_capacity_ = default_initial_capacity_;

    // 计算属性：内联函数，减少函数调用开销
    [[nodiscard]] inline size_t size() const noexcept { return finish_ - start_; }
    [[nodiscard]] inline size_t capacity() const noexcept { return end_of_storage_ - start_; }

    // 性能统计
    static inline vector_stats stats_;

protected:
    // 默认容量0初始化
    // 性能优化：使用noexcept标记不会抛出异常的函数
    vector_entity() noexcept : alloc_(Allocator()) {
        try {
            start_ = alloc_.allocate(initial_capacity_);
            finish_ = start_;
            end_of_storage_ = start_ + initial_capacity_;
        } catch (const std::exception& e) {
            throw allocator_exception(e.what());
        }
    }

    // 指定容量0初始化
    explicit vector_entity(size_t capacity) : alloc_(Allocator()) {// explicit 修饰的但参数构造函数不能用于隐式转换，多参数构造函数本身就不会发生隐式转换，char*常作为连续内存的指针类型，也不希望显式转换
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
    // 性能优化：使用完美转发和可变参数模板
    template<typename... Args>
    vector_entity(size_t capacity, Args&&... args) : alloc_(Allocator()) {
        capacity = (capacity <= initial_capacity_) ? initial_capacity_ : capacity;
        constexpr int argsnum = sizeof...(args);
        try {
            if(capacity >= argsnum) {//正常分配内存
                start_ = static_cast<typename traits::pointer>(alloc_.allocate(capacity));
                // 用args构造对象
                // 性能优化：使用emplace构造对象
                for (size_type i = 0; i < argsnum; ++i) {
                    alloc_.construct(start_ + i, std::forward<Args>(args)...);
                }
                finish_ = start_ + argsnum;
                end_of_storage_ = start_ + capacity;
            } else {//按照args数量分配内存，或自动计算一个合适的容量分配
                start_ = alloc_.allocate(argsnum);
                //构造对象
                for (size_t i = 0; i < argsnum; ++i) {
                    alloc_.construct(start_ + i, std::forward<Args>(args)...);
                }
                finish_ = start_ + argsnum;
                end_of_storage_ = start_ + capacity;
            }
        } catch (const std::exception& e) {
            // 异常安全：清理已构造的对象
            if (start_) {
                for (T* p = start_; p != finish_; ++p) {
                    alloc_.destroy(p);
                }
                alloc_.deallocate(start_, (capacity>argsnum)?capacity:argsnum);
            }
            throw allocator_exception(e.what());
        }
    }

    // 移动构造：使用移动语义
    vector_entity(vector_entity&& x) noexcept 
        : start_(x.start_)
        , finish_(x.finish_)
        , end_of_storage_(x.end_of_storage_)
        , alloc_(std::move(x.alloc_))
        , growth_factor_(x.growth_factor_)
        , initial_capacity_(x.initial_capacity_) {
        x.start_ = x.finish_ = x.end_of_storage_ = nullptr;
    }

    // 性能优化：使用noexcept标记析构函数
    ~vector_entity() noexcept {
        if (start_) {
            alloc_.deallocate(start_, end_of_storage_ - start_);
            start_ = finish_ = end_of_storage_ = nullptr;
        }
    }

public:
    // 指针相关属性：使用[[nodiscard]]防止忽略返回值
    [[nodiscard]] T* get_start() const noexcept { return start_; }
    void set_start(T* ptr) noexcept { start_ = ptr; }
    [[nodiscard]] T* get_finish() const noexcept { return finish_; }
    void set_finish(T* ptr) noexcept { finish_ = ptr; }
    [[nodiscard]] T* get_end_of_storage() const noexcept { return end_of_storage_; }
    void set_end_of_storage(T* ptr) noexcept { end_of_storage_ = ptr; }

    // 分配器相关属性：使用引用返回避免拷贝
    [[nodiscard]] Allocator& get_allocator() noexcept { return alloc_; }
    [[nodiscard]] const Allocator& get_allocator() const noexcept { return alloc_; }

    // 计算属性（只读）：使用constexpr在编译期计算
    [[nodiscard]] constexpr size_t get_size() const noexcept { return size(); }
    [[nodiscard]] constexpr size_t get_capacity() const noexcept { return capacity(); }

    // 配置属性：使用constexpr在编译期计算
    [[nodiscard]] constexpr size_t get_growth_factor() const noexcept { return growth_factor_; }
    void set_growth_factor(size_t factor) noexcept { growth_factor_ = factor; }
    [[nodiscard]] constexpr size_t get_initial_capacity() const noexcept { return initial_capacity_; }
    void set_initial_capacity(size_t capacity) noexcept { initial_capacity_ = capacity; }

    // 计算新容量：使用constexpr在编译期计算
    [[nodiscard]] constexpr size_t calculate_new_capacity(size_t min_capacity) const noexcept {
        if (min_capacity <= get_capacity()) {
            return get_capacity();
        }
        return std::max(get_capacity() * get_growth_factor(), min_capacity);
    }

    // 获取当前模式
    [[nodiscard]] static constexpr VectorMode get_mode() noexcept { return Mode; }

    // 性能统计接口
    [[nodiscard]] static const vector_stats& get_stats() noexcept { return stats_; }
    static void reset_stats() noexcept { stats_.reset(); }

    // 性能监控辅助函数
    void record_allocation(size_t size) noexcept {
        stats_.allocations++;
        stats_.total_allocated += size;
        stats_.max_allocated = std::max(stats_.max_allocated, stats_.total_allocated);
    }

    void record_deallocation(size_t size) noexcept {
        stats_.deallocations++;
        stats_.total_allocated -= size;
    }

    void record_reallocation() noexcept {
        stats_.reallocations++;
    }

    void record_copy() noexcept {
        stats_.copies++;
    }

    void record_move() noexcept {
        stats_.moves++;
    }

    void record_comparison() noexcept {
        stats_.comparisons++;
    }

    void record_resize() noexcept {
        stats_.resizes++;
    }

    void record_insertion() noexcept {
        stats_.insertions++;
    }

    void record_erasure() noexcept {
        stats_.erasures++;
    }
};

// 接口层：vector_interface
template<typename T, typename Allocator, VectorMode Mode = VectorMode::Safe>
class vector_interface : public type_inherit<vector_interface<T, Allocator, Mode>, sequence_traits<T, Allocator, Mode>> {
public:
    using traits = sequence_traits<T, Allocator, Mode>;
    //按需提升为当前作用域的类型别名
    using iterator = typename traits::iterator;
    using const_iterator = typename traits::const_iterator;
    using reverse_iterator = typename traits::reverse_iterator;
    using const_reverse_iterator = typename traits::const_reverse_iterator;
    using reference = typename traits::reference;
    using const_reference = typename traits::const_reference;
    using pointer = typename traits::pointer;
    using const_pointer = typename traits::const_pointer;
    using size_type = typename traits::size_type;
    using value_type = typename traits::value_type;

    // 获取迭代器/指针
    virtual iterator begin() noexcept = 0;                    // 获取首元素迭代器
    virtual const_iterator begin() const noexcept = 0;        // 获取首元素常量迭代器
    virtual iterator end() noexcept = 0;                      // 获取尾后迭代器
    virtual const_iterator end() const noexcept = 0;          // 获取尾后常量迭代器
    virtual reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }           // 获取反向首元素迭代器
    virtual const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }  // 获取反向首元素常量迭代器
    virtual reverse_iterator rend() noexcept { return reverse_iterator(begin()); }           // 获取反向尾后迭代器
    virtual const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }  // 获取反向尾后常量迭代器

    // 1.2 元素访问
    virtual reference operator[](size_type n) = 0;           // 下标访问（无边界检查）
    virtual const_reference operator[](size_type n) const = 0;  // 常量下标访问
    virtual reference at(size_type n) = 0;                   // 下标访问（有边界检查）
    virtual const_reference at(size_type n) const = 0;       // 常量下标访问（有边界检查）
    virtual reference front() { return *begin(); }           // 访问首元素
    virtual const_reference front() const { return *begin(); }  // 常量访问首元素
    virtual reference back() { return *(end() - 1); }        // 访问尾元素
    virtual const_reference back() const { return *(end() - 1); }  // 常量访问尾元素
    virtual pointer data() noexcept { return begin(); }      // 获取底层数据指针
    virtual const_pointer data() const noexcept { return begin(); }  // 获取常量底层数据指针

    // 1.3 容量查询
    virtual size_type size() const noexcept = 0;             // 获取元素数量
    virtual size_type capacity() const noexcept = 0;         // 获取容量
    virtual bool empty() const noexcept { return size() == 0; }  // 检查是否为空

    // 2.1 尾部添加
    virtual void push_back(const value_type& x) = 0;        // 尾部添加元素（拷贝）
    virtual void push_back(value_type&& x) = 0;             // 尾部添加元素（移动）

    // 2.2 指定位置插入
    virtual iterator insert(const_iterator position, const value_type& x) = 0;  // 插入单个元素（拷贝）
    virtual iterator insert(const_iterator position, value_type&& x) = 0;       // 插入单个元素（移动）
    virtual iterator insert(const_iterator position, size_type n, const value_type& x) = 0;  // 插入n个相同元素
    template<typename InputIterator>
    iterator insert(const_iterator position, InputIterator first, InputIterator last);   // 插入范围元素

    // 3.1 容量调整
    virtual void reserve(size_type n) = 0;                  // 预留空间
    virtual void resize(size_type new_size, const value_type& x = value_type()) = 0;  // 调整大小
    virtual void shrink_to_fit() = 0;                       // 收缩到合适大小

    // 3.2 赋值操作
    virtual vector_interface& operator=(const vector_interface& x) = 0;  // 拷贝赋值
    virtual vector_interface& operator=(vector_interface&& x) noexcept = 0;  // 移动赋值
    virtual void assign(size_type n, const value_type& val) = 0;  // 填充赋值
    template<typename InputIterator>
    void assign(InputIterator first, InputIterator last);   // 范围赋值

    // 3.3 交换操作
    virtual void swap(vector_interface& x) noexcept = 0;    // 交换两个容器

    // 4.1 尾部删除
    virtual void pop_back() = 0;                            // 删除尾部元素

    // 4.2 指定位置删除
    virtual iterator erase(const_iterator position) = 0;     // 删除单个元素
    virtual iterator erase(const_iterator first, const_iterator last) = 0;  // 删除范围元素

    // 4.3 清空操作
    virtual void clear() noexcept = 0;                      // 清空容器

    // 4.4 模式相关操作
    [[nodiscard]] static constexpr VectorMode get_mode() noexcept { return Mode; }
    [[nodiscard]] static constexpr bool is_safe_mode() noexcept { return Mode == VectorMode::Safe; }
    [[nodiscard]] static constexpr bool is_fast_mode() noexcept { return Mode == VectorMode::Fast; }

    // 性能统计接口
    [[nodiscard]] static const vector_stats& get_stats() noexcept { 
        return vector_entity<T, Allocator, Mode>::get_stats(); 
    }
    static void reset_stats() noexcept { 
        vector_entity<T, Allocator, Mode>::reset_stats(); 
    }
};

// 实现类：vector 
template<typename T, typename Allocator, VectorMode Mode = VectorMode::Safe>
class vector : public vector_entity<T, Allocator, Mode>, public vector_interface<T, Allocator, Mode> {
public:
    using entity = vector_entity<T, Allocator, Mode>;
    using interface = vector_interface<T, Allocator, Mode>;
    using traits = sequence_traits<T, Allocator, Mode>;
    // 按需提升为当前作用域的类型别名
    using value_type = typename traits::value_type;
    using allocator_type = typename traits::allocator_type;
    using pointer = typename traits::pointer;
    using const_pointer = typename traits::const_pointer;
    using reference = typename traits::reference;
    using const_reference = typename traits::const_reference;
    using size_type = typename traits::size_type;
    using iterator = typename traits::iterator;
    using const_iterator = typename traits::const_iterator;
    using reverse_iterator = typename traits::reverse_iterator;
    using const_reverse_iterator = typename traits::const_reverse_iterator;

    // 构造与析构
    vector() : entity() {}
    explicit vector(size_t n) : entity(n, allocator_type()) { fill_initialize(n, value_type()); }
    vector(size_t n, const value_type& value) : entity(n, allocator_type()) { fill_initialize(n, value); }
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

    // 五、元素访问
    reference operator[](size_type n) override { 
        if constexpr (Mode == VectorMode::Safe) {
            return at(n);
        } else {
            return *(begin() + n);
        }
    }
    const_reference operator[](size_type n) const override { 
        if constexpr (Mode == VectorMode::Safe) {
            return at(n);
        } else {
            return *(begin() + n);
        }
    }
    reference at(size_type n) override {
        if (n >= size()) {
            throw std::out_of_range("vector::at - index out of range");
        }
        return (*this)[n];
    }
    const_reference at(size_type n) const override {
        if (n >= size()) {
            throw std::out_of_range("vector::at - index out of range");
        }
        return (*this)[n];
    }
    reference front() override { return *begin(); }
    const_reference front() const override { return *begin(); }
    reference back() override { return *(end() - 1); }
    const_reference back() const override { return *(end() - 1); }
    pointer data() noexcept override { return begin(); }
    const_pointer data() const noexcept override { return begin(); }

    // 六、修改操作
    void push_back(const value_type& x) override {
        entity::record_insertion();
        entity::record_copy();
        if (entity::get_finish() != entity::get_end_of_storage()) {
            entity::get_allocator().construct(entity::get_finish(), x);
            entity::set_finish(entity::get_finish() + 1);
        } else {
            insert_aux(end(), x);
        }
    }

    void push_back(value_type&& x) override {
        entity::record_insertion();
        entity::record_move();
        if (entity::get_finish() != entity::get_end_of_storage()) {
            entity::get_allocator().construct(entity::get_finish(), std::move(x));
            entity::set_finish(entity::get_finish() + 1);
        } else {
            insert_aux(end(), std::move(x));
        }
    }

    void pop_back() override {
        if (begin() == end()) {
            throw std::out_of_range("vector::pop_back - vector is empty");
        }
        entity::set_finish(entity::get_finish() - 1);
        entity::get_allocator().destroy(entity::get_finish());
    }

    iterator insert(const_iterator position, const value_type& x) override {
        return insert_aux(position, x);
    }

    iterator insert(const_iterator position, value_type&& x) override {
        return insert_aux(position, std::move(x));
    }

    iterator insert(const_iterator position, size_type n, const value_type& x) override {
        if (n == 0) return position;
        
        size_type pos = position - begin();
        if (size() + n > capacity()) {
            size_type new_capacity = entity::calculate_new_capacity(size() + n);
            pointer new_start = entity::get_allocator().allocate(new_capacity);
            pointer new_finish = new_start;
            
            try {
                // 复制position之前的元素
                new_finish = std::uninitialized_copy(begin(), position, new_start);
                // 插入n个x
                new_finish = std::uninitialized_fill_n(new_finish, n, x);
                // 复制position之后的元素
                new_finish = std::uninitialized_copy(position, end(), new_finish);
            } catch (...) {
                destroy(new_start, new_finish);
                entity::get_allocator().deallocate(new_start, new_capacity);
                throw;
            }
            
            destroy(begin(), end());
            entity::get_allocator().deallocate(entity::get_start(), capacity());
            
            entity::set_start(new_start);
            entity::set_finish(new_finish);
            entity::set_end_of_storage(new_start + new_capacity);
        } else {
            pointer old_finish = entity::get_finish();
            entity::set_finish(entity::get_finish() + n);
            
            // 移动position之后的元素
            std::move_backward(position, old_finish, entity::get_finish());
            // 插入n个x
            std::fill_n(position, n, x);
        }
        
        return begin() + pos;
    }

    template<typename InputIterator>
    iterator insert(const_iterator position, InputIterator first, InputIterator last) {
        size_type pos = position - begin();
        size_type n = std::distance(first, last);
        
        if (n == 0) return position;
        
        if (size() + n > capacity()) {
            size_type new_capacity = entity::calculate_new_capacity(size() + n);
            pointer new_start = entity::get_allocator().allocate(new_capacity);
            pointer new_finish = new_start;
            
            try {
                // 复制position之前的元素
                new_finish = std::uninitialized_copy(begin(), position, new_start);
                // 插入[first, last)范围的元素
                new_finish = std::uninitialized_copy(first, last, new_finish);
                // 复制position之后的元素
                new_finish = std::uninitialized_copy(position, end(), new_finish);
            } catch (...) {
                destroy(new_start, new_finish);
                entity::get_allocator().deallocate(new_start, new_capacity);
                throw;
            }
            
            destroy(begin(), end());
            entity::get_allocator().deallocate(entity::get_start(), capacity());
            
            entity::set_start(new_start);
            entity::set_finish(new_finish);
            entity::set_end_of_storage(new_start + new_capacity);
        } else {
            pointer old_finish = entity::get_finish();
            entity::set_finish(entity::get_finish() + n);
            
            // 移动position之后的元素
            std::move_backward(position, old_finish, entity::get_finish());
            // 插入[first, last)范围的元素
            std::copy(first, last, position);
        }
        
        return begin() + pos;
    }

    iterator erase(const_iterator position) override {
        if (position == end()) {
            throw std::out_of_range("vector::erase - position out of range");
        }
        iterator result = const_cast<iterator>(position);
        std::move(position + 1, end(), result);
        entity::set_finish(entity::get_finish() - 1);
        entity::get_allocator().destroy(entity::get_finish());
        return result;
    }

    iterator erase(const_iterator first, const_iterator last) override {
        if (first == end() || first > last || last > end()) {
            throw std::out_of_range("vector::erase - range out of range");
        }
        iterator result = const_cast<iterator>(first);
        std::move(last, end(), result);
        size_type n = last - first;
        for (size_type i = 0; i < n; ++i) {
            entity::get_allocator().destroy(entity::get_finish() - n + i);
        }
        entity::set_finish(entity::get_finish() - n);
        return result;
    }

    void clear() noexcept override {
        destroy(begin(), end());
        entity::set_finish(entity::get_start());
    }

    void reserve(size_type n) override {
        if (n > capacity()) {
            pointer new_start = entity::get_allocator().allocate(n);
            pointer new_finish = new_start;
            
            try {
                new_finish = std::uninitialized_copy(begin(), end(), new_start);
            } catch (...) {
                entity::get_allocator().deallocate(new_start, n);
                throw;
            }
            
            destroy(begin(), end());
            entity::get_allocator().deallocate(entity::get_start(), capacity());
            
            entity::set_start(new_start);
            entity::set_finish(new_finish);
            entity::set_end_of_storage(new_start + n);
        }
    }

    void resize(size_type new_size, const value_type& x = value_type()) override {
        if (new_size < size()) {
            destroy(begin() + new_size, end());
            entity::set_finish(begin() + new_size);
        } else if (new_size > size()) {
            if (new_size > capacity()) {
                reserve(new_size);
            }
            std::uninitialized_fill(end(), begin() + new_size, x);
            entity::set_finish(begin() + new_size);
        }
    }

    void shrink_to_fit() override {
        if (size() < capacity()) {
            pointer new_start = entity::get_allocator().allocate(size());
            pointer new_finish = new_start;
            
            try {
                new_finish = std::uninitialized_copy(begin(), end(), new_start);
            } catch (...) {
                entity::get_allocator().deallocate(new_start, size());
                throw;
            }
            
            destroy(begin(), end());
            entity::get_allocator().deallocate(entity::get_start(), capacity());
            
            entity::set_start(new_start);
            entity::set_finish(new_finish);
            entity::set_end_of_storage(new_start + size());
        }
    }

    void assign(size_type n, const value_type& val) override {
        if (n > capacity()) {
            pointer new_start = entity::get_allocator().allocate(n);
            pointer new_finish = new_start;
            
            try {
                new_finish = std::uninitialized_fill_n(new_start, n, val);
            } catch (...) {
                entity::get_allocator().deallocate(new_start, n);
                throw;
            }
            
            destroy(begin(), end());
            entity::get_allocator().deallocate(entity::get_start(), capacity());
            
            entity::set_start(new_start);
            entity::set_finish(new_finish);
            entity::set_end_of_storage(new_start + n);
        } else {
            destroy(begin(), end());
            entity::set_finish(std::uninitialized_fill_n(begin(), n, val));
        }
    }

    template<typename InputIterator>
    void assign(InputIterator first, InputIterator last) {
        clear();
        if constexpr (Mode == VectorMode::Fast) {
            // 快速模式：直接分配足够空间
            size_t count = std::distance(first, last);
            reserve(count);
            while (first != last) {
                push_back(*first++);
            }
        } else {
            // 安全模式：逐个插入并检查异常
            try {
                while (first != last) {
                    push_back(*first++);
                }
            } catch (...) {
                clear();
                throw;
            }
        }
    }

    void swap(vector& x) noexcept override {
        std::swap(entity::get_start(), x.entity::get_start());
        std::swap(entity::get_finish(), x.entity::get_finish());
        std::swap(entity::get_end_of_storage(), x.entity::get_end_of_storage());
        std::swap(entity::get_allocator(), x.entity::get_allocator());
    }

private:
    // 辅助函数
    void fill_initialize(size_type n, const value_type& value) {
        for (size_type i = 0; i < n; ++i) {
            push_back(value);
        }
    }

    template<typename InputIterator>
    void range_initialize(InputIterator first, InputIterator last) {
        for (InputIterator it = first; it != last; ++it) {
            push_back(*it);
        }
    }

    void destroy(iterator first, iterator last) {
        for (iterator it = first; it != last; ++it) {
            entity::get_allocator().destroy(it);
        }
    }

    template<typename... Args>
    iterator insert_aux(const_iterator position, Args&&... args) {
        size_type pos = position - begin();
        if (entity::get_finish() != entity::get_end_of_storage()) {
            // 移动position之后的元素
            entity::get_allocator().construct(entity::get_finish(), std::move(*(entity::get_finish() - 1)));
            entity::set_finish(entity::get_finish() + 1);
            std::move_backward(begin() + pos, entity::get_finish() - 2, entity::get_finish() - 1);
            // 在position处构造新元素
            entity::get_allocator().destroy(begin() + pos);
            entity::get_allocator().construct(begin() + pos, std::forward<Args>(args)...);
        } else {
            size_type new_capacity = entity::calculate_new_capacity(size() + 1);
            pointer new_start = entity::get_allocator().allocate(new_capacity);
            pointer new_finish = new_start;
            try {
                // 复制position之前的元素
                new_finish = std::uninitialized_copy(begin(), position, new_start);
                // 在position处构造新元素
                entity::get_allocator().construct(new_finish, std::forward<Args>(args)...);
                ++new_finish;
                // 复制position之后的元素
                new_finish = std::uninitialized_copy(position, end(), new_finish);
            } catch (...) {
                destroy(new_start, new_finish);
                entity::get_allocator().deallocate(new_start, new_capacity);
                throw;
            }
            destroy(begin(), end());
            entity::get_allocator().deallocate(entity::get_start(), capacity());
            entity::set_start(new_start);
            entity::set_finish(new_finish);
            entity::set_end_of_storage(new_start + new_capacity);
        }
        return begin() + pos;
    }

    // 编译期优化：根据类型选择不同的实现
    template<typename U = T>
    void resize(size_type new_size, const value_type& x = value_type()) {
        if constexpr (std::is_trivially_copyable_v<U>) {
            // 对于平凡类型，直接调整大小
            if (new_size > size()) {
                reserve(new_size);
                std::uninitialized_fill_n(end(), new_size - size(), x);
            } else if (new_size < size()) {
                destroy(begin() + new_size, end());
            }
            entity::set_finish(begin() + new_size);
        } else {
            // 对于非平凡类型，逐个构造/销毁
            if (new_size > size()) {
                reserve(new_size);
                while (size() < new_size) {
                    push_back(x);
                }
            } else if (new_size < size()) {
                while (size() > new_size) {
                    pop_back();
                }
            }
        }
        entity::record_resize();
    }

    // 编译期优化：根据分配器类型选择不同的实现
    template<typename U = Allocator>
    void reserve(size_type n) {
        if constexpr (std::is_same_v<U, std::allocator<T>>) {
            // 使用STL分配器
            if (n > capacity()) {
                pointer new_start = entity::get_allocator().allocate(n);
                pointer new_finish = new_start;
                try {
                    new_finish = std::uninitialized_move(begin(), end(), new_start);
                } catch (...) {
                    entity::get_allocator().deallocate(new_start, n);
                    throw;
                }
                destroy(begin(), end());
                entity::get_allocator().deallocate(entity::get_start(), capacity());
                entity::set_start(new_start);
                entity::set_finish(new_finish);
                entity::set_end_of_storage(new_start + n);
            }
        } else {
            // 使用自定义分配器
            if (n > capacity()) {
                pointer new_start = entity::get_allocator().allocate(n);
                pointer new_finish = new_start;
                try {
                    new_finish = std::uninitialized_move(begin(), end(), new_start);
                } catch (...) {
                    entity::get_allocator().deallocate(new_start, n);
                    throw;
                }
                destroy(begin(), end());
                entity::get_allocator().deallocate(entity::get_start(), capacity());
                entity::set_start(new_start);
                entity::set_finish(new_finish);
                entity::set_end_of_storage(new_start + n);
            }
        }
        entity::record_allocation(n * sizeof(T));
    }

    // 编译期优化：根据模式选择不同的比较实现
    template<typename U = T>
    bool operator==(const vector& x) const {
        if constexpr (Mode == VectorMode::Fast) {
            // 快速模式：直接比较
            return size() == x.size() && std::equal(begin(), end(), x.begin());
        } else {
            // 安全模式：逐个比较并检查异常
            if (size() != x.size()) return false;
            try {
                return std::equal(begin(), end(), x.begin());
            } catch (...) {
                return false;
            }
        }
    }

    // 编译期优化：根据类型选择不同的排序实现
    template<typename U = T>
    void sort() {
        if constexpr (std::is_arithmetic_v<U>) {
            // 对于算术类型，使用快速排序
            std::sort(begin(), end());
        } else if constexpr (std::is_same_v<U, std::string>) {
            // 对于字符串，使用归并排序
            std::stable_sort(begin(), end());
        } else {
            // 对于其他类型，使用默认排序
            std::sort(begin(), end());
        }
    }
};

} // namespace Cat