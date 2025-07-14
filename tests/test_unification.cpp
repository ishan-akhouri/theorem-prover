#include <iostream>
#include <cassert>
#include "../src/term/unification.hpp"
#include "../src/term/term_db.hpp"

using namespace theorem_prover;

void test_basic_unification()
{
    std::cout << "Testing basic unification..." << std::endl;

    // Test 1: Unifying identical constants
    auto const_a = make_constant("a");
    auto const_a2 = make_constant("a");
    auto result = Unifier::unify(const_a, const_a2);
    assert(result.success);
    assert(result.substitution.empty()); // No substitution needed

    // Test 2: Unifying different constants (should fail)
    auto const_b = make_constant("b");
    result = Unifier::unify(const_a, const_b);
    assert(!result.success);

    std::cout << "Basic unification tests passed!" << std::endl;
}

void test_variable_unification()
{
    std::cout << "Testing variable unification..." << std::endl;

    // Test 1: Variable with constant
    auto var_0 = make_variable(0);
    auto const_a = make_constant("a");
    auto result = Unifier::unify(var_0, const_a);
    assert(result.success);
    assert(result.substitution.size() == 1);
    assert(result.substitution.count(0) == 1);

    // Test 2: Two different variables
    auto var_1 = make_variable(1);
    result = Unifier::unify(var_0, var_1);
    assert(result.success);
    assert(result.substitution.size() == 1);

    // Test 3: Variable with itself
    result = Unifier::unify(var_0, var_0);
    assert(result.success);
    assert(result.substitution.empty());

    std::cout << "Variable unification tests passed!" << std::endl;
}

void test_function_unification()
{
    std::cout << "Testing function unification..." << std::endl;

    // Test 1: f(a) with f(b) - should fail
    auto f_a = make_function_application("f", {make_constant("a")});
    auto f_b = make_function_application("f", {make_constant("b")});
    auto result = Unifier::unify(f_a, f_b);
    assert(!result.success);

    // Test 2: f(X) with f(a) - should succeed
    auto f_var = make_function_application("f", {make_variable(0)});
    auto f_const = make_function_application("f", {make_constant("a")});
    result = Unifier::unify(f_var, f_const);
    assert(result.success);
    assert(result.substitution.size() == 1);
    assert(result.substitution.count(0) == 1);

    // Test 3: Different function symbols - should fail
    auto g_a = make_function_application("g", {make_constant("a")});
    result = Unifier::unify(f_a, g_a);
    assert(!result.success);

    // Test 4: Different arity - should fail
    auto f_ab = make_function_application("f", {make_constant("a"), make_constant("b")});
    result = Unifier::unify(f_a, f_ab);
    assert(!result.success);

    std::cout << "Function unification tests passed!" << std::endl;
}

void test_complex_unification()
{
    std::cout << "Testing complex unification..." << std::endl;

    // Test 1: f(X, g(Y)) with f(a, g(b))
    auto inner_g_var = make_function_application("g", {make_variable(1)});
    auto f_complex_var = make_function_application("f", {make_variable(0), inner_g_var});

    auto inner_g_const = make_function_application("g", {make_constant("b")});
    auto f_complex_const = make_function_application("f", {make_constant("a"), inner_g_const});

    auto result = Unifier::unify(f_complex_var, f_complex_const);
    assert(result.success);
    assert(result.substitution.size() == 2);
    assert(result.substitution.count(0) == 1); // X -> a
    assert(result.substitution.count(1) == 1); // Y -> b

    // Test 2: f(X, X) with f(a, b) - should fail
    auto f_same_var = make_function_application("f", {make_variable(0), make_variable(0)});
    auto f_diff_const = make_function_application("f", {make_constant("a"), make_constant("b")});
    result = Unifier::unify(f_same_var, f_diff_const);
    assert(!result.success);

    // Test 3: f(X, X) with f(a, a) - should succeed
    auto f_same_const = make_function_application("f", {make_constant("a"), make_constant("a")});
    result = Unifier::unify(f_same_var, f_same_const);
    assert(result.success);
    assert(result.substitution.size() == 1);
    assert(result.substitution.count(0) == 1); // X -> a

    std::cout << "Complex unification tests passed!" << std::endl;
}

void test_occurs_check()
{
    std::cout << "Testing occurs check..." << std::endl;

    // Test 1: X with f(X) - should fail due to occurs check
    auto var_x = make_variable(0);
    auto f_x = make_function_application("f", {var_x});
    auto result = Unifier::unify(var_x, f_x);
    assert(!result.success);

    // Test 2: X with f(Y) - should succeed
    auto var_y = make_variable(1);
    auto f_y = make_function_application("f", {var_y});
    result = Unifier::unify(var_x, f_y);
    assert(result.success);

    // Test 3: More complex occurs check: X with g(f(X))
    auto f_x_inner = make_function_application("f", {var_x});
    auto g_f_x = make_function_application("g", {f_x_inner});
    result = Unifier::unify(var_x, g_f_x);
    assert(!result.success);

    std::cout << "Occurs check tests passed!" << std::endl;
}

void test_logical_connectives()
{
    std::cout << "Testing logical connective unification..." << std::endl;

    // Test 1: P ∧ Q with X ∧ Y
    auto p = make_constant("P");
    auto q = make_constant("Q");
    auto x = make_variable(0);
    auto y = make_variable(1);

    auto p_and_q = make_and(p, q);
    auto x_and_y = make_and(x, y);

    auto result = Unifier::unify(p_and_q, x_and_y);
    assert(result.success);
    assert(result.substitution.size() == 2);

    // Test 2: P ∧ Q with P ∨ Q - should fail
    auto p_or_q = make_or(p, q);
    result = Unifier::unify(p_and_q, p_or_q);
    assert(!result.success);

    // Test 3: P → Q with X → Y
    auto p_implies_q = make_implies(p, q);
    auto x_implies_y = make_implies(x, y);
    result = Unifier::unify(p_implies_q, x_implies_y);
    assert(result.success);
    assert(result.substitution.size() == 2);

    // Test 4: ¬P with ¬X
    auto not_p = make_not(p);
    auto not_x = make_not(x);
    result = Unifier::unify(not_p, not_x);
    assert(result.success);
    assert(result.substitution.size() == 1);

    std::cout << "Logical connective unification tests passed!" << std::endl;
}

void test_quantifier_unification()
{
    std::cout << "Testing quantifier unification..." << std::endl;

    // Test 1: ∀x.P(x) with ∀y.Q(y) - variables should unify at depth 1
    auto p_x = make_function_application("P", {make_variable(0)});
    auto q_y = make_function_application("Q", {make_variable(0)});

    auto forall_p = make_forall("x", p_x);
    auto forall_q = make_forall("y", q_y);

    // This should fail because P and Q are different predicates
    auto result = Unifier::unify(forall_p, forall_q);
    assert(!result.success);

    // Test 2: ∀x.P(x) with ∀y.P(y) - should succeed
    auto p_y = make_function_application("P", {make_variable(0)});
    auto forall_p2 = make_forall("y", p_y);
    result = Unifier::unify(forall_p, forall_p2);
    assert(result.success);

    // Test 3: ∃x.P(x) with ∃y.P(y) - should succeed
    auto exists_p = make_exists("x", p_x);
    auto exists_p2 = make_exists("y", p_y);
    result = Unifier::unify(exists_p, exists_p2);
    assert(result.success);

    std::cout << "Quantifier unification tests passed!" << std::endl;
}

void test_substitution_composition()
{
    std::cout << "Testing substitution composition..." << std::endl;

    // Create substitution 1: X -> a
    SubstitutionMap subst1;
    subst1[0] = make_constant("a");

    // Create substitution 2: Y -> f(X)
    SubstitutionMap subst2;
    subst2[1] = make_function_application("f", {make_variable(0)});

    // Compose them
    auto composed = Unifier::compose_substitutions(subst1, subst2);

    // Should have both bindings, with Y -> f(a) after composition
    assert(composed.size() == 2);
    assert(composed.count(0) == 1);
    assert(composed.count(1) == 1);

    // Check that Y is bound to f(a), not f(X)
    auto y_binding = composed[1];
    assert(y_binding->kind() == TermDB::TermKind::FUNCTION_APPLICATION);
    auto func = std::dynamic_pointer_cast<FunctionApplicationDB>(y_binding);
    assert(func->symbol() == "f");
    assert(func->arguments().size() == 1);
    assert(func->arguments()[0]->kind() == TermDB::TermKind::CONSTANT);

    std::cout << "Substitution composition tests passed!" << std::endl;
}

void test_unifiable_predicate()
{
    std::cout << "Testing unifiable predicate..." << std::endl;

    auto var_x = make_variable(0);
    auto const_a = make_constant("a");
    auto f_x = make_function_application("f", {var_x});

    // Should be unifiable
    assert(Unifier::unifiable(var_x, const_a));
    assert(Unifier::unifiable(const_a, const_a));

    // Should not be unifiable
    assert(!Unifier::unifiable(var_x, f_x)); // Occurs check
    assert(!Unifier::unifiable(make_constant("a"), make_constant("b")));

    std::cout << "Unifiable predicate tests passed!" << std::endl;
}

int main()
{
    std::cout << "===== Running Unification Tests =====" << std::endl;

    test_basic_unification();
    test_variable_unification();
    test_function_unification();
    test_complex_unification();
    test_occurs_check();
    test_logical_connectives();
    test_quantifier_unification();
    test_substitution_composition();
    test_unifiable_predicate();

    std::cout << "\n===== All Unification Tests Passed! =====" << std::endl;
    return 0;
}