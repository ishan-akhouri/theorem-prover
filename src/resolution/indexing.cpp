#include "indexing.hpp"
#include <iostream>
#include <algorithm>

namespace theorem_prover
{

    LiteralIndex::LiteralIndex() {}

    void LiteralIndex::insert_clause(ClausePtr clause)
    {
        if (!clause)
            return;

        // Add this clause to the index for each of its literals
        for (const auto &literal : clause->literals())
        {
            bool polarity = literal.is_positive();
            std::string pred_symbol = get_predicate_symbol(literal.atom());
            size_t arity = get_arity(literal.atom());

            index_[polarity][pred_symbol][arity].push_back(clause);
        }
    }

    void LiteralIndex::remove_clause(ClausePtr clause)
    {
        if (!clause)
            return;

        // Remove this clause from the index for each of its literals
        for (const auto &literal : clause->literals())
        {
            bool polarity = literal.is_positive();
            std::string pred_symbol = get_predicate_symbol(literal.atom());
            size_t arity = get_arity(literal.atom());

            auto polarity_it = index_.find(polarity);
            if (polarity_it != index_.end())
            {
                auto pred_it = polarity_it->second.find(pred_symbol);
                if (pred_it != polarity_it->second.end())
                {
                    auto arity_it = pred_it->second.find(arity);
                    if (arity_it != pred_it->second.end())
                    {
                        remove_clause_from_bucket(arity_it->second, clause);
                    }
                }
            }
        }
    }

    std::vector<ClausePtr> LiteralIndex::get_resolution_candidates(const Literal &literal)
    {
        // Look for literals with OPPOSITE polarity, SAME predicate, SAME arity
        bool opposite_polarity = !literal.is_positive();
        std::string pred_symbol = get_predicate_symbol(literal.atom());
        size_t arity = get_arity(literal.atom());

        auto polarity_it = index_.find(opposite_polarity);
        if (polarity_it == index_.end())
        {
            return {};
        }

        auto pred_it = polarity_it->second.find(pred_symbol);
        if (pred_it == polarity_it->second.end())
        {
            return {};
        }

        auto arity_it = pred_it->second.find(arity);
        if (arity_it == pred_it->second.end())
        {
            return {};
        }

        return arity_it->second;
    }

    void LiteralIndex::clear()
    {
        index_.clear();
    }

    size_t LiteralIndex::size() const
    {
        size_t total = 0;
        for (const auto &[polarity, pred_map] : index_)
        {
            for (const auto &[pred_symbol, arity_map] : pred_map)
            {
                for (const auto &[arity, clause_list] : arity_map)
                {
                    total += clause_list.size();
                }
            }
        }
        return total;
    }

    void LiteralIndex::print_statistics() const
    {
        std::cout << "=== Literal Index Statistics ===" << std::endl;
        std::cout << "Total indexed literals: " << size() << std::endl;

        for (const auto &[polarity, pred_map] : index_)
        {
            std::cout << "Polarity " << (polarity ? "positive" : "negative") << ":" << std::endl;

            for (const auto &[pred_symbol, arity_map] : pred_map)
            {
                for (const auto &[arity, clause_list] : arity_map)
                {
                    std::cout << "  " << pred_symbol << "/" << arity
                              << ": " << clause_list.size() << " clauses" << std::endl;
                }
            }
        }
    }

    std::string LiteralIndex::get_predicate_symbol(const TermDBPtr &term) const
    {
        switch (term->kind())
        {
        case TermDB::TermKind::CONSTANT:
        {
            auto constant = std::dynamic_pointer_cast<ConstantDB>(term);
            return constant->symbol();
        }
        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            return func_app->symbol();
        }
        case TermDB::TermKind::VARIABLE:
        {
            return "_VAR_";
        }
        default:
            return "_OTHER_";
        }
    }

    size_t LiteralIndex::get_arity(const TermDBPtr &term) const
    {
        switch (term->kind())
        {
        case TermDB::TermKind::CONSTANT:
        {
            return 0; // Constants have arity 0
        }
        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            return func_app->arguments().size();
        }
        case TermDB::TermKind::VARIABLE:
        {
            return 0; // Variables treated as arity 0 for indexing
        }
        default:
            return 0;
        }
    }

    void LiteralIndex::remove_clause_from_bucket(std::vector<ClausePtr> &bucket, ClausePtr clause)
    {
        bucket.erase(
            std::remove_if(bucket.begin(), bucket.end(),
                           [clause](const ClausePtr &c)
                           { return c == clause; }),
            bucket.end());
    }

} // namespace theorem_prover