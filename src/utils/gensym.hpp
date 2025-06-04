#pragma once

#include <string>
#include <atomic>

namespace theorem_prover
{

    /**
     * Generates a fresh unique symbol with the given prefix
     *
     * This is used for creating fresh variable names during
     * term conversion and manipulation to avoid name clashes.
     *
     * @param prefix The prefix for the generated symbol
     * @return A unique string with the given prefix
     */
    inline std::string gensym(const std::string &prefix)
    {
        // Use a static counter to ensure uniqueness across calls
        static std::atomic<unsigned long> counter(0);
        return prefix + "_" + std::to_string(counter++);
    }

} // namespace theorem_prover