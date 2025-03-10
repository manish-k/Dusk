#pragma once

namespace dusk
{

inline size_t getAlignment(size_t instanceSize, size_t minOffsetAlignment)
{
    if (minOffsetAlignment > 0)
        return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);

    return instanceSize;
}

} // namespace dusk