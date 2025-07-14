#pragma once

#include "term_db.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>

namespace theorem_prover
{

    /**
     * @brief Abstract base class for term orderings
     *
     * A term ordering must be:
     * - Well-founded (no infinite decreasing chains)
     * - Total on ground terms
     * - Stable under substitution
     * - Compatible with the term structure (subterm property)
     */
    class TermOrdering
    {
    public:
        virtual ~TermOrdering() = default;

        /**
         * @brief Compare two terms: s > t
         * @param s First term
         * @param t Second term
         * @return true if s is strictly greater than t
         */
        virtual bool greater(const TermDBPtr &s, const TermDBPtr &t) const = 0;

        /**
         * @brief Compare two terms: s ≥ t
         * @param s First term
         * @param t Second term
         * @return true if s is greater than or equal to t
         */
        virtual bool greater_equal(const TermDBPtr &s, const TermDBPtr &t) const;

        /**
         * @brief Check if two terms are equivalent in the ordering
         * @param s First term
         * @param t Second term
         * @return true if s ~ t (neither s > t nor t > s)
         */
        virtual bool equivalent(const TermDBPtr &s, const TermDBPtr &t) const;

        /**
         * @brief Compare two terms: s < t
         * @param s First term
         * @param t Second term
         * @return true if s is strictly less than t
         */
        bool less(const TermDBPtr &s, const TermDBPtr &t) const
        {
            return greater(t, s);
        }

        /**
         * @brief Compare two terms: s ≤ t
         * @param s First term
         * @param t Second term
         * @return true if s is less than or equal to t
         */
        bool less_equal(const TermDBPtr &s, const TermDBPtr &t) const
        {
            return greater_equal(t, s);
        }
    };

    /**
     * @brief Precedence relation on function symbols
     *
     * Defines a strict partial order on function symbols used by LPO.
     * Must be extended to a total order for completeness.
     */
    class Precedence
    {
    public:
        Precedence() = default;

        /**
         * @brief Set precedence: f > g
         * @param f Higher precedence symbol
         * @param g Lower precedence symbol
         */
        void set_greater(const std::string &f, const std::string &g);

        /**
         * @brief Check if f > g in precedence
         * @param f First symbol
         * @param g Second symbol
         * @return true if f has higher precedence than g
         */
        bool greater(const std::string &f, const std::string &g) const;

        /**
         * @brief Check if symbols have equal precedence
         * @param f First symbol
         * @param g Second symbol
         * @return true if f and g have equal precedence
         */
        bool equal(const std::string &f, const std::string &g) const;

        /**
         * @brief Get all symbols in the precedence relation
         * @return Set of all symbols with defined precedence
         */
        std::unordered_set<std::string> get_symbols() const;

        /**
         * @brief Extend to total order using lexicographic comparison
         *
         * For symbols not explicitly ordered, use lexicographic string comparison.
         * This ensures totality while preserving the explicitly set precedences.
         */
        bool total_greater(const std::string &f, const std::string &g) const;

    private:
        // Adjacency list representation of precedence DAG
        std::unordered_map<std::string, std::unordered_set<std::string>> precedence_graph_;

        // Simple string-based cache for efficiency (f + "|" + g -> bool)
        mutable std::unordered_map<std::string, bool> cache_;

        // Compute transitive closure
        bool compute_transitive_greater(const std::string &f, const std::string &g) const;

        // Helper to create cache key
        std::string make_cache_key(const std::string &f, const std::string &g) const
        {
            return f + "|" + g;
        }
    };

    /**
     * @brief Status function for function symbols in LPO
     *
     * Determines whether argument comparison is lexicographic or multiset-based.
     * All symbols default to lexicographic status.
     */
    enum class ArgumentStatus
    {
        LEXICOGRAPHIC, // Compare arguments left-to-right lexicographically
        MULTISET       // Compare arguments as multisets (commutative symbols)
    };

    /**
     * @brief Lexicographic Path Ordering (LPO)
     *
     * Standard implementation of LPO following:
     * s >_lpo t iff:
     * 1. s = f(s1,...,sn), t ∈ {s1,...,sn}, and s ≥_lpo t, or
     * 2. s = f(s1,...,sn), t = g(t1,...,tm), and either:
     *    a) f >_prec g and s >_lpo ti for all i, or
     *    b) f =_prec g, s >_lpo ti for all i, and (s1,...,sn) >_lex (t1,...,tm)
     *
     * Variables are handled as the smallest elements in the ordering.
     */
    class LexicographicPathOrdering : public TermOrdering
    {
    public:
        /**
         * @brief Construct LPO with given precedence
         * @param precedence Precedence relation on function symbols
         */
        explicit LexicographicPathOrdering(std::shared_ptr<Precedence> precedence);

        /**
         * @brief Construct LPO with default precedence (lexicographic on symbol names)
         */
        LexicographicPathOrdering();

        bool greater(const TermDBPtr &s, const TermDBPtr &t) const override;

        /**
         * @brief Set argument status for a function symbol
         * @param symbol Function symbol
         * @param status Lexicographic or multiset status
         */
        void set_argument_status(const std::string &symbol, ArgumentStatus status);

        /**
         * @brief Get the precedence relation
         * @return Shared pointer to the precedence
         */
        std::shared_ptr<Precedence> get_precedence() const { return precedence_; }

    private:
        std::shared_ptr<Precedence> precedence_;
        std::unordered_map<std::string, ArgumentStatus> argument_status_;

        // Core LPO comparison methods
        bool lpo_greater(const TermDBPtr &s, const TermDBPtr &t) const;
        bool lpo_greater_equal(const TermDBPtr &s, const TermDBPtr &t) const;

        // Helper methods
        bool is_variable(const TermDBPtr &term) const;
        bool contains_variable(const TermDBPtr &term, const TermDBPtr &var) const;
        bool all_greater(const TermDBPtr &s, const std::vector<TermDBPtr> &terms) const;
        bool lexicographic_greater(const std::vector<TermDBPtr> &args1,
                                   const std::vector<TermDBPtr> &args2) const;
        bool multiset_greater(const std::vector<TermDBPtr> &args1,
                              const std::vector<TermDBPtr> &args2) const;

        // Extract function symbol and arguments
        std::pair<std::string, std::vector<TermDBPtr>>
        decompose_term(const TermDBPtr &term) const;
    };

    // Factory functions
    std::shared_ptr<LexicographicPathOrdering> make_lpo();
    std::shared_ptr<LexicographicPathOrdering> make_lpo(std::shared_ptr<Precedence> precedence);

} // namespace theorem_prover