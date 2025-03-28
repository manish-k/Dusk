#pragma once

#include <memory>
#include <vector>
#include <array>
#include <ExcaliburHash.h>

namespace dusk
{
template <typename T>
using Unique = std::unique_ptr<T>;

template <typename T>
using Shared = std::shared_ptr<T>;

template <typename T, typename... Args>
constexpr Unique<T> createUnique(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T, typename... Args>
constexpr Shared<T> createShared(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T>
using DynamicArray = std::vector<T>;

template <typename T, std::size_t N>
using Array = std::array<T, N>;

template <typename K, typename V>
using HashMap = Excalibur::HashTable<K, V>;

template <typename T>
using HashSet = Excalibur::HashSet<T>;
} // namespace dusk