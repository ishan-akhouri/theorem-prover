#pragma once

#include "../term/term_db.hpp"
#include "../term/unification.hpp"
#include "../term/substitution.hpp"
#include "../term/rewriting.hpp"
#include <vector>
#include <memory>
#include <string>
#include <unordered_set>

namespace theorem_prover
{

    class Clause;

    using ClausePtr = std::shared_ptr<Clause>;

    /**
     * Represents a literal in a clause (positive or negative)
     */
    class Literal
    {
    public:
        Literal(const TermDBPtr &atom, bool positive = true);

        const TermDBPtr &atom() const { return atom_; }
        bool is_positive() const { return positive_; }
        bool is_negative() const { return !positive_; }

        // Create negation of this literal
        Literal negate() const;

        // Check if two literals are complementary (same atom, opposite polarity)
        bool is_complementary(const Literal &other) const;

        // Equality and hashing for containers
        bool equals(const Literal &other) const;
        std::size_t hash() const;

        std::string to_string() const;

    private:
        TermDBPtr atom_; // The atomic formula
        bool positive_;  // True for positive literal, false for negative
    };

    using LiteralPtr = std::shared_ptr<Literal>;

    /**
     * Represents a clause (disjunction of literals)
     */
    class Clause
    {
    public:
        Clause(const std::vector<Literal> &literals = {});
        Clause(const std::vector<LiteralPtr> &literals);

        const std::vector<Literal> &literals() const { return literals_; }
        std::size_t size() const { return literals_.size(); }
        bool is_empty() const { return literals_.empty(); }
        bool is_unit() const { return literals_.size() == 1; }

        // Check if clause is a tautology (contains complementary literals)
        bool is_tautology() const;

        // Remove duplicate literals and tautologies
        Clause simplify() const;

        // Apply substitution to all literals in clause
        Clause substitute(const SubstitutionMap &subst) const;

        // Rename variables to avoid conflicts
        Clause rename_variables(std::size_t offset) const;

        // Equality and hashing
        bool equals(const Clause &other) const;
        std::size_t hash() const;

        std::string to_string() const;

        // Subsumption checking
        bool subsumes(const Clause &other) const;
        static bool subsumes(const ClausePtr &c1, const ClausePtr &c2);

    private:
        std::vector<Literal> literals_;
        mutable std::size_t hash_cache_;
        mutable bool hash_computed_;

        void compute_hash() const;

        // Subsumption helpers
        static bool try_all_literal_mappings(const ClausePtr &c1, const ClausePtr &c2);
        static bool find_consistent_mapping(const ClausePtr &c1, const ClausePtr &c2,
                                            int lit_idx, std::vector<int> &mapping,
                                            std::vector<bool> &used);
        static bool check_substitution_consistency(const ClausePtr &c1, const ClausePtr &c2,
                                                   const std::vector<int> &mapping);
        static bool can_unify_literals(const Literal &lit1, const Literal &lit2);
    };

    /**
     * Result of a resolution step
     */
    struct ResolutionResult
    {
        bool success;
        ClausePtr resolvent;
        std::string reason;

        ResolutionResult(bool success = false, ClausePtr resolvent = nullptr,
                         const std::string &reason = "")
            : success(success), resolvent(resolvent), reason(reason) {}

        static ResolutionResult make_failure(const std::string &reason)
        {
            return ResolutionResult(false, nullptr, reason);
        }

        static ResolutionResult make_success(ClausePtr resolvent)
        {
            return ResolutionResult(true, resolvent);
        }
    };

    /**
     * Core resolution inference rule
     */
    class ResolutionInference
    {
    public:
        /**
         * Apply resolution between two clauses
         *
         * @param clause1 First clause
         * @param clause2 Second clause
         * @return ResolutionResult containing the resolvent or failure reason
         */
        static ResolutionResult resolve(const ClausePtr &clause1,
                                        const ClausePtr &clause2);

        /**
         * Try to resolve on a specific literal pair
         *
         * @param clause1 First clause
         * @param clause2 Second clause
         * @param lit1_idx Index of literal in first clause
         * @param lit2_idx Index of literal in second clause
         * @return ResolutionResult
         */
        static ResolutionResult resolve_on_literals(const ClausePtr &clause1,
                                                    const ClausePtr &clause2,
                                                    std::size_t lit1_idx,
                                                    std::size_t lit2_idx);

        /**
         * Apply factoring to a clause (remove duplicate literals after unification)
         *
         * @param clause The clause to factor
         * @return Factored clause
         */
        static ClausePtr factor(const ClausePtr &clause);

    private:
        /**
         * Find the next available variable index for renaming
         */
        static std::size_t find_max_variable_index(const ClausePtr &clause1,
                                                   const ClausePtr &clause2);
    };

    /**
     * Paramodulation inference for equality reasoning
     */
    class ParamodulationInference
    {
    public:
        /**
         * Apply paramodulation between an equality clause and a target clause
         * @param equality_clause Clause containing an equality literal
         * @param target_clause Clause to rewrite using the equality
         * @param eq_lit_idx Index of equality literal in equality_clause
         * @param target_lit_idx Index of target literal in target_clause
         * @param position Position in target literal to rewrite
         * @return Paramodulation result
         */
        static ResolutionResult paramodulate(const ClausePtr &equality_clause,
                                             const ClausePtr &target_clause,
                                             std::size_t eq_lit_idx,
                                             std::size_t target_lit_idx,
                                             const Position &position);

        /**
         * Find all possible paramodulation positions in a clause
         */
        static std::vector<std::tuple<std::size_t, Position, TermDBPtr>>
        find_paramod_positions(const ClausePtr &clause);

    private:
        /**
         * Apply equality substitution at a specific position in a term
         */
        static TermDBPtr apply_equality_at_position(const TermDBPtr &term,
                                                    const Position &position,
                                                    const TermDBPtr &from_term,
                                                    const TermDBPtr &to_term,
                                                    const SubstitutionMap &substitution);
    };

    /**
     * Enhanced resolution system with paramodulation for equality reasoning
     */
    class ResolutionWithParamodulation
    {
    public:
        /**
         * Perform resolution with paramodulation between two clauses
         * @param clause1 First clause
         * @param clause2 Second clause
         * @return Vector of all possible resolvents (resolution + paramodulation)
         */
        static std::vector<ClausePtr> resolve_with_paramodulation(const ClausePtr &clause1,
                                                                  const ClausePtr &clause2);

        /**
         * Try standard resolution between two clauses
         */
        static std::vector<ClausePtr> try_resolution(const ClausePtr &clause1,
                                                     const ClausePtr &clause2);

        /**
         * Try paramodulation in both directions between two clauses
         */
        static std::vector<ClausePtr> try_paramodulation(const ClausePtr &clause1,
                                                         const ClausePtr &clause2);

        /**
         * Check if a clause contains equality literals
         */
        static bool has_equality_literals(const ClausePtr &clause);

        /**
         * Get indices of all equality literals in a clause
         */
        static std::vector<std::size_t> get_equality_literal_indices(const ClausePtr &clause);
    };
} // namespace theorem_prover