#include "unification.hpp"
#include <stdexcept>
#include <sstream>

namespace theorem_prover
{

    UnificationResult Unifier::unify(const TermDBPtr &term1,
                                     const TermDBPtr &term2,
                                     std::size_t depth)
    {
        SubstitutionMap substitution;

        if (unify_impl(term1, term2, substitution, depth))
        {
            return UnificationResult::make_success(substitution);
        }
        else
        {
            return UnificationResult::make_failure("Terms cannot be unified");
        }
    }

    bool Unifier::unifiable(const TermDBPtr &term1,
                            const TermDBPtr &term2,
                            std::size_t depth)
    {
        SubstitutionMap dummy_substitution;
        return unify_impl(term1, term2, dummy_substitution, depth);
    }

    bool Unifier::unify_impl(const TermDBPtr &term1,
                             const TermDBPtr &term2,
                             SubstitutionMap &substitution,
                             std::size_t depth)
    {
        // Base case: terms are already equal
        if (*term1 == *term2)
        {
            return true;
        }

        // Apply existing substitutions to both terms
        auto subst_term1 = SubstitutionEngine::substitute(term1, substitution, depth);
        auto subst_term2 = SubstitutionEngine::substitute(term2, substitution, depth);

        // If substitution made them equal, we're done
        if (*subst_term1 == *subst_term2)
        {
            return true;
        }

        // Case 1: First term is a free variable
        if (subst_term1->kind() == TermDB::TermKind::VARIABLE)
        {
            auto var1 = std::dynamic_pointer_cast<VariableDB>(subst_term1);
            if (is_free_variable(var1->index(), depth))
            {
                // Occurs check
                if (occurs_check(var1->index(), subst_term2, depth))
                {
                    return false;
                }

                // Add binding to substitution
                substitution[var1->index() - depth] = subst_term2;
                return true;
            }
        }

        // Case 2: Second term is a free variable
        if (subst_term2->kind() == TermDB::TermKind::VARIABLE)
        {
            auto var2 = std::dynamic_pointer_cast<VariableDB>(subst_term2);
            if (is_free_variable(var2->index(), depth))
            {
                // Occurs check
                if (occurs_check(var2->index(), subst_term1, depth))
                {
                    return false;
                }

                // Add binding to substitution
                substitution[var2->index() - depth] = subst_term1;
                return true;
            }
        }

        // Case 3: Both are constants
        if (subst_term1->kind() == TermDB::TermKind::CONSTANT &&
            subst_term2->kind() == TermDB::TermKind::CONSTANT)
        {
            auto const1 = std::dynamic_pointer_cast<ConstantDB>(subst_term1);
            auto const2 = std::dynamic_pointer_cast<ConstantDB>(subst_term2);
            return const1->symbol() == const2->symbol();
        }

        // Case 4: Both are function applications
        if (subst_term1->kind() == TermDB::TermKind::FUNCTION_APPLICATION &&
            subst_term2->kind() == TermDB::TermKind::FUNCTION_APPLICATION)
        {
            auto func1 = std::dynamic_pointer_cast<FunctionApplicationDB>(subst_term1);
            auto func2 = std::dynamic_pointer_cast<FunctionApplicationDB>(subst_term2);

            // Function symbols must match
            if (func1->symbol() != func2->symbol())
            {
                return false;
            }

            // Argument counts must match
            if (func1->arguments().size() != func2->arguments().size())
            {
                return false;
            }

            // Unify all arguments
            for (size_t i = 0; i < func1->arguments().size(); ++i)
            {
                if (!unify_impl(func1->arguments()[i], func2->arguments()[i],
                                substitution, depth))
                {
                    return false;
                }
            }

            return true;
        }

        // Case 5: Both are quantifiers (same type)
        if (subst_term1->kind() == TermDB::TermKind::FORALL &&
            subst_term2->kind() == TermDB::TermKind::FORALL)
        {
            auto forall1 = std::dynamic_pointer_cast<ForallDB>(subst_term1);
            auto forall2 = std::dynamic_pointer_cast<ForallDB>(subst_term2);

            // Unify bodies at increased depth
            return unify_impl(forall1->body(), forall2->body(), substitution, depth + 1);
        }

        if (subst_term1->kind() == TermDB::TermKind::EXISTS &&
            subst_term2->kind() == TermDB::TermKind::EXISTS)
        {
            auto exists1 = std::dynamic_pointer_cast<ExistsDB>(subst_term1);
            auto exists2 = std::dynamic_pointer_cast<ExistsDB>(subst_term2);

            // Unify bodies at increased depth
            return unify_impl(exists1->body(), exists2->body(), substitution, depth + 1);
        }

        // Case 6: Both are logical connectives
        if (subst_term1->kind() == TermDB::TermKind::AND &&
            subst_term2->kind() == TermDB::TermKind::AND)
        {
            auto and1 = std::dynamic_pointer_cast<AndDB>(subst_term1);
            auto and2 = std::dynamic_pointer_cast<AndDB>(subst_term2);

            return unify_impl(and1->left(), and2->left(), substitution, depth) &&
                   unify_impl(and1->right(), and2->right(), substitution, depth);
        }

        if (subst_term1->kind() == TermDB::TermKind::OR &&
            subst_term2->kind() == TermDB::TermKind::OR)
        {
            auto or1 = std::dynamic_pointer_cast<OrDB>(subst_term1);
            auto or2 = std::dynamic_pointer_cast<OrDB>(subst_term2);

            return unify_impl(or1->left(), or2->left(), substitution, depth) &&
                   unify_impl(or1->right(), or2->right(), substitution, depth);
        }

        if (subst_term1->kind() == TermDB::TermKind::IMPLIES &&
            subst_term2->kind() == TermDB::TermKind::IMPLIES)
        {
            auto impl1 = std::dynamic_pointer_cast<ImpliesDB>(subst_term1);
            auto impl2 = std::dynamic_pointer_cast<ImpliesDB>(subst_term2);

            return unify_impl(impl1->antecedent(), impl2->antecedent(), substitution, depth) &&
                   unify_impl(impl1->consequent(), impl2->consequent(), substitution, depth);
        }

        if (subst_term1->kind() == TermDB::TermKind::NOT &&
            subst_term2->kind() == TermDB::TermKind::NOT)
        {
            auto not1 = std::dynamic_pointer_cast<NotDB>(subst_term1);
            auto not2 = std::dynamic_pointer_cast<NotDB>(subst_term2);

            return unify_impl(not1->body(), not2->body(), substitution, depth);
        }

        // Terms have different structures and cannot be unified
        return false;
    }

    bool Unifier::occurs_check(std::size_t var_index,
                               const TermDBPtr &term,
                               std::size_t depth)
    {
        switch (term->kind())
        {
        case TermDB::TermKind::VARIABLE:
        {
            auto var = std::dynamic_pointer_cast<VariableDB>(term);
            return is_free_variable(var->index(), depth) &&
                   (var->index() - depth) == (var_index - depth);
        }

        case TermDB::TermKind::CONSTANT:
            return false;

        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto func = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            for (const auto &arg : func->arguments())
            {
                if (occurs_check(var_index, arg, depth))
                {
                    return true;
                }
            }
            return false;
        }

        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(term);
            return occurs_check(var_index, forall->body(), depth + 1);
        }

        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(term);
            return occurs_check(var_index, exists->body(), depth + 1);
        }

        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(term);
            return occurs_check(var_index, and_term->left(), depth) ||
                   occurs_check(var_index, and_term->right(), depth);
        }

        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(term);
            return occurs_check(var_index, or_term->left(), depth) ||
                   occurs_check(var_index, or_term->right(), depth);
        }

        case TermDB::TermKind::IMPLIES:
        {
            auto implies = std::dynamic_pointer_cast<ImpliesDB>(term);
            return occurs_check(var_index, implies->antecedent(), depth) ||
                   occurs_check(var_index, implies->consequent(), depth);
        }

        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(term);
            return occurs_check(var_index, not_term->body(), depth);
        }

        default:
            return false;
        }
    }

    bool Unifier::is_free_variable(std::size_t var_index, std::size_t depth)
    {
        return var_index >= depth;
    }

    SubstitutionMap Unifier::compose_substitutions(const SubstitutionMap &subst1,
                                                   const SubstitutionMap &subst2)
    {
        SubstitutionMap result;

        // First, add all bindings from subst1
        for (const auto &[var, term] : subst1)
        {
            // Apply subst2 to the term from subst1
            result[var] = SubstitutionEngine::substitute(term, subst2, 0);
        }

        // Then add bindings from subst2, applying subst1 to their terms
        for (const auto &[var, term] : subst2)
        {
            if (result.find(var) == result.end())
            {
                // Apply subst1 to the term from subst2
                result[var] = SubstitutionEngine::substitute(term, subst1, 0);
            }
        }

        return result;
    }

} // namespace theorem_prover