#pragma once

#include <functional>

namespace dusk
{
// from: https://stackoverflow.com/a/57595105
template <typename T, typename... Rest>
void hashCombine(std::size_t& seed, const T& v, const Rest&... rest)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    (hashCombine(seed, rest), ...);
}

// from: http://www.cse.yorku.ca/~oz/hash.html
inline size_t hash(const char* s)
{
    if (s == nullptr) return 0;

    size_t h = 5381;
    int    c;
    while ((c = *s++))
        h = ((h << 5) + h) + c; // DJB2 hash algorithm
    return h;
}
} // namespace dusk