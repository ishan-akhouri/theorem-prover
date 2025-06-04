#pragma once

#include <memory>
#include <unordered_map>
#include "term_db.hpp"

namespace theorem_prover
{

    /**
     * Mapping from De Bruijn indices to terms for substitution
     */
    using SubstitutionMap = std::unordered_map<std::size_t, TermDBPtr>;

    /**
     * SubstitutionEngine handles term substitution with correct
     * handling of De Bruijn indices and variable capture avoidance.
     */
    class SubstitutionEngine
    {
    public:
        /**
         * Apply a substitution to a term
         *
         * @param term The term to perform substitution on
         * @param subst Mapping from variable indices to replacement terms
         * @param depth Current binding depth (for recursive calls)
         * @return The result of substitution
         */
        static TermDBPtr substitute(const TermDBPtr &term,
                                    const SubstitutionMap &subst,
                                    std::size_t depth = 0);

        /**
         * Shift the free variables in a term by a specified amount
         *
         * This is used during substitution to adjust De Bruijn indices
         * when a term is moved into a different binding context.
         *
         * @param term The term to shift
         * @param amount The amount to shift by
         * @param cutoff Variables with indices below cutoff are not shifted
         * @return The shifted term
         */
        static TermDBPtr shift(const TermDBPtr &term,
                               int amount,
                               std::size_t cutoff = 0);
    };

} // namespace theorem_prover