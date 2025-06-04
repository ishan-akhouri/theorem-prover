#pragma once

#include <cstddef>

namespace theorem_prover
{

    /**
     * Combines a seed hash value with another hash value
     * Uses the Boost hash_combine implementation
     */
    inline void hash_combine(std::size_t &seed, std::size_t value)
    {
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

} // namespace theorem_prover