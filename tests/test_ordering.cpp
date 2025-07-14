#include <iostream>
#include <cassert>
#include <chrono>
#include "../src/term/ordering.hpp"
#include "../src/term/term_db.hpp"

using namespace theorem_prover;

void test_precedence_basics()
{
    std::cout << "Testing precedence basics..." << std::endl;

    auto precedence = std::make_shared<Precedence>();

    // Test reflexivity (should be false for strict ordering)
    assert(!precedence->greater("f", "f"));
    assert(precedence->equal("f", "f"));

    // Test explicit precedence setting
    precedence->set_greater("f", "g");
    assert(precedence->greater("f", "g"));
    assert(!precedence->greater("g", "f"));
    assert(!precedence->equal("f", "g"));

    // Test transitivity
    precedence->set_greater("g", "h");
    assert(precedence->greater("f", "h"));
    assert(precedence->greater("g", "h"));

    // Test total ordering fallback
    assert(precedence->total_greater("z", "a")); // lexicographic fallback
    assert(!precedence->total_greater("a", "z"));

    std::cout << "Precedence basics tests passed!" << std::endl;
}

void test_variable_minimality()
{
    std::cout << "Testing variable minimality..." << std::endl;

    auto lpo = make_lpo();

    // Create test terms
    auto x = make_variable(0);
    auto y = make_variable(1);
    auto a = make_constant("a");
    auto f_a = make_function_application("f", {a});

    // Variables are not greater than other variables
    assert(!lpo->greater(x, y));
    assert(!lpo->greater(y, x));
    assert(lpo->equivalent(x, y));

    // Non-variables are greater than variables
    assert(lpo->greater(a, x));
    assert(lpo->greater(f_a, x));
    assert(!lpo->greater(x, a));
    assert(!lpo->greater(x, f_a));

    // Variables are equivalent to themselves
    assert(lpo->equivalent(x, x));
    assert(!lpo->greater(x, x));

    std::cout << "Variable minimality tests passed!" << std::endl;
}

void test_subterm_property()
{
    std::cout << "Testing subterm property..." << std::endl;

    auto lpo = make_lpo();

    // Create test terms
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto f_a = make_function_application("f", {a});
    auto g_a_b = make_function_application("g", {a, b});
    auto h_f_a = make_function_application("h", {f_a});
    auto f_g_a_b = make_function_application("f", {g_a_b});

    // Direct subterm: f(a) > a
    assert(lpo->greater(f_a, a));
    assert(!lpo->greater(a, f_a));

    // Indirect subterm: h(f(a)) > a
    assert(lpo->greater(h_f_a, a));
    assert(lpo->greater(h_f_a, f_a));

    // Multiple arguments: g(a,b) > a and g(a,b) > b
    assert(lpo->greater(g_a_b, a));
    assert(lpo->greater(g_a_b, b));

    // Nested: f(g(a,b)) > g(a,b) > a
    assert(lpo->greater(f_g_a_b, g_a_b));
    assert(lpo->greater(f_g_a_b, a));
    assert(lpo->greater(f_g_a_b, b));

    std::cout << "Subterm property tests passed!" << std::endl;
}

void test_lexicographic_comparison()
{
    std::cout << "Testing lexicographic comparison..." << std::endl;

    auto lpo = make_lpo();

    // Create test terms
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto c = make_constant("c");

    auto f_a_b = make_function_application("f", {a, b});
    auto f_a_c = make_function_application("f", {a, c});
    auto f_b_a = make_function_application("f", {b, a});

    // When function symbols are equal, compare arguments lexicographically
    // Default precedence uses lexicographic ordering on symbol names
    if (lpo->get_precedence()->total_greater("c", "b"))
    {
        assert(lpo->greater(f_a_c, f_a_b));
    }
    else
    {
        assert(lpo->greater(f_a_b, f_a_c));
    }

    // f(b,a) vs f(a,b) comparison
    if (lpo->get_precedence()->total_greater("b", "a"))
    {
        assert(lpo->greater(f_b_a, f_a_b));
    }
    else
    {
        assert(lpo->greater(f_a_b, f_b_a));
    }

    std::cout << "Lexicographic comparison tests passed!" << std::endl;
}

void test_precedence_comparison()
{
    std::cout << "Testing precedence comparison..." << std::endl;

    auto precedence = std::make_shared<Precedence>();
    auto lpo = make_lpo(precedence);

    // Set up precedence: h > g > f
    precedence->set_greater("h", "g");
    precedence->set_greater("g", "f");

    auto x = make_variable(0);
    auto a = make_constant("a");
    auto f_a = make_function_application("f", {a});

    auto f_x = make_function_application("f", {x});
    auto g_x = make_function_application("g", {x});
    auto h_x = make_function_application("h", {x});

    // Higher precedence functions dominate
    assert(lpo->greater(h_x, g_x));
    assert(lpo->greater(g_x, f_x));
    assert(lpo->greater(h_x, f_x));

    // Test with different arguments
    auto h_a = make_function_application("h", {a});
    auto g_f_a = make_function_application("g", {f_a});

    // h(a) > g(f(a)) because h > g in precedence
    assert(lpo->greater(h_a, g_f_a));

    std::cout << "Precedence comparison tests passed!" << std::endl;
}

void test_logical_connectives()
{
    std::cout << "Testing logical connectives..." << std::endl;

    auto lpo = make_lpo();

    // Create test terms
    auto a = make_constant("a");
    auto b = make_constant("b");

    // Create logical formulas
    auto and_ab = make_and(a, b);
    auto or_ab = make_or(a, b);
    auto not_a = make_not(a);
    auto implies_ab = make_implies(a, b);

    // Logical connectives should be greater than their subformulas
    assert(lpo->greater(and_ab, a));
    assert(lpo->greater(and_ab, b));
    assert(lpo->greater(or_ab, a));
    assert(lpo->greater(or_ab, b));
    assert(lpo->greater(not_a, a));
    assert(lpo->greater(implies_ab, a));
    assert(lpo->greater(implies_ab, b));

    std::cout << "Logical connectives tests passed!" << std::endl;
}

void test_ordering_properties()
{
    std::cout << "Testing ordering properties..." << std::endl;

    auto lpo = make_lpo();

    // Create test terms
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto x = make_variable(0);
    auto f_a = make_function_application("f", {a});
    auto g_a_b = make_function_application("g", {a, b});
    auto h_f_a = make_function_application("h", {f_a});

    std::vector<TermDBPtr> test_terms = {a, b, x, f_a, g_a_b, h_f_a};

    // Test irreflexivity: no term is greater than itself
    for (const auto &term : test_terms)
    {
        assert(!lpo->greater(term, term));
    }

    // Test asymmetry: if s > t then not t > s
    for (size_t i = 0; i < test_terms.size(); ++i)
    {
        for (size_t j = 0; j < test_terms.size(); ++j)
        {
            if (i != j)
            {
                auto s = test_terms[i];
                auto t = test_terms[j];
                if (lpo->greater(s, t))
                {
                    assert(!lpo->greater(t, s));
                }
            }
        }
    }

    // Test that either s > t, t > s, or s ~ t (totality on ground terms)
    for (size_t i = 0; i < test_terms.size(); ++i)
    {
        for (size_t j = 0; j < test_terms.size(); ++j)
        {
            auto s = test_terms[i];
            auto t = test_terms[j];
            bool s_greater = lpo->greater(s, t);
            bool t_greater = lpo->greater(t, s);
            bool equivalent = lpo->equivalent(s, t);

            // Exactly one should be true
            assert((s_greater && !t_greater && !equivalent) ||
                   (!s_greater && t_greater && !equivalent) ||
                   (!s_greater && !t_greater && equivalent));
        }
    }

    std::cout << "Ordering properties tests passed!" << std::endl;
}

void test_complex_nesting()
{
    std::cout << "Testing complex nesting..." << std::endl;

    auto lpo = make_lpo();

    // Create test terms
    auto x = make_variable(0);
    auto y = make_variable(1);
    auto a = make_constant("a");
    auto f_x = make_function_application("f", {x});

    // Create complex nested terms
    auto g_x_y = make_function_application("g", {x, y});
    auto f_g_x_y = make_function_application("f", {g_x_y});
    auto f_y = make_function_application("f", {y});
    auto g_f_x_f_y = make_function_application("g", {f_x, f_y});

    // Both terms should be greater than their subterms
    assert(lpo->greater(f_g_x_y, x));
    assert(lpo->greater(f_g_x_y, y));
    assert(lpo->greater(g_f_x_f_y, x));
    assert(lpo->greater(g_f_x_f_y, y));

    // Test specific comparison based on precedence
    if (lpo->get_precedence()->total_greater("f", "g"))
    {
        assert(lpo->greater(f_g_x_y, g_f_x_f_y));
    }
    else
    {
        assert(lpo->greater(g_f_x_f_y, f_g_x_y));
    }

    std::cout << "Complex nesting tests passed!" << std::endl;
}

void test_edge_cases()
{
    std::cout << "Testing edge cases..." << std::endl;

    auto lpo = make_lpo();

    // Create test terms
    auto a = make_constant("a");
    auto f_a = make_function_application("f", {a});

    // Empty argument lists (constants)
    assert(!lpo->greater(a, a));
    assert(lpo->equivalent(a, a));

    // Mixed arity comparisons
    auto f_empty = make_function_application("f", {});
    assert(lpo->greater(f_a, f_empty)); // f(a) > f() by argument comparison

    // Very deep nesting
    auto h_a = make_function_application("h", {a});
    auto g_h_a = make_function_application("g", {h_a});
    auto deep_term = make_function_application("f", {g_h_a});

    assert(lpo->greater(deep_term, a));
    assert(lpo->greater(deep_term, h_a));

    std::cout << "Edge cases tests passed!" << std::endl;
}

void test_argument_status()
{
    std::cout << "Testing argument status..." << std::endl;

    auto lpo = make_lpo();

    // Set some function to use multiset comparison
    lpo->set_argument_status("g", ArgumentStatus::MULTISET);

    auto a = make_constant("a");
    auto b = make_constant("b");

    // For now, multiset falls back to lexicographic
    // This test ensures the interface works correctly
    auto g1 = make_function_application("g", {a, b});
    auto g2 = make_function_application("g", {b, a});

    // The result depends on implementation, but should be consistent
    bool result1 = lpo->greater(g1, g2);
    bool result2 = lpo->greater(g2, g1);
    assert(result1 || result2 || lpo->equivalent(g1, g2));

    std::cout << "Argument status tests passed!" << std::endl;
}

void test_performance()
{
    std::cout << "Testing performance..." << std::endl;

    auto lpo = make_lpo();

    // Create test terms
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto c = make_constant("c");

    // Create moderately complex terms
    std::vector<TermDBPtr> complex_terms;

    for (int i = 0; i < 10; ++i)
    {
        auto g_a_b = make_function_application("g", {a, b});
        auto term = make_function_application("f" + std::to_string(i), {g_a_b, c});
        complex_terms.push_back(term);
    }

    // Time a reasonable number of comparisons
    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < complex_terms.size(); ++i)
    {
        for (size_t j = 0; j < complex_terms.size(); ++j)
        {
            lpo->greater(complex_terms[i], complex_terms[j]);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should complete in reasonable time (less than 100ms for this test)
    assert(duration.count() < 100);
    std::cout << "  Performance test completed in " << duration.count() << " ms" << std::endl;

    std::cout << "Performance tests passed!" << std::endl;
}

int main()
{
    std::cout << "===== Running Ordering Tests =====" << std::endl;

    test_precedence_basics();
    test_variable_minimality();
    test_subterm_property();
    test_lexicographic_comparison();
    test_precedence_comparison();
    test_logical_connectives();
    test_ordering_properties();
    test_complex_nesting();
    test_edge_cases();
    test_argument_status();
    test_performance();

    std::cout << "\n===== All Ordering Tests Passed! =====" << std::endl;
    return 0;
}