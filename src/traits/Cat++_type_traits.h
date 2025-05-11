#pragma once
#include <cstddef>
#include <iterator>
#include <utility>  // for std::pair

namespace Cat {

//allocator类型特征
template<bool threads, typename T>
class allocator_traits {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;  
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
};

// 序列容器类型特征
template<typename T, typename Allocator>
struct sequence_traits {
    using value_type =             T;
    using allocator_type =         Allocator;
    using size_type =              std::size_t;
    using difference_type =        std::ptrdiff_t;
    using pointer =                T*;
    using const_pointer =          const T*;
    using reference =              T&;
    using const_reference =        const T&;
    using iterator =               pointer;
    using const_iterator =         const_pointer;
    using reverse_iterator =       std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
};

// 关联容器类型特征
template<typename Key, typename Value, typename Allocator>
struct associative_traits {
    using key_type =               Key;
    using value_type =             std::pair<const Key, Value>;
    using allocator_type =         Allocator;
    using size_type =              std::size_t;
    using difference_type =        std::ptrdiff_t;
    using pointer =                value_type*;
    using const_pointer =          const value_type*;
    using reference =              value_type&;
    using const_reference =        const value_type&;
    using iterator =               pointer;
    using const_iterator =         const_pointer;
    using reverse_iterator =       std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
};

//allocator类型继承宏
#define IMPORT_ALLOCATOR_TYPES(Traits) \
    using value_type = typename Traits::value_type; \
    using pointer = typename Traits::pointer; \
    using const_pointer = typename Traits::const_pointer; \
    using reference = typename Traits::reference; \
    using const_reference = typename Traits::const_reference; \
    using size_type = typename Traits::size_type; \
    using difference_type = typename Traits::difference_type


// 序列容器类型继承宏
#define IMPORT_SEQUENCE_TYPES(Traits) \
    using value_type = typename Traits::value_type; \
    using allocator_type = typename Traits::allocator_type; \
    using size_type = typename Traits::size_type; \
    using difference_type = typename Traits::difference_type; \
    using pointer = typename Traits::pointer; \
    using const_pointer = typename Traits::const_pointer; \
    using reference = typename Traits::reference; \
    using const_reference = typename Traits::const_reference; \
    using iterator = typename Traits::iterator; \
    using const_iterator = typename Traits::const_iterator; \
    using reverse_iterator = typename Traits::reverse_iterator; \
    using const_reverse_iterator = typename Traits::const_reverse_iterator

// 关联容器类型继承宏
#define IMPORT_ASSOCIATIVE_TYPES(Traits) \
    IMPORT_TYPES(Traits); \
    using key_type = typename Traits::key_type

} // namespace Cat 