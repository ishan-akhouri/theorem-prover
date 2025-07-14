#include "ordering.hpp"
#include "../utils/hash.hpp"
#include <algorithm>
#include <queue>
#include <unordered_set>

namespace theorem_prover
{

    bool TermOrdering::greater_equal(const TermDBPtr &s, const TermDBPtr &t) const
    {
        return greater(s, t) || equivalent(s, t);
    }

    bool TermOrdering::equivalent(const TermDBPtr &s, const TermDBPtr &t) const
    {
        return !greater(s, t) && !greater(t, s);
    }

    void Precedence::set_greater(const std::string &f, const std::string &g)
    {
        // Add edge f -> g (f has higher precedence than g)
        precedence_graph_[f].insert(g);

        // Clear cache since precedence relation changed
        cache_.clear();
    }

    bool Precedence::greater(const std::string &f, const std::string &g) const
    {
        if (f == g)
            return false;

        // Check cache first
        std::string key = make_cache_key(f, g);
        auto it = cache_.find(key);
        if (it != cache_.end())
        {
            return it->second;
        }

        // Compute and cache result
        bool result = compute_transitive_greater(f, g);
        cache_[key] = result;
        return result;
    }

    bool Precedence::equal(const std::string &f, const std::string &g) const
    {
        return f == g;
    }

    std::unordered_set<std::string> Precedence::get_symbols() const
    {
        std::unordered_set<std::string> symbols;

        for (const auto &[source, targets] : precedence_graph_)
        {
            symbols.insert(source);
            for (const auto &target : targets)
            {
                symbols.insert(target);
            }
        }

        return symbols;
    }

    bool Precedence::total_greater(const std::string &f, const std::string &g) const
    {
        if (f == g)
            return false;

        // First check explicit precedence relation
        if (greater(f, g))
            return true;
        if (greater(g, f))
            return false;

        // Fallback to lexicographic comparison for totality
        return f > g;
    }

    bool Precedence::compute_transitive_greater(const std::string &f, const std::string &g) const
    {
        // BFS to check if there's a path from f to g in precedence graph
        std::queue<std::string> queue;
        std::unordered_set<std::string> visited;

        queue.push(f);
        visited.insert(f);

        while (!queue.empty())
        {
            std::string current = queue.front();
            queue.pop();

            auto it = precedence_graph_.find(current);
            if (it == precedence_graph_.end())
                continue;

            for (const auto &neighbor : it->second)
            {
                if (neighbor == g)
                    return true;

                if (visited.find(neighbor) == visited.end())
                {
                    visited.insert(neighbor);
                    queue.push(neighbor);
                }
            }
        }

        return false;
    }

    LexicographicPathOrdering::LexicographicPathOrdering(std::shared_ptr<Precedence> precedence)
        : precedence_(precedence) {}

    LexicographicPathOrdering::LexicographicPathOrdering()
        : precedence_(std::make_shared<Precedence>()) {}

    bool LexicographicPathOrdering::greater(const TermDBPtr &s, const TermDBPtr &t) const
    {
        return lpo_greater(s, t);
    }

    void LexicographicPathOrdering::set_argument_status(const std::string &symbol, ArgumentStatus status)
    {
        argument_status_[symbol] = status;
    }

    bool LexicographicPathOrdering::lpo_greater(const TermDBPtr &s, const TermDBPtr &t) const
    {
        // Variables are minimal elements
        if (is_variable(s) && is_variable(t))
        {
            return false; // No variable is greater than another
        }
        if (is_variable(s))
        {
            return false; // Variables are smaller than all non-variables
        }
        if (is_variable(t))
        {
            return !is_variable(s); // Non-variables are greater than variables
        }

        // Both s and t are non-variables
        auto [f, s_args] = decompose_term(s);
        auto [g, t_args] = decompose_term(t);

        // Case 1: t is a proper subterm of s and s ≥_lpo t
        for (const auto &s_arg : s_args)
        {
            if (*s_arg == *t)
            {
                return true; // t is a direct subterm of s
            }
            if (lpo_greater_equal(s_arg, t))
            {
                return true; // t is dominated by a subterm of s
            }
        }

        // Case 2: f >_prec g and s >_lpo ti for all ti
        if (precedence_->total_greater(f, g))
        {
            return all_greater(s, t_args);
        }

        // Case 3: f =_prec g, s >_lpo ti for all ti, and args(s) >_lex args(t)
        if (precedence_->equal(f, g))
        {
            if (!all_greater(s, t_args))
            {
                return false;
            }

            // Compare arguments according to status
            auto status_it = argument_status_.find(f);
            ArgumentStatus status = (status_it != argument_status_.end())
                                        ? status_it->second
                                        : ArgumentStatus::LEXICOGRAPHIC;

            if (status == ArgumentStatus::LEXICOGRAPHIC)
            {
                return lexicographic_greater(s_args, t_args);
            }
            else
            {
                return multiset_greater(s_args, t_args);
            }
        }

        return false;
    }

    bool LexicographicPathOrdering::lpo_greater_equal(const TermDBPtr &s, const TermDBPtr &t) const
    {
        return (*s == *t) || lpo_greater(s, t);
    }

    bool LexicographicPathOrdering::is_variable(const TermDBPtr &term) const
    {
        return term->kind() == TermDB::TermKind::VARIABLE;
    }

    bool LexicographicPathOrdering::contains_variable(const TermDBPtr &term, const TermDBPtr &var) const
    {
        if (*term == *var)
            return true;

        switch (term->kind())
        {
        case TermDB::TermKind::VARIABLE:
        case TermDB::TermKind::CONSTANT:
            return false;

        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            for (const auto &arg : func_app->arguments())
            {
                if (contains_variable(arg, var))
                    return true;
            }
            return false;
        }

        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(term);
            return contains_variable(forall->body(), var);
        }

        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(term);
            return contains_variable(exists->body(), var);
        }

        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(term);
            return contains_variable(and_term->left(), var) ||
                   contains_variable(and_term->right(), var);
        }

        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(term);
            return contains_variable(or_term->left(), var) ||
                   contains_variable(or_term->right(), var);
        }

        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(term);
            return contains_variable(not_term->body(), var);
        }

        case TermDB::TermKind::IMPLIES:
        {
            auto implies = std::dynamic_pointer_cast<ImpliesDB>(term);
            return contains_variable(implies->antecedent(), var) ||
                   contains_variable(implies->consequent(), var);
        }

        default:
            return false;
        }
    }

    bool LexicographicPathOrdering::all_greater(const TermDBPtr &s,
                                                const std::vector<TermDBPtr> &terms) const
    {
        for (const auto &t : terms)
        {
            if (!lpo_greater(s, t))
            {
                return false;
            }
        }
        return true;
    }

    bool LexicographicPathOrdering::lexicographic_greater(const std::vector<TermDBPtr> &args1,
                                                          const std::vector<TermDBPtr> &args2) const
    {
        size_t min_size = std::min(args1.size(), args2.size());

        // Compare corresponding arguments
        for (size_t i = 0; i < min_size; ++i)
        {
            if (lpo_greater(args1[i], args2[i]))
            {
                return true;
            }
            if (lpo_greater(args2[i], args1[i]))
            {
                return false;
            }
            // If equal, continue to next argument
        }

        // If all compared arguments are equal, longer list is greater
        return args1.size() > args2.size();
    }

    bool LexicographicPathOrdering::multiset_greater(const std::vector<TermDBPtr> &args1,
                                                     const std::vector<TermDBPtr> &args2) const
    {
        // Simplified multiset comparison for now
        // TODO: Implement proper multiset extension of LPO
        // For now, fall back to lexicographic comparison
        return lexicographic_greater(args1, args2);
    }

    std::pair<std::string, std::vector<TermDBPtr>>
    LexicographicPathOrdering::decompose_term(const TermDBPtr &term) const
    {
        switch (term->kind())
        {
        case TermDB::TermKind::CONSTANT:
        {
            auto constant = std::dynamic_pointer_cast<ConstantDB>(term);
            return {constant->symbol(), {}};
        }

        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            return {func_app->symbol(), func_app->arguments()};
        }

        case TermDB::TermKind::VARIABLE:
        {
            auto variable = std::dynamic_pointer_cast<VariableDB>(term);
            return {"_VAR_" + std::to_string(variable->index()), {}};
        }

        // Handle logical connectives as function symbols
        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(term);
            return {"∧", {and_term->left(), and_term->right()}};
        }

        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(term);
            return {"∨", {or_term->left(), or_term->right()}};
        }

        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(term);
            return {"¬", {not_term->body()}};
        }

        case TermDB::TermKind::IMPLIES:
        {
            auto implies = std::dynamic_pointer_cast<ImpliesDB>(term);
            return {"→", {implies->antecedent(), implies->consequent()}};
        }

        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(term);
            return {"∀", {forall->body()}};
        }

        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(term);
            return {"∃", {exists->body()}};
        }

        default:
            return {"_UNKNOWN_", {}};
        }
    }

    // Factory functions
    std::shared_ptr<LexicographicPathOrdering> make_lpo()
    {
        return std::make_shared<LexicographicPathOrdering>();
    }

    std::shared_ptr<LexicographicPathOrdering> make_lpo(std::shared_ptr<Precedence> precedence)
    {
        return std::make_shared<LexicographicPathOrdering>(precedence);
    }

} // namespace theorem_prover