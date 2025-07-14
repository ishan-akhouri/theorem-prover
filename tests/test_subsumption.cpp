#include <iostream>
#include <cassert>
#include "../src/resolution/clause.hpp"
#include "../src/term/term_db.hpp"

using namespace theorem_prover;

void test_basic_unification_subsumption()
{
    std::cout << "Testing basic unification subsumption..." << std::endl;

    // Test P(x) subsumes P(a) ∨ Q(b)
    auto p_x = make_function_application("P", {make_variable(0)});
    auto p_a = make_function_application("P", {make_constant("a")});
    auto q_b = make_function_application("Q", {make_constant("b")});

    auto clause1 = std::make_shared<Clause>(std::vector<Literal>{Literal(p_x, true)});
    auto clause2 = std::make_shared<Clause>(std::vector<Literal>{
        Literal(p_a, true), Literal(q_b, true)});

    assert(Clause::subsumes(clause1, clause2));
    assert(!Clause::subsumes(clause2, clause1)); // Reverse shouldn't work

    std::cout << "  P(x) correctly subsumes P(a) ∨ Q(b)" << std::endl;
}

void test_no_subsumption_cases()
{
    std::cout << "Testing non-subsumption cases..." << std::endl;

    auto p_x = make_function_application("P", {make_variable(0)});
    auto q_y = make_function_application("Q", {make_variable(1)});

    auto clause1 = std::make_shared<Clause>(std::vector<Literal>{Literal(p_x, true)});
    auto clause2 = std::make_shared<Clause>(std::vector<Literal>{Literal(q_y, true)});

    // P(x) should not subsume Q(y)
    assert(!Clause::subsumes(clause1, clause2));

    std::cout << "  P(x) correctly does not subsume Q(y)" << std::endl;
}

void test_polarity_mismatch()
{
    std::cout << "Testing polarity mismatch..." << std::endl;

    auto p_x = make_function_application("P", {make_variable(0)});
    auto p_a = make_function_application("P", {make_constant("a")});

    auto pos_clause = std::make_shared<Clause>(std::vector<Literal>{Literal(p_x, true)});
    auto neg_clause = std::make_shared<Clause>(std::vector<Literal>{Literal(p_a, false)});

    // P(x) should not subsume ¬P(a) due to polarity mismatch
    assert(!Clause::subsumes(pos_clause, neg_clause));

    std::cout << "  P(x) correctly does not subsume ¬P(a)" << std::endl;
}

void test_inconsistent_substitution()
{
    std::cout << "Testing inconsistent substitution..." << std::endl;

    // P(x) ∨ Q(x) should NOT subsume P(a) ∨ Q(b) - inconsistent substitution
    auto p_x = make_function_application("P", {make_variable(0)});
    auto q_x = make_function_application("Q", {make_variable(0)});
    auto p_a = make_function_application("P", {make_constant("a")});
    auto q_b = make_function_application("Q", {make_constant("b")});

    auto clause1 = std::make_shared<Clause>(std::vector<Literal>{
        Literal(p_x, true), Literal(q_x, true)});
    auto clause2 = std::make_shared<Clause>(std::vector<Literal>{
        Literal(p_a, true), Literal(q_b, true)});

    assert(!Clause::subsumes(clause1, clause2));

    std::cout << "  P(x) ∨ Q(x) correctly does not subsume P(a) ∨ Q(b)" << std::endl;
}

void test_consistent_substitution()
{
    std::cout << "Testing consistent substitution..." << std::endl;

    // P(x) ∨ Q(x) SHOULD subsume P(a) ∨ Q(a) ∨ R(b) - consistent substitution
    auto p_x = make_function_application("P", {make_variable(0)});
    auto q_x = make_function_application("Q", {make_variable(0)});
    auto p_a = make_function_application("P", {make_constant("a")});
    auto q_a = make_function_application("Q", {make_constant("a")});
    auto r_b = make_function_application("R", {make_constant("b")});

    auto clause1 = std::make_shared<Clause>(std::vector<Literal>{
        Literal(p_x, true), Literal(q_x, true)});
    auto clause2 = std::make_shared<Clause>(std::vector<Literal>{
        Literal(p_a, true), Literal(q_a, true), Literal(r_b, true)});

    assert(Clause::subsumes(clause1, clause2));

    std::cout << "  P(x) ∨ Q(x) correctly subsumes P(a) ∨ Q(a) ∨ R(b)" << std::endl;
}

void test_multi_variable_subsumption()
{
    std::cout << "Testing multi-variable subsumption..." << std::endl;

    // P(x,y) ∨ Q(z) should subsume P(a,b) ∨ Q(c) ∨ R(d)
    auto p_xy = make_function_application("P", {make_variable(0), make_variable(1)});
    auto q_z = make_function_application("Q", {make_variable(2)});
    auto p_ab = make_function_application("P", {make_constant("a"), make_constant("b")});
    auto q_c = make_function_application("Q", {make_constant("c")});
    auto r_d = make_function_application("R", {make_constant("d")});

    auto clause1 = std::make_shared<Clause>(std::vector<Literal>{
        Literal(p_xy, true), Literal(q_z, true)});
    auto clause2 = std::make_shared<Clause>(std::vector<Literal>{
        Literal(p_ab, true), Literal(q_c, true), Literal(r_d, true)});

    assert(Clause::subsumes(clause1, clause2));

    std::cout << "  P(x,y) ∨ Q(z) correctly subsumes P(a,b) ∨ Q(c) ∨ R(d)" << std::endl;
}

void test_empty_clause_subsumption()
{
    std::cout << "Testing empty clause subsumption..." << std::endl;

    auto empty_clause = std::make_shared<Clause>();
    auto p_a = make_function_application("P", {make_constant("a")});
    auto unit_clause = std::make_shared<Clause>(std::vector<Literal>{Literal(p_a, true)});

    // Empty clause subsumes everything
    assert(Clause::subsumes(empty_clause, unit_clause));

    // Nothing (except empty clause) subsumes empty clause
    assert(!Clause::subsumes(unit_clause, empty_clause));
    assert(Clause::subsumes(empty_clause, empty_clause));

    std::cout << "  Empty clause subsumption working correctly" << std::endl;
}

int main()
{
    std::cout << "===== Running Subsumption Tests =====" << std::endl;

    test_basic_unification_subsumption();
    test_no_subsumption_cases();
    test_polarity_mismatch();
    test_inconsistent_substitution();
    test_consistent_substitution();
    test_multi_variable_subsumption();
    test_empty_clause_subsumption();

    std::cout << "\n===== All Subsumption Tests Passed! =====" << std::endl;
    return 0;
}