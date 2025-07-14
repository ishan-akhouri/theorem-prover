#pragma once

#include "clause.hpp"
#include "cnf_converter.hpp"
#include "../term/term_db.hpp"
#include "../completion/knuth_bendix.hpp"
#include "indexing.hpp"
#include <vector>
#include <memory>
#include <unordered_set>
#include <queue>
#include <functional>

namespace theorem_prover
{

    /**
     * Result of a resolution proof attempt
     */
    struct ResolutionProofResult
    {
        enum class Status
        {
            PROVED,    // Empty clause derived - theorem is true
            DISPROVED, // Contradiction found - theorem is false
            TIMEOUT,   // Search exceeded time/iteration limits
            SATURATED, // No new clauses can be derived
            UNKNOWN    // Could not determine satisfiability
        };

        Status status;
        std::vector<ClausePtr> proof_clauses; // Clauses used in the proof
        std::vector<ClausePtr> final_clauses; // All clauses at termination
        std::string explanation;
        size_t iterations;
        double time_elapsed_ms;

        ResolutionProofResult(Status status, const std::string &explanation = "")
            : status(status), explanation(explanation), iterations(0), time_elapsed_ms(0.0) {}

        bool is_proved() const { return status == Status::PROVED; }
        bool is_disproved() const { return status == Status::DISPROVED; }
        bool is_timeout() const { return status == Status::TIMEOUT; }
        bool is_conclusive() const { return is_proved() || is_disproved(); }
    };

    /**
     * Configuration for resolution proof search
     */
    struct ResolutionConfig
    {
        size_t max_iterations = 10000;
        double max_time_ms = 30000.0; // 30 seconds
        size_t max_clauses = 100000;
        bool use_subsumption = true;
        bool use_tautology_deletion = true;
        bool use_factoring = true;
        bool use_paramodulation = false;
        // NEW: KB preprocessing options
        bool use_kb_preprocessing = false;
        double kb_preprocessing_timeout = 5.0; // Max time for KB attempt (seconds)
        size_t kb_max_rules = 50;              // Max rules to accept from KB
        size_t kb_max_equations = 20;          // Max equations to send to KB

        KBConfig kb_config; // Full KB configuration

        // Clause selection strategy
        enum class SelectionStrategy
        {
            FIFO,              // First in, first out
            SMALLEST_FIRST,    // Select smallest clauses first
            UNIT_PREFERENCE,   // Prefer unit clauses
            NEGATIVE_SELECTION // Prefer clauses with negative literals
        } selection_strategy = SelectionStrategy::UNIT_PREFERENCE;
    };

    /**
     * Clause selection and management for resolution
     */
    class ClauseSet
    {
    public:
        ClauseSet(const ResolutionConfig &config);

        // Add a clause to the set
        void add_clause(ClausePtr clause);

        // Check if set contains empty clause
        bool contains_empty_clause() const;

        // Get next clause for resolution based on selection strategy
        ClausePtr select_clause();

        // Get all clauses
        const std::vector<ClausePtr> &clauses() const { return clauses_; }

        // Check if no more clauses to process
        bool is_empty() const;

        // Get total number of clauses
        size_t size() const { return clauses_.size(); }

        // Clear all clauses
        void clear();

        std::vector<ClausePtr> get_resolution_candidates(const Literal &literal);

    private:
        std::vector<ClausePtr> clauses_;
        std::queue<ClausePtr> processing_queue_;
        std::unordered_set<size_t> clause_hashes_; // For duplicate detection
        ResolutionConfig config_;
        size_t next_clause_index_;
        LiteralIndex literal_index_;

        // Check if clause is subsumed by existing clauses
        bool is_subsumed(ClausePtr clause) const;

        // Remove clauses subsumed by the new clause
        void remove_subsumed_clauses(ClausePtr clause);

        // Check if two clauses are variants (same up to variable renaming)
        bool are_variants(ClausePtr clause1, ClausePtr clause2) const;
    };

    /**
     * Main resolution theorem prover
     */
    class ResolutionProver
    {
    public:
        ResolutionProver(const ResolutionConfig &config = ResolutionConfig{});

        /**
         * Prove a theorem using resolution
         *
         * @param goal The theorem to prove
         * @param hypotheses Additional hypotheses/axioms
         * @return ResolutionProofResult indicating success/failure and proof details
         */
        ResolutionProofResult prove(const TermDBPtr &goal,
                                    const std::vector<TermDBPtr> &hypotheses = {});

        /**
         * Check satisfiability of a set of formulas
         *
         * @param formulas The formulas to check
         * @return ResolutionProofResult indicating satisfiability
         */
        ResolutionProofResult check_satisfiability(const std::vector<TermDBPtr> &formulas);

        /**
         * Prove using a set of clauses directly
         *
         * @param clauses The clause set to prove from
         * @return ResolutionProofResult
         */
        ResolutionProofResult prove_from_clauses(const std::vector<ClausePtr> &clauses);

    private:
        ResolutionConfig config_;

        /**
         * Main resolution loop
         */
        ResolutionProofResult resolution_loop(ClauseSet &clause_set);

        /**
         * Apply resolution between two clauses and return all resolvents
         */
        std::vector<ClausePtr> resolve_clauses(ClausePtr clause1, ClausePtr clause2);

        /**
         * Apply factoring to a clause
         */
        ClausePtr factor_clause(ClausePtr clause);

        /**
         * Check if search should terminate due to limits
         */
        bool should_terminate(size_t iterations, double elapsed_ms, size_t clause_count) const;

        /**
         * Convert theorem proving problem to refutation problem
         * Hypotheses ∪ {¬Goal} should be unsatisfiable if Goal follows from Hypotheses
         */
        std::vector<TermDBPtr> setup_refutation_problem(const TermDBPtr &goal,
                                                        const std::vector<TermDBPtr> &hypotheses);
        // NEW: KB Integration Methods

        /**
         * @brief Try KB completion on equality clauses
         */
        KBResult try_kb_preprocessing(std::vector<ClausePtr> &clauses);

        /**
         * @brief Extract unit equality clauses for KB processing
         */
        std::vector<Equation> extract_equality_equations(const std::vector<ClausePtr> &clauses);

        /**
         * @brief Convert KB rules back to clauses and integrate
         */
        std::vector<ClausePtr> integrate_kb_rules(const std::vector<ClausePtr> &original_clauses,
                                                  const std::vector<TermRewriteRule> &kb_rules);

        /**
         * @brief Check if clause is unit equality: single literal with = symbol
         */
        bool is_unit_equality_clause(const ClausePtr &clause);

        /**
         * @brief Convert unit equality clause to equation
         */
        Equation clause_to_equation(const ClausePtr &clause);

        /**
         * @brief Convert rewrite rule to unit clause
         */
        ClausePtr rule_to_clause(const TermRewriteRule &rule);
        void debug_kb_integration(const std::vector<ClausePtr> &before_clauses,
                                  const std::vector<ClausePtr> &after_clauses,
                                  const KBResult &kb_result);
    };

    /**
     * Utility functions for resolution proving
     */
    namespace resolution_utils
    {

        /**
         * Print clause set for debugging
         */
        void print_clause_set(const std::vector<ClausePtr> &clauses, const std::string &title = "");

        /**
         * Check if a clause set is satisfiable (basic check)
         */
        bool is_obviously_satisfiable(const std::vector<ClausePtr> &clauses);

        /**
         * Check if a clause set is obviously unsatisfiable
         */
        bool is_obviously_unsatisfiable(const std::vector<ClausePtr> &clauses);

        /**
         * Generate statistics about a clause set
         */
        struct ClauseSetStats
        {
            size_t total_clauses;
            size_t unit_clauses;
            size_t horn_clauses;
            size_t max_clause_size;
            double avg_clause_size;
        };

        ClauseSetStats analyze_clause_set(const std::vector<ClausePtr> &clauses);
    }

} // namespace theorem_prover