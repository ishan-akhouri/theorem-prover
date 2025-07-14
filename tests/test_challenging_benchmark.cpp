#include <iostream>
#include <cassert>
#include <chrono>
#include <vector>
#include <iomanip>
#include "../src/term/term_db.hpp"
#include "../src/resolution/resolution_prover.hpp"

using namespace theorem_prover;
using namespace std::chrono;

struct ChallengingBenchmarkResult {
    std::string config_name;
    std::string problem_name;
    ResolutionProofResult::Status status;
    double time_ms;
    size_t iterations;
    size_t final_clauses;
    
    ChallengingBenchmarkResult(const std::string& config, const std::string& problem, 
                              const ResolutionProofResult& result, double time)
        : config_name(config), problem_name(problem), status(result.status), 
          time_ms(time), iterations(result.iterations), final_clauses(result.final_clauses.size()) {}
};

void print_challenging_benchmark_header() {
    std::cout << std::setw(30) << "Problem" 
              << std::setw(25) << "Configuration" 
              << std::setw(12) << "Status" 
              << std::setw(12) << "Time(ms)" 
              << std::setw(10) << "Iterations" 
              << std::setw(12) << "Clauses" << std::endl;
    std::cout << std::string(100, '-') << std::endl;
}

void print_challenging_result(const ChallengingBenchmarkResult& result) {
    std::string status_str;
    switch (result.status) {
        case ResolutionProofResult::Status::PROVED: status_str = "PROVED"; break;
        case ResolutionProofResult::Status::TIMEOUT: status_str = "TIMEOUT"; break;
        case ResolutionProofResult::Status::SATURATED: status_str = "SATURATED"; break;
        default: status_str = "OTHER"; break;
    }
    
    std::cout << std::setw(30) << result.problem_name
              << std::setw(25) << result.config_name 
              << std::setw(12) << status_str
              << std::setw(12) << std::fixed << std::setprecision(1) << result.time_ms
              << std::setw(10) << result.iterations 
              << std::setw(12) << result.final_clauses << std::endl;
}

std::vector<ChallengingBenchmarkResult> run_challenging_benchmark(
    const std::string& test_name,
    const TermDBPtr& goal,
    const std::vector<TermDBPtr>& hypotheses) {
    
    std::cout << "\n=== " << test_name << " ===" << std::endl;
    std::vector<ChallengingBenchmarkResult> results;
    
    // Configuration 1: Basic Resolution (Expected to fail)
    {
        ResolutionConfig config;
        config.use_paramodulation = false;
        config.use_kb_preprocessing = false;
        config.max_iterations = 3000;
        config.max_time_ms = 30000.0; // 30 seconds
        config.max_clauses = 50000;
        
        ResolutionProver prover(config);
        auto start = high_resolution_clock::now();
        auto result = prover.prove(goal, hypotheses);
        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
        
        results.emplace_back("Basic Resolution", test_name, result, duration_ms);
    }
    
    // Configuration 2: Standard Paramodulation
    {
        ResolutionConfig config;
        config.use_paramodulation = true;
        config.use_kb_preprocessing = false;
        config.max_iterations = 3000;
        config.max_time_ms = 30000.0;
        config.max_clauses = 50000;
        
        ResolutionProver prover(config);
        auto start = high_resolution_clock::now();
        auto result = prover.prove(goal, hypotheses);
        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
        
        results.emplace_back("Standard Paramodulation", test_name, result, duration_ms);
    }
    
    // Configuration 3: KB-Optimized Paramodulation
    {
        ResolutionConfig config;
        config.use_paramodulation = true;
        config.use_kb_preprocessing = true;
        config.kb_preprocessing_timeout = 10.0; // Longer timeout for complex cases
        config.kb_max_equations = 25;
        config.kb_max_rules = 50;
        config.max_iterations = 3000;
        config.max_time_ms = 30000.0;
        config.max_clauses = 50000;
        
        ResolutionProver prover(config);
        auto start = high_resolution_clock::now();
        auto result = prover.prove(goal, hypotheses);
        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<microseconds>(end - start).count() / 1000.0;
        
        results.emplace_back("KB-Optimized Paramod", test_name, result, duration_ms);
    }
    
    return results;
}

void test_long_equality_chain() {
    // Test: a=b ∧ b=c ∧ c=d ∧ d=e ∧ e=f ∧ f=g ∧ g=h ∧ P(a) ⊢ P(h)
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto c = make_constant("c");
    auto d = make_constant("d");
    auto e = make_constant("e");
    auto f = make_constant("f");
    auto g = make_constant("g");
    auto h = make_constant("h");
    
    auto eq1 = make_function_application("=", {a, b});
    auto eq2 = make_function_application("=", {b, c});
    auto eq3 = make_function_application("=", {c, d});
    auto eq4 = make_function_application("=", {d, e});
    auto eq5 = make_function_application("=", {e, f});
    auto eq6 = make_function_application("=", {f, g});
    auto eq7 = make_function_application("=", {g, h});
    
    auto P_a = make_function_application("P", {a});
    auto P_h = make_function_application("P", {h});
    
    // Chain all equalities together
    auto chain1 = make_and(eq1, eq2);
    auto chain2 = make_and(chain1, eq3);
    auto chain3 = make_and(chain2, eq4);
    auto chain4 = make_and(chain3, eq5);
    auto chain5 = make_and(chain4, eq6);
    auto chain6 = make_and(chain5, eq7);
    auto hypothesis = make_and(chain6, P_a);
    
    auto results = run_challenging_benchmark("Long Equality Chain (7 steps)", P_h, {hypothesis});
    
    print_challenging_benchmark_header();
    for (const auto& result : results) {
        print_challenging_result(result);
    }
}

void test_multiple_function_interactions() {
    // Test: f(x)=g(x) ∧ g(x)=h(x) ∧ h(x)=j(x) ∧ P(f(a),g(b),h(c)) ⊢ P(j(a),j(b),j(c))
    auto x = make_variable(0);
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto c = make_constant("c");
    
    auto f_x = make_function_application("f", {x});
    auto g_x = make_function_application("g", {x});
    auto h_x = make_function_application("h", {x});
    auto j_x = make_function_application("j", {x});
    
    auto f_a = make_function_application("f", {a});
    auto g_b = make_function_application("g", {b});
    auto h_c = make_function_application("h", {c});
    
    auto j_a = make_function_application("j", {a});
    auto j_b = make_function_application("j", {b});
    auto j_c = make_function_application("j", {c});
    
    auto eq1 = make_function_application("=", {f_x, g_x});
    auto eq2 = make_function_application("=", {g_x, h_x});
    auto eq3 = make_function_application("=", {h_x, j_x});
    
    auto P_original = make_function_application("P", {f_a, g_b, h_c});
    auto P_goal = make_function_application("P", {j_a, j_b, j_c});
    
    auto eq_chain = make_and(eq1, make_and(eq2, eq3));
    auto hypothesis = make_and(eq_chain, P_original);
    
    auto results = run_challenging_benchmark("Multiple Function Interactions", P_goal, {hypothesis});
    
    for (const auto& result : results) {
        print_challenging_result(result);
    }
}

void test_recursive_function_definitions() {
    // Test: f(f(x))=x ∧ g(f(x))=g(x) ∧ P(f(f(f(a)))) ⊢ P(f(a))
    auto x = make_variable(0);
    auto a = make_constant("a");
    
    auto f_x = make_function_application("f", {x});
    auto f_f_x = make_function_application("f", {f_x});
    auto g_x = make_function_application("g", {x});
    auto g_f_x = make_function_application("g", {f_x});
    
    auto f_a = make_function_application("f", {a});
    auto f_f_a = make_function_application("f", {f_a});
    auto f_f_f_a = make_function_application("f", {f_f_a});
    
    auto eq1 = make_function_application("=", {f_f_x, x}); // f(f(x)) = x
    auto eq2 = make_function_application("=", {g_f_x, g_x}); // g(f(x)) = g(x)
    
    auto P_complex = make_function_application("P", {f_f_f_a});
    auto P_simple = make_function_application("P", {f_a});
    
    auto eq_chain = make_and(eq1, eq2);
    auto hypothesis = make_and(eq_chain, P_complex);
    
    auto results = run_challenging_benchmark("Recursive Function Definitions", P_simple, {hypothesis});
    
    for (const auto& result : results) {
        print_challenging_result(result);
    }
}

void test_deep_nested_structures() {
    // Test: f(g(h(a)))=b ∧ g(h(a))=c ∧ h(a)=d ∧ P(f(f(g(h(a))))) ⊢ P(f(b))
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto c = make_constant("c");
    auto d = make_constant("d");
    
    auto h_a = make_function_application("h", {a});
    auto g_h_a = make_function_application("g", {h_a});
    auto f_g_h_a = make_function_application("f", {g_h_a});
    auto f_f_g_h_a = make_function_application("f", {f_g_h_a});
    auto f_b = make_function_application("f", {b});
    
    auto eq1 = make_function_application("=", {f_g_h_a, b});
    auto eq2 = make_function_application("=", {g_h_a, c});
    auto eq3 = make_function_application("=", {h_a, d});
    
    auto P_complex = make_function_application("P", {f_f_g_h_a});
    auto P_simple = make_function_application("P", {f_b});
    
    auto eq_chain = make_and(eq1, make_and(eq2, eq3));
    auto hypothesis = make_and(eq_chain, P_complex);
    
    auto results = run_challenging_benchmark("Deep Nested Structures", P_simple, {hypothesis});
    
    for (const auto& result : results) {
        print_challenging_result(result);
    }
}

void test_associativity_like_problem() {
    // Test: +(+(a,b),c) = +(a,+(b,c)) ∧ P(+(+(+(a,b),c),d)) ⊢ P(+(a,+(+(b,c),d)))
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto c = make_constant("c");
    auto d = make_constant("d");
    
    auto a_plus_b = make_function_application("+", {a, b});
    auto b_plus_c = make_function_application("+", {b, c});
    auto ab_plus_c = make_function_application("+", {a_plus_b, c});
    auto a_plus_bc = make_function_application("+", {a, b_plus_c});
    
    auto abc_plus_d = make_function_application("+", {ab_plus_c, d});
    auto bc_plus_d = make_function_application("+", {b_plus_c, d});
    auto a_plus_bcd = make_function_application("+", {a, bc_plus_d});
    
    auto eq1 = make_function_application("=", {ab_plus_c, a_plus_bc}); // Associativity
    
    auto P_left_assoc = make_function_application("P", {abc_plus_d});
    auto P_right_assoc = make_function_application("P", {a_plus_bcd});
    
    auto hypothesis = make_and(eq1, P_left_assoc);
    
    auto results = run_challenging_benchmark("Associativity-like Problem", P_right_assoc, {hypothesis});
    
    for (const auto& result : results) {
        print_challenging_result(result);
    }
}

void test_large_equality_system() {
    // Test: Large system with 12 variables chained in complex ways
    auto vars = std::vector<TermDBPtr>();
    for (int i = 0; i < 12; ++i) {
        vars.push_back(make_constant("v" + std::to_string(i)));
    }
    
    // Create a complex web of equalities: v0=v1, v1=v2, v2=v3, ..., v10=v11
    // Plus some cross-connections: v0=v6, v3=v9, v5=v11
    std::vector<TermDBPtr> equalities;
    
    // Linear chain
    for (int i = 0; i < 11; ++i) {
        equalities.push_back(make_function_application("=", {vars[i], vars[i+1]}));
    }
    
    // Cross-connections to create more complex equality relationships
    equalities.push_back(make_function_application("=", {vars[0], vars[6]}));
    equalities.push_back(make_function_application("=", {vars[3], vars[9]}));
    equalities.push_back(make_function_application("=", {vars[5], vars[11]}));
    
    // Combine all equalities
    auto hypothesis = equalities[0];
    for (size_t i = 1; i < equalities.size(); ++i) {
        hypothesis = make_and(hypothesis, equalities[i]);
    }
    
    // Add a predicate
    auto P_v0 = make_function_application("P", {vars[0]});
    hypothesis = make_and(hypothesis, P_v0);
    
    // Goal: prove P(v11)
    auto P_v11 = make_function_application("P", {vars[11]});
    
    auto results = run_challenging_benchmark("Large Equality System (12 vars)", P_v11, {hypothesis});
    
    for (const auto& result : results) {
        print_challenging_result(result);
    }
}

void test_mixed_complexity_web() {
    // Test: Combination of function applications, nesting, and equality chains
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto c = make_constant("c");
    auto d = make_constant("d");
    auto e = make_constant("e");
    
    // Complex nested terms
    auto f_a = make_function_application("f", {a});
    auto g_b = make_function_application("g", {b});
    auto h_c = make_function_application("h", {c});
    auto f_g_b = make_function_application("f", {g_b});
    auto g_h_c = make_function_application("g", {h_c});
    auto h_f_a = make_function_application("h", {f_a});
    
    // Equality web
    auto eq1 = make_function_application("=", {f_a, g_b});
    auto eq2 = make_function_application("=", {g_b, h_c});
    auto eq3 = make_function_application("=", {f_g_b, d});
    auto eq4 = make_function_application("=", {g_h_c, e});
    auto eq5 = make_function_application("=", {h_f_a, f_g_b});
    
    // Complex predicate with multiple nested applications
    auto complex_term = make_function_application("Q", {h_f_a, g_h_c, f_g_b});
    auto goal_term = make_function_application("Q", {d, e, d});
    
    auto eq_web = make_and(eq1, make_and(eq2, make_and(eq3, make_and(eq4, eq5))));
    auto hypothesis = make_and(eq_web, complex_term);
    
    auto results = run_challenging_benchmark("Mixed Complexity Web", goal_term, {hypothesis});
    
    for (const auto& result : results) {
        print_challenging_result(result);
    }
}

int main() {
    std::cout << "===== Challenging Equality Reasoning Benchmarks =====" << std::endl;
    std::cout << "Testing performance limits of different resolution approaches.\n" << std::endl;
    
    try {
        test_long_equality_chain();
       // test_multiple_function_interactions();
        test_recursive_function_definitions();
        test_deep_nested_structures();
        test_associativity_like_problem();
        test_large_equality_system();
        test_mixed_complexity_web();
        
        std::cout << "\n===== Performance Analysis Summary =====" << std::endl;
        std::cout << "• Basic Resolution: Should fail/timeout on all equality problems" << std::endl;
        std::cout << "• Standard Paramodulation: May struggle with complex nested structures" << std::endl;
        std::cout << "• KB-Optimized: Should excel where completion succeeds, struggle where it fails" << std::endl;
        std::cout << "\nLook for dramatic performance differences and resource limit hits!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}