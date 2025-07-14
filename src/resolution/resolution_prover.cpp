#include "resolution_prover.hpp"
#include "indexing.hpp"
#include "clause.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <unordered_map>

using namespace std::chrono;

namespace theorem_prover
{

    ClauseSet::ClauseSet(const ResolutionConfig &config)
        : config_(config), next_clause_index_(0) {}

    void ClauseSet::add_clause(ClausePtr clause)
    {
        if (!clause || clause->is_tautology())
        {
            return;
        }

        auto simplified = std::make_shared<Clause>(clause->simplify());

        size_t clause_hash = simplified->hash();
        if (clause_hashes_.find(clause_hash) != clause_hashes_.end())
        {
            return;
        }

        if (config_.use_subsumption && is_subsumed(simplified))
        {
            return;
        }

        if (config_.use_subsumption)
        {
            remove_subsumed_clauses(simplified);
        }

        clauses_.push_back(simplified);
        processing_queue_.push(simplified);
        clause_hashes_.insert(clause_hash);

        // Add to index for efficient retrieval
        literal_index_.insert_clause(simplified);
    }

    bool ClauseSet::contains_empty_clause() const
    {
        for (const auto &clause : clauses_)
        {
            if (clause->is_empty())
            {
                return true;
            }
        }
        return false;
    }

    ClausePtr ClauseSet::select_clause()
    {
        if (processing_queue_.empty())
        {
            return nullptr;
        }

        switch (config_.selection_strategy)
        {
        case ResolutionConfig::SelectionStrategy::FIFO:
        {
            auto clause = processing_queue_.front();
            processing_queue_.pop();
            return clause;
        }

        case ResolutionConfig::SelectionStrategy::SMALLEST_FIRST:
        {
            // Find smallest clause in queue - safer implementation
            if (processing_queue_.size() == 1)
            {
                auto clause = processing_queue_.front();
                processing_queue_.pop();
                return clause;
            }

            // Convert queue to vector for safer manipulation
            std::vector<ClausePtr> queue_contents;
            while (!processing_queue_.empty())
            {
                queue_contents.push_back(processing_queue_.front());
                processing_queue_.pop();
            }

            if (queue_contents.empty())
            {
                return nullptr;
            }

            // Find smallest clause
            auto smallest_it = std::min_element(queue_contents.begin(), queue_contents.end(),
                                                [](const ClausePtr &a, const ClausePtr &b)
                                                {
                                                    return a->size() < b->size();
                                                });

            ClausePtr smallest = *smallest_it;

            // Put back all except the smallest
            for (const auto &clause : queue_contents)
            {
                if (clause != smallest)
                {
                    processing_queue_.push(clause);
                }
            }

            return smallest;
        }

        case ResolutionConfig::SelectionStrategy::UNIT_PREFERENCE:
        {
            if (processing_queue_.size() == 1)
            {
                auto clause = processing_queue_.front();
                processing_queue_.pop();
                return clause;
            }

            // Convert queue to vector for safer manipulation
            std::vector<ClausePtr> queue_contents;
            while (!processing_queue_.empty())
            {
                queue_contents.push_back(processing_queue_.front());
                processing_queue_.pop();
            }

            if (queue_contents.empty())
            {
                return nullptr;
            }

            // Look for unit clauses first
            ClausePtr unit_clause = nullptr;
            for (const auto &clause : queue_contents)
            {
                if (clause->is_unit())
                {
                    unit_clause = clause;
                    break;
                }
            }

            ClausePtr selected = unit_clause ? unit_clause : queue_contents[0];

            // Put back all except the selected clause
            for (const auto &clause : queue_contents)
            {
                if (clause != selected)
                {
                    processing_queue_.push(clause);
                }
            }

            return selected;
        }

        case ResolutionConfig::SelectionStrategy::NEGATIVE_SELECTION:
        {
            // Just use FIFO for now - can implement proper negative selection later
            auto clause = processing_queue_.front();
            processing_queue_.pop();
            return clause;
        }

        default:
        {
            auto clause = processing_queue_.front();
            processing_queue_.pop();
            return clause;
        }
        }
    }

    bool ClauseSet::is_empty() const
    {
        return processing_queue_.empty();
    }

    void ClauseSet::clear()
    {
        clauses_.clear();
        while (!processing_queue_.empty())
        {
            processing_queue_.pop();
        }
        clause_hashes_.clear();
        next_clause_index_ = 0;

        // CLEAR INDEX
        literal_index_.clear();
    }

    bool ClauseSet::is_subsumed(ClausePtr clause) const
    {
        // A clause is subsumed if any existing clause subsumes it
        for (const auto &existing : clauses_)
        {
            if (Clause::subsumes(existing, clause))
            {
                return true;
            }
        }
        return false;
    }
    std::vector<ClausePtr> ClauseSet::get_resolution_candidates(const Literal &literal)
    {
        return literal_index_.get_resolution_candidates(literal);
    }

    void ClauseSet::remove_subsumed_clauses(ClausePtr clause)
    {
        // Remove clauses that are subsumed by the new clause
        auto it = clauses_.begin();
        while (it != clauses_.end())
        {
            if (Clause::subsumes(clause, *it))
            {
                clause_hashes_.erase((*it)->hash());
                it = clauses_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    bool ClauseSet::are_variants(ClausePtr clause1, ClausePtr clause2) const
    {
        // Simplified variant check - would need proper variable renaming
        return clause1->equals(*clause2);
    }

    ResolutionProver::ResolutionProver(const ResolutionConfig &config)
        : config_(config) {}

    ResolutionProofResult ResolutionProver::prove(const TermDBPtr &goal,
                                                  const std::vector<TermDBPtr> &hypotheses)
    {
        // Convert to refutation problem: Hypotheses ∪ {¬Goal} should be unsatisfiable
        auto refutation_formulas = setup_refutation_problem(goal, hypotheses);

        // Convert to CNF
        std::vector<ClausePtr> all_clauses;
        for (const auto &formula : refutation_formulas)
        {
            auto cnf_clauses = CNFConverter::to_cnf(formula);
            all_clauses.insert(all_clauses.end(), cnf_clauses.begin(), cnf_clauses.end());
        }

        // NEW: Optional KB preprocessing
        if (config_.use_kb_preprocessing)
        {
            auto kb_result = try_kb_preprocessing(all_clauses);

            // Log KB results for analysis
            if (config_.kb_config.verbose && kb_result.status == KBResult::Status::SUCCESS)
            {
                std::cout << "KB preprocessing: " << kb_result.final_rules.size()
                          << " rules generated in " << kb_result.elapsed_time_seconds
                          << " seconds" << std::endl;
            }
        }

        return prove_from_clauses(all_clauses);
    }

    ResolutionProofResult ResolutionProver::check_satisfiability(const std::vector<TermDBPtr> &formulas)
    {
        // Convert to CNF
        std::vector<ClausePtr> all_clauses;
        for (const auto &formula : formulas)
        {
            auto cnf_clauses = CNFConverter::to_cnf(formula);
            all_clauses.insert(all_clauses.end(), cnf_clauses.begin(), cnf_clauses.end());
        }

        auto result = prove_from_clauses(all_clauses);

        // Flip the interpretation for satisfiability checking
        if (result.status == ResolutionProofResult::Status::PROVED)
        {
            result.status = ResolutionProofResult::Status::DISPROVED;
            result.explanation = "Formula set is unsatisfiable";
        }
        else if (result.status == ResolutionProofResult::Status::SATURATED)
        {
            result.status = ResolutionProofResult::Status::PROVED;
            result.explanation = "Formula set is satisfiable";
        }

        return result;
    }

    ResolutionProofResult ResolutionProver::prove_from_clauses(const std::vector<ClausePtr> &clauses)
    {
        ClauseSet clause_set(config_);

        // Find the maximum variable index across all clauses to ensure fresh variables
        std::size_t max_var_index = 0;
        for (const auto &clause : clauses)
        {
            for (const auto &lit : clause->literals())
            {
                auto clause_max = get_max_variable_index(lit.atom());
                max_var_index = std::max(max_var_index, clause_max);
            }
        }

        // Add clauses with proper variable standardization
        std::size_t var_offset = max_var_index + 1;
        for (const auto &clause : clauses)
        {
            // Rename variables to ensure disjoint variable spaces
            auto standardized_clause = std::make_shared<Clause>(
                clause->rename_variables(var_offset));

            clause_set.add_clause(standardized_clause);

            // Update offset for next clause
            for (const auto &lit : standardized_clause->literals())
            {
                auto clause_max = get_max_variable_index(lit.atom());
                var_offset = std::max(var_offset, clause_max + 1);
            }
        }

        // Check for immediate empty clause
        if (clause_set.contains_empty_clause())
        {
            ResolutionProofResult result(ResolutionProofResult::Status::PROVED,
                                         "Empty clause found in initial clause set");
            result.final_clauses = clause_set.clauses();
            return result;
        }

        return resolution_loop(clause_set);
    }

    ResolutionProofResult ResolutionProver::resolution_loop(ClauseSet &clause_set)
    {
        auto start_time = high_resolution_clock::now();
        size_t iterations = 0;

        while (!clause_set.is_empty())
        {
            auto current_time = high_resolution_clock::now();
            double elapsed_ms = duration_cast<microseconds>(current_time - start_time).count() / 1000.0;

            // Check termination conditions
            if (should_terminate(iterations, elapsed_ms, clause_set.size()))
            {
                if (iterations >= config_.max_iterations)
                {
                    ResolutionProofResult result(ResolutionProofResult::Status::TIMEOUT,
                                                 "Maximum iterations exceeded");
                    result.iterations = iterations;
                    result.time_elapsed_ms = elapsed_ms;
                    result.final_clauses = clause_set.clauses();
                    return result;
                }
                if (elapsed_ms >= config_.max_time_ms)
                {
                    ResolutionProofResult result(ResolutionProofResult::Status::TIMEOUT,
                                                 "Time limit exceeded");
                    result.iterations = iterations;
                    result.time_elapsed_ms = elapsed_ms;
                    result.final_clauses = clause_set.clauses();
                    return result;
                }
                if (clause_set.size() >= config_.max_clauses)
                {
                    ResolutionProofResult result(ResolutionProofResult::Status::TIMEOUT,
                                                 "Maximum clauses exceeded");
                    result.iterations = iterations;
                    result.time_elapsed_ms = elapsed_ms;
                    result.final_clauses = clause_set.clauses();
                    return result;
                }
            }

            // Select clause for resolution
            auto selected_clause = clause_set.select_clause();
            if (!selected_clause)
            {
                break;
            }

            // Safety check
            if (!selected_clause)
            {
                std::cout << "Warning: null clause selected" << std::endl;
                break;
            }

            // Try to resolve using INDEX instead of brute force
            bool new_clause_added = false;

            // For each literal in the selected clause, find resolution candidates
            for (const auto &literal : selected_clause->literals())
            {
                std::vector<ClausePtr> candidates;

                if (config_.use_paramodulation)
                {
                    // For paramodulation, we need to try ALL clauses, not just complementary ones
                    candidates = clause_set.clauses();
                }
                else
                {
                    candidates = clause_set.get_resolution_candidates(literal);
                }

                for (const auto &candidate_clause : candidates)
                {
                    if (!candidate_clause || selected_clause == candidate_clause)
                    {
                        continue;
                    }

                    // Attempt resolution (with or without paramodulation)
                    std::vector<ClausePtr> resolvents;
                    if (config_.use_paramodulation)
                    {

                        resolvents = ResolutionWithParamodulation::resolve_with_paramodulation(
                            selected_clause, candidate_clause);
                    }
                    else
                    {
                        auto single_resolvent = resolve_clauses(selected_clause, candidate_clause);
                        resolvents.insert(resolvents.end(), single_resolvent.begin(), single_resolvent.end());
                    }

                    for (auto resolvent : resolvents)
                    {
                        if (!resolvent)
                        {
                            continue;
                        }

                        if (resolvent->is_empty())
                        {
                            // Found empty clause - proof complete!
                            ResolutionProofResult result(ResolutionProofResult::Status::PROVED,
                                                         "Empty clause derived - theorem proved");
                            result.iterations = iterations;
                            result.time_elapsed_ms = elapsed_ms;
                            result.final_clauses = clause_set.clauses();
                            return result;
                        }

                        // Add new resolvent
                        size_t old_size = clause_set.size();
                        clause_set.add_clause(resolvent);
                        if (clause_set.size() > old_size)
                        {
                            new_clause_added = true;
                        }
                    }

                    // Safety check for infinite loops
                    if (clause_set.size() > config_.max_clauses)
                    {
                        break;
                    }
                }

                if (clause_set.size() > config_.max_clauses)
                {
                    break;
                }
            }

            // Apply factoring if enabled
            if (config_.use_factoring)
            {
                auto factored = factor_clause(selected_clause);
                if (factored && !factored->equals(*selected_clause))
                {
                    size_t old_size = clause_set.size();
                    clause_set.add_clause(factored);
                    if (clause_set.size() > old_size)
                    {
                        new_clause_added = true;
                    }
                }
            }

            iterations++;
        }

        // No more clauses to process and no empty clause found
        auto end_time = high_resolution_clock::now();
        double elapsed_ms = duration_cast<microseconds>(end_time - start_time).count() / 1000.0;

        ResolutionProofResult result(ResolutionProofResult::Status::SATURATED,
                                     "Clause set is saturated - no new clauses can be derived");
        result.iterations = iterations;
        result.time_elapsed_ms = elapsed_ms;
        result.final_clauses = clause_set.clauses();
        return result;
    }

    std::vector<ClausePtr> ResolutionProver::resolve_clauses(ClausePtr clause1, ClausePtr clause2)
    {
        std::vector<ClausePtr> resolvents;

        auto resolution_result = ResolutionInference::resolve(clause1, clause2);
        if (resolution_result.success)
        {
            resolvents.push_back(resolution_result.resolvent);
        }

        return resolvents;
    }

    ClausePtr ResolutionProver::factor_clause(ClausePtr clause)
    {
        return ResolutionInference::factor(clause);
    }

    bool ResolutionProver::should_terminate(size_t iterations, double elapsed_ms, size_t clause_count) const
    {
        return iterations >= config_.max_iterations ||
               elapsed_ms >= config_.max_time_ms ||
               clause_count >= config_.max_clauses;
    }

    std::vector<TermDBPtr> ResolutionProver::setup_refutation_problem(const TermDBPtr &goal,
                                                                      const std::vector<TermDBPtr> &hypotheses)
    {
        std::vector<TermDBPtr> formulas = hypotheses;

        // Add negation of goal for refutation
        auto negated_goal = make_not(goal);
        formulas.push_back(negated_goal);

        return formulas;
    }

    // Utility functions
    namespace resolution_utils
    {

        void print_clause_set(const std::vector<ClausePtr> &clauses, const std::string &title)
        {
            if (!title.empty())
            {
                std::cout << title << std::endl;
                std::cout << std::string(title.length(), '=') << std::endl;
            }

            for (size_t i = 0; i < clauses.size(); ++i)
            {
                std::cout << "Clause " << i << ": " << clauses[i]->to_string() << std::endl;
            }
            std::cout << std::endl;
        }

        bool is_obviously_satisfiable(const std::vector<ClausePtr> &clauses)
        {
            // Very basic check - if no clauses, it's satisfiable
            return clauses.empty();
        }

        bool is_obviously_unsatisfiable(const std::vector<ClausePtr> &clauses)
        {
            // Check for empty clause
            for (const auto &clause : clauses)
            {
                if (clause->is_empty())
                {
                    return true;
                }
            }
            return false;
        }

        ClauseSetStats analyze_clause_set(const std::vector<ClausePtr> &clauses)
        {
            ClauseSetStats stats;
            stats.total_clauses = clauses.size();
            stats.unit_clauses = 0;
            stats.horn_clauses = 0;
            stats.max_clause_size = 0;

            size_t total_literals = 0;

            for (const auto &clause : clauses)
            {
                size_t size = clause->size();
                total_literals += size;

                if (size > stats.max_clause_size)
                {
                    stats.max_clause_size = size;
                }

                if (clause->is_unit())
                {
                    stats.unit_clauses++;
                }

                // Check if Horn clause (at most one positive literal)
                size_t positive_count = 0;
                for (const auto &lit : clause->literals())
                {
                    if (lit.is_positive())
                    {
                        positive_count++;
                    }
                }
                if (positive_count <= 1)
                {
                    stats.horn_clauses++;
                }
            }

            stats.avg_clause_size = clauses.empty() ? 0.0 : static_cast<double>(total_literals) / clauses.size();

            return stats;
        }

    } // namespace resolution_utils

    KBResult ResolutionProver::try_kb_preprocessing(std::vector<ClausePtr> &clauses)
    {
        // Extract equality equations from unit clauses
        auto equations = extract_equality_equations(clauses);

        if (equations.empty())
        {
            // No equality clauses to process
            return KBResult::make_success({}, "No equality clauses found");
        }

        if (equations.size() > config_.kb_max_equations)
        {
            // Too many equations - skip KB preprocessing
            return KBResult::make_failure("Too many equations for KB preprocessing");
        }

        // Configure KB with tight resource limits
        KBConfig kb_config = config_.kb_config;
        kb_config.max_time_seconds = config_.kb_preprocessing_timeout;
        kb_config.max_rules = config_.kb_max_rules;
        kb_config.verbose = false; // Keep quiet during preprocessing

        // Create term ordering and run KB completion
        auto ordering = std::make_shared<LexicographicPathOrdering>();
        KnuthBendixCompletion kb(ordering, kb_config);

        auto result = kb.complete(equations);

        if (result.status == KBResult::Status::SUCCESS && !result.final_rules.empty())
        {
            // KB succeeded - integrate rules back into clause set
            auto original_clauses = clauses; // Save original for debugging
            clauses = integrate_kb_rules(clauses, result.final_rules);

            // Simple debug output
            std::cout << "KB Debug: " << original_clauses.size() << " -> " << clauses.size()
                      << " clauses, " << result.final_rules.size() << " KB rules" << std::endl;
        }

        return result;
    }
    std::vector<Equation> ResolutionProver::extract_equality_equations(const std::vector<ClausePtr> &clauses)
    {
        std::vector<Equation> equations;

        for (const auto &clause : clauses)
        {
            if (is_unit_equality_clause(clause))
            {
                try
                {
                    equations.push_back(clause_to_equation(clause));
                }
                catch (const std::exception &e)
                {
                    // Skip malformed equality clauses
                    continue;
                }
            }
        }

        return equations;
    }

    std::vector<ClausePtr> ResolutionProver::integrate_kb_rules(const std::vector<ClausePtr> &original_clauses,
                                                                const std::vector<TermRewriteRule> &kb_rules)
    {
        std::vector<ClausePtr> updated_clauses;

        std::cout << "  Integration Debug:" << std::endl;
        std::cout << "    Original clauses: " << original_clauses.size() << std::endl;

        // Add non-equality clauses unchanged
        size_t non_equality_count = 0;
        for (const auto &clause : original_clauses)
        {
            if (!is_unit_equality_clause(clause))
            {
                updated_clauses.push_back(clause);
                non_equality_count++;
            }
            else
            {
                std::cout << "    Removing equality clause: " << clause->to_string() << std::endl;
            }
        }
        std::cout << "    Non-equality clauses kept: " << non_equality_count << std::endl;

        // Convert KB rules to clauses and add them
        std::cout << "    KB rules to convert: " << kb_rules.size() << std::endl;
        for (const auto &rule : kb_rules)
        {
            auto rule_clause = rule_to_clause(rule);
            if (rule_clause)
            {
                updated_clauses.push_back(rule_clause);
                std::cout << "    Added rule as clause: " << rule_clause->to_string() << std::endl;
            }
            else
            {
                std::cout << "    Failed to convert rule: " << rule.to_string() << std::endl;
            }
        }

        std::cout << "    Final clause count: " << updated_clauses.size() << std::endl;
        return updated_clauses;
    }

    bool ResolutionProver::is_unit_equality_clause(const ClausePtr &clause)
    {
        if (!clause || !clause->is_unit())
        {
            return false;
        }

        const auto &literal = clause->literals()[0];

        // Only process positive equality literals
        if (literal.is_negative())
        {
            return false;
        }

        const auto &atom = literal.atom();

        // Check if atom is an equality (function application with name "=")
        if (atom->kind() == TermDB::TermKind::FUNCTION_APPLICATION)
        {
            auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(atom);
            return func_app && func_app->symbol() == "=" && func_app->arguments().size() == 2;
        }

        return false;
    }

    Equation ResolutionProver::clause_to_equation(const ClausePtr &clause)
    {
        if (!is_unit_equality_clause(clause))
        {
            throw std::invalid_argument("Clause is not a unit equality clause");
        }

        const auto &literal = clause->literals()[0];
        const auto &atom = literal.atom();
        auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(atom);

        const auto &args = func_app->arguments();
        TermDBPtr left = args[0];
        TermDBPtr right = args[1];

        // Handle negation: ¬(s = t) becomes s ≠ t (not suitable for KB)
        if (literal.is_negative())
        {
            throw std::invalid_argument("Negative equality literal cannot be converted to equation");
        }

        return Equation(left, right, "clause_" + std::to_string(clause->hash()));
    }

    ClausePtr ResolutionProver::rule_to_clause(const TermRewriteRule &rule)
    {
        // Convert rule l → r to unit clause containing literal l = r
        auto equality_term = make_function_application("=", {rule.lhs(), rule.rhs()});
        Literal equality_literal(equality_term, true); // Positive literal

        return std::make_shared<Clause>(std::vector<Literal>{equality_literal});
    }

    void ResolutionProver::debug_kb_integration(const std::vector<ClausePtr> &before_clauses,
                                                const std::vector<ClausePtr> &after_clauses,
                                                const KBResult &kb_result)
    {
        std::cout << "\n=== KB Integration Debug ===" << std::endl;
        std::cout << "Clauses before KB: " << before_clauses.size() << std::endl;
        for (size_t i = 0; i < before_clauses.size(); ++i)
        {
            std::cout << "  " << i << ": " << before_clauses[i]->to_string() << std::endl;
        }

        std::cout << "KB Status: ";
        switch (kb_result.status)
        {
        case KBResult::Status::SUCCESS:
            std::cout << "SUCCESS";
            break;
        case KBResult::Status::FAILURE:
            std::cout << "FAILURE";
            break;
        case KBResult::Status::TIMEOUT:
            std::cout << "TIMEOUT";
            break;
        default:
            std::cout << "OTHER";
            break;
        }
        std::cout << std::endl;

        std::cout << "KB Rules generated: " << kb_result.final_rules.size() << std::endl;
        for (size_t i = 0; i < kb_result.final_rules.size(); ++i)
        {
            std::cout << "  Rule " << i << ": " << kb_result.final_rules[i].to_string() << std::endl;
        }

        std::cout << "Clauses after KB: " << after_clauses.size() << std::endl;
        for (size_t i = 0; i < after_clauses.size(); ++i)
        {
            std::cout << "  " << i << ": " << after_clauses[i]->to_string() << std::endl;
        }
        std::cout << "=========================" << std::endl;
    }
} // namespace theorem_prover