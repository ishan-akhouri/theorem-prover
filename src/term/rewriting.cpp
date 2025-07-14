#include "rewriting.hpp"
#include "substitution.hpp"
#include "unification.hpp"
#include "../utils/gensym.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace theorem_prover
{

    TermRewriteRule::TermRewriteRule(const TermDBPtr &lhs, const TermDBPtr &rhs, const std::string &name)
        : lhs_(lhs), rhs_(rhs), name_(name.empty() ? "unnamed_rule" : name) {}

    bool TermRewriteRule::is_oriented(const TermOrdering &ordering) const
    {
        return ordering.greater(lhs_, rhs_);
    }

    std::optional<TermRewriteRule> TermRewriteRule::orient(const TermOrdering &ordering) const
    {
        if (ordering.greater(lhs_, rhs_))
        {
            return *this; // Already correctly oriented
        }
        else if (ordering.greater(rhs_, lhs_))
        {
            return TermRewriteRule(rhs_, lhs_, name_); // Swap sides
        }
        else
        {
            return std::nullopt; // Terms are equivalent, cannot orient
        }
    }

    bool TermRewriteRule::equals(const TermRewriteRule &other) const
    {
        return *lhs_ == *other.lhs_ && *rhs_ == *other.rhs_;
    }

    std::string TermRewriteRule::to_string() const
    {
        std::ostringstream oss;
        oss << name_ << ": ";

        // Simple term printing for now
        if (lhs_->kind() == TermDB::TermKind::CONSTANT)
        {
            auto const_lhs = std::dynamic_pointer_cast<ConstantDB>(lhs_);
            oss << const_lhs->symbol();
        }
        else
        {
            oss << "term_" << lhs_->hash();
        }

        oss << " → ";

        if (rhs_->kind() == TermDB::TermKind::CONSTANT)
        {
            auto const_rhs = std::dynamic_pointer_cast<ConstantDB>(rhs_);
            oss << const_rhs->symbol();
        }
        else
        {
            oss << "term_" << rhs_->hash();
        }

        return oss.str();
    }

    bool Position::is_prefix_of(const Position &other) const
    {
        if (path_.size() > other.path_.size())
            return false;

        for (size_t i = 0; i < path_.size(); ++i)
        {
            if (path_[i] != other.path_[i])
                return false;
        }

        return true;
    }

    std::string Position::to_string() const
    {
        if (is_root())
            return "[]";

        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < path_.size(); ++i)
        {
            if (i > 0)
                oss << ".";
            oss << path_[i];
        }
        oss << "]";
        return oss.str();
    }

    RewriteSystem::RewriteSystem(std::shared_ptr<TermOrdering> ordering)
        : ordering_(ordering) {}

    bool RewriteSystem::add_rule(const TermDBPtr &lhs, const TermDBPtr &rhs, const std::string &name)
    {
        TermRewriteRule temp_rule(lhs, rhs, name.empty() ? generate_rule_name() : name);
        auto oriented = temp_rule.orient(*ordering_);

        if (!oriented)
        {
            return false; // Cannot orient (terms are equivalent)
        }

        return add_rule(*oriented);
    }

    bool RewriteSystem::add_rule(const TermRewriteRule &rule)
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
                return false; // Rule already exists
            }
        }

        rules_.push_back(rule);
        return true;
    }

    bool RewriteSystem::remove_rule(const std::string &name)
    {
        auto it = std::find_if(rules_.begin(), rules_.end(),
                               [&name](const TermRewriteRule &rule)
                               {
                                   return rule.name() == name;
                               });

        if (it != rules_.end())
        {
            rules_.erase(it);
            return true;
        }

        return false;
    }

    RewriteResult RewriteSystem::rewrite_step(const TermDBPtr &term) const
    {
        // Try to apply rules at root position first
        auto root_result = rewrite_at(term, Position());
        if (root_result.success)
        {
            return root_result;
        }

        // Recursively try at subterm positions
        switch (term->kind())
        {
        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            for (size_t i = 0; i < func_app->arguments().size(); ++i)
            {
                auto sub_result = rewrite_step(func_app->arguments()[i]);
                if (sub_result.success)
                {
                    // Rebuild term with rewritten subterm
                    std::vector<TermDBPtr> new_args = func_app->arguments();
                    new_args[i] = sub_result.result;

                    auto new_term = make_function_application(func_app->symbol(), new_args);
                    return RewriteResult::success_at(new_term,
                                                     Position().descend(i),
                                                     sub_result.rule_name);
                }
            }
            break;
        }

        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(term);

            // Try left side
            auto left_result = rewrite_step(and_term->left());
            if (left_result.success)
            {
                auto new_term = make_and(left_result.result, and_term->right());
                return RewriteResult::success_at(new_term,
                                                 Position().descend(0),
                                                 left_result.rule_name);
            }

            // Try right side
            auto right_result = rewrite_step(and_term->right());
            if (right_result.success)
            {
                auto new_term = make_and(and_term->left(), right_result.result);
                return RewriteResult::success_at(new_term,
                                                 Position().descend(1),
                                                 right_result.rule_name);
            }
            break;
        }

        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(term);

            // Try left side
            auto left_result = rewrite_step(or_term->left());
            if (left_result.success)
            {
                auto new_term = make_or(left_result.result, or_term->right());
                return RewriteResult::success_at(new_term,
                                                 Position().descend(0),
                                                 left_result.rule_name);
            }

            // Try right side
            auto right_result = rewrite_step(or_term->right());
            if (right_result.success)
            {
                auto new_term = make_or(or_term->left(), right_result.result);
                return RewriteResult::success_at(new_term,
                                                 Position().descend(1),
                                                 right_result.rule_name);
            }
            break;
        }

        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(term);
            auto body_result = rewrite_step(not_term->body());
            if (body_result.success)
            {
                auto new_term = make_not(body_result.result);
                return RewriteResult::success_at(new_term,
                                                 Position().descend(0),
                                                 body_result.rule_name);
            }
            break;
        }

        case TermDB::TermKind::IMPLIES:
        {
            auto implies = std::dynamic_pointer_cast<ImpliesDB>(term);

            // Try antecedent
            auto ant_result = rewrite_step(implies->antecedent());
            if (ant_result.success)
            {
                auto new_term = make_implies(ant_result.result, implies->consequent());
                return RewriteResult::success_at(new_term,
                                                 Position().descend(0),
                                                 ant_result.rule_name);
            }

            // Try consequent
            auto cons_result = rewrite_step(implies->consequent());
            if (cons_result.success)
            {
                auto new_term = make_implies(implies->antecedent(), cons_result.result);
                return RewriteResult::success_at(new_term,
                                                 Position().descend(1),
                                                 cons_result.rule_name);
            }
            break;
        }

        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(term);
            auto body_result = rewrite_step(forall->body());
            if (body_result.success)
            {
                auto new_term = make_forall(forall->variable_hint(), body_result.result);
                return RewriteResult::success_at(new_term,
                                                 Position().descend(0),
                                                 body_result.rule_name);
            }
            break;
        }

        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(term);
            auto body_result = rewrite_step(exists->body());
            if (body_result.success)
            {
                auto new_term = make_exists(exists->variable_hint(), body_result.result);
                return RewriteResult::success_at(new_term,
                                                 Position().descend(0),
                                                 body_result.rule_name);
            }
            break;
        }

        default:
            // Variables and constants cannot be rewritten
            break;
        }

        return RewriteResult::failure();
    }

    RewriteResult RewriteSystem::rewrite_at(const TermDBPtr &term, const Position &position) const
    {
        // Get subterm at position
        auto subterm = subterm_at(term, position);
        if (!subterm)
        {
            return RewriteResult::failure();
        }

        // Try to apply each rule to the subterm
        for (const auto &rule : rules_)
        {
            auto rewritten = try_apply_rule(subterm, rule);
            if (rewritten)
            {
                // Replace subterm in original term
                auto new_term = replace_at(term, position, rewritten);
                if (new_term)
                {
                    return RewriteResult::success_at(new_term, position, rule.name());
                }
            }
        }

        return RewriteResult::failure();
    }

    TermDBPtr RewriteSystem::normalize(const TermDBPtr &term, size_t max_steps) const
    {
        TermDBPtr current = term;

        for (size_t step = 0; step < max_steps; ++step)
        {
            auto result = rewrite_step(current);
            if (!result.success)
            {
                break; // No more rewrites possible
            }
            current = result.result;
        }

        return current;
    }

    bool RewriteSystem::is_normal_form(const TermDBPtr &term) const
    {
        auto result = rewrite_step(term);
        return !result.success;
    }

    TermDBPtr RewriteSystem::subterm_at(const TermDBPtr &term, const Position &position)
    {
        if (position.is_root())
        {
            return term;
        }

        if (position.path().empty())
        {
            return term;
        }

        size_t index = position.path()[0];
        Position rest(std::vector<size_t>(position.path().begin() + 1, position.path().end()));

        switch (term->kind())
        {
        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            if (index >= func_app->arguments().size())
                return nullptr;
            return subterm_at(func_app->arguments()[index], rest);
        }

        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(term);
            if (index == 0)
                return subterm_at(and_term->left(), rest);
            if (index == 1)
                return subterm_at(and_term->right(), rest);
            return nullptr;
        }

        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(term);
            if (index == 0)
                return subterm_at(or_term->left(), rest);
            if (index == 1)
                return subterm_at(or_term->right(), rest);
            return nullptr;
        }

        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(term);
            if (index == 0)
                return subterm_at(not_term->body(), rest);
            return nullptr;
        }

        case TermDB::TermKind::IMPLIES:
        {
            auto implies = std::dynamic_pointer_cast<ImpliesDB>(term);
            if (index == 0)
                return subterm_at(implies->antecedent(), rest);
            if (index == 1)
                return subterm_at(implies->consequent(), rest);
            return nullptr;
        }

        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(term);
            if (index == 0)
                return subterm_at(forall->body(), rest);
            return nullptr;
        }

        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(term);
            if (index == 0)
                return subterm_at(exists->body(), rest);
            return nullptr;
        }

        default:
            return nullptr; // Variables and constants have no subterms
        }
    }

    TermDBPtr RewriteSystem::replace_at(const TermDBPtr &term, const Position &position,
                                        const TermDBPtr &replacement)
    {
        if (position.is_root())
        {
            return replacement;
        }

        if (position.path().empty())
        {
            return replacement;
        }

        size_t index = position.path()[0];
        Position rest(std::vector<size_t>(position.path().begin() + 1, position.path().end()));

        switch (term->kind())
        {
        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            if (index >= func_app->arguments().size())
                return nullptr;

            std::vector<TermDBPtr> new_args = func_app->arguments();
            new_args[index] = replace_at(new_args[index], rest, replacement);
            if (!new_args[index])
                return nullptr;

            return make_function_application(func_app->symbol(), new_args);
        }

        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(term);
            if (index == 0)
            {
                auto new_left = replace_at(and_term->left(), rest, replacement);
                if (!new_left)
                    return nullptr;
                return make_and(new_left, and_term->right());
            }
            if (index == 1)
            {
                auto new_right = replace_at(and_term->right(), rest, replacement);
                if (!new_right)
                    return nullptr;
                return make_and(and_term->left(), new_right);
            }
            return nullptr;
        }

        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(term);
            if (index == 0)
            {
                auto new_left = replace_at(or_term->left(), rest, replacement);
                if (!new_left)
                    return nullptr;
                return make_or(new_left, or_term->right());
            }
            if (index == 1)
            {
                auto new_right = replace_at(or_term->right(), rest, replacement);
                if (!new_right)
                    return nullptr;
                return make_or(or_term->left(), new_right);
            }
            return nullptr;
        }

        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(term);
            if (index == 0)
            {
                auto new_body = replace_at(not_term->body(), rest, replacement);
                if (!new_body)
                    return nullptr;
                return make_not(new_body);
            }
            return nullptr;
        }

        case TermDB::TermKind::IMPLIES:
        {
            auto implies = std::dynamic_pointer_cast<ImpliesDB>(term);
            if (index == 0)
            {
                auto new_ant = replace_at(implies->antecedent(), rest, replacement);
                if (!new_ant)
                    return nullptr;
                return make_implies(new_ant, implies->consequent());
            }
            if (index == 1)
            {
                auto new_cons = replace_at(implies->consequent(), rest, replacement);
                if (!new_cons)
                    return nullptr;
                return make_implies(implies->antecedent(), new_cons);
            }
            return nullptr;
        }

        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(term);
            if (index == 0)
            {
                auto new_body = replace_at(forall->body(), rest, replacement);
                if (!new_body)
                    return nullptr;
                return make_forall(forall->variable_hint(), new_body);
            }
            return nullptr;
        }

        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(term);
            if (index == 0)
            {
                auto new_body = replace_at(exists->body(), rest, replacement);
                if (!new_body)
                    return nullptr;
                return make_exists(exists->variable_hint(), new_body);
            }
            return nullptr;
        }

        default:
            return nullptr; // Variables and constants have no subterms
        }
    }

    std::vector<Position> RewriteSystem::find_redex_positions(const TermDBPtr &term,
                                                              const TermRewriteRule &rule) const
    {
        std::vector<Position> positions;

        // Try to match at root
        if (try_apply_rule(term, rule))
        {
            positions.push_back(Position());
        }

        // Recursively find positions in subterms
        switch (term->kind())
        {
        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            for (size_t i = 0; i < func_app->arguments().size(); ++i)
            {
                auto sub_positions = find_redex_positions(func_app->arguments()[i], rule);
                for (const auto &pos : sub_positions)
                {
                    positions.push_back(Position().descend(i));
                }
            }
            break;
        }

        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(term);
            auto left_positions = find_redex_positions(and_term->left(), rule);
            auto right_positions = find_redex_positions(and_term->right(), rule);

            for (const auto &pos : left_positions)
            {
                positions.push_back(Position().descend(0));
            }
            for (const auto &pos : right_positions)
            {
                positions.push_back(Position().descend(1));
            }
            break;
        }

        // Similar for other logical connectives...
        default:
            break;
        }

        return positions;
    }

    bool RewriteSystem::joinable(const TermDBPtr &term1, const TermDBPtr &term2,
                                 size_t max_steps) const
    {
        auto norm1 = normalize(term1, max_steps);
        auto norm2 = normalize(term2, max_steps);
        return *norm1 == *norm2;
    }

    TermDBPtr RewriteSystem::try_apply_rule(const TermDBPtr &term, const TermRewriteRule &rule) const
    {
        // Try to unify term with rule's left-hand side
        auto unif_result = Unifier::unify(rule.lhs(), term);
        if (!unif_result.success)
        {
            return nullptr;
        }

        // Apply substitution to right-hand side
        return SubstitutionEngine::substitute(rule.rhs(), unif_result.substitution);
    }

    std::string RewriteSystem::generate_rule_name() const
    {
        static int counter = 0;
        return "rule_" + std::to_string(counter++);
    }

    // Equation implementation
    std::optional<TermRewriteRule> Equation::orient(const TermOrdering &ordering) const
    {
        if (ordering.greater(lhs_, rhs_))
        {
            return TermRewriteRule(lhs_, rhs_, name_);
        }
        else if (ordering.greater(rhs_, lhs_))
        {
            return TermRewriteRule(rhs_, lhs_, name_);
        }
        else
        {
            return std::nullopt; // Terms are equivalent
        }
    }

    bool Equation::is_orientable(const TermOrdering &ordering) const
    {
        return !ordering.equivalent(lhs_, rhs_);
    }

    std::string Equation::to_string() const
    {
        std::ostringstream oss;
        oss << name_ << ": ";

        // Simple term printing
        if (lhs_->kind() == TermDB::TermKind::CONSTANT)
        {
            auto const_lhs = std::dynamic_pointer_cast<ConstantDB>(lhs_);
            oss << const_lhs->symbol();
        }
        else
        {
            oss << "term_" << lhs_->hash();
        }

        oss << " = ";

        if (rhs_->kind() == TermDB::TermKind::CONSTANT)
        {
            auto const_rhs = std::dynamic_pointer_cast<ConstantDB>(rhs_);
            oss << const_rhs->symbol();
        }
        else
        {
            oss << "term_" << rhs_->hash();
        }

        return oss.str();
    }

    // Factory functions
    std::shared_ptr<RewriteSystem> make_rewrite_system(std::shared_ptr<TermOrdering> ordering)
    {
        return std::make_shared<RewriteSystem>(ordering);
    }

} // namespace theorem_prover