#include <iostream>
#include <cassert>
#include "../src/resolution/cnf_converter.hpp"
#include "../src/resolution/resolution_prover.hpp"
#include "../src/term/term_db.hpp"

using namespace theorem_prover;

void test_multiple_same_variable_quantifiers()
{
    std::cout << "Testing multiple quantifiers with same variable names..." << std::endl;

    // Test ∀x.P(x) ∧ ∀x.Q(x) - should not create variable conflicts
    auto p_x = make_function_application("P", {make_variable(0)});
    auto q_x = make_function_application("Q", {make_variable(0)});

    auto forall_p = make_forall("x", p_x);
    auto forall_q = make_forall("x", q_x);
    auto formula = make_and(forall_p, forall_q);

    // Convert to CNF - should handle variable standardization properly
    auto clauses = CNFConverter::to_cnf(formula);

    // Should produce 2 clauses: P(X0) and Q(X1) with different variable indices
    assert(clauses.size() == 2);

    // Check that the clauses have different variable indices
    bool variables_are_different = true;
    if (clauses.size() >= 2)
    {
        auto clause1_vars = find_all_variables(clauses[0]->literals()[0].atom());
        auto clause2_vars = find_all_variables(clauses[1]->literals()[0].atom());

        // Variables should be disjoint between clauses
        for (auto var1 : clause1_vars)
        {
            if (clause2_vars.count(var1) > 0)
            {
                variables_are_different = false;
                break;
            }
        }
    }

    assert(variables_are_different);
    std::cout << "  Variables properly standardized across quantifiers" << std::endl;
}

void test_nested_quantifier_standardization()
{
    std::cout << "Testing nested quantifier standardization..." << std::endl;

    // Test ∀x.∃y.P(x,y) ∧ ∀y.∃x.Q(x,y)
    auto p_xy = make_function_application("P", {make_variable(1), make_variable(0)});
    auto q_xy = make_function_application("Q", {make_variable(1), make_variable(0)});

    auto exists_y_p = make_exists("y", p_xy);
    auto forall_x_exists_y_p = make_forall("x", exists_y_p);

    auto exists_x_q = make_exists("x", q_xy);
    auto forall_y_exists_x_q = make_forall("y", exists_x_q);

    auto complex_formula = make_and(forall_x_exists_y_p, forall_y_exists_x_q);

    // Should not crash during CNF conversion
    auto clauses = CNFConverter::to_cnf(complex_formula);
    assert(clauses.size() >= 2);

    std::cout << "  Nested quantifiers handled without variable conflicts" << std::endl;
}

void test_resolution_with_standardized_variables()
{
    std::cout << "Testing resolution with properly standardized variables..." << std::endl;

    ResolutionProver prover;

    // Test ∀x.P(x), ∀x.(P(x) → Q(x)) ⊢ ∀x.Q(x)
    // This was failing before proper variable standardization

    auto p_x = make_function_application("P", {make_variable(0)});
    auto q_x = make_function_application("Q", {make_variable(0)});

    auto forall_p = make_forall("x", p_x);
    auto p_implies_q = make_implies(p_x, q_x);
    auto forall_impl = make_forall("x", p_implies_q);
    auto forall_q = make_forall("x", q_x);

    std::vector<TermDBPtr> hypotheses = {forall_p, forall_impl};
    auto result = prover.prove(forall_q, hypotheses);

    // Should now succeed with proper variable standardization
    assert(result.is_proved());
    std::cout << "  Resolution with quantified formulas proved correctly" << std::endl;
}

void test_variable_discovery_utilities()
{
    std::cout << "Testing variable discovery utilities..." << std::endl;

    // Test find_all_variables on f(X, g(Y, Z))
    auto x = make_variable(0);
    auto y = make_variable(1);
    auto z = make_variable(2);

    auto g_yz = make_function_application("g", {y, z});
    auto f_xyz = make_function_application("f", {x, g_yz});

    auto variables = find_all_variables(f_xyz);
    assert(variables.size() == 3);
    assert(variables.count(0) == 1); // X
    assert(variables.count(1) == 1); // Y
    assert(variables.count(2) == 1); // Z

    // Test get_max_variable_index
    auto max_var = get_max_variable_index(f_xyz);
    assert(max_var == 2);

    std::cout << "  Variable discovery utilities working correctly" << std::endl;
}

void test_clause_variable_renaming()
{
    std::cout << "Testing clause variable renaming..." << std::endl;

    // Create clause P(X) ∨ Q(Y)
    auto p_x = make_function_application("P", {make_variable(0)});
    auto q_y = make_function_application("Q", {make_variable(1)});

    std::vector<Literal> literals = {
        Literal(p_x, true),
        Literal(q_y, true)};

    Clause original_clause(literals);

    // Rename variables with offset 10
    auto renamed_clause = original_clause.rename_variables(10);

    // Variables should now be 10, 11 instead of 0, 1
    auto renamed_vars_p = find_all_variables(renamed_clause.literals()[0].atom());
    auto renamed_vars_q = find_all_variables(renamed_clause.literals()[1].atom());

    assert(renamed_vars_p.count(10) == 1);
    assert(renamed_vars_q.count(11) == 1);

    // Original variables should not be present
    assert(renamed_vars_p.count(0) == 0);
    assert(renamed_vars_p.count(1) == 0);
    assert(renamed_vars_q.count(0) == 0);
    assert(renamed_vars_q.count(1) == 0);

    std::cout << "  Clause variable renaming working correctly" << std::endl;
}

int main()
{
    std::cout << "===== Running Variable Standardization Tests =====" << std::endl;

    test_variable_discovery_utilities();
    test_clause_variable_renaming();
    test_multiple_same_variable_quantifiers();
    test_nested_quantifier_standardization();
    test_resolution_with_standardized_variables();

    std::cout << "\n===== All Variable Standardization Tests Passed! =====" << std::endl;
    return 0;
}