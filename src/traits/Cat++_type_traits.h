#pragma once
#include <cstddef>
#include <memory>
#include "../Cat++_config.h"  // for VectorMode, Mode

namespace Cat {

// 1. 基础类型工具
// 1.1 类型映射工具
// 用例：type_identity<int>::type x = 42;
template<typename T>
struct type_identity {
    using type = T;
};

// 1.2 类型转换工具
// 用例：conditional_t<true, int, double> x = 42;  // x是int类型
template<bool B, typename T, typename F>
struct conditional {
    using type = T;
};

template<typename T, typename F>
struct conditional<false, T, F> {
    using type = F;
};

// 1.3 SFINAE工具
// 用例：void_t<decltype(std::declval<T>().foo())>  // 检查T是否有foo()成员函数
template<typename...>
struct void_t {
    using type = void;
};

// 1.4 类型比较工具
// 用例：is_same_v<int, int>  // true
template<typename T, typename U>
struct is_same {
    static constexpr bool value = false;
};

template<typename T>
struct is_same<T, T> {
    static constexpr bool value = true;
};

// 1.5 类型选择工具
// 用例：enable_if_t<is_integral_v<T>, int> foo(T x) { return x; }  // 只接受整数类型
template<bool B, typename T = void>
struct enable_if {};

template<typename T>
struct enable_if<true, T> {
    using type = T;
};

// 2. 类型检测系统
// 2.1 基础类型检测
// 用例：is_integral_v<int>  // true
template<typename T>
struct is_integral {
    static constexpr bool value = false;
};

template<>
struct is_integral<int> { static constexpr bool value = true; };
template<>
struct is_integral<long> { static constexpr bool value = true; };
template<>
struct is_integral<long long> { static constexpr bool value = true; };
template<>
struct is_integral<unsigned int> { static constexpr bool value = true; };
template<>
struct is_integral<unsigned long> { static constexpr bool value = true; };
template<>
struct is_integral<unsigned long long> { static constexpr bool value = true; };

// 2.2 类型转换检测
// 用例：is_convertible_v<int, double>  // true
template<typename From, typename To>
struct is_convertible {
    static constexpr bool value = false;  // 需要实现
};

// 3. 成员检测系统
// 3.1 成员类型存在性检测
// 用例：has_value_type_v<vector<int>>  // true
template<typename T, typename = void>
struct has_value_type {
    static constexpr bool value = false;
};

template<typename T>
struct has_value_type<T, typename void_t<typename T::value_type>::type> {
    static constexpr bool value = true;
};

// 3.2 成员函数存在性检测
// 用例：has_allocate_v<allocator<int>>  // true
template<typename T, typename = void>
struct has_allocate {
    static constexpr bool value = false;
};

template<typename T>
struct has_allocate<T, typename void_t<
    decltype(T().allocate(std::size_t{}))
>::type> {
    static constexpr bool value = true;
};

// 3.3 成员函数返回值检测
// 用例：has_allocate_return_v<allocator<int>, int*>  // true
template<typename T, typename ReturnType, typename = void>
struct has_allocate_return {
    static constexpr bool value = false;
};

template<typename T, typename ReturnType>
struct has_allocate_return<T, ReturnType, typename void_t<
    typename is_same<
        decltype(T().allocate(std::size_t{})),
        ReturnType
    >::type
>::type> {
    static constexpr bool value = true;
};

// 4. 表达式检测系统
// 4.1 表达式合法性检测
// 用例：is_valid_expr_v<iterator>  // 检查是否支持++操作
template<typename T, typename = void>
struct is_valid_expr {
    static constexpr bool value = false;
};

template<typename T>
struct is_valid_expr<T, typename void_t<
    decltype(T()++)
>::type> {
    static constexpr bool value = true;
};

// 4.2 表达式返回值检测
// 用例：expr_return_type_v<iterator, iterator&>  // 检查++操作返回值类型
template<typename T, typename ReturnType, typename = void>
struct expr_return_type {
    static constexpr bool value = false;
};

template<typename T, typename ReturnType>
struct expr_return_type<T, ReturnType, typename void_t<
    typename is_same<
        decltype(T()++),
        ReturnType
    >::type
>::type> {
    static constexpr bool value = true;
};

// 5. 复合类型检测系统
// 5.1 分配器类型检测
// 用例：is_allocator_v<allocator<int>>  // true
template<typename T>
struct is_allocator {
    static constexpr bool value = 
        has_value_type<T>::value &&
        has_allocate<T>::value;
};

// 5.2 迭代器类型检测
// 用例：is_iterator_v<vector<int>::iterator>  // true
template<typename T>
struct is_iterator {
    static constexpr bool value = 
        has_value_type<T>::value &&
        is_valid_expr<T>::value;
};

// 5.3 容器类型检测
// 用例：is_container_v<vector<int>>  // true
template<typename T>
struct is_container {
    static constexpr bool value = 
        has_value_type<T>::value &&
        has_value_type<T>::value;  // 这里需要修改为正确的检测
};

// 6. 类型别名系统
// 6.1 类型检测别名
// 用例：if constexpr (is_integral_v<T>) { ... }
template<typename T>
inline constexpr bool is_integral_v = is_integral<T>::value;

template<typename T, typename U>
inline constexpr bool is_same_v = is_same<T, U>::value;

// 6.2 成员检测别名
// 用例：if constexpr (has_value_type_v<T>) { ... }
template<typename T>
inline constexpr bool has_value_type_v = has_value_type<T>::value;

template<typename T>
inline constexpr bool has_allocate_v = has_allocate<T>::value;

// 6.3 表达式检测别名
// 用例：if constexpr (is_valid_expr_v<T>) { ... }
template<typename T>
inline constexpr bool is_valid_expr_v = is_valid_expr<T>::value;

// 6.4 复合类型检测别名
// 用例：if constexpr (is_allocator_v<T>) { ... }
template<typename T>
inline constexpr bool is_allocator_v = is_allocator<T>::value;

template<typename T>
inline constexpr bool is_iterator_v = is_iterator<T>::value;

template<typename T>
inline constexpr bool is_container_v = is_container<T>::value;

// 7. 类型约束系统
// 7.1 类型约束工具
// 用例：enable_if_integral_t<T> foo(T x) { return x; }
template<typename T>
struct enable_if_integral {
    using type = typename enable_if<is_integral<T>::value>::type;
};

// 7.2 成员约束工具
// 用例：enable_if_has_value_type_t<T> foo(T x) { return x; }
template<typename T>
struct enable_if_has_value_type {
    using type = typename enable_if<has_value_type<T>::value>::type;
};

// 8. 类型转换系统
// 8.1 类型转换工具
// 用例：remove_reference_t<int&> x = 42;  // x是int类型
template<typename T>
struct remove_reference {
    using type = T;
};

template<typename T>
struct remove_reference<T&> {
    using type = T;
};

template<typename T>
struct remove_reference<T&&> {
    using type = T;
};

// 8.2 类型重绑定工具
// 用例：rebind_t<allocator<int>, double>  // 得到allocator<double>
template<typename T, typename U>
struct rebind {
    using type = U;
};

// 8.3 类型转换别名
// 用例：remove_reference_t<int&> x = 42;
template<typename T>
using remove_reference_t = typename remove_reference<T>::type;

template<typename T, typename U>
using rebind_t = typename rebind<T, U>::type;

// 9. 类型特征系统
// 9.1 类型特征工具
// 用例：is_trivial_v<int>  // true
template<typename T>
struct is_trivial {
    static constexpr bool value = false;  // 需要实现
};

template<typename T>
struct is_pod {
    static constexpr bool value = false;  // 需要实现
};

// 9.2 类型特征别名
// 用例：if constexpr (is_trivial_v<T>) { ... }
template<typename T>
inline constexpr bool is_trivial_v = is_trivial<T>::value;

template<typename T>
inline constexpr bool is_pod_v = is_pod<T>::value;

// 10. 模式相关类型系统
// 10.1 基础类型特征
// 用例：base_traits<int, Mode::Safe>::pointer p;  // p是shared_ptr<int>
template<typename T, Mode M = Mode::Safe>
struct base_traits {
    using value_type = T;
    using pointer = std::conditional_t<M == Mode::Safe,
        std::shared_ptr<T>,
        T*
    >;
    using const_pointer = std::conditional_t<M == Mode::Safe,
        std::shared_ptr<const T>,
        const T*
    >;
    using reference = std::conditional_t<M == Mode::Safe,
        std::shared_ptr<T>&,
        T&
    >;
    using const_reference = std::conditional_t<M == Mode::Safe,
        std::shared_ptr<const T>&,
        const T&
    >;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using iterator = std::conditional_t<M == Mode::Safe,
        std::shared_ptr<T>,
        T*
    >;
    using const_iterator = std::conditional_t<M == Mode::Safe,
        std::shared_ptr<const T>,
        const T*
    >;
    using reverse_iterator = std::conditional_t<M == Mode::Safe,
        std::shared_ptr<T>,
        T*
    >;
    using const_reverse_iterator = std::conditional_t<M == Mode::Safe,
        std::shared_ptr<const T>,
        const T*
    >;

    // 使用类型转换系统中的rebind
    template<typename U>
    using rebind = rebind_t<base_traits<T, M>, base_traits<U, M>>;
};

// 10.2 序列容器类型特征
// 用例：sequence_traits<vector<int>>::iterator it;
template<typename T, Mode M = Mode::Safe>
struct sequence_traits : public base_traits<T, M> {
    using base = base_traits<T, M>;
    using typename base::value_type;
    using typename base::pointer;
    using typename base::const_pointer;
    using typename base::reference;
    using typename base::const_reference;
    using typename base::size_type;
    using typename base::difference_type;
    using typename base::iterator;
    using typename base::const_iterator;
    using typename base::reverse_iterator;
    using typename base::const_reverse_iterator;

    // 使用基类的rebind
    template<typename U>
    using rebind = typename base::template rebind<U>;
};

// 10.3 关联容器类型特征
// 用例：associative_traits<map<int, string>>::iterator it;
template<typename T, Mode M = Mode::Safe>
struct associative_traits : public base_traits<T, M> {
    using base = base_traits<T, M>;
    using typename base::value_type;
    using typename base::pointer;
    using typename base::const_pointer;
    using typename base::reference;
    using typename base::const_reference;
    using typename base::size_type;
    using typename base::difference_type;
    using typename base::iterator;
    using typename base::const_iterator;
    using typename base::reverse_iterator;
    using typename base::const_reverse_iterator;

    // 使用基类的rebind
    template<typename U>
    using rebind = typename base::template rebind<U>;
};

// 11. 接口兼容性检查
// 11.1 STL兼容性检查
// 用例：if constexpr (is_stl_compatible_v<MyContainer>) { ... }
template<typename T>
struct is_stl_compatible {
    static constexpr bool value = 
        has_value_type<T>::value &&
        has_allocate<T>::value;
};

// 11.2 容器兼容性检查
// 用例：if constexpr (is_container_compatible_v<MyContainer>) { ... }
template<typename T>
struct is_container_compatible {
    static constexpr bool value = 
        has_value_type<T>::value &&
        has_allocate<T>::value &&
        is_valid_expr<T>::value;
};

// 11.3 迭代器兼容性检查
// 用例：if constexpr (is_iterator_compatible_v<MyIterator>) { ... }
template<typename T>
struct is_iterator_compatible {
    static constexpr bool value = 
        has_value_type<T>::value &&
        is_valid_expr<T>::value;
};

} // namespace Cat 