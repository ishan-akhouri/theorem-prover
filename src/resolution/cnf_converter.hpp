#pragma once

#include "../term/term_db.hpp"
#include "clause.hpp"
#include <vector>
#include <memory>
#include <string>

namespace theorem_prover
{

    /**
     * Converts first-order logic formulas to Conjunctive Normal Form (CNF)
     * The conversion process follows these steps:
     * 1. Eliminate implications and biconditionals
     * 2. Move negations inward (De Morgan's laws)
     * 3. Standardize variables apart
     * 4. Move quantifiers outward (prenex form)
     * 5. Eliminate existential quantifiers (Skolemization)
     * 6. Distribute OR over AND
     * 7. Extract clauses
     */
    class CNFConverter
    {
    public:
        /**
         * Convert a formula to CNF and return set of clauses
         */
        static std::vector<ClausePtr> to_cnf(const TermDBPtr &formula);

        /**
         * Convert a formula to CNF with variable renaming
         */
        static std::vector<ClausePtr> to_cnf_with_renaming(const TermDBPtr &formula,
                                                           std::size_t variable_offset = 0);

        // Make these public for testing
        /**
         * Step 1: Eliminate implications and biconditionals
         */
        static TermDBPtr eliminate_implications(const TermDBPtr &formula);

        /**
         * Step 2: Move negations inward using De Morgan's laws
         */
        static TermDBPtr move_negations_inward(const TermDBPtr &formula);

        /**
         * Step 3: Standardize variables apart (rename bound variables)
         */
        static TermDBPtr standardize_variables(const TermDBPtr &formula,
                                               std::size_t &variable_counter);

        /**
         * Step 4: Move quantifiers to the front (prenex form)
         */
        static TermDBPtr to_prenex_form(const TermDBPtr &formula);

        /**
         * Step 5: Eliminate existential quantifiers (Skolemization)
         */
        static TermDBPtr skolemize(const TermDBPtr &formula,
                                   const std::vector<std::size_t> &universal_vars,
                                   std::size_t &skolem_counter);

        /**
         * Step 6: Distribute OR over AND
         */
        static TermDBPtr distribute_or_over_and(const TermDBPtr &formula);

        /**
         * Step 7: Extract clauses from the CNF formula
         */
        static std::vector<ClausePtr> extract_clauses(const TermDBPtr &cnf_formula);

    private:
        /**
         * Helper: Extract literals from a disjunction
         */
        static std::vector<Literal> extract_literals(const TermDBPtr &disjunction);

        /**
         * Helper: Check if a term is in CNF form
         */
        static bool is_cnf(const TermDBPtr &formula);

        /**
         * Helper: Generate Skolem function name
         */
        static std::string generate_skolem_name(std::size_t counter);

        /**
         * Helper: Find all free variables in a term
         */
        static std::vector<std::size_t> find_free_variables(const TermDBPtr &term,
                                                            std::size_t depth = 0);
    };

    /**
     * Utility class for managing Skolem functions during conversion
     */
    class SkolemManager
    {
    public:
        SkolemManager() : counter_(0) {}

        /**
         * Create a new Skolem function with given arity
         */
        TermDBPtr create_skolem_function(const std::vector<std::size_t> &universal_vars);

        /**
         * Get the next Skolem function name
         */
        std::string next_skolem_name();

    private:
        std::size_t counter_;
    };

} // namespace theorem_prover