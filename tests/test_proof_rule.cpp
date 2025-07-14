#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include "../src/proof/proof_state.hpp"
#include "../src/rule/proof_rule.hpp"
#include "../src/term/term_db.hpp"
#include "../src/term/substitution.hpp"

using namespace theorem_prover;

// Utility for printing test results
#define TEST(name)                                   \
    std::cout << "Running test: " << name << "... "; \
    try                                              \
    {

#define END_TEST                                          \
    std::cout << "PASSED" << std::endl;                   \
    }                                                     \
    catch (const std::exception &e)                       \
    {                                                     \
        std::cout << "FAILED: " << e.what() << std::endl; \
        return false;                                     \
    }

// Create a simple propositional formula for testing
TermDBPtr create_proposition(const std::string &symbol)
{
    return make_constant(symbol);
}

// Create an implication formula: A -> B
TermDBPtr create_implication(const TermDBPtr &a, const TermDBPtr &b)
{
    return make_implies(a, b);
}

// Create a conjunction formula: A ∧ B
TermDBPtr create_conjunction(const TermDBPtr &a, const TermDBPtr &b)
{
    return make_and(a, b);
}

// Create a disjunction formula: A ∨ B
TermDBPtr create_disjunction(const TermDBPtr &a, const TermDBPtr &b)
{
    return make_or(a, b);
}

// Create a negation formula: ¬A
TermDBPtr create_negation(const TermDBPtr &a)
{
    return make_not(a);
}

// Helper functions to set up states and apply rules
ProofStatePtr add_hypothesis(ProofContext &context, const ProofStatePtr &state,
                             const std::string &name, const TermDBPtr &formula)
{
    std::vector<Hypothesis> new_hyps = {Hypothesis(name, formula)};
    return context.apply_rule(state, "assumption", {}, new_hyps, state->goal());
}

// Test Modus Ponens rule
bool test_modus_ponens()
{
    TEST("Modus Ponens - Basic Application")
    ProofContext context;

    // Create propositions P and Q
    auto p = create_proposition("P");
    auto q = create_proposition("Q");
    auto p_implies_q = create_implication(p, q);

    // Create initial state with goal Q
    auto initial_state = context.create_initial_state(q);

    // Add hypotheses P and P → Q
    auto state_with_p = add_hypothesis(context, initial_state, "p_hyp", p);
    auto state_with_both = add_hypothesis(context, state_with_p, "implies_hyp", p_implies_q);

    // Create and apply the rule
    ModusPonensRule mp_rule("p_hyp", "implies_hyp");

    // Check if rule is applicable
    assert(mp_rule.is_applicable(state_with_both));

    // Apply the rule
    std::optional<ConstraintViolation> violation;
    auto results = mp_rule.apply(context, state_with_both, violation);

    // Should have exactly one result state
    assert(results.size() == 1);

    // Result state should have a new hypothesis with formula Q
    auto result_state = results[0];
    bool found_conclusion = false;

    for (const auto &hyp : result_state->hypotheses())
    {
        if (*hyp.formula() == *q)
        {
            found_conclusion = true;
            break;
        }
    }

    assert(found_conclusion);
    END_TEST

    TEST("Modus Ponens - Not Applicable")
    ProofContext context;

    // Create propositions P, Q, and R
    auto p = create_proposition("P");
    auto q = create_proposition("Q");
    auto r = create_proposition("R");
    auto q_implies_r = create_implication(q, r);

    // Create initial state with goal R
    auto initial_state = context.create_initial_state(r);

    // Add hypotheses P and Q → R (not matching the pattern for modus ponens)
    auto state_with_p = add_hypothesis(context, initial_state, "p_hyp", p);
    auto state_with_both = add_hypothesis(context, state_with_p, "implies_hyp", q_implies_r);

    // Create and check the rule
    ModusPonensRule mp_rule("p_hyp", "implies_hyp");

    // Rule should not be applicable
    assert(!mp_rule.is_applicable(state_with_both));
    END_TEST

    return true;
}

// Test And Introduction rule
bool test_and_introduction()
{
    TEST("And Introduction - Basic Application")
    ProofContext context;

    // Create propositions P and Q
    auto p = create_proposition("P");
    auto q = create_proposition("Q");
    auto p_and_q = create_conjunction(p, q);

    // Create initial state with goal P ∧ Q
    auto initial_state = context.create_initial_state(p_and_q);

    // Add hypotheses P and Q
    auto state_with_p = add_hypothesis(context, initial_state, "p_hyp", p);
    auto state_with_both = add_hypothesis(context, state_with_p, "q_hyp", q);

    // Create and apply the rule
    AndIntroRule and_rule("p_hyp", "q_hyp");

    // Check if rule is applicable
    assert(and_rule.is_applicable(state_with_both));

    // Apply the rule
    std::optional<ConstraintViolation> violation;
    auto results = and_rule.apply(context, state_with_both, violation);

    // Should have exactly one result state
    assert(results.size() == 1);

    // Result state should have a new hypothesis with formula P ∧ Q
    auto result_state = results[0];
    bool found_conjunction = false;

    for (const auto &hyp : result_state->hypotheses())
    {
        if (*hyp.formula() == *p_and_q)
        {
            found_conjunction = true;
            break;
        }
    }

    assert(found_conjunction);
    END_TEST

    return true;
}

// Test And Elimination rule
bool test_and_elimination()
{
    TEST("And Elimination - Left Component")
    ProofContext context;

    // Create propositions P and Q
    auto p = create_proposition("P");
    auto q = create_proposition("Q");
    auto p_and_q = create_conjunction(p, q);

    // Create initial state with goal P
    auto initial_state = context.create_initial_state(p);

    // Add hypothesis P ∧ Q
    auto state_with_conjunction = add_hypothesis(context, initial_state, "conj_hyp", p_and_q);

    // Create and apply the rule for left elimination
    AndElimRule left_rule("conj_hyp", true);

    // Check if rule is applicable
    assert(left_rule.is_applicable(state_with_conjunction));

    // Apply the rule
    std::optional<ConstraintViolation> violation;
    auto results = left_rule.apply(context, state_with_conjunction, violation);

    // Should have exactly one result state
    assert(results.size() == 1);

    // Result state should have a new hypothesis with formula P
    auto result_state = results[0];
    bool found_left = false;

    for (const auto &hyp : result_state->hypotheses())
    {
        if (*hyp.formula() == *p)
        {
            found_left = true;
            break;
        }
    }

    assert(found_left);
    END_TEST

    TEST("And Elimination - Right Component")
    ProofContext context;

    // Create propositions P and Q
    auto p = create_proposition("P");
    auto q = create_proposition("Q");
    auto p_and_q = create_conjunction(p, q);

    // Create initial state with goal Q
    auto initial_state = context.create_initial_state(q);

    // Add hypothesis P ∧ Q
    auto state_with_conjunction = add_hypothesis(context, initial_state, "conj_hyp", p_and_q);

    // Create and apply the rule for right elimination
    AndElimRule right_rule("conj_hyp", false);

    // Check if rule is applicable
    assert(right_rule.is_applicable(state_with_conjunction));

    // Apply the rule
    std::optional<ConstraintViolation> violation;
    auto results = right_rule.apply(context, state_with_conjunction, violation);

    // Should have exactly one result state
    assert(results.size() == 1);

    // Result state should have a new hypothesis with formula Q
    auto result_state = results[0];
    bool found_right = false;

    for (const auto &hyp : result_state->hypotheses())
    {
        if (*hyp.formula() == *q)
        {
            found_right = true;
            break;
        }
    }

    assert(found_right);
    END_TEST

    return true;
}

// Test Or Introduction rule
bool test_or_introduction()
{
    TEST("Or Introduction - Left Component")
    ProofContext context;

    // Create propositions P and Q
    auto p = create_proposition("P");
    auto q = create_proposition("Q");
    auto p_or_q = create_disjunction(p, q);

    // Create initial state with goal P ∨ Q
    auto initial_state = context.create_initial_state(p_or_q);

    // Add hypothesis P
    auto state_with_p = add_hypothesis(context, initial_state, "p_hyp", p);

    // Create and apply the rule for introducing P on the left
    OrIntroRule left_rule("p_hyp", q, true);

    // Check if rule is applicable
    assert(left_rule.is_applicable(state_with_p));

    // Apply the rule
    std::optional<ConstraintViolation> violation;
    auto results = left_rule.apply(context, state_with_p, violation);

    // Should have exactly one result state
    assert(results.size() == 1);

    // Result state should have a new hypothesis with formula P ∨ Q
    auto result_state = results[0];
    bool found_disjunction = false;

    for (const auto &hyp : result_state->hypotheses())
    {
        if (*hyp.formula() == *p_or_q)
        {
            found_disjunction = true;
            break;
        }
    }

    assert(found_disjunction);
    END_TEST

    TEST("Or Introduction - Right Component")
    ProofContext context;

    // Create propositions P and Q
    auto p = create_proposition("P");
    auto q = create_proposition("Q");
    auto p_or_q = create_disjunction(p, q);

    // Create initial state with goal P ∨ Q
    auto initial_state = context.create_initial_state(p_or_q);

    // Add hypothesis Q
    auto state_with_q = add_hypothesis(context, initial_state, "q_hyp", q);

    // Create and apply the rule for introducing Q on the right
    OrIntroRule right_rule("q_hyp", p, false);

    // Check if rule is applicable
    assert(right_rule.is_applicable(state_with_q));

    // Apply the rule
    std::optional<ConstraintViolation> violation;
    auto results = right_rule.apply(context, state_with_q, violation);

    // Should have exactly one result state
    assert(results.size() == 1);

    // Result state should have a new hypothesis with formula P ∨ Q
    auto result_state = results[0];
    bool found_disjunction = false;

    for (const auto &hyp : result_state->hypotheses())
    {
        if (*hyp.formula() == *p_or_q)
        {
            found_disjunction = true;
            break;
        }
    }

    assert(found_disjunction);
    END_TEST

    return true;
}

// Test Implication Introduction rule
bool test_implies_introduction()
{
    TEST("Implication Introduction")
    ProofContext context;

    // Create propositions P and Q
    auto p = create_proposition("P");
    auto q = create_proposition("Q");
    auto p_implies_q = create_implication(p, q);

    // Create initial state with goal P → Q
    auto initial_state = context.create_initial_state(p_implies_q);

    // Create and apply the rule
    ImpliesIntroRule implies_rule;

    // Check if rule is applicable
    assert(implies_rule.is_applicable(initial_state));

    // Apply the rule
    std::optional<ConstraintViolation> violation;
    auto results = implies_rule.apply(context, initial_state, violation);

    // Should have exactly one result state
    assert(results.size() == 1);

    // Result state should have P as a hypothesis and Q as the goal
    auto result_state = results[0];

    // Check if P is now a hypothesis
    bool found_p_hyp = false;
    for (const auto &hyp : result_state->hypotheses())
    {
        if (*hyp.formula() == *p)
        {
            found_p_hyp = true;
            break;
        }
    }
    assert(found_p_hyp);

    // Check if Q is now the goal
    assert(*result_state->goal() == *q);
    END_TEST

    TEST("Implication Introduction - Not Applicable")
    ProofContext context;

    // Create a non-implication goal
    auto p = create_proposition("P");
    auto initial_state = context.create_initial_state(p);

    // Create the rule
    ImpliesIntroRule implies_rule;

    // Check that rule is not applicable for non-implication goals
    assert(!implies_rule.is_applicable(initial_state));
    END_TEST

    return true;
}

// Test Contradiction rule
bool test_contradiction()
{
    TEST("Contradiction Rule")
    ProofContext context;

    // Create propositions P and ¬P
    auto p = create_proposition("P");
    auto not_p = create_negation(p);
    auto q = create_proposition("Q"); // Goal we want to prove from contradiction

    // Create initial state with goal Q
    auto initial_state = context.create_initial_state(q);

    // Add hypotheses P and ¬P
    auto state_with_p = add_hypothesis(context, initial_state, "p_hyp", p);
    auto state_with_contradiction = add_hypothesis(context, state_with_p, "not_p_hyp", not_p);

    // Create and apply the rule
    ContradictionRule contradiction_rule("p_hyp", "not_p_hyp");

    // Check if rule is applicable
    assert(contradiction_rule.is_applicable(state_with_contradiction));

    // Apply the rule
    std::optional<ConstraintViolation> violation;
    auto results = contradiction_rule.apply(context, state_with_contradiction, violation);

    // Should have exactly one result state
    assert(results.size() == 1);

    // Result state should be marked as proved
    auto result_state = results[0];
    assert(result_state->is_proved());
    assert(result_state->certification().status == ProofCertification::Status::CONTRADICTION);
    END_TEST

    return true;
}

// Test complex proof sequence
bool test_complex_proof()
{
    TEST("Complex Proof - Modus Ponens with And Introduction")
    ProofContext context;

    // Create formulas for: (P ∧ Q) → R, then prove R from P and Q
    auto p = create_proposition("P");
    auto q = create_proposition("Q");
    auto r = create_proposition("R");
    auto p_and_q = create_conjunction(p, q);
    auto implies = create_implication(p_and_q, r);

    // Create initial state with goal R
    auto initial_state = context.create_initial_state(r);

    // Add hypotheses P, Q, and (P ∧ Q) → R
    auto state_with_p = add_hypothesis(context, initial_state, "p_hyp", p);
    auto state_with_pq = add_hypothesis(context, state_with_p, "q_hyp", q);
    auto state_with_all = add_hypothesis(context, state_with_pq, "implies_hyp", implies);

    // First step: Use And Introduction to derive P ∧ Q
    AndIntroRule and_rule("p_hyp", "q_hyp");
    std::optional<ConstraintViolation> violation1;
    auto and_results = and_rule.apply(context, state_with_all, violation1);
    assert(and_results.size() == 1);
    auto state_with_conjunction = and_results[0];

    // Find the name of the P ∧ Q hypothesis
    std::string conjunction_hyp_name;
    for (const auto &hyp : state_with_conjunction->hypotheses())
    {
        if (*hyp.formula() == *p_and_q)
        {
            conjunction_hyp_name = hyp.name();
            break;
        }
    }
    assert(!conjunction_hyp_name.empty());

    // Second step: Use Modus Ponens with P ∧ Q and (P ∧ Q) → R to derive R
    ModusPonensRule mp_rule(conjunction_hyp_name, "implies_hyp");
    std::optional<ConstraintViolation> violation2;
    auto mp_results = mp_rule.apply(context, state_with_conjunction, violation2);
    assert(mp_results.size() == 1);
    auto final_state = mp_results[0];

    // Final state should have a hypothesis with formula R
    bool found_conclusion = false;
    for (const auto &hyp : final_state->hypotheses())
    {
        if (*hyp.formula() == *r)
        {
            found_conclusion = true;
            break;
        }
    }

    assert(found_conclusion);
    END_TEST

    return true;
}

// Main test runner
int main()
{
    bool all_passed = true;

    std::cout << "===== Running Proof Rule Tests =====" << std::endl;

    all_passed &= test_modus_ponens();
    all_passed &= test_and_introduction();
    all_passed &= test_and_elimination();
    all_passed &= test_or_introduction();
    all_passed &= test_implies_introduction();
    all_passed &= test_contradiction();
    all_passed &= test_complex_proof();

    if (all_passed)
    {
        std::cout << "===== All tests passed! =====" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "===== Some tests failed! =====" << std::endl;
        return 1;
    }
}