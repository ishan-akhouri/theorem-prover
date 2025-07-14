#pragma once

#include "term_db.hpp"
#include "substitution.hpp"
#include <optional>

namespace theorem_prover
{

    /**
     * Result of a unification attempt
     */
    struct UnificationResult
    {
        bool success;
        SubstitutionMap substitution;
        std::string error_message;

        UnificationResult(bool success = false, const SubstitutionMap &subst = {},
                          const std::string &error = "")
            : success(success), substitution(subst), error_message(error) {}

        static UnificationResult make_failure(const std::string &reason)
        {
            return UnificationResult(false, {}, reason);
        }

        static UnificationResult make_success(const SubstitutionMap &subst)
        {
            return UnificationResult(true, subst);
        }
    };

    /**
     * First-order logic unification algorithm
     *
     * Implements the Robinson unification algorithm with occurs check
     * for terms represented using De Bruijn indices.
     */
    class Unifier
    {
    public:
        /**
         * Unify two terms, returning the most general unifier (MGU)
         *
         * @param term1 First term to unify
         * @param term2 Second term to unify
         * @param depth Current binding depth (for De Bruijn handling)
         * @return UnificationResult containing success flag and substitution
         */
        static UnificationResult unify(const TermDBPtr &term1,
                                       const TermDBPtr &term2,
                                       std::size_t depth = 0);

        /**
         * Check if two terms are unifiable without computing the substitution
         *
         * @param term1 First term
         * @param term2 Second term
         * @param depth Current binding depth
         * @return true if terms are unifiable
         */
        static bool unifiable(const TermDBPtr &term1,
                              const TermDBPtr &term2,
                              std::size_t depth = 0);

        /**
         * Apply a substitution to another substitution (composition)
         *
         * @param subst1 First substitution
         * @param subst2 Second substitution
         * @return Composed substitution
         */
        static SubstitutionMap compose_substitutions(const SubstitutionMap &subst1,
                                                     const SubstitutionMap &subst2);

    private:
        /**
         * Main unification algorithm implementation
         *
         * @param term1 First term
         * @param term2 Second term
         * @param substitution Current substitution (modified in-place)
         * @param depth Current binding depth
         * @return true if unification succeeds
         */
        static bool unify_impl(const TermDBPtr &term1,
                               const TermDBPtr &term2,
                               SubstitutionMap &substitution,
                               std::size_t depth);

        /**
         * Occurs check: ensure a variable doesn't occur in a term
         *
         * @param var_index Variable De Bruijn index to check
         * @param term Term to search in
         * @param depth Current binding depth
         * @return true if variable occurs in term (bad for unification)
         */
        static bool occurs_check(std::size_t var_index,
                                 const TermDBPtr &term,
                                 std::size_t depth);

        /**
         * Check if a variable is free at the current depth
         *
         * @param var_index Variable De Bruijn index
         * @param depth Current binding depth
         * @return true if variable is free
         */
        static bool is_free_variable(std::size_t var_index, std::size_t depth);
    };

} // namespace theorem_prover