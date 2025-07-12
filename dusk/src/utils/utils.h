#pragma once

#include "dusk.h"

#include <string>
#include <numeric>

#define DUSK_STRING_DELEMITER ";"

namespace dusk
{

inline size_t getAlignment(size_t instanceSize, size_t minOffsetAlignment)
{
    if (minOffsetAlignment > 0)
        return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);

    return instanceSize;
}

inline std::string combineStrings(const DynamicArray<std::string>& stringList)
{
    if (stringList.empty())
        return "";

    return std::accumulate(
        std::next(stringList.begin()),
        stringList.end(),
        stringList[0],
        [](const std::string& a, const std::string& b)
        {
            return a + DUSK_STRING_DELEMITER + b;
        });
}

} // namespace dusk