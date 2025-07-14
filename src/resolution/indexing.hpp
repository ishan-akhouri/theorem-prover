#pragma once

#include "clause.hpp"
#include <unordered_map>
#include <vector>
#include <string>

namespace theorem_prover
{

    class LiteralIndex
    {
    public:
        LiteralIndex();

        // Index management
        void insert_clause(ClausePtr clause);
        void remove_clause(ClausePtr clause);
        void clear();

        // Query interface
        std::vector<ClausePtr> get_resolution_candidates(const Literal &literal);

        // Statistics
        size_t size() const;
        void print_statistics() const;

    private:
        // Index structure: polarity -> predicate_symbol -> arity -> list of clauses
        std::unordered_map<bool,
                           std::unordered_map<std::string,
                                              std::unordered_map<size_t, std::vector<ClausePtr>>>>
            index_;

        // Helper functions
        std::string get_predicate_symbol(const TermDBPtr &term) const;
        size_t get_arity(const TermDBPtr &term) const;
        void remove_clause_from_bucket(std::vector<ClausePtr> &bucket, ClausePtr clause);
    };

} // namespace theorem_prover