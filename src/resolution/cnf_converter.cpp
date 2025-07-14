#include "cnf_converter.hpp"
#include "../utils/gensym.hpp"
#include <algorithm>
#include <sstream>
#include <stack>

namespace theorem_prover
{

    std::vector<ClausePtr> CNFConverter::to_cnf(const TermDBPtr &formula)
    {
        return to_cnf_with_renaming(formula, 0);
    }

    std::vector<ClausePtr> CNFConverter::to_cnf_with_renaming(const TermDBPtr &formula,
                                                              std::size_t variable_offset)
    {
        // Step 1: Eliminate implications
        auto step1 = eliminate_implications(formula);

        // Step 2: Move negations inward
        auto step2 = move_negations_inward(step1);

        // Step 3: Standardize variables
        std::size_t var_counter = variable_offset;
        auto step3 = standardize_variables(step2, var_counter);

        // Step 4: Convert to prenex form
        auto step4 = to_prenex_form(step3);

        // Step 5: Skolemize
        std::size_t skolem_counter = 0;
        std::vector<std::size_t> universal_vars;
        auto step5 = skolemize(step4, universal_vars, skolem_counter);

        // Step 6: Distribute OR over AND
        auto step6 = distribute_or_over_and(step5);

        // Step 7: Extract clauses
        return extract_clauses(step6);
    }

    TermDBPtr CNFConverter::eliminate_implications(const TermDBPtr &formula)
    {
        switch (formula->kind())
        {
        case TermDB::TermKind::IMPLIES:
        {
            auto implies = std::dynamic_pointer_cast<ImpliesDB>(formula);
            auto antecedent = eliminate_implications(implies->antecedent());
            auto consequent = eliminate_implications(implies->consequent());

            // P → Q becomes ¬P ∨ Q
            auto neg_antecedent = make_not(antecedent);
            return make_or(neg_antecedent, consequent);
        }

        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(formula);
            auto left = eliminate_implications(and_term->left());
            auto right = eliminate_implications(and_term->right());
            return make_and(left, right);
        }

        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(formula);
            auto left = eliminate_implications(or_term->left());
            auto right = eliminate_implications(or_term->right());
            return make_or(left, right);
        }

        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(formula);
            auto body = eliminate_implications(not_term->body());
            return make_not(body);
        }

        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(formula);
            auto body = eliminate_implications(forall->body());
            return make_forall(forall->variable_hint(), body);
        }

        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(formula);
            auto body = eliminate_implications(exists->body());
            return make_exists(exists->variable_hint(), body);
        }

        default:
            // Variables, constants, function applications remain unchanged
            return formula;
        }
    }

    TermDBPtr CNFConverter::move_negations_inward(const TermDBPtr &formula)
    {
        switch (formula->kind())
        {
        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(formula);
            auto body = not_term->body();

            switch (body->kind())
            {
            case TermDB::TermKind::NOT:
            {
                // ¬¬P becomes P
                auto inner_not = std::dynamic_pointer_cast<NotDB>(body);
                return move_negations_inward(inner_not->body());
            }

            case TermDB::TermKind::AND:
            {
                // ¬(P ∧ Q) becomes ¬P ∨ ¬Q
                auto and_term = std::dynamic_pointer_cast<AndDB>(body);
                auto neg_left = make_not(and_term->left());
                auto neg_right = make_not(and_term->right());
                auto moved_left = move_negations_inward(neg_left);
                auto moved_right = move_negations_inward(neg_right);
                return make_or(moved_left, moved_right);
            }

            case TermDB::TermKind::OR:
            {
                // ¬(P ∨ Q) becomes ¬P ∧ ¬Q
                auto or_term = std::dynamic_pointer_cast<OrDB>(body);
                auto neg_left = make_not(or_term->left());
                auto neg_right = make_not(or_term->right());
                auto moved_left = move_negations_inward(neg_left);
                auto moved_right = move_negations_inward(neg_right);
                return make_and(moved_left, moved_right);
            }

            case TermDB::TermKind::FORALL:
            {
                // ¬∀x.P(x) becomes ∃x.¬P(x)
                auto forall = std::dynamic_pointer_cast<ForallDB>(body);
                auto neg_body = make_not(forall->body());
                auto moved_body = move_negations_inward(neg_body);
                return make_exists(forall->variable_hint(), moved_body);
            }

            case TermDB::TermKind::EXISTS:
            {
                // ¬∃x.P(x) becomes ∀x.¬P(x)
                auto exists = std::dynamic_pointer_cast<ExistsDB>(body);
                auto neg_body = make_not(exists->body());
                auto moved_body = move_negations_inward(neg_body);
                return make_forall(exists->variable_hint(), moved_body);
            }

            default:
                // Negation of atomic formula - keep as is
                return formula;
            }
        }

        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(formula);
            auto left = move_negations_inward(and_term->left());
            auto right = move_negations_inward(and_term->right());
            return make_and(left, right);
        }

        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(formula);
            auto left = move_negations_inward(or_term->left());
            auto right = move_negations_inward(or_term->right());
            return make_or(left, right);
        }

        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(formula);
            auto body = move_negations_inward(forall->body());
            return make_forall(forall->variable_hint(), body);
        }

        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(formula);
            auto body = move_negations_inward(exists->body());
            return make_exists(exists->variable_hint(), body);
        }

        default:
            return formula;
        }
    }

    TermDBPtr CNFConverter::standardize_variables(const TermDBPtr &formula,
                                                  std::size_t &variable_counter)
    {
        switch (formula->kind())
        {
        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(formula);

            // Create fresh variable for this quantifier
            std::size_t fresh_var = variable_counter++;

            // Create substitution for bound variable (De Bruijn index 0)
            SubstitutionMap subst;
            subst[0] = make_variable(fresh_var);

            // Apply substitution to body and continue standardization
            auto substituted_body = SubstitutionEngine::substitute(forall->body(), subst);
            return standardize_variables(substituted_body, variable_counter);
        }

        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(formula);

            // Create fresh variable for this quantifier
            std::size_t fresh_var = variable_counter++;

            // Create substitution for bound variable (De Bruijn index 0)
            SubstitutionMap subst;
            subst[0] = make_variable(fresh_var);

            // Apply substitution to body and continue standardization
            auto substituted_body = SubstitutionEngine::substitute(exists->body(), subst);
            return standardize_variables(substituted_body, variable_counter);
        }

        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(formula);
            auto left = standardize_variables(and_term->left(), variable_counter);
            auto right = standardize_variables(and_term->right(), variable_counter);
            return make_and(left, right);
        }

        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(formula);
            auto left = standardize_variables(or_term->left(), variable_counter);
            auto right = standardize_variables(or_term->right(), variable_counter);
            return make_or(left, right);
        }

        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(formula);
            auto body = standardize_variables(not_term->body(), variable_counter);
            return make_not(body);
        }

        default:
            return formula;
        }
    }

    TermDBPtr CNFConverter::to_prenex_form(const TermDBPtr &formula)
    {
        // Simplified prenex form conversion
        // In practice, this would need to handle variable conflicts and proper quantifier extraction
        return formula;
    }

    TermDBPtr CNFConverter::skolemize(const TermDBPtr &formula,
                                      const std::vector<std::size_t> &universal_vars,
                                      std::size_t &skolem_counter)
    {
        switch (formula->kind())
        {
        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(formula);

            // Create Skolem function
            std::string skolem_name = generate_skolem_name(skolem_counter++);

            // Create Skolem term - either constant or function depending on universal variables
            TermDBPtr skolem_term;
            if (universal_vars.empty())
            {
                // No universal variables - use Skolem constant
                skolem_term = make_constant(skolem_name);
            }
            else
            {
                // Create Skolem function with universal variables as arguments
                std::vector<TermDBPtr> args;
                for (auto var_idx : universal_vars)
                {
                    args.push_back(make_variable(var_idx));
                }
                skolem_term = make_function_application(skolem_name, args);
            }

            // Substitute the existential variable with the Skolem term
            SubstitutionMap subst;
            subst[0] = skolem_term; // Replace De Bruijn index 0

            auto substituted_body = SubstitutionEngine::substitute(exists->body(), subst);

            // Continue skolemization on the body
            return skolemize(substituted_body, universal_vars, skolem_counter);
        }

        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(formula);

            // Add this variable to universal context
            std::vector<std::size_t> new_universal_vars = universal_vars;
            new_universal_vars.push_back(0); // De Bruijn index for this quantifier

            auto body = skolemize(forall->body(), new_universal_vars, skolem_counter);
            return make_forall(forall->variable_hint(), body);
        }

        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(formula);
            auto left = skolemize(and_term->left(), universal_vars, skolem_counter);
            auto right = skolemize(and_term->right(), universal_vars, skolem_counter);
            return make_and(left, right);
        }

        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(formula);
            auto left = skolemize(or_term->left(), universal_vars, skolem_counter);
            auto right = skolemize(or_term->right(), universal_vars, skolem_counter);
            return make_or(left, right);
        }

        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(formula);
            auto body = skolemize(not_term->body(), universal_vars, skolem_counter);
            return make_not(body);
        }

        default:
            return formula;
        }
    }

    TermDBPtr CNFConverter::distribute_or_over_and(const TermDBPtr &formula)
    {
        switch (formula->kind())
        {
        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(formula);
            auto left = distribute_or_over_and(or_term->left());
            auto right = distribute_or_over_and(or_term->right());

            // Check if either side is an AND that needs distribution
            if (left->kind() == TermDB::TermKind::AND)
            {
                // (P ∧ Q) ∨ R becomes (P ∨ R) ∧ (Q ∨ R)
                auto and_left = std::dynamic_pointer_cast<AndDB>(left);
                auto dist_left = make_or(and_left->left(), right);
                auto dist_right = make_or(and_left->right(), right);
                auto result_left = distribute_or_over_and(dist_left);
                auto result_right = distribute_or_over_and(dist_right);
                return make_and(result_left, result_right);
            }

            if (right->kind() == TermDB::TermKind::AND)
            {
                // P ∨ (Q ∧ R) becomes (P ∨ Q) ∧ (P ∨ R)
                auto and_right = std::dynamic_pointer_cast<AndDB>(right);
                auto dist_left = make_or(left, and_right->left());
                auto dist_right = make_or(left, and_right->right());
                auto result_left = distribute_or_over_and(dist_left);
                auto result_right = distribute_or_over_and(dist_right);
                return make_and(result_left, result_right);
            }

            return make_or(left, right);
        }

        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(formula);
            auto left = distribute_or_over_and(and_term->left());
            auto right = distribute_or_over_and(and_term->right());
            return make_and(left, right);
        }

        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(formula);
            auto body = distribute_or_over_and(not_term->body());
            return make_not(body);
        }

        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(formula);
            auto body = distribute_or_over_and(forall->body());
            return make_forall(forall->variable_hint(), body);
        }

        default:
            return formula;
        }
    }

    std::vector<ClausePtr> CNFConverter::extract_clauses(const TermDBPtr &cnf_formula)
    {
        std::vector<ClausePtr> clauses;

        // Handle the case where the entire formula is one clause
        if (cnf_formula->kind() != TermDB::TermKind::AND)
        {
            auto literals = extract_literals(cnf_formula);
            clauses.push_back(std::make_shared<Clause>(literals));
            return clauses;
        }

        // Handle conjunction of clauses
        std::stack<TermDBPtr> to_process;
        to_process.push(cnf_formula);

        while (!to_process.empty())
        {
            auto current = to_process.top();
            to_process.pop();

            if (current->kind() == TermDB::TermKind::AND)
            {
                auto and_term = std::dynamic_pointer_cast<AndDB>(current);
                to_process.push(and_term->left());
                to_process.push(and_term->right());
            }
            else
            {
                // This should be a disjunction or literal
                auto literals = extract_literals(current);
                clauses.push_back(std::make_shared<Clause>(literals));
            }
        }

        return clauses;
    }

    std::vector<Literal> CNFConverter::extract_literals(const TermDBPtr &disjunction)
    {
        std::vector<Literal> literals;

        if (disjunction->kind() != TermDB::TermKind::OR)
        {
            // Single literal
            if (disjunction->kind() == TermDB::TermKind::NOT)
            {
                auto not_term = std::dynamic_pointer_cast<NotDB>(disjunction);
                literals.emplace_back(not_term->body(), false);
            }
            else
            {
                literals.emplace_back(disjunction, true);
            }
            return literals;
        }

        // Handle disjunction
        std::stack<TermDBPtr> to_process;
        to_process.push(disjunction);

        while (!to_process.empty())
        {
            auto current = to_process.top();
            to_process.pop();

            if (current->kind() == TermDB::TermKind::OR)
            {
                auto or_term = std::dynamic_pointer_cast<OrDB>(current);
                to_process.push(or_term->left());
                to_process.push(or_term->right());
            }
            else
            {
                // This is a literal
                if (current->kind() == TermDB::TermKind::NOT)
                {
                    auto not_term = std::dynamic_pointer_cast<NotDB>(current);
                    literals.emplace_back(not_term->body(), false);
                }
                else
                {
                    literals.emplace_back(current, true);
                }
            }
        }

        return literals;
    }

    std::string CNFConverter::generate_skolem_name(std::size_t counter)
    {
        return "sk" + std::to_string(counter);
    }

    std::vector<std::size_t> CNFConverter::find_free_variables(const TermDBPtr &term,
                                                               std::size_t depth)
    {
        // Simplified implementation - would need complete traversal
        std::vector<std::size_t> vars;
        // TODO: Implement full variable discovery
        return vars;
    }

    // SkolemManager implementation
    TermDBPtr SkolemManager::create_skolem_function(const std::vector<std::size_t> &universal_vars)
    {
        std::string name = next_skolem_name();

        if (universal_vars.empty())
        {
            return make_constant(name);
        }
        else
        {
            std::vector<TermDBPtr> args;
            for (auto var_idx : universal_vars)
            {
                args.push_back(make_variable(var_idx));
            }
            return make_function_application(name, args);
        }
    }

    std::string SkolemManager::next_skolem_name()
    {
        return "sk" + std::to_string(counter_++);
    }

} // namespace theorem_prover