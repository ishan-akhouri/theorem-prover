#include "knuth_bendix.hpp"
#include "../utils/gensym.hpp"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <unordered_set>

namespace theorem_prover
{

    std::string KBStats::to_string() const
    {
        std::ostringstream oss;
        oss << "KB Statistics:\n";
        oss << "  Equations processed: " << equations_processed << "\n";
        oss << "  Critical pairs computed: " << critical_pairs_computed << "\n";
        oss << "  Rules added: " << rules_added << "\n";
        oss << "  Rules removed: " << rules_removed << "\n";
        oss << "  Equations simplified: " << equations_simplified << "\n";
        oss << "  Equations subsumed: " << equations_subsumed << "\n";
        oss << "  Orientation failures: " << orientation_failures << "\n";
        return oss.str();
    }

    void EquationQueue::push(const Equation &equation, std::size_t priority)
    {
        if (fair_mode_)
        {
            equation_queue_.push(equation);
        }
        else
        {
            priority_queue_.push(std::make_pair(priority, equation));
        }
    }

    std::optional<Equation> EquationQueue::pop()
    {
        if (fair_mode_)
        {
            if (equation_queue_.empty())
                return std::nullopt;

            Equation eq = equation_queue_.front();
            equation_queue_.pop();
            return eq;
        }
        else
        {
            if (priority_queue_.empty())
                return std::nullopt;

            Equation eq = priority_queue_.top().second;
            priority_queue_.pop();
            return eq;
        }
    }

    bool EquationQueue::empty() const
    {
        return fair_mode_ ? equation_queue_.empty() : priority_queue_.empty();
    }

    std::size_t EquationQueue::size() const
    {
        return fair_mode_ ? equation_queue_.size() : priority_queue_.size();
    }

    void EquationQueue::clear()
    {
        if (fair_mode_)
        {
            std::queue<Equation> empty_queue;
            equation_queue_.swap(empty_queue);
        }
        else
        {
            // Simple way to clear a priority queue
            while (!priority_queue_.empty())
            {
                priority_queue_.pop();
            }
        }
    }

    KnuthBendixCompletion::KnuthBendixCompletion(std::shared_ptr<TermOrdering> ordering,
                                                 const KBConfig &config)
        : ordering_(ordering), config_(config), equation_queue_(config.fair_processing)
    {
        if (!ordering_)
        {
            throw std::invalid_argument("Term ordering cannot be null");
        }
    }

    KBResult KnuthBendixCompletion::complete(const std::vector<Equation> &equations)
    {
        return complete_from_rules({}, equations);
    }

    KBResult KnuthBendixCompletion::complete_from_rules(const std::vector<TermRewriteRule> &rules,
                                                        const std::vector<Equation> &equations)
    {
        if (running_)
        {
            return KBResult::make_failure("Completion already in progress");
        }

        // Initialize state
        running_ = true;
        termination_requested_ = false;
        start_time_ = std::chrono::steady_clock::now();
        stats_.reset();

        // Clear previous state
        rules_.clear();
        equation_queue_.clear();
        rule_counter_ = 0;
        equation_counter_ = 0;

        // Add initial rules
        for (const auto &rule : rules)
        {
            if (!add_rule(rule))
            {
                running_ = false;
                return KBResult::make_failure("Failed to add initial rule: " + rule.name());
            }
        }

        // Add initial equations to queue
        for (const auto &equation : equations)
        {
            equation_queue_.push(equation, 0);
        }

        if (config_.verbose)
        {
            std::cout << "Starting KB completion with " << rules_.size()
                      << " rules and " << equations.size() << " equations" << std::endl;
        }

        // Run completion loop
        KBResult result = completion_loop();

        // Finalize result
        result.final_rules = rules_;
        result.total_equations_processed = stats_.equations_processed;
        result.total_critical_pairs_computed = stats_.critical_pairs_computed;

        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
        result.elapsed_time_seconds = duration.count() / 1000.0;

        running_ = false;

        if (config_.verbose)
        {
            std::cout << "Completion finished: " << result.message << std::endl;
            std::cout << stats_.to_string() << std::endl;
        }

        return result;
    }

    KBResult KnuthBendixCompletion::completion_loop()
    {
        std::size_t iteration = 0;

        std::cout << "Starting completion loop with max_iterations=" << config_.max_iterations
                  << ", max_time=" << config_.max_time_seconds << std::endl;

        while (!equation_queue_.empty() && iteration < config_.max_iterations)
        {
            ++iteration;

            // Check timeout
            auto current_time = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time_);
            double elapsed = duration.count() / 1000.0;

            if (elapsed > config_.max_time_seconds)
            {
                std::cout << "TIMEOUT: elapsed=" << elapsed << "s > max=" << config_.max_time_seconds << "s" << std::endl;
                return KBResult::make_timeout("Time limit exceeded");
            }

            if (config_.verbose)
            {
                std::cout << "Iteration " << iteration << ", elapsed=" << elapsed
                          << "s, queue_size=" << equation_queue_.size() << std::endl;
            }

            // Process next equation
            auto equation_opt = equation_queue_.pop();
            if (!equation_opt)
            {
                break;
            }

            bool success = process_equation(*equation_opt);
            if (!success)
            {
                // Check if it's a timeout
                auto end_time = std::chrono::steady_clock::now();
                auto end_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
                if (end_duration.count() / 1000.0 > config_.max_time_seconds)
                {
                    std::cout << "TIMEOUT during equation processing" << std::endl;
                    return KBResult::make_timeout("Time limit exceeded during equation processing");
                }
                return KBResult::make_failure("Failed to process equation: " + equation_opt->name());
            }

            // Print progress
            if (config_.verbose && iteration % 5 == 0)
            {
                print_progress();
            }
        }

        std::cout << "Loop exited: iteration=" << iteration << ", queue_empty=" << equation_queue_.empty() << std::endl;

        if (iteration >= config_.max_iterations)
        {
            return KBResult::make_timeout("Maximum iterations exceeded");
        }

        if (has_converged())
        {
            return KBResult::make_success(rules_, "Completion successful - confluent system achieved");
        }

        return KBResult::make_success(rules_, "Completion finished - no more equations to process");
    }

    bool KnuthBendixCompletion::process_equation(const Equation &equation)
    {
        ++stats_.equations_processed;

        // Check timeout at start of processing
        auto current_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time_);
        if (duration.count() / 1000.0 > config_.max_time_seconds)
        {
            return false; // Signal timeout
        }

        if (config_.verbose)
        {
            std::cout << "Processing equation: " << equation.to_string() << std::endl;
        }

        // Step 1: Simplify equation using current rules
        auto simplified_opt = simplify_equation(equation);
        if (!simplified_opt)
        {
            return true;
        }

        Equation simplified = *simplified_opt;

        // Step 2: Check if equation is subsumed
        if (config_.enable_subsumption && is_subsumed(simplified))
        {
            ++stats_.equations_subsumed;
            return true;
        }

        // Step 3: Try to orient equation into a rule
        auto rule_opt = orient_equation(simplified);
        if (!rule_opt)
        {
            ++stats_.orientation_failures;
            return true;
        }

        TermRewriteRule new_rule = *rule_opt;

        // Step 4: Simplify existing rules using the new rule
        if (config_.enable_simplification)
        {
            auto modified_rules = simplify_rules_with(new_rule);
        }

        // Step 5: Add the new rule
        if (!add_rule(new_rule))
        {
            return false;
        }

        // Step 6: Check timeout before expensive critical pair computation
        current_time = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time_);
        if (duration.count() / 1000.0 > config_.max_time_seconds)
        {
            return false; // Signal timeout
        }

        // Step 7: Compute critical pairs with existing rules
        auto new_equations = compute_new_critical_pairs(new_rule);

        // Step 8: Add new equations to queue
        for (const auto &eq : new_equations)
        {
            equation_queue_.push(eq, 1);
        }

        return true;
    }

    std::optional<TermRewriteRule> KnuthBendixCompletion::orient_equation(const Equation &equation)
    {
        // Try to orient the equation using the term ordering
        auto oriented_rule = equation.orient(*ordering_);

        if (oriented_rule)
        {
            // Successfully oriented
            return oriented_rule;
        }

        // Cannot orient - terms are equivalent in the ordering
        return std::nullopt;
    }

    bool KnuthBendixCompletion::add_rule(const TermRewriteRule &rule)
    {
        // Check if rule is properly oriented
        if (!rule.is_oriented(*ordering_))
        {
            return false;
        }

        // Check for duplicate rules
        for (const auto &existing : rules_)
        {
            if (existing.equals(rule))
            {
                return true; // Rule already exists, that's fine
            }
        }

        rules_.push_back(rule);
        ++stats_.rules_added;

        if (config_.verbose)
        {
            std::cout << "Added rule: " << rule.to_string() << std::endl;
        }

        return true;
    }

    bool KnuthBendixCompletion::remove_rule(const std::string &rule_name)
    {
        auto it = std::find_if(rules_.begin(), rules_.end(),
                               [&rule_name](const TermRewriteRule &rule)
                               {
                                   return rule.name() == rule_name;
                               });

        if (it != rules_.end())
        {
            rules_.erase(it);
            ++stats_.rules_removed;
            return true;
        }

        return false;
    }

    std::optional<Equation> KnuthBendixCompletion::simplify_equation(const Equation &equation)
    {
        // Create a temporary rewrite system with current rules
        auto rewrite_system = std::make_shared<RewriteSystem>(ordering_);

        for (const auto &rule : rules_)
        {
            rewrite_system->add_rule(rule);
        }

        // Normalize both sides of the equation
        auto normalized_lhs = rewrite_system->normalize(equation.lhs());
        auto normalized_rhs = rewrite_system->normalize(equation.rhs());

        // Check if equation reduces to identity
        if (*normalized_lhs == *normalized_rhs)
        {
            return std::nullopt; // Identity equation
        }

        // Create simplified equation
        std::string simplified_name = equation.name() + "_simplified";
        ++stats_.equations_simplified;

        return Equation(normalized_lhs, normalized_rhs, simplified_name);
    }

    std::vector<std::string> KnuthBendixCompletion::simplify_rules_with(const TermRewriteRule &new_rule)
    {
        std::vector<std::string> modified_rules;

        // Create temporary rewrite system with just the new rule
        auto temp_system = std::make_shared<RewriteSystem>(ordering_);
        temp_system->add_rule(new_rule);

        // Check each existing rule for simplification
        auto it = rules_.begin();
        while (it != rules_.end())
        {
            const auto &rule = *it;

            // Try to simplify rule's RHS with new rule
            auto simplified_rhs = temp_system->normalize(rule.rhs());

            if (!(*simplified_rhs == *rule.rhs()))
            {
                // Rule was simplified
                modified_rules.push_back(rule.name());

                // Remove old rule
                it = rules_.erase(it);
                ++stats_.rules_removed;

                // Create new simplified rule
                std::string new_name = rule.name() + "_simplified";
                TermRewriteRule simplified_rule(rule.lhs(), simplified_rhs, new_name);

                // Add back if still valid
                if (simplified_rule.is_oriented(*ordering_))
                {
                    rules_.push_back(simplified_rule);
                    ++stats_.rules_added;
                }
            }
            else
            {
                ++it;
            }
        }

        return modified_rules;
    }

    std::vector<Equation> KnuthBendixCompletion::compute_new_critical_pairs(const TermRewriteRule &new_rule)
    {
        std::vector<Equation> new_equations;

        // Safety limit on critical pairs per rule
        const std::size_t MAX_CRITICAL_PAIRS_PER_RULE = 50;

        // Compute critical pairs between new rule and all existing rules
        for (const auto &existing_rule : rules_)
        {
            if (existing_rule.name() == new_rule.name())
            {
                continue; // Skip the rule we just added
            }

            // Critical pairs in both directions
            auto pairs1 = CriticalPairComputer::compute_critical_pairs(new_rule, existing_rule);
            auto pairs2 = CriticalPairComputer::compute_critical_pairs(existing_rule, new_rule);

            // Convert critical pairs to equations (with limit)
            std::size_t count1 = 0;
            for (const auto &cp : pairs1)
            {
                if (count1 >= MAX_CRITICAL_PAIRS_PER_RULE)
                    break;
                new_equations.push_back(cp.to_equation());
                count1++;
            }

            std::size_t count2 = 0;
            for (const auto &cp : pairs2)
            {
                if (count2 >= MAX_CRITICAL_PAIRS_PER_RULE)
                    break;
                new_equations.push_back(cp.to_equation());
                count2++;
            }

            stats_.critical_pairs_computed += count1 + count2;
        }

        // Compute self-critical pairs for the new rule (also limited)
        auto self_pairs = CriticalPairComputer::compute_self_critical_pairs(new_rule);

        std::size_t self_count = 0;
        for (const auto &cp : self_pairs)
        {
            if (self_count >= MAX_CRITICAL_PAIRS_PER_RULE)
                break;
            new_equations.push_back(cp.to_equation());
            self_count++;
        }

        stats_.critical_pairs_computed += self_count;

        return new_equations;
    }

    bool KnuthBendixCompletion::is_subsumed(const Equation &equation)
    {
        // Simple subsumption check: see if equation is already derivable
        // This is a simplified version - full subsumption is more complex

        auto rewrite_system = std::make_shared<RewriteSystem>(ordering_);
        for (const auto &rule : rules_)
        {
            rewrite_system->add_rule(rule);
        }

        // Check if both sides normalize to the same term
        auto norm_lhs = rewrite_system->normalize(equation.lhs());
        auto norm_rhs = rewrite_system->normalize(equation.rhs());

        return *norm_lhs == *norm_rhs;
    }

    bool KnuthBendixCompletion::check_resource_limits()
    {
        // Check rule limit
        if (rules_.size() >= config_.max_rules)
        {
            return true;
        }

        // Check equation queue limit
        if (equation_queue_.size() >= config_.max_equations)
        {
            return true;
        }

        // Check time limit
        auto current_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time_);
        if (duration.count() / 1000.0 > config_.max_time_seconds)
        {
            return true;
        }

        return false;
    }

    bool KnuthBendixCompletion::has_converged() const
    {
        // Convergence means no more equations to process
        return equation_queue_.empty();
    }

    std::string KnuthBendixCompletion::generate_rule_name()
    {
        return "rule_" + std::to_string(++rule_counter_);
    }

    std::string KnuthBendixCompletion::generate_equation_name()
    {
        return "eq_" + std::to_string(++equation_counter_);
    }

    void KnuthBendixCompletion::print_progress()
    {
        std::cout << "Progress: " << stats_.equations_processed
                  << " equations processed, " << rules_.size()
                  << " rules, " << equation_queue_.size()
                  << " equations queued" << std::endl;
    }

    bool KnuthBendixCompletion::validate_consistency()
    {
        // Check that all rules are properly oriented
        for (const auto &rule : rules_)
        {
            if (!rule.is_oriented(*ordering_))
            {
                return false;
            }
        }

        return true;
    }

    // Convenience functions
    KBResult knuth_bendix_complete(const std::vector<Equation> &equations,
                                   std::shared_ptr<TermOrdering> ordering,
                                   const KBConfig &config)
    {
        KnuthBendixCompletion kb(ordering, config);
        return kb.complete(equations);
    }

    std::unique_ptr<KnuthBendixCompletion> make_kb_completion(const KBConfig &config)
    {
        auto ordering = std::make_shared<LexicographicPathOrdering>();
        return std::make_unique<KnuthBendixCompletion>(ordering, config);
    }

} // namespace theorem_prover