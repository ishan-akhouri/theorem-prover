#pragma once

#include "../rule/proof_rule.hpp"
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace theorem_prover
{

    /**
     * @brief A tactic is a function that transforms proof states
     *
     * Unlike rules which perform single logical steps, tactics represent
     * higher-level strategies that may involve multiple rule applications.
     * Tactics can be composed to create complex proof search strategies.
     */
    using Tactic = std::function<std::vector<ProofStatePtr>(
        ProofContext &,
        const ProofStatePtr &,
        std::optional<ConstraintViolation> &)>;

    // Core tactic constructors

    /**
     * @brief Creates a tactic from a proof rule
     *
     * @param rule The rule to convert into a tactic
     * @param app_context Optional application context for the rule
     * @return Tactic A tactic that applies the given rule
     */
    Tactic from_rule(const ProofRule &rule,
                     const std::optional<RuleApplicationContext> &app_context = std::nullopt);

    /**
     * @brief Creates a tactic from a proof rule pointer
     *
     * @param rule_ptr Shared pointer to the rule
     * @param app_context Optional application context for the rule
     * @return Tactic A tactic that applies the given rule
     */
    Tactic from_rule_ptr(const ProofRulePtr &rule_ptr,
                         const std::optional<RuleApplicationContext> &app_context = std::nullopt);

    /**
     * @brief A tactic that always fails
     *
     * @return Tactic A tactic that produces no successor states
     */
    Tactic fail();

    // Core tactic combinators

    /**
     * @brief Applies a tactic repeatedly until it fails or no new states are produced
     *
     * @param tactic The tactic to apply repeatedly
     * @return Tactic A tactic that applies the given tactic until fixpoint
     */
    Tactic repeat(const Tactic &tactic);

    /**
     * @brief Applies a tactic at most n times
     *
     * @param tactic The tactic to apply
     * @param max_iterations Maximum number of iterations
     * @return Tactic A tactic that applies the given tactic at most n times
     */
    Tactic repeat_n(const Tactic &tactic, size_t max_iterations);

    /**
     * @brief Tries each tactic in sequence until one succeeds
     *
     * @param tactics Vector of tactics to try
     * @return Tactic A tactic that tries each given tactic in sequence
     */
    Tactic first(const std::vector<Tactic> &tactics);

    /**
     * @brief Applies the second tactic to each state produced by the first tactic
     *
     * @param first_tactic The first tactic to apply
     * @param second_tactic The second tactic to apply to results of the first
     * @return Tactic A tactic that composes the two tactics sequentially
     */
    Tactic then(const Tactic &first_tactic, const Tactic &second_tactic);

    /**
     * @brief Tries the first tactic, and if it fails, tries the second
     *
     * @param first_tactic The first tactic to try
     * @param second_tactic The fallback tactic
     * @return Tactic A tactic that tries the first tactic or falls back to the second
     */
    Tactic orelse(const Tactic &first_tactic, const Tactic &second_tactic);

    /**
     * @brief Tries to apply a tactic, but continues with the original state if it fails
     *
     * @param tactic The tactic to try
     * @return Tactic A tactic that tries to apply the given tactic but doesn't fail
     */
    Tactic try_tactic(const Tactic &tactic);

    /**
     * @brief Conditionally applies a tactic based on a predicate
     *
     * @param predicate Function that determines if the tactic should be applied
     * @param tactic The tactic to apply if the predicate is true
     * @return Tactic A tactic that conditionally applies the given tactic
     */
    Tactic tactic_if(std::function<bool(const ProofStatePtr &)> predicate, const Tactic &tactic);

    /**
     * @brief Applies tactics in parallel and returns the combined results
     *
     * @param tactics Vector of tactics to apply in parallel
     * @return Tactic A tactic that applies all tactics in parallel
     */
    Tactic parallel(const std::vector<Tactic> &tactics);

    /**
     * @brief Logs tactic application for debugging
     *
     * @param name Name to identify this tactic in logs
     * @param tactic The tactic to apply and log
     * @return Tactic A tactic that logs its application
     */
    Tactic tactic_log(const std::string &name, const Tactic &tactic);

    /**
     * @brief Applies a tactic only if the goal matches a specific pattern
     *
     * @param matcher Function that determines if the goal matches
     * @param tactic The tactic to apply if the goal matches
     * @return Tactic A tactic that selectively applies based on goal shape
     */
    Tactic goal_matcher(std::function<bool(const TermDBPtr &)> matcher, const Tactic &tactic);

    /**
     * @brief Applies a tactic to all hypotheses matching a predicate
     *
     * @param pred Predicate function that selects hypotheses
     * @param tactic_factory Function that creates a tactic for a specific hypothesis
     * @return Tactic A tactic that applies the factory-created tactics to matching hypotheses
     */
    Tactic for_each_hypothesis(
        std::function<bool(const Hypothesis &)> pred,
        std::function<Tactic(const std::string &)> tactic_factory);

    /**
     * @brief Container for organizing and accessing named tactics
     */
    class TacticLibrary
    {
    public:
        TacticLibrary();

        /**
         * @brief Register a tactic with a name
         *
         * @param name Name to register the tactic under
         * @param tactic The tactic to register
         */
        void register_tactic(const std::string &name, const Tactic &tactic);

        /**
         * @brief Get a registered tactic by name
         *
         * @param name Name of the tactic to retrieve
         * @return std::optional<Tactic> The tactic if found, nullopt otherwise
         */
        std::optional<Tactic> get_tactic(const std::string &name) const;

        /**
         * @brief Check if a tactic with the given name exists
         *
         * @param name Name to check
         * @return true if a tactic with the name exists
         */
        bool has_tactic(const std::string &name) const;

        /**
         * @brief Get names of all registered tactics
         *
         * @return std::vector<std::string> Names of all registered tactics
         */
        std::vector<std::string> get_tactic_names() const;

    private:
        std::unordered_map<std::string, Tactic> tactics_;
    };

    // Common pre-built tactics

    /**
     * @brief Simplifies propositional formulas
     *
     * Applies propositional simplification rules like
     * and-elimination, or-introduction, etc.
     *
     * @return Tactic A tactic for propositional simplification
     */
    Tactic propositional_simplify();

    /**
     * @brief Attempts to solve the goal using basic logical rules
     *
     * @return Tactic A tactic for automatic solving of simple goals
     */
    Tactic auto_solve();

    /**
     * @brief Applies standard introduction rules based on the goal's structure
     *
     * For example, if the goal is P→Q, applies implication introduction.
     *
     * @return Tactic A tactic for introducing logical connectives
     */
    Tactic intro();

    /**
     * @brief Applies standard elimination rules to matching hypotheses
     *
     * For example, applies and-elimination to conjunctions in hypotheses.
     *
     * @return Tactic A tactic for eliminating logical connectives
     */
    Tactic elim();

    /**
     * @brief Instantiates universal quantifiers with specific terms
     *
     * @param forall_hyp_name Name of the universal quantifier hypothesis
     * @param term Term to instantiate with
     * @return Tactic A tactic to instantiate a universal quantifier
     */
    Tactic instantiate(const std::string &forall_hyp_name, const TermDBPtr &term);

    /**
     * @brief Applies rewrite rules to simplify terms using equalities
     *
     * @return Tactic A tactic for term rewriting
     */
    Tactic rewrite();

    /**
     * @brief Creates a tactic to introduce a new hypothesis
     *
     * @param formula Formula to assume
     * @param name Optional name for the hypothesis
     * @return Tactic A tactic to introduce a new hypothesis
     */
    Tactic assume(const TermDBPtr &formula, const std::string &name = "");

    /**
     * @brief Tries to find a contradiction in the current hypotheses
     *
     * @return Tactic A tactic to detect and use contradictions
     */
    Tactic contradiction();

    Tactic form_useful_conjunctions();

    Tactic match_mp_antecedent();

    Tactic handle_conjunctive_goal();

    Tactic smart_repeat(const Tactic &tactic, size_t max_iterations = 10);

    Tactic direct_proof();

    Tactic sequence(const std::vector<Tactic> &tactics);

} // namespace theorem_prover