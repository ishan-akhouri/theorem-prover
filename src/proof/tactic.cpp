#include "tactic.hpp"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <set>

namespace theorem_prover
{

    // Core tactic constructors

    Tactic from_rule(const ProofRule &rule,
                     const std::optional<RuleApplicationContext> &app_context)
    {
        return [&rule, app_context](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            if (!rule.is_applicable(state, app_context))
            {
                return {};
            }

            return rule.apply(context, state, violation, app_context);
        };
    }

    Tactic from_rule_ptr(const ProofRulePtr &rule_ptr,
                         const std::optional<RuleApplicationContext> &app_context)
    {
        return [rule_ptr, app_context](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            if (!rule_ptr->is_applicable(state, app_context))
            {
                return {};
            }

            return rule_ptr->apply(context, state, violation, app_context);
        };
    }

    Tactic fail()
    {
        return [](
                   ProofContext &,
                   const ProofStatePtr &,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_RULE_APPLICATION,
                "Explicit failure tactic");
            return {};
        };
    }

    // Core tactic combinators
    Tactic repeat(const Tactic &tactic)
    {
        return [tactic](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            // Print all hypotheses
            for (const auto &hyp : state->hypotheses())
            {
                std::cout << "  - " << hyp.name() << " (Kind: " << static_cast<int>(hyp.formula()->kind()) << ")" << std::endl;
            }

            std::vector<ProofStatePtr> results = {state};
            std::vector<ProofStatePtr> new_results;
            std::set<std::size_t> seen_states;

            // Track the state hashes we've seen to avoid cycles
            seen_states.insert(state->hash());

            int iteration = 0;
            bool made_progress = true;

            while (made_progress)
            {
                made_progress = false;
                new_results.clear();

                // Apply the tactic to each result
                int result_index = 0;
                for (const auto &result_state : results)
                {
                    // Print current hypotheses for this state
                    for (const auto &hyp : result_state->hypotheses())
                    {
                        std::cout << "  - " << hyp.name() << " (Kind: " << static_cast<int>(hyp.formula()->kind()) << ")" << std::endl;
                    }

                    std::optional<ConstraintViolation> local_violation;
                    auto next_states = tactic(context, result_state, local_violation);

                    if (next_states.empty())
                    {
                        // Keep original state if no next state
                        new_results.push_back(result_state);
                        continue;
                    }

                    bool added_new_state = false;
                    int next_state_index = 0;

                    for (const auto &next_state : next_states)
                    {
                        ++next_state_index;
                        // Print hypotheses in the next state
                        for (const auto &hyp : next_state->hypotheses())
                        {
                            std::cout << "  - " << hyp.name() << " (Kind: " << static_cast<int>(hyp.formula()->kind()) << ")" << std::endl;
                        }

                        if (seen_states.find(next_state->hash()) == seen_states.end())
                        {
                            // Add new unseen states
                            new_results.push_back(next_state);
                            seen_states.insert(next_state->hash());
                            added_new_state = true;
                        }
                    }

                    if (!added_new_state)
                    {
                        new_results.push_back(result_state);
                    }
                    else
                    {
                        made_progress = true;
                    }
                }
                if (made_progress)
                {
                    results = new_results;
                }
            }

            // Verify if we have P in any of the results
            for (const auto &result : results)
            {
                bool found_p = false;
                for (const auto &hyp : result->hypotheses())
                {
                    if (hyp.formula()->kind() == TermDB::TermKind::CONSTANT)
                    {
                        auto constant = std::dynamic_pointer_cast<ConstantDB>(hyp.formula());
                        if (constant && constant->symbol() == "P")
                        {
                            found_p = true;
                            break;
                        }
                    }
                }
            }

            return results;
        };
    }

    Tactic repeat_n(const Tactic &tactic, size_t max_iterations)
    {
        return [tactic, max_iterations](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            std::vector<ProofStatePtr> results = {state};
            std::vector<ProofStatePtr> new_results;
            std::set<std::size_t> seen_states;

            // Track the state hashes we've seen to avoid cycles
            seen_states.insert(state->hash());

            size_t iterations = 0;
            bool made_progress = true;

            while (made_progress && iterations < max_iterations)
            {
                made_progress = false;
                new_results.clear();
                iterations++;

                // Apply the tactic to each result
                for (const auto &result_state : results)
                {
                    std::optional<ConstraintViolation> local_violation;
                    auto next_states = tactic(context, result_state, local_violation);

                    if (!next_states.empty())
                    {
                        made_progress = true;

                        // Add new states, avoiding duplicates
                        for (const auto &next_state : next_states)
                        {
                            if (seen_states.find(next_state->hash()) == seen_states.end())
                            {
                                new_results.push_back(next_state);
                                seen_states.insert(next_state->hash());
                            }
                        }
                    }
                    else
                    {
                        // If tactic failed for this state, keep the original state
                        new_results.push_back(result_state);
                    }
                }

                if (made_progress)
                {
                    results = new_results;
                }
            }

            return results;
        };
    }

    Tactic first(const std::vector<Tactic> &tactics)
    {
        return [tactics](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            for (size_t i = 0; i < tactics.size(); ++i)
            {
                const auto &tactic = tactics[i];
                std::optional<ConstraintViolation> local_violation;

                auto results = tactic(context, state, local_violation);

                if (!results.empty())
                {
                    // Log details of the result state
                    for (const auto &result : results)
                    {
                        for (const auto &hyp : result->hypotheses())
                        {
                            std::cout << "  - " << hyp.name() << " (Kind: " << static_cast<int>(hyp.formula()->kind()) << ")";

                            // Print more details for constants
                            if (hyp.formula()->kind() == TermDB::TermKind::CONSTANT)
                            {
                                auto constant = std::dynamic_pointer_cast<ConstantDB>(hyp.formula());
                                if (constant)
                                {
                                    std::cout << " Symbol: " << constant->symbol();
                                }
                            }
                            std::cout << std::endl;
                        }
                    }

                    return results; // Return on the first success
                }
                else
                {
                    if (local_violation)
                    {
                        std::cout << ": " << local_violation->message();
                    }
                    std::cout << std::endl;
                }
            }
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_RULE_APPLICATION,
                "All tactics in 'first' combinator failed");
            return {};
        };
    }

    Tactic then(const Tactic &first_tactic, const Tactic &second_tactic)
    {
        return [first_tactic, second_tactic](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            // Apply first tactic
            std::optional<ConstraintViolation> first_violation;
            auto intermediate_states = first_tactic(context, state, first_violation);

            if (intermediate_states.empty())
            {
                violation = first_violation;
                return {};
            }

            // Apply second tactic to each intermediate state
            std::vector<ProofStatePtr> results;

            for (const auto &intermediate_state : intermediate_states)
            {
                std::optional<ConstraintViolation> second_violation;
                auto next_states = second_tactic(context, intermediate_state, second_violation);

                // Collect all results
                results.insert(results.end(), next_states.begin(), next_states.end());
            }

            // If no states were produced, propagate the last violation
            if (results.empty())
            {
                violation = ConstraintViolation(
                    ConstraintViolation::ViolationType::INVALID_RULE_APPLICATION,
                    "Second tactic in 'then' combinator failed on all intermediate states");
            }

            return results;
        };
    }

    Tactic orelse(const Tactic &first_tactic, const Tactic &second_tactic)
    {
        return [first_tactic, second_tactic](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            // Try first tactic
            std::optional<ConstraintViolation> first_violation;
            auto first_results = first_tactic(context, state, first_violation);

            if (!first_results.empty())
            {
                return first_results;
            }

            // If first tactic failed, try second tactic
            std::optional<ConstraintViolation> second_violation;
            auto second_results = second_tactic(context, state, second_violation);

            if (!second_results.empty())
            {
                return second_results;
            }

            // Both tactics failed, report the second violation
            violation = second_violation;
            return {};
        };
    }

    Tactic try_tactic(const Tactic &tactic)
    {
        return [tactic](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            std::optional<ConstraintViolation> local_violation;
            auto results = tactic(context, state, local_violation);

            if (!results.empty())
            {
                return results;
            }

            // If tactic failed, return the original state
            return {state};
        };
    }

    Tactic tactic_if(std::function<bool(const ProofStatePtr &)> predicate, const Tactic &tactic)
    {
        return [predicate, tactic](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            if (predicate(state))
            {
                return tactic(context, state, violation);
            }

            // If predicate is false, return empty vector to indicate failure
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_RULE_APPLICATION,
                "Predicate evaluation failed");
            return {};
        };
    }

    Tactic parallel(const std::vector<Tactic> &tactics)
    {
        return [tactics](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            std::vector<ProofStatePtr> results;

            // Apply all tactics in parallel
            for (const auto &tactic : tactics)
            {
                std::optional<ConstraintViolation> local_violation;
                auto tactic_results = tactic(context, state, local_violation);

                // Collect all results
                results.insert(results.end(), tactic_results.begin(), tactic_results.end());
            }

            // Remove any duplicate states based on their hash
            std::set<std::size_t> seen_hashes;
            std::vector<ProofStatePtr> unique_results;

            for (const auto &result : results)
            {
                if (seen_hashes.find(result->hash()) == seen_hashes.end())
                {
                    unique_results.push_back(result);
                    seen_hashes.insert(result->hash());
                }
            }

            return unique_results;
        };
    }

    Tactic tactic_log(const std::string &name, const Tactic &tactic)
    {
        return [name, tactic](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            std::cout << "Applying tactic '" << name << "' to state with hash " << state->hash() << std::endl;
            std::cout << "  Goal type: " << static_cast<int>(state->goal()->kind()) << std::endl;
            std::cout << "  Hypotheses: " << state->hypotheses().size() << std::endl;

            auto results = tactic(context, state, violation);

            std::cout << "Tactic '" << name << "' produced " << results.size() << " result states" << std::endl;

            if (violation)
            {
                std::cout << "  Violation: " << violation->message() << std::endl;
            }

            return results;
        };
    }

    Tactic goal_matcher(std::function<bool(const TermDBPtr &)> matcher, const Tactic &tactic)
    {
        return [matcher, tactic](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            if (matcher(state->goal()))
            {
                return tactic(context, state, violation);
            }

            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                "Goal does not match the required pattern");
            return {};
        };
    }

    Tactic for_each_hypothesis(
        std::function<bool(const Hypothesis &)> pred,
        std::function<Tactic(const std::string &)> tactic_factory)
    {

        return [pred, tactic_factory](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            // Collect matching hypotheses
            std::vector<std::string> matching_hyps;

            for (const auto &hyp : state->hypotheses())
            {
                if (pred(hyp))
                {
                    matching_hyps.push_back(hyp.name());
                }
            }

            if (matching_hyps.empty())
            {
                violation = ConstraintViolation(
                    ConstraintViolation::ViolationType::INVALID_RULE_APPLICATION,
                    "No hypotheses matched the predicate");
                return {};
            }

            // Create a tactic for each matching hypothesis and try them in sequence
            std::vector<Tactic> tactics;
            for (const auto &hyp_name : matching_hyps)
            {
                tactics.push_back(tactic_factory(hyp_name));
            }

            // Use the first combinator to try each tactic until one succeeds
            return first(tactics)(context, state, violation);
        };
    }

    // TacticLibrary implementation

    TacticLibrary::TacticLibrary() {}

    void TacticLibrary::register_tactic(const std::string &name, const Tactic &tactic)
    {
        tactics_[name] = tactic;
    }

    std::optional<Tactic> TacticLibrary::get_tactic(const std::string &name) const
    {
        auto it = tactics_.find(name);
        if (it != tactics_.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    bool TacticLibrary::has_tactic(const std::string &name) const
    {
        return tactics_.find(name) != tactics_.end();
    }

    std::vector<std::string> TacticLibrary::get_tactic_names() const
    {
        std::vector<std::string> names;
        for (const auto &[name, _] : tactics_)
        {
            names.push_back(name);
        }
        return names;
    }

    // Common pre-built tactics

    Tactic propositional_simplify()
    {
        return repeat(
            for_each_hypothesis(
                // Predicate to match conjunction hypotheses
                [](const Hypothesis &hyp)
                {
                    return hyp.formula()->kind() == TermDB::TermKind::AND;
                },
                // Factory to create elimination tactics for each matched hypothesis
                [](const std::string &hyp_name)
                {
                    // Try both left and right elimination
                    return first({from_rule_ptr(AndElimRule::create_left(hyp_name)),
                                  from_rule_ptr(AndElimRule::create_right(hyp_name))});
                }));
    }

    Tactic auto_solve()
    {
        // Try various tactics to solve the goal
        return repeat(
            first({// Try to find a hypothesis that directly proves the goal
                   orelse(
                       // If contradiction in hypotheses, we can prove anything
                       contradiction(),
                       // Check if any hypothesis matches the goal
                       for_each_hypothesis(
                           // Match any hypothesis
                           [](const Hypothesis &)
                           { return true; },
                           // Factory to create a goal-checking tactic
                           [](const std::string &hyp_name)
                           {
                               return tactic_if(
                                   // Predicate to check if hypothesis formula matches goal
                                   [hyp_name](const ProofStatePtr &state)
                                   {
                                       const Hypothesis *hyp = state->find_hypothesis(hyp_name);
                                       return hyp && *hyp->formula() == *state->goal();
                                   },
                                   // If it matches, derive a new state marking it as proved
                                   [hyp_name](
                                       ProofContext &context,
                                       const ProofStatePtr &state,
                                       std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
                                   {
                                       auto new_state = context.apply_rule(
                                           state,
                                           "direct_proof",
                                           {hyp_name},
                                           {}, // No new hypotheses
                                           state->goal());

                                       new_state->mark_as_proved(
                                           ProofCertification::Status::PROVED_BY_RULE,
                                           "Direct proof from hypothesis " + hyp_name);

                                       return {new_state};
                                   });
                           })),

                   // Simplify hypotheses with elimination rules
                   elim(),

                   // Apply introduction rules based on goal structure
                   intro()}));
    }

    Tactic intro()
    {
        // Apply introduction rules based on goal structure
        return goal_matcher(
            // Check goal structure to determine which intro rule to apply
            [](const TermDBPtr &goal)
            {
                return goal->kind() == TermDB::TermKind::IMPLIES ||
                       goal->kind() == TermDB::TermKind::FORALL ||
                       goal->kind() == TermDB::TermKind::AND;
            },
            tactic_if(
                // Check for implication goal
                [](const ProofStatePtr &state)
                {
                    return state->goal()->kind() == TermDB::TermKind::IMPLIES;
                },
                from_rule_ptr(std::make_shared<ImpliesIntroRule>())));
    }

    Tactic elim()
    {
        // Apply elimination rules to hypotheses
        return for_each_hypothesis(
            // Match hypotheses that have elimination rules
            [](const Hypothesis &hyp)
            {
                auto kind = hyp.formula()->kind();
                return kind == TermDB::TermKind::AND ||
                       kind == TermDB::TermKind::OR ||
                       kind == TermDB::TermKind::FORALL;
            },
            // Create appropriate elimination tactic based on hypothesis type
            [](const std::string &hyp_name) -> Tactic
            {
                return tactic_if(
                    [hyp_name](const ProofStatePtr &state)
                    {
                        const Hypothesis *hyp = state->find_hypothesis(hyp_name);
                        if (!hyp)
                            return false;

                        auto kind = hyp->formula()->kind();
                        return kind == TermDB::TermKind::AND;
                    },
                    // For conjunction, try both left and right elimination
                    first({from_rule_ptr(AndElimRule::create_left(hyp_name)),
                           from_rule_ptr(AndElimRule::create_right(hyp_name))}));
            });
    }

    Tactic instantiate(const std::string &forall_hyp_name, const TermDBPtr &term)
    {
        auto rule = std::make_shared<ForallElimRule>(forall_hyp_name, term);
        return from_rule_ptr(rule);
    }

    Tactic rewrite()
    {
        // Find equalities and use them for rewriting
        return for_each_hypothesis(
            // Match equality hypotheses
            [](const Hypothesis &hyp)
            {
                if (hyp.formula()->kind() != TermDB::TermKind::FUNCTION_APPLICATION)
                {
                    return false;
                }

                auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(hyp.formula());
                return func_app->symbol() == "=";
            },
            // Create rewrite tactic for each equality
            [](const std::string &hyp_name)
            {
                // Try both left-to-right and right-to-left rewriting
                return first({from_rule_ptr(RewriteRule::create_left_to_right(hyp_name)),
                              from_rule_ptr(RewriteRule::create_right_to_left(hyp_name))});
            });
    }

    Tactic assume(const TermDBPtr &formula, const std::string &name)
    {
        auto rule = std::make_shared<AssumptionRule>(formula, name);
        return from_rule_ptr(rule);
    }

    Tactic contradiction()
    {
        // Look for contradictory hypotheses
        return for_each_hypothesis(
            // Match hypotheses that might have negations
            [](const Hypothesis &hyp)
            {
                return true; // Consider all hypotheses
            },
            // Create contradiction tactic for each pair of potentially contradictory hypotheses
            [](const std::string &hyp_name) -> Tactic
            {
                return for_each_hypothesis(
                    // Find negation hypotheses
                    [](const Hypothesis &hyp)
                    {
                        return hyp.formula()->kind() == TermDB::TermKind::NOT;
                    },
                    // Try to form contradiction with the original hypothesis
                    [hyp_name](const std::string &not_hyp_name)
                    {
                        auto rule = std::make_shared<ContradictionRule>(hyp_name, not_hyp_name);
                        return from_rule_ptr(rule);
                    });
            });
    }

    /**
     * Creates a tactic that forms useful conjunctions from hypotheses
     *
     * This tactic scans all hypotheses to find implications. For each implication
     * of the form P ∧ Q → R, it tries to find hypotheses matching P and Q, and
     * combines them using AndIntroRule. This enables subsequent modus ponens.
     *
     * Additionally, it proactively forms conjunctions between formulas with the same
     * predicate symbol, which is necessary for complex proofs like the FOL benchmark.
     *
     * @return Tactic A tactic that builds strategic conjunctions
     */
    Tactic form_useful_conjunctions()
    {
        return [](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            std::vector<ProofStatePtr> results;

            // First pass: Collect all implications in the hypotheses
            std::vector<std::pair<std::string, TermDBPtr>> implications;

            for (const auto &hyp : state->hypotheses())
            {
                if (hyp.formula()->kind() == TermDB::TermKind::IMPLIES)
                {
                    auto implies = std::dynamic_pointer_cast<ImpliesDB>(hyp.formula());
                    implications.push_back({hyp.name(), implies->antecedent()});
                }
            }

            // For each implication, check if its antecedent is a conjunction
            for (const auto &[impl_name, antecedent] : implications)
            {
                if (antecedent->kind() == TermDB::TermKind::AND)
                {
                    auto and_term = std::dynamic_pointer_cast<AndDB>(antecedent);
                    auto left_term = and_term->left();
                    auto right_term = and_term->right();

                    // Find all hypotheses matching the left and right parts of the conjunction
                    std::vector<std::string> left_hyp_names;
                    std::vector<std::string> right_hyp_names;

                    // Collect all potential matches for left and right terms independently
                    for (const auto &hyp : state->hypotheses())
                    {
                        // Skip implications to avoid circular reasoning
                        if (hyp.formula()->kind() == TermDB::TermKind::IMPLIES)
                        {
                            continue;
                        }

                        // Check both conditions independently (not using else if)
                        if (*hyp.formula() == *left_term)
                        {
                            left_hyp_names.push_back(hyp.name());
                        }

                        if (*hyp.formula() == *right_term)
                        {
                            right_hyp_names.push_back(hyp.name());
                        }
                    }

                    // Try all combinations of matching left and right hypotheses
                    for (const auto &left_hyp_name : left_hyp_names)
                    {
                        for (const auto &right_hyp_name : right_hyp_names)
                        {
                            // Don't use the same hypothesis for both parts
                            if (left_hyp_name == right_hyp_name)
                            {
                                continue;
                            }

                            // Check if this conjunction already exists
                            auto candidate = make_and(left_term, right_term);
                            bool already_present = false;

                            for (const auto &h : state->hypotheses())
                            {
                                if (*h.formula() == *candidate)
                                {
                                    already_present = true;
                                    break;
                                }
                            }

                            if (already_present)
                                continue;

                            // Create and apply the AndIntroRule
                            auto and_intro_rule = std::make_shared<AndIntroRule>(
                                left_hyp_name, right_hyp_name);

                            std::optional<ConstraintViolation> local_violation;
                            auto new_states = and_intro_rule->apply(
                                context, state, local_violation);

                            // Add the results if successful
                            if (!new_states.empty())
                            {
                                results.insert(results.end(), new_states.begin(), new_states.end());
                            }
                        }
                    }
                }
            }

            // Second pass, proactively form conjunctions between formulas with same predicate
            // Group hypotheses by predicate symbol
            std::unordered_map<std::string, std::vector<std::pair<std::string, TermDBPtr>>> pred_groups;

            for (const auto &hyp : state->hypotheses())
            {
                // Only consider function applications (predicates)
                if (hyp.formula()->kind() == TermDB::TermKind::FUNCTION_APPLICATION)
                {
                    auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(hyp.formula());
                    if (func_app)
                    {
                        // Group by predicate symbol
                        pred_groups[func_app->symbol()].push_back({hyp.name(), hyp.formula()});
                    }
                }
            }

            // For each predicate group, try to form useful conjunctions
            for (const auto &[pred_symbol, formulas] : pred_groups)
            {
                // Only attempt if we have at least 2 formulas with same predicate
                if (formulas.size() >= 2)
                {
                    for (size_t i = 0; i < formulas.size(); ++i)
                    {
                        for (size_t j = i + 1; j < formulas.size(); ++j)
                        {
                            // Don't use the same hypothesis for both parts
                            if (formulas[i].first == formulas[j].first)
                            {
                                continue;
                            }

                            // Check if this conjunction already exists
                            auto candidate = make_and(formulas[i].second, formulas[j].second);
                            bool already_present = false;

                            for (const auto &h : state->hypotheses())
                            {
                                if (*h.formula() == *candidate)
                                {
                                    already_present = true;
                                    break;
                                }
                            }

                            if (already_present)
                                continue;

                            // Create and apply the AndIntroRule
                            auto and_intro_rule = std::make_shared<AndIntroRule>(
                                formulas[i].first, formulas[j].first);

                            std::optional<ConstraintViolation> local_violation;
                            auto new_states = and_intro_rule->apply(
                                context, state, local_violation);

                            // Add the results if successful
                            if (!new_states.empty())
                            {
                                results.insert(results.end(), new_states.begin(), new_states.end());
                            }
                        }
                    }
                }
            }

            // Third pass, Goal-oriented conjunction formation
            if (state->goal()->kind() == TermDB::TermKind::AND)
            {
                auto and_goal = std::dynamic_pointer_cast<AndDB>(state->goal());
                auto left_goal = and_goal->left();
                auto right_goal = and_goal->right();

                // Find hypotheses matching the left and right parts of the goal
                std::vector<std::string> left_hyp_names;
                std::vector<std::string> right_hyp_names;

                for (const auto &hyp : state->hypotheses())
                {
                    if (*hyp.formula() == *left_goal)
                    {
                        left_hyp_names.push_back(hyp.name());
                    }
                    else if (*hyp.formula() == *right_goal)
                    {
                        right_hyp_names.push_back(hyp.name());
                    }
                }

                // Try all combinations of matching left and right hypotheses
                for (const auto &left_hyp_name : left_hyp_names)
                {
                    for (const auto &right_hyp_name : right_hyp_names)
                    {
                        // Don't use the same hypothesis for both parts
                        if (left_hyp_name == right_hyp_name)
                        {
                            continue;
                        }

                        // Check if this conjunction already exists
                        auto candidate = make_and(left_goal, right_goal);
                        bool already_present = false;

                        for (const auto &h : state->hypotheses())
                        {
                            if (*h.formula() == *candidate)
                            {
                                already_present = true;
                                break;
                            }
                        }

                        if (already_present)
                            continue;

                        // Create and apply the AndIntroRule
                        auto and_intro_rule = std::make_shared<AndIntroRule>(
                            left_hyp_name, right_hyp_name);

                        std::optional<ConstraintViolation> local_violation;
                        auto new_states = and_intro_rule->apply(
                            context, state, local_violation);

                        // Add the results if successful
                        if (!new_states.empty())
                        {
                            // These states should match the goal directly
                            for (auto &new_state : new_states)
                            {
                                // Check if state now proves the goal
                                for (const auto &hyp : new_state->hypotheses())
                                {
                                    if (*hyp.formula() == *state->goal())
                                    {
                                        // Mark this state as proven
                                        new_state->mark_as_proved(
                                            ProofCertification::Status::PROVED_BY_RULE,
                                            "Goal achieved by conjunction formation: " + left_hyp_name + " and " + right_hyp_name);
                                        break;
                                    }
                                }
                            }

                            results.insert(results.end(), new_states.begin(), new_states.end());
                        }
                    }
                }
            }

            // If no results were produced, return the original state to avoid blocking
            if (results.empty())
            {
                return {state};
            }

            return results;
        };
    }

    /**
     * Creates a tactic that applies modus ponens when the antecedent exists
     *
     * This tactic searches for implications in the hypotheses and applies
     * modus ponens only when a hypothesis matching the antecedent exists.
     * This is more efficient than trying all combinations of hypotheses.
     *
     * @return Tactic A tactic that strategically applies modus ponens
     */
    Tactic match_mp_antecedent()
    {
        return [](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            std::vector<ProofStatePtr> results;

            int impl_count = 0;
            // Collect all implications in the hypotheses
            for (const auto &impl_hyp : state->hypotheses())
            {
                // Skip non-implications
                if (impl_hyp.formula()->kind() != TermDB::TermKind::IMPLIES)
                {
                    continue;
                }
                impl_count++;

                auto implies = std::dynamic_pointer_cast<ImpliesDB>(impl_hyp.formula());
                auto antecedent = implies->antecedent();
                auto consequent = implies->consequent();

                // Look for a hypothesis matching the antecedent
                for (const auto &ant_hyp : state->hypotheses())
                {
                    // Skip the implication itself
                    if (ant_hyp.name() == impl_hyp.name())
                    {
                        continue;
                    }

                    // Check if this hypothesis matches the antecedent
                    bool match = *ant_hyp.formula() == *antecedent;

                    if (match)
                    {
                        // Apply modus ponens
                        auto mp_rule = std::make_shared<ModusPonensRule>(
                            ant_hyp.name(), impl_hyp.name());

                        std::optional<ConstraintViolation> local_violation;
                        auto new_states = mp_rule->apply(context, state, local_violation);

                        // Always add states resulting from modus ponens
                        for (const auto &new_state : new_states)
                        {
                            // Check if this new state directly proves its goal
                            if (*consequent == *new_state->goal())
                            {
                                new_state->mark_as_proved(
                                    ProofCertification::Status::PROVED_BY_RULE,
                                    "Direct proof through modus ponens");
                            }

                            results.push_back(new_state);
                        }
                    }
                }
            }

            // If no results were produced, return the original state to avoid blocking
            if (results.empty())
            {
                return {state};
            }

            return results;
        };
    }

    /**
     * Creates a tactic that handles conjunctive goals by splitting them into subgoals
     *
     * When the goal is of the form A ∧ B, this tactic splits it into two separate
     * proof obligations: one for A and one for B. This enables more focused proof search
     * for each component, which can then be combined using AndIntroRule.
     *
     * @return Tactic A tactic for conjunctive goal handling
     */
    Tactic handle_conjunctive_goal()
    {
        return [](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            // Only apply if the goal is a conjunction
            if (state->goal()->kind() != TermDB::TermKind::AND)
            {
                return {state}; // Return original state unchanged
            }

            auto and_goal = std::dynamic_pointer_cast<AndDB>(state->goal());
            auto left_goal = and_goal->left();
            auto right_goal = and_goal->right();

            // Check if we already have either the left or right goal as a hypothesis
            bool have_left = false;
            bool have_right = false;
            std::string left_hyp_name;
            std::string right_hyp_name;

            for (const auto &hyp : state->hypotheses())
            {
                if (*hyp.formula() == *left_goal)
                {
                    have_left = true;
                    left_hyp_name = hyp.name();
                }
                if (*hyp.formula() == *right_goal)
                {
                    have_right = true;
                    right_hyp_name = hyp.name();
                }
            }

            // If we already have both components, just apply AndIntroRule
            if (have_left && have_right)
            {
                auto and_intro_rule = std::make_shared<AndIntroRule>(left_hyp_name, right_hyp_name);
                std::optional<ConstraintViolation> local_violation;
                auto new_states = and_intro_rule->apply(context, state, local_violation);

                // Check if any of the new states satisfy the original goal
                std::vector<ProofStatePtr> valid_states;
                for (const auto &new_state : new_states)
                {
                    for (const auto &hyp : new_state->hypotheses())
                    {
                        if (*hyp.formula() == *state->goal())
                        {
                            // Create a state that marks the proof complete
                            auto completed_state = context.apply_rule(
                                new_state,
                                "direct_proof",
                                {hyp.name()},
                                {}, // No new hypotheses
                                state->goal());

                            completed_state->mark_as_proved(
                                ProofCertification::Status::PROVED_BY_RULE,
                                "Proof completed by conjunction introduction");

                            valid_states.push_back(completed_state);
                            break;
                        }
                    }
                }

                if (!valid_states.empty())
                {
                    return valid_states;
                }
            }

            std::vector<ProofStatePtr> result_states;

            // Create a new state with left component as goal (if we don't have it yet)
            if (!have_left)
            {
                auto left_state = context.apply_rule(
                    state,
                    "and_left_goal",
                    {},       // No premise names
                    {},       // No new hypotheses
                    left_goal // New goal is the left component
                );
                result_states.push_back(left_state);
            }

            // Create a new state with right component as goal (if we don't have it yet)
            if (!have_right)
            {
                auto right_state = context.apply_rule(
                    state,
                    "and_right_goal",
                    {},        // No premise names
                    {},        // No new hypotheses
                    right_goal // New goal is the right component
                );
                result_states.push_back(right_state);
            }

            // If we didn't create any new states, return the original
            if (result_states.empty())
            {
                return {state};
            }

            return result_states;
        };
    }

    /**
     * Creates a tactic that repeatedly applies a tactic, tracking semantic progress
     *
     * Unlike the standard repeat() which only tracks syntactic state differences,
     * this tactic monitors semantic progress by tracking newly derived formulas
     * and logical distance to the goal. This prevents both cyclical reasoning and
     * premature termination when progress is being made semantically but not visible
     * through state hashes alone.
     *
     * @param tactic The tactic to repeat
     * @param max_iterations Maximum number of iterations (default: 10)
     * @return Tactic A tactic that repeatedly applies the given tactic, tracking semantic progress
     */
    Tactic smart_repeat(const Tactic &tactic, size_t max_iterations)
    {
        return [tactic, max_iterations](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            std::vector<ProofStatePtr> current_states = {state};
            std::vector<ProofStatePtr> result_states;

            // Track seen formulas (not just state hashes) to detect semantic progress
            std::unordered_set<std::size_t> seen_formulas;

            // Initialize seen_formulas with formulas from the initial state
            for (const auto &hyp : state->hypotheses())
            {
                seen_formulas.insert(hyp.formula()->hash());
            }

            // Track states that we've already processed to avoid cycles
            std::unordered_set<std::size_t> processed_states;
            processed_states.insert(state->hash());

            size_t iteration = 0;
            bool made_progress = true;

            // Keep track of proven states
            std::vector<ProofStatePtr> proven_states;

            while (made_progress && iteration < max_iterations)
            {
                iteration++;
                made_progress = false;
                result_states.clear();

                // Process each state in the current set
                for (const auto &current_state : current_states)
                {
                    // Skip if we've already proven the goal
                    if (current_state->is_proved())
                    {
                        proven_states.push_back(current_state);
                        continue;
                    }

                    // Apply the tactic
                    std::optional<ConstraintViolation> local_violation;
                    auto next_states = tactic(context, current_state, local_violation);

                    if (next_states.empty())
                    {
                        // If tactic produced no results, keep the original state
                        result_states.push_back(current_state);
                        continue;
                    }

                    // Process each resulting state
                    for (const auto &next_state : next_states)
                    {
                        // Skip if we've already processed this exact state
                        if (processed_states.find(next_state->hash()) != processed_states.end())
                        {
                            continue;
                        }

                        // Track this state
                        processed_states.insert(next_state->hash());

                        // Check if this state is proven
                        if (next_state->is_proved())
                        {
                            proven_states.push_back(next_state);
                            made_progress = true;
                            continue;
                        }

                        // Check for new formulas (semantic progress)
                        bool new_formula_found = false;
                        for (const auto &hyp : next_state->hypotheses())
                        {
                            auto formula_hash = hyp.formula()->hash();
                            if (seen_formulas.find(formula_hash) == seen_formulas.end())
                            {
                                seen_formulas.insert(formula_hash);
                                new_formula_found = true;
                            }
                        }

                        // If we found a new formula, we made semantic progress
                        if (new_formula_found)
                        {
                            made_progress = true;
                            result_states.push_back(next_state);
                        }
                        // Otherwise, check if it's closer to the goal in some way
                        else
                        {
                            // This is where we could add additional heuristics:
                            // 1. Count matching subformulas with the goal
                            // 2. Track elimination of disjunctions/implications
                            // 3. Measure structural similarity to goal

                            // For now, use a simple heuristic:
                            // Keep states with fewer hypotheses (more focused)
                            if (next_state->hypotheses().size() < current_state->hypotheses().size())
                            {
                                made_progress = true;
                                result_states.push_back(next_state);
                            }
                            else
                            {
                                // If no clear progress, keep the state but don't mark progress
                                result_states.push_back(next_state);
                            }
                        }
                    }
                }

                // If we found proven states, return them immediately
                if (!proven_states.empty())
                {
                    return proven_states;
                }

                // Update current states for next iteration
                if (made_progress || result_states.size() != current_states.size())
                {
                    current_states = result_states;
                }
            }
            if (proven_states.empty() && result_states.empty())
            {
                // If we didn't find any proven states or make any progress,
                // return the original state so the benchmark can check it
                return {state};
            }

            // Return the final set of states (or proven states if any)
            // If both are empty but we have current states, return those instead
            return proven_states.empty()
                       ? (result_states.empty() ? current_states : result_states)
                       : proven_states;
        };
    }

    /**
     * Creates a tactic that applies direct proof when a hypothesis matches the goal
     *
     * This tactic scans all hypotheses to find one that exactly matches the current
     * goal. If found, it marks the proof as complete without requiring additional steps.
     * For conjunctive goals, it also tries to find hypotheses matching the left and right
     * parts and combines them using AndIntroRule.
     *
     * @return Tactic A tactic that applies direct proof when possible
     */

    Tactic direct_proof()
    {
        return [](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            // Skip if already proven
            if (state->is_proved())
            {
                return {state};
            }

            // Get the current goal
            auto goal = state->goal();
            // Check if any hypothesis matches the goal exactly
            for (const auto &hyp : state->hypotheses())
            {
                bool match = *hyp.formula() == *goal;
                if (match)
                {
                    // Create a new state that marks the proof complete
                    auto new_state = context.apply_rule(
                        state,
                        "direct_proof",
                        {hyp.name()}, // Using the matching hypothesis as premise
                        {},           // No new hypotheses needed
                        goal          // Keep the same goal
                    );

                    // Mark this state as proved
                    new_state->mark_as_proved(
                        ProofCertification::Status::PROVED_BY_RULE,
                        "Direct proof from hypothesis " + hyp.name());
                    return {new_state};
                }
            }

            // Special handling for conjunction goals
            if (goal->kind() == TermDB::TermKind::AND)
            {
                auto and_goal = std::dynamic_pointer_cast<AndDB>(goal);
                auto left_goal = and_goal->left();
                auto right_goal = and_goal->right();

                // Find hypotheses matching the left and right parts of the goal
                std::string left_hyp_name;
                std::string right_hyp_name;

                for (const auto &hyp : state->hypotheses())
                {
                    if (*hyp.formula() == *left_goal)
                    {
                        left_hyp_name = hyp.name();
                    }
                    else if (*hyp.formula() == *right_goal)
                    {
                        right_hyp_name = hyp.name();
                    }
                }

                // If we found both parts, form the conjunction and mark as proven
                if (!left_hyp_name.empty() && !right_hyp_name.empty())
                {
                    // Create a named conjunction using AND-introduction
                    auto and_intro_rule = std::make_shared<AndIntroRule>(
                        left_hyp_name, right_hyp_name);

                    std::optional<ConstraintViolation> and_violation;
                    auto and_results = and_intro_rule->apply(context, state, and_violation);

                    if (!and_results.empty())
                    {
                        auto final_state = and_results[0];

                        // Find the hypothesis containing the conjunction
                        std::string conj_hyp_name;
                        for (const auto &hyp : final_state->hypotheses())
                        {
                            if (*hyp.formula() == *goal)
                            {
                                conj_hyp_name = hyp.name();
                                break;
                            }
                        }

                        if (!conj_hyp_name.empty())
                        {
                            // Mark as proven and return
                            final_state->mark_as_proved(
                                ProofCertification::Status::PROVED_BY_RULE,
                                "Direct proof from conjunction of " + left_hyp_name + " and " + right_hyp_name);

                            return {final_state};
                        }
                        // Even if we couldn't find the exact hypothesis name, return the AND-intro result
                        return and_results;
                    }
                }
            }
            // If no direct proof found, return empty vector
            return {};
        };
    }

    Tactic sequence(const std::vector<Tactic> &tactics)
    {
        return [tactics](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            std::vector<ProofStatePtr> current_states = {state};
            std::vector<ProofStatePtr> next_states;

            // Apply each tactic in sequence to all current states
            for (const auto &tactic : tactics)
            {
                next_states.clear();

                for (const auto &current_state : current_states)
                {
                    std::optional<ConstraintViolation> local_violation;
                    auto results = tactic(context, current_state, local_violation);

                    // Add all results to next_states
                    if (!results.empty())
                    {
                        next_states.insert(next_states.end(), results.begin(), results.end());
                    }
                    else
                    {
                        // Keep the original state if tactic didn't produce results
                        next_states.push_back(current_state);
                    }
                }

                // Update current_states for next tactic
                current_states = next_states;
            }

            return current_states;
        };
    }

} // namespace theorem_prover