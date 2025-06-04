#include "substitution.hpp"
#include <stdexcept>
#include <iostream>

namespace theorem_prover
{

    TermDBPtr SubstitutionEngine::shift(const TermDBPtr &term, int amount, std::size_t cutoff)
    {
        // Base case: no shift needed
        if (amount == 0)
        {
            return term;
        }

        switch (term->kind())
        {
        case TermDB::TermKind::VARIABLE:
        {
            auto var = std::dynamic_pointer_cast<VariableDB>(term);
            std::size_t index = var->index();

            if (index >= cutoff)
            {
                // Only shift free variables (index >= cutoff)
                int new_index = static_cast<int>(index) + amount;
                if (new_index < 0)
                {
                    throw std::runtime_error("Variable index would become negative after shift");
                }
                return make_variable(static_cast<std::size_t>(new_index));
            }
            else
            {
                // Leave bound variables unchanged
                return term;
            }
        }
        case TermDB::TermKind::CONSTANT:
            // Constants don't have indices to shift
            return term;

        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            std::vector<TermDBPtr> shifted_args;
            shifted_args.reserve(app->arguments().size());

            for (const auto &arg : app->arguments())
            {
                shifted_args.push_back(shift(arg, amount, cutoff));
            }

            return make_function_application(app->symbol(), shifted_args);
        }
        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(term);
            // Increase cutoff when entering a binder
            auto shifted_body = shift(forall->body(), amount, cutoff + 1);
            return make_forall(forall->variable_hint(), shifted_body);
        }
        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(term);
            // Increase cutoff when entering a binder
            auto shifted_body = shift(exists->body(), amount, cutoff + 1);
            return make_exists(exists->variable_hint(), shifted_body);
        }
        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(term);
            auto shifted_left = shift(and_term->left(), amount, cutoff);
            auto shifted_right = shift(and_term->right(), amount, cutoff);
            return make_and(shifted_left, shifted_right);
        }
        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(term);
            auto shifted_left = shift(or_term->left(), amount, cutoff);
            auto shifted_right = shift(or_term->right(), amount, cutoff);
            return make_or(shifted_left, shifted_right);
        }
        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(term);
            auto shifted_body = shift(not_term->body(), amount, cutoff);
            return make_not(shifted_body);
        }
        case TermDB::TermKind::IMPLIES:
        {
            auto implies = std::dynamic_pointer_cast<ImpliesDB>(term);
            auto shifted_antecedent = shift(implies->antecedent(), amount, cutoff);
            auto shifted_consequent = shift(implies->consequent(), amount, cutoff);
            return make_implies(shifted_antecedent, shifted_consequent);
        }
        default:
            throw std::runtime_error("Unsupported term kind in shift operation");
        }
    }

    TermDBPtr SubstitutionEngine::substitute(const TermDBPtr &term,
                                             const SubstitutionMap &subst,
                                             std::size_t depth)
    {
        // Early return if substitution map is empty
        if (subst.empty())
        {
            return term;
        }

        switch (term->kind())
        {
        case TermDB::TermKind::VARIABLE:
        {
            auto var = std::dynamic_pointer_cast<VariableDB>(term);
            std::size_t index = var->index();

            // Check if this is a bound variable
            if (index < depth)
            {
                // Bound variables are never substituted
                return term;
            }

            // For free variables, adjust the index by the current depth
            std::size_t free_index = index - depth;
            auto it = subst.find(free_index);
            if (it != subst.end())
            {
                // Found a substitution, shift the replacement term to avoid variable capture
                return shift(it->second, static_cast<int>(depth), depth);
            }

            // No substitution found
            return term;
        }
        case TermDB::TermKind::CONSTANT:
            // Constants are never substituted
            return term;

        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            std::vector<TermDBPtr> subst_args;
            subst_args.reserve(app->arguments().size());

            bool any_changed = false;
            for (const auto &arg : app->arguments())
            {
                auto subst_arg = substitute(arg, subst, depth);
                any_changed = any_changed || (subst_arg != arg);
                subst_args.push_back(subst_arg);
            }

            if (!any_changed)
            {
                return term;
            }

            return make_function_application(app->symbol(), subst_args);
        }
        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(term);

            // Increment depth when entering a binder
            auto subst_body = substitute(forall->body(), subst, depth + 1);

            if (subst_body == forall->body())
            {
                return term;
            }

            return make_forall(forall->variable_hint(), subst_body);
        }
        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(term);

            // Increment depth when entering a binder
            auto subst_body = substitute(exists->body(), subst, depth + 1);

            if (subst_body == exists->body())
            {
                return term;
            }

            return make_exists(exists->variable_hint(), subst_body);
        }
        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(term);
            auto subst_left = substitute(and_term->left(), subst, depth);
            auto subst_right = substitute(and_term->right(), subst, depth);

            if (subst_left == and_term->left() && subst_right == and_term->right())
            {
                return term;
            }

            return make_and(subst_left, subst_right);
        }
        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(term);
            auto subst_left = substitute(or_term->left(), subst, depth);
            auto subst_right = substitute(or_term->right(), subst, depth);

            if (subst_left == or_term->left() && subst_right == or_term->right())
            {
                return term;
            }

            return make_or(subst_left, subst_right);
        }
        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(term);
            auto subst_body = substitute(not_term->body(), subst, depth);

            if (subst_body == not_term->body())
            {
                return term;
            }

            return make_not(subst_body);
        }
        case TermDB::TermKind::IMPLIES:
        {
            auto implies = std::dynamic_pointer_cast<ImpliesDB>(term);
            auto subst_antecedent = substitute(implies->antecedent(), subst, depth);
            auto subst_consequent = substitute(implies->consequent(), subst, depth);

            if (subst_antecedent == implies->antecedent() &&
                subst_consequent == implies->consequent())
            {
                return term;
            }

            return make_implies(subst_antecedent, subst_consequent);
        }
        default:
            throw std::runtime_error("Unsupported term kind in substitute operation");
        }
    }

} // namespace theorem_prover