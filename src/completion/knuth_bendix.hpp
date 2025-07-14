#pragma once

#include "../term/term_db.hpp"
#include "../term/rewriting.hpp"
#include "../term/ordering.hpp"
#include "critical_pairs.hpp"
#include <memory>
#include <vector>
#include <queue>
#include <unordered_set>
#include <chrono>
#include <optional>

namespace theorem_prover
{

    /**
     * @brief Configuration parameters for Knuth-Bendix completion
     */
    struct KBConfig
    {
        std::size_t max_iterations = 10000; // Maximum completion iterations
        std::size_t max_rules = 1000;       // Maximum number of rules
        std::size_t max_equations = 5000;   // Maximum number of equations in queue
        double max_time_seconds = 300.0;    // Maximum time limit (5 minutes)
        bool enable_simplification = true;  // Enable rule simplification
        bool enable_subsumption = true;     // Enable equation subsumption
        bool fair_processing = true;        // Process equations in fair order
        bool verbose = false;               // Enable verbose output

        KBConfig() = default;
    };

    /**
     * @brief Result of Knuth-Bendix completion attempt
     */
    struct KBResult
    {
        enum class Status
        {
            SUCCESS,        // Completion succeeded (confluent system)
            FAILURE,        // Completion failed (orientation failure)
            TIMEOUT,        // Time limit exceeded
            RESOURCE_LIMIT, // Memory/rule limit exceeded
            UNKNOWN         // Unknown status
        };

        Status status = Status::UNKNOWN;
        std::string message;
        std::vector<TermRewriteRule> final_rules;
        std::size_t iterations = 0;
        std::size_t total_equations_processed = 0;
        std::size_t total_critical_pairs_computed = 0;
        double elapsed_time_seconds = 0.0;

        KBResult() = default;
        KBResult(Status s, const std::string &msg) : status(s), message(msg) {}

        static KBResult make_success(const std::vector<TermRewriteRule> &rules, const std::string &msg = "")
        {
            KBResult result(Status::SUCCESS, msg);
            result.final_rules = rules;
            return result;
        }

        static KBResult make_failure(const std::string &msg)
        {
            return KBResult(Status::FAILURE, msg);
        }

        static KBResult make_timeout(const std::string &msg)
        {
            return KBResult(Status::TIMEOUT, msg);
        }

        static KBResult make_resource_limit(const std::string &msg)
        {
            return KBResult(Status::RESOURCE_LIMIT, msg);
        }
    };

    /**
     * @brief Equation queue for fair processing during completion
     *
     * Maintains equations in order of generation to ensure fairness.
     * Prevents starvation where some equations never get processed.
     */
    class EquationQueue
    {
    public:
        EquationQueue(bool fair_mode = true) : fair_mode_(fair_mode) {}

        /**
         * @brief Add equation to the queue
         * @param equation Equation to add
         * @param priority Priority level (lower = higher priority)
         */
        void push(const Equation &equation, std::size_t priority = 0);

        /**
         * @brief Get next equation from queue
         * @return Next equation to process, or nullopt if empty
         */
        std::optional<Equation> pop();

        /**
         * @brief Check if queue is empty
         */
        bool empty() const;

        /**
         * @brief Get current queue size
         */
        std::size_t size() const;

        /**
         * @brief Clear all equations from queue
         */
        void clear();

    private:
        bool fair_mode_;
        std::queue<Equation> equation_queue_;

        // Custom comparator for priority queue
        struct PriorityComparator
        {
            bool operator()(const std::pair<std::size_t, Equation> &a,
                            const std::pair<std::size_t, Equation> &b) const
            {
                return a.first > b.first; // Greater priority value = lower priority
            }
        };

        std::priority_queue<std::pair<std::size_t, Equation>,
                            std::vector<std::pair<std::size_t, Equation>>,
                            PriorityComparator>
            priority_queue_;
    };

    /**
     * @brief Statistics tracker for completion process
     */
    struct KBStats
    {
        std::size_t equations_processed = 0;
        std::size_t critical_pairs_computed = 0;
        std::size_t rules_added = 0;
        std::size_t rules_removed = 0;
        std::size_t equations_simplified = 0;
        std::size_t equations_subsumed = 0;
        std::size_t orientation_failures = 0;

        void reset()
        {
            equations_processed = 0;
            critical_pairs_computed = 0;
            rules_added = 0;
            rules_removed = 0;
            equations_simplified = 0;
            equations_subsumed = 0;
            orientation_failures = 0;
        }

        std::string to_string() const;
    };

    /**
     * @brief Main Knuth-Bendix completion algorithm implementation
     *
     * Implements the classic KB completion procedure:
     * 1. Initialize with input equations
     * 2. Orient equations into rules using term ordering
     * 3. Compute critical pairs between rules
     * 4. Simplify new equations using existing rules
     * 5. Add new rules and repeat until convergence
     *
     * Features:
     * - Configurable resource limits and timeouts
     * - Fair equation processing to prevent starvation
     * - Rule simplification and subsumption
     * - Detailed statistics and progress tracking
     */
    class KnuthBendixCompletion
    {
    public:
        /**
         * @brief Construct KB completion with ordering and configuration
         * @param ordering Term ordering for rule orientation
         * @param config Configuration parameters
         */
        KnuthBendixCompletion(std::shared_ptr<TermOrdering> ordering,
                              const KBConfig &config = KBConfig());

        /**
         * @brief Run completion on a set of input equations
         * @param equations Initial set of equations to complete
         * @return Result of completion attempt
         */
        KBResult complete(const std::vector<Equation> &equations);

        /**
         * @brief Run completion starting from existing rules
         * @param rules Initial set of rules
         * @param equations Additional equations to process
         * @return Result of completion attempt
         */
        KBResult complete_from_rules(const std::vector<TermRewriteRule> &rules,
                                     const std::vector<Equation> &equations = {});

        /**
         * @brief Get current rewrite system
         * @return Current set of rules
         */
        const std::vector<TermRewriteRule> &current_rules() const { return rules_; }

        /**
         * @brief Get current statistics
         * @return Statistics about completion process
         */
        const KBStats &statistics() const { return stats_; }

        /**
         * @brief Check if completion is currently running
         * @return true if completion is in progress
         */
        bool is_running() const { return running_; }

        /**
         * @brief Request termination of current completion
         */
        void request_termination() { termination_requested_ = true; }

    private:
        std::shared_ptr<TermOrdering> ordering_;
        KBConfig config_;
        std::vector<TermRewriteRule> rules_;
        EquationQueue equation_queue_;
        KBStats stats_;
        bool running_ = false;
        bool termination_requested_ = false;
        std::chrono::steady_clock::time_point start_time_;

        // Core completion algorithm steps

        /**
         * @brief Main completion loop
         * @return Result of completion
         */
        KBResult completion_loop();

        /**
         * @brief Process next equation from queue
         * @param equation Equation to process
         * @return true if processing should continue
         */
        bool process_equation(const Equation &equation);

        /**
         * @brief Orient an equation into a rewrite rule
         * @param equation Equation to orient
         * @return Oriented rule, or nullopt if orientation fails
         */
        std::optional<TermRewriteRule> orient_equation(const Equation &equation);

        /**
         * @brief Add a new rule to the system
         * @param rule Rule to add
         * @return true if rule was successfully added
         */
        bool add_rule(const TermRewriteRule &rule);

        /**
         * @brief Remove a rule from the system
         * @param rule_name Name of rule to remove
         * @return true if rule was found and removed
         */
        bool remove_rule(const std::string &rule_name);

        /**
         * @brief Simplify an equation using current rules
         * @param equation Equation to simplify
         * @return Simplified equation, or nullopt if equation reduces to identity
         */
        std::optional<Equation> simplify_equation(const Equation &equation);

        /**
         * @brief Simplify existing rules using a new rule
         * @param new_rule New rule to use for simplification
         * @return Vector of rules that were modified or removed
         */
        std::vector<std::string> simplify_rules_with(const TermRewriteRule &new_rule);

        /**
         * @brief Compute critical pairs between a new rule and existing rules
         * @param new_rule New rule to compute critical pairs for
         * @return Vector of critical pairs as equations
         */
        std::vector<Equation> compute_new_critical_pairs(const TermRewriteRule &new_rule);

        /**
         * @brief Check if an equation is subsumed by existing rules
         * @param equation Equation to check
         * @return true if equation is subsumed (can be ignored)
         */
        bool is_subsumed(const Equation &equation);

        /**
         * @brief Check resource limits and time constraints
         * @return true if limits are exceeded
         */
        bool check_resource_limits();

        /**
         * @brief Check if completion has converged
         * @return true if no more work needs to be done
         */
        bool has_converged() const;

        /**
         * @brief Generate unique rule name
         * @return Fresh rule name
         */
        std::string generate_rule_name();

        /**
         * @brief Generate unique equation name
         * @return Fresh equation name
         */
        std::string generate_equation_name();

        /**
         * @brief Print progress information (if verbose mode enabled)
         */
        void print_progress();

        /**
         * @brief Validate internal consistency of rule system
         * @return true if system is consistent
         */
        bool validate_consistency();

        // Utility counters
        std::size_t rule_counter_ = 0;
        std::size_t equation_counter_ = 0;
    };

    /**
     * @brief Convenience function for simple completion
     * @param equations Input equations
     * @param ordering Term ordering to use
     * @param config Optional configuration
     * @return Completion result
     */
    KBResult knuth_bendix_complete(const std::vector<Equation> &equations,
                                   std::shared_ptr<TermOrdering> ordering,
                                   const KBConfig &config = KBConfig());

    /**
     * @brief Create KB completion with default LPO ordering
     * @param config Configuration parameters
     * @return KB completion instance
     */
    std::unique_ptr<KnuthBendixCompletion> make_kb_completion(const KBConfig &config = KBConfig());

} // namespace theorem_prover