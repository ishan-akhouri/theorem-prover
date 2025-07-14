#pragma once

#include "../term/term_db.hpp"
#include "../term/rewriting.hpp"
#include "../term/unification.hpp"
#include "../term/substitution.hpp"
#include <memory>
#include <vector>
#include <optional>

namespace theorem_prover
{

    /**
     * @brief Represents a critical pair between two rewrite rules
     *
     * A critical pair (s,t) arises when:
     * - We have rules l₁ → r₁ and l₂ → r₂
     * - l₁ unifies with a non-variable subterm of l₂ at some position
     * - s = l₂[r₁]_p and t = r₂ under the unifying substitution
     */
    struct CriticalPair
    {
        TermDBPtr left;          // First term of the pair
        TermDBPtr right;         // Second term of the pair
        std::string rule1_name;  // Name of first rule involved
        std::string rule2_name;  // Name of second rule involved
        Position position;       // Position where overlap occurred
        SubstitutionMap unifier; // Unifying substitution

        CriticalPair(const TermDBPtr &l, const TermDBPtr &r,
                     const std::string &r1, const std::string &r2,
                     const Position &pos, const SubstitutionMap &unif)
            : left(l), right(r), rule1_name(r1), rule2_name(r2),
              position(pos), unifier(unif) {}

        /**
         * @brief Convert to equation for completion processing
         */
        Equation to_equation() const;

        /**
         * @brief String representation for debugging
         */
        std::string to_string() const;
    };

    /**
     * @brief Critical pair computation engine
     *
     * Implements the systematic computation of critical pairs between
     * rewrite rules, which are essential for Knuth-Bendix completion.
     */
    class CriticalPairComputer
    {
    public:
        /**
         * @brief Compute all critical pairs between two rules
         *
         * @param rule1 First rewrite rule
         * @param rule2 Second rewrite rule
         * @return Vector of critical pairs found
         */
        static std::vector<CriticalPair> compute_critical_pairs(
            const TermRewriteRule &rule1,
            const TermRewriteRule &rule2);

        /**
         * @brief Compute critical pairs for a rule with itself
         *
         * @param rule The rewrite rule
         * @return Vector of critical pairs (self-overlaps)
         */
        static std::vector<CriticalPair> compute_self_critical_pairs(
            const TermRewriteRule &rule);

        /**
         * @brief Compute all critical pairs for a set of rules
         *
         * @param rules Vector of rewrite rules
         * @return Vector of all critical pairs between rules
         */
        static std::vector<CriticalPair> compute_all_critical_pairs(
            const std::vector<TermRewriteRule> &rules);

    private:
        /**
         * @brief Find all positions where rule1's lhs could overlap with rule2's lhs
         *
         * @param rule1 First rule (will be unified at found positions)
         * @param rule2 Second rule (contains the positions)
         * @return Vector of positions and corresponding unifiers
         */
        static std::vector<std::pair<Position, SubstitutionMap>>
        find_overlap_positions(const TermRewriteRule &rule1,
                               const TermRewriteRule &rule2);

        /**
         * @brief Check if two terms can unify at a given position
         *
         * @param term1 First term (will be unified whole)
         * @param term2 Second term (subterm at position will be unified)
         * @param position Position in term2 to extract subterm
         * @return Unification result if successful
         */
        static std::optional<SubstitutionMap>
        try_unify_at_position(const TermDBPtr &term1,
                              const TermDBPtr &term2,
                              const Position &position);

        /**
         * @brief Rename variables in a rule to avoid conflicts
         *
         * @param rule Rule to rename
         * @param offset Offset to add to variable indices
         * @return Rule with renamed variables
         */
        static TermRewriteRule rename_rule_variables(const TermRewriteRule &rule,
                                                     std::size_t offset);

        /**
         * @brief Find all non-variable positions in a term
         *
         * @param term Term to analyze
         * @return Vector of positions where non-variable subterms occur
         */
        static std::vector<Position> find_non_variable_positions(const TermDBPtr &term);
    };

} // namespace theorem_prover