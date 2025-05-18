#pragma once
#include <cstddef>
#include <iterator>
#include <utility>  // for std::pair
#include <memory>   // for smart pointers
#include <concepts> // for concepts
#include "../Cat++_config.h"  // for VectorMode

namespace Cat {

// 基础概念定义
template<typename T>
concept HasValueType = requires {
    typename T::value_type;
};

template<typename T>
concept HasPointer = requires {
    typename T::pointer;
};

template<typename T>
concept HasReference = requires {
    typename T::reference;
};

template<typename T>
concept HasIterator = requires {
    typename T::iterator;
};

// 分配器概念
template<typename T>
concept Allocator = requires(T a, typename T::value_type* p, std::size_t n) {
    { a.allocate(n) } -> std::same_as<typename T::pointer>;
    { a.deallocate(p, n) } -> std::same_as<void>;
    { a.max_size() } -> std::same_as<typename T::size_type>;
    requires HasValueType<T>;
    requires HasPointer<T>;
    requires HasReference<T>;
};

// 迭代器概念
template<typename T>
concept Iterator = requires(T i) {
    { *i } -> std::same_as<typename T::reference>;
    { ++i } -> std::same_as<T&>;
    { i++ } -> std::same_as<T>;
    requires HasValueType<T>;
    requires HasPointer<T>;
    requires HasReference<T>;
};

// 容器概念
template<typename T>
concept Container = requires(T c) {
    { c.begin() } -> Iterator;
    { c.end() } -> Iterator;
    { c.size() } -> std::same_as<typename T::size_type>;
    requires HasValueType<T>;
    requires HasIterator<T>;
};

// 序列容器概念
template<typename T>
concept SequenceContainer = Container<T> && requires(T c, typename T::value_type v) {
    { c.push_back(v) } -> std::same_as<void>;
    { c.pop_back() } -> std::same_as<void>;
    { c.front() } -> std::same_as<typename T::reference>;
    { c.back() } -> std::same_as<typename T::reference>;
};

// 关联容器概念
template<typename T>
concept AssociativeContainer = Container<T> && requires(T c, typename T::key_type k) {
    { c.find(k) } -> Iterator;
    { c.count(k) } -> std::same_as<typename T::size_type>;
    { c.contains(k) } -> std::same_as<bool>;
};

// 可哈希类型概念
template<typename T>
concept Hashable = requires(T a) {
    { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
};

// 可比较类型概念
template<typename T>
concept Comparable = requires(T a, T b) {
    { a < b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
    { a <= b } -> std::convertible_to<bool>;
    { a >= b } -> std::convertible_to<bool>;
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
};

// 可移动类型概念
template<typename T>
concept Movable = std::is_move_constructible_v<T> && std::is_move_assignable_v<T>;

// 可复制类型概念
template<typename T>
concept Copyable = std::is_copy_constructible_v<T> && std::is_copy_assignable_v<T>;

// 可默认构造类型概念
template<typename T>
concept DefaultConstructible = std::is_default_constructible_v<T>;

// 可销毁类型概念
template<typename T>
concept Destructible = std::is_destructible_v<T>;

// 可交换类型概念
template<typename T>
concept Swappable = requires(T& a, T& b) {
    std::swap(a, b);
};

// 可调用类型概念
template<typename F, typename... Args>
concept Callable = requires(F&& f, Args&&... args) {
    std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
};

// 可迭代类型概念
template<typename T>
concept Iterable = requires(T t) {
    { t.begin() } -> Iterator;
    { t.end() } -> Iterator;
    requires std::same_as<decltype(t.begin()), decltype(t.end())>;
};

// 可随机访问类型概念
template<typename T>
concept RandomAccess = Iterable<T> && requires(T t, typename T::size_type n) {
    { t[n] } -> std::same_as<typename T::reference>;
    { t.at(n) } -> std::same_as<typename T::reference>;
};

} // namespace Cat 