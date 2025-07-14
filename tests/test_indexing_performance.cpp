#include <iostream>
#include <cassert>
#include <chrono>
#include "../src/resolution/resolution_prover.hpp"
#include "../src/term/term_db.hpp"

using namespace theorem_prover;
using namespace std::chrono;

void test_large_clause_set_performance()
{
    std::cout << "Testing indexing performance on large clause set..." << std::endl;

    ResolutionProver prover;

    // Create many clauses: P(1), P(2), ..., P(50), Q(1), Q(2), ..., Q(50)
    std::vector<TermDBPtr> hypotheses;

    for (int i = 1; i <= 50; ++i)
    {
        auto const_i = make_constant("c" + std::to_string(i));
        auto p_i = make_function_application("P", {const_i});
        auto q_i = make_function_application("Q", {const_i});

        hypotheses.push_back(p_i);
        hypotheses.push_back(q_i);

        // Add implications P(i) → Q(i)
        auto impl = make_implies(p_i, q_i);
        hypotheses.push_back(impl);
    }

    std::cout << "  Generated " << hypotheses.size() << " hypotheses" << std::endl;

    // Goal: Q(c25) (should be provable via P(c25) → Q(c25) and P(c25))
    auto target_const = make_constant("c25");
    auto goal = make_function_application("Q", {target_const});

    auto start_time = high_resolution_clock::now();
    auto result = prover.prove(goal, hypotheses);
    auto end_time = high_resolution_clock::now();

    auto duration = duration_cast<milliseconds>(end_time - start_time);

    assert(result.is_proved());
    std::cout << "  Proof completed in " << duration.count() << " ms" << std::endl;
    std::cout << "  Iterations: " << result.iterations << std::endl;
    std::cout << "  Final clause count: " << result.final_clauses.size() << std::endl;
}

void test_indexing_correctness()
{
    std::cout << "Testing indexing correctness..." << std::endl;

    ResolutionProver prover;

    // Simple test: P(a), P(a) → Q(a) ⊢ Q(a)
    auto a = make_constant("a");
    auto p_a = make_function_application("P", {a});
    auto q_a = make_function_application("Q", {a});
    auto impl = make_implies(p_a, q_a);

    std::vector<TermDBPtr> hypotheses = {p_a, impl};
    auto result = prover.prove(q_a, hypotheses);

    assert(result.is_proved());
    std::cout << "  Basic modus ponens works with indexing" << std::endl;
}

void test_no_false_positives()
{
    std::cout << "Testing no false positives in indexing..." << std::endl;

    ResolutionProver prover;

    // P(a) should NOT prove Q(b)
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto p_a = make_function_application("P", {a});
    auto q_b = make_function_application("Q", {b});

    std::vector<TermDBPtr> hypotheses = {p_a};
    auto result = prover.prove(q_b, hypotheses);

    assert(!result.is_proved());
    std::cout << "  Correctly failed to prove unrelated goal" << std::endl;
}

void test_complex_resolution_with_indexing()
{
    std::cout << "Testing complex resolution with indexing..." << std::endl;

    ResolutionProver prover;

    // More complex: P(a) ∨ Q(b), ¬P(a), ¬Q(b) ∨ R(c) ⊢ R(c)
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto c = make_constant("c");

    auto p_a = make_function_application("P", {a});
    auto q_b = make_function_application("Q", {b});
    auto r_c = make_function_application("R", {c});

    auto clause1 = make_or(p_a, q_b);           // P(a) ∨ Q(b)
    auto clause2 = make_not(p_a);               // ¬P(a)
    auto clause3 = make_or(make_not(q_b), r_c); // ¬Q(b) ∨ R(c)

    std::vector<TermDBPtr> hypotheses = {clause1, clause2, clause3};
    auto result = prover.prove(r_c, hypotheses);

    assert(result.is_proved());
    std::cout << "  Complex disjunctive syllogism works with indexing" << std::endl;
}

void test_indexing_with_quantifiers()
{
    std::cout << "Testing indexing with quantified formulas..." << std::endl;

    ResolutionProver prover;

    // ∀x.P(x), ∀x.(P(x) → Q(x)) ⊢ ∀x.Q(x)
    auto p_x = make_function_application("P", {make_variable(0)});
    auto q_x = make_function_application("Q", {make_variable(0)});

    auto forall_p = make_forall("x", p_x);
    auto impl = make_implies(p_x, q_x);
    auto forall_impl = make_forall("x", impl);
    auto forall_q = make_forall("x", q_x);

    std::vector<TermDBPtr> hypotheses = {forall_p, forall_impl};
    auto result = prover.prove(forall_q, hypotheses);

    assert(result.is_proved());
    std::cout << "  Quantified reasoning works with indexing" << std::endl;
}

void test_scaling_behavior()
{
    std::cout << "Testing scaling behavior..." << std::endl;

    // Test different problem sizes to see indexing benefits
    std::vector<int> sizes = {10, 20, 30};

    for (int size : sizes)
    {
        ResolutionProver prover;
        std::vector<TermDBPtr> hypotheses;

        // Generate size*3 clauses
        for (int i = 1; i <= size; ++i)
        {
            auto const_i = make_constant("c" + std::to_string(i));
            auto p_i = make_function_application("P", {const_i});
            auto q_i = make_function_application("Q", {const_i});

            hypotheses.push_back(p_i);
            hypotheses.push_back(make_implies(p_i, q_i));
        }

        // Goal: Q(c1)
        auto goal = make_function_application("Q", {make_constant("c1")});

        auto start_time = high_resolution_clock::now();
        auto result = prover.prove(goal, hypotheses);
        auto end_time = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(end_time - start_time);

        assert(result.is_proved());
        std::cout << "  Size " << size << ": " << duration.count() << " μs, "
                  << result.iterations << " iterations" << std::endl;
    }
}

int main()
{
    std::cout << "===== Running Indexing Performance Tests =====" << std::endl;

    test_indexing_correctness();
    test_no_false_positives();
    test_complex_resolution_with_indexing();
    test_indexing_with_quantifiers();
    test_scaling_behavior();
    test_large_clause_set_performance();

    std::cout << "\n===== All Indexing Tests Passed! =====" << std::endl;
    return 0;
}