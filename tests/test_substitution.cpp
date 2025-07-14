#include "../src/term/term_db.hpp"
#include "../src/term/substitution.hpp"
#include <cassert>
#include <iostream>

using namespace theorem_prover;

void debug_term_structure(const TermDBPtr &term, int depth = 0)
{
    std::string indent(depth * 2, ' ');

    switch (term->kind())
    {
    case TermDB::TermKind::VARIABLE:
    {
        auto var = std::dynamic_pointer_cast<VariableDB>(term);
        std::cout << indent << "Variable(index=" << var->index() << ")\n";
        break;
    }
    case TermDB::TermKind::CONSTANT:
    {
        auto constant = std::dynamic_pointer_cast<ConstantDB>(term);
        std::cout << indent << "Constant('" << constant->symbol() << "')\n";
        break;
    }
    case TermDB::TermKind::FUNCTION_APPLICATION:
    {
        auto app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
        std::cout << indent << "FunctionApplication('" << app->symbol() << "', [\n";
        for (const auto &arg : app->arguments())
        {
            debug_term_structure(arg, depth + 1);
        }
        std::cout << indent << "])\n";
        break;
    }
    case TermDB::TermKind::FORALL:
    {
        auto forall = std::dynamic_pointer_cast<ForallDB>(term);
        std::cout << indent << "Forall('" << forall->variable_hint() << "', \n";
        debug_term_structure(forall->body(), depth + 1);
        std::cout << indent << ")\n";
        break;
    }
    case TermDB::TermKind::EXISTS:
    {
        auto exists = std::dynamic_pointer_cast<ExistsDB>(term);
        std::cout << indent << "Exists('" << exists->variable_hint() << "', \n";
        debug_term_structure(exists->body(), depth + 1);
        std::cout << indent << ")\n";
        break;
    }
    case TermDB::TermKind::AND:
    {
        auto and_term = std::dynamic_pointer_cast<AndDB>(term);
        std::cout << indent << "And(\n";
        debug_term_structure(and_term->left(), depth + 1);
        debug_term_structure(and_term->right(), depth + 1);
        std::cout << indent << ")\n";
        break;
    }
    case TermDB::TermKind::OR:
    {
        auto or_term = std::dynamic_pointer_cast<OrDB>(term);
        std::cout << indent << "Or(\n";
        debug_term_structure(or_term->left(), depth + 1);
        debug_term_structure(or_term->right(), depth + 1);
        std::cout << indent << ")\n";
        break;
    }
    case TermDB::TermKind::NOT:
    {
        auto not_term = std::dynamic_pointer_cast<NotDB>(term);
        std::cout << indent << "Not(\n";
        debug_term_structure(not_term->body(), depth + 1);
        std::cout << indent << ")\n";
        break;
    }
    case TermDB::TermKind::IMPLIES:
    {
        auto implies = std::dynamic_pointer_cast<ImpliesDB>(term);
        std::cout << indent << "Implies(\n";
        debug_term_structure(implies->antecedent(), depth + 1);
        debug_term_structure(implies->consequent(), depth + 1);
        std::cout << indent << ")\n";
        break;
    }
    default:
        std::cout << indent << "Unknown term type\n";
        break;
    }
}

// Test that bound variables are not substituted
void test_basic_substitution()
{
    auto x = make_variable(0);
    auto forall_x = make_forall("x", x); // ∀x. x

    std::cout << "Test: Bound variables should not be substituted\n";

    SubstitutionMap subst;
    subst[0] = make_constant("A");

    auto result = SubstitutionEngine::substitute(forall_x, subst);

    bool equals = result == forall_x;
    std::cout << "Result equals original: " << (equals ? "yes" : "no") << "\n";

    assert(result == forall_x); // bound variable must not be substituted
    std::cout << "Basic substitution test passed\n";
}

// Test that free variables are correctly substituted using their true free variable index
void test_free_substitution()
{
    // ∀x. x → y
    auto x = make_variable(0); // index 0 - bound by forall
    auto y = make_variable(1); // index 1 - free variable, true index is 1-1=0 at depth 1

    std::cout << "Variable x is at index: " << std::dynamic_pointer_cast<VariableDB>(x)->index() << std::endl;
    std::cout << "Variable y is at index: " << std::dynamic_pointer_cast<VariableDB>(y)->index() << std::endl;

    auto body = make_implies(x, y);
    auto term = make_forall("x", body);

    std::cout << "Original term structure: ∀x. x → y\n";
    debug_term_structure(term);

    // In De Bruijn substitution, the free variable y at index 1 at depth 1 has true index 0
    SubstitutionMap subst;
    subst[0] = make_constant("B");

    std::cout << "Substituting free variable with true index 0 with constant 'B'\n";

    auto result = SubstitutionEngine::substitute(term, subst);

    std::cout << "Result structure:\n";
    debug_term_structure(result);

    auto expected = make_forall("x", make_implies(x, make_constant("B")));
    std::cout << "Expected structure:\n";
    debug_term_structure(expected);

    bool equals = *result == *expected;
    std::cout << "Result equals expected: " << (equals ? "yes" : "no") << "\n";

    assert(*result == *expected);
    std::cout << "Free substitution test passed\n";
}

// Test that substitution works under nested binders
void test_nested_substitution()
{
    // ∀x. ∃y. z  (where z is a free variable with index 2, true index 0 at depth 2)
    auto z = make_variable(2); // free variable, true index 0 at depth 2
    auto term = make_forall("x", make_exists("y", z));

    std::cout << "Original term structure: ∀x. ∃y. z\n";
    debug_term_structure(term);

    // In De Bruijn substitution, the free variable z at index 2 at depth 2 has true index 0
    SubstitutionMap subst;
    subst[0] = make_constant("Z");

    std::cout << "Substituting free variable with true index 0 with constant 'Z'\n";

    auto result = SubstitutionEngine::substitute(term, subst);

    std::cout << "Result structure:\n";
    debug_term_structure(result);

    auto expected = make_forall("x", make_exists("y", make_constant("Z")));
    std::cout << "Expected structure:\n";
    debug_term_structure(expected);

    bool equals = *result == *expected;
    std::cout << "Result equals expected: " << (equals ? "yes" : "no") << "\n";

    assert(*result == *expected);
    std::cout << "Nested substitution test passed\n";
}

// Test the shift operation directly
void test_shifting()
{
    auto x = make_variable(0);
    auto shifted = SubstitutionEngine::shift(x, 2, 0);

    std::cout << "Shifting variable(0) by 2 with cutoff 0 should give variable(2)\n";
    std::cout << "Result: variable(" << std::dynamic_pointer_cast<VariableDB>(shifted)->index() << ")\n";

    assert(std::dynamic_pointer_cast<VariableDB>(shifted)->index() == 2);

    // Shift only free variables
    auto y = make_variable(1);
    auto body = make_and(x, y);

    std::cout << "Original body: x ∧ y (variable(0) ∧ variable(1))\n";
    debug_term_structure(body);

    auto shifted_body = SubstitutionEngine::shift(body, 1, 1);

    std::cout << "After shifting by 1 with cutoff 1:\n";
    debug_term_structure(shifted_body);

    // x should remain at index 0 (since index < cutoff), y should be shifted to 2
    auto expected = make_and(make_variable(0), make_variable(2));

    std::cout << "Expected:\n";
    debug_term_structure(expected);

    bool equals = *shifted_body == *expected;
    std::cout << "Result equals expected: " << (equals ? "yes" : "no") << "\n";

    assert(*shifted_body == *expected);
    std::cout << "Shifting test passed\n";
}

// Test more complex substitution with multiple binders
void test_complex_substitution()
{
    // ∀x. ∀y. (x → y) → z
    auto x = make_variable(1); // index 1 - bound by outer forall
    auto y = make_variable(0); // index 0 - bound by inner forall
    auto z = make_variable(2); // index 2 - free variable, true index 0 at depth 2

    auto implication1 = make_implies(x, y);
    auto implication2 = make_implies(implication1, z);
    auto body = make_forall("y", implication2);
    auto term = make_forall("x", body);

    std::cout << "Original term: ∀x. ∀y. (x → y) → z\n";
    debug_term_structure(term);

    // In De Bruijn substitution, the free variable z at index 2 at depth 2 has true index 0
    SubstitutionMap subst;
    subst[0] = make_constant("C");

    std::cout << "Substituting free variable with true index 0 with constant 'C'\n";

    auto result = SubstitutionEngine::substitute(term, subst);

    std::cout << "Result structure:\n";
    debug_term_structure(result);

    // Expect: ∀x. ∀y. (x → y) → C
    auto expected_inner = make_implies(x, y);
    auto expected_outer = make_implies(expected_inner, make_constant("C"));
    auto expected_body = make_forall("y", expected_outer);
    auto expected = make_forall("x", expected_body);

    std::cout << "Expected structure:\n";
    debug_term_structure(expected);

    bool equals = *result == *expected;
    std::cout << "Result equals expected: " << (equals ? "yes" : "no") << "\n";

    assert(*result == *expected);
    std::cout << "Complex substitution test passed\n";
}

// Test that substitution avoids variable capture
void test_substitution_capture_avoidance()
{
    // ∀x. y where y is free
    auto y = make_variable(1); // Free variable, true index 0 at depth 1
    auto term = make_forall("x", y);

    std::cout << "Original term: ∀x. y\n";
    debug_term_structure(term);

    // Substitute y with a term containing a free variable
    // that would be captured if we're not careful: variable(0)
    auto inner_var = make_variable(0);

    std::cout << "Substituting y (true index 0) with variable(0)\n";

    SubstitutionMap subst;
    subst[0] = inner_var;

    auto result = SubstitutionEngine::substitute(term, subst);

    std::cout << "Result structure:\n";
    debug_term_structure(result);

    // The expected result should have the variable properly shifted
    // to avoid being captured: ∀x. 0 (the innermost bound variable)
    auto expected = make_forall("x", make_variable(0));

    std::cout << "Expected structure:\n";
    debug_term_structure(expected);

    bool equals = *result == *expected;
    std::cout << "Result equals expected: " << (equals ? "yes" : "no") << "\n";

    assert(*result == *expected);
    std::cout << "Substitution capture avoidance test passed\n";
}

void test_reference_to_outer_bound_variable()
{
    // ∀x. ∃y. x
    auto x_ref = make_variable(1); // Reference to x from inside ∃y
    auto term = make_forall("x", make_exists("y", x_ref));

    std::cout << "Term referencing outer bound variable: ∀x. ∃y. x\n";
    debug_term_structure(term);

    // This is a reference to a bound variable, not a free variable
    // No substitution should happen in standard De Bruijn substitution
    SubstitutionMap subst;
    subst[1] = make_constant("Z"); // This should NOT affect the bound variable

    auto result = SubstitutionEngine::substitute(term, subst);

    std::cout << "Result after attempted substitution of bound variable:\n";
    debug_term_structure(result);

    // The result should be unchanged - bound variables are not substituted
    bool equals = *result == *term;
    std::cout << "Result unchanged: " << (equals ? "yes" : "no") << "\n";

    assert(*result == *term);
    std::cout << "Reference to outer bound variable test passed\n";
}

int main()
{
    std::cout << "Running substitution tests...\n";
    test_basic_substitution();
    test_free_substitution();
    test_nested_substitution();
    test_shifting();
    test_complex_substitution();
    test_substitution_capture_avoidance();
    test_reference_to_outer_bound_variable();
    std::cout << "All substitution tests passed!\n";
    return 0;
}