#include <iostream>
#include <cassert>
#include <chrono>
#include <vector>
#include <iomanip>
#include "../src/term/term_db.hpp"
#include "../src/resolution/resolution_prover.hpp"

using namespace theorem_prover;

struct BenchmarkResult {
    std::string config_name;
    ResolutionProofResult::Status status;
    double time_ms;
    size_t iterations;
    size_t final_clauses;
    
    BenchmarkResult(const std::string& name, const ResolutionProofResult& result, double time)
        : config_name(name), status(result.status), time_ms(time), 
          iterations(result.iterations), final_clauses(result.final_clauses.size()) {}
};

void print_benchmark_header() {
    std::cout << std::setw(25) << "Configuration" 
              << std::setw(12) << "Status" 
              << std::setw(10) << "Time(ms)" 
              << std::setw(10) << "Iterations" 
              << std::setw(12) << "Clauses" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
}

void print_benchmark_result(const BenchmarkResult& result) {
    std::string status_str;
    switch (result.status) {
        case ResolutionProofResult::Status::PROVED: status_str = "PROVED"; break;
        case ResolutionProofResult::Status::TIMEOUT: status_str = "TIMEOUT"; break;
        case ResolutionProofResult::Status::SATURATED: status_str = "SATURATED"; break;
        default: status_str = "OTHER"; break;
    }
    
    std::cout << std::setw(25) << result.config_name 
              << std::setw(12) << status_str
              << std::setw(10) << std::fixed << std::setprecision(1) << result.time_ms
              << std::setw(10) << result.iterations 
              << std::setw(12) << result.final_clauses << std::endl;
}

std::vector<BenchmarkResult> run_benchmark(const std::string& test_name,
                                          const TermDBPtr& goal,
                                          const std::vector<TermDBPtr>& hypotheses) {
    std::cout << "\n=== " << test_name << " ===" << std::endl;
    std::vector<BenchmarkResult> results;
    
    // Configuration 1: Basic Resolution (Baseline)
    {
        ResolutionConfig config;
        config.use_paramodulation = false;
        config.use_kb_preprocessing = false;
        config.max_iterations = 2000;
        config.max_time_ms = 10000.0;
        
        ResolutionProver prover(config);
        auto start = std::chrono::high_resolution_clock::now();
        auto result = prover.prove(goal, hypotheses);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        results.emplace_back("Basic Resolution", result, duration);
    }
    
    // Configuration 2: Standard Paramodulation
    {
        ResolutionConfig config;
        config.use_paramodulation = true;
        config.use_kb_preprocessing = false;
        config.max_iterations = 2000;
        config.max_time_ms = 10000.0;
        
        ResolutionProver prover(config);
        auto start = std::chrono::high_resolution_clock::now();
        auto result = prover.prove(goal, hypotheses);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        results.emplace_back("Standard Paramodulation", result, duration);
    }
    
    // Configuration 3: KB-Optimized Paramodulation
    {
        ResolutionConfig config;
        config.use_paramodulation = true;
        config.use_kb_preprocessing = true;
        config.kb_preprocessing_timeout = 3.0;
        config.kb_max_equations = 15;
        config.kb_max_rules = 30;
        config.max_iterations = 2000;
        config.max_time_ms = 10000.0;
        
        ResolutionProver prover(config);
        auto start = std::chrono::high_resolution_clock::now();
        auto result = prover.prove(goal, hypotheses);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        results.emplace_back("KB-Optimized Paramod", result, duration);
    }
    
    print_benchmark_header();
    for (const auto& result : results) {
        print_benchmark_result(result);
    }
    
    return results;
}

void test_simple_equality() {
    // Test: a = b ∧ P(a) ⊢ P(b)
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto P_a = make_function_application("P", {a});
    auto P_b = make_function_application("P", {b});
    auto equality = make_function_application("=", {a, b});
    auto hypothesis = make_and(equality, P_a);
    
    run_benchmark("Simple Equality: a = b ∧ P(a) ⊢ P(b)", P_b, {hypothesis});
}

void test_transitivity() {
    // Test: a = b ∧ b = c ⊢ a = c
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto c = make_constant("c");
    auto eq1 = make_function_application("=", {a, b});
    auto eq2 = make_function_application("=", {b, c});
    auto eq3 = make_function_application("=", {a, c});
    auto hypothesis = make_and(eq1, eq2);
    
    run_benchmark("Transitivity: a = b ∧ b = c ⊢ a = c", eq3, {hypothesis});
}

void test_function_equality() {
    // Test: f(a) = b ∧ P(f(a)) ⊢ P(b)
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto f_a = make_function_application("f", {a});
    auto P_f_a = make_function_application("P", {f_a});
    auto P_b = make_function_application("P", {b});
    auto equality = make_function_application("=", {f_a, b});
    auto hypothesis = make_and(equality, P_f_a);
    
    run_benchmark("Function Equality: f(a) = b ∧ P(f(a)) ⊢ P(b)", P_b, {hypothesis});
}

void test_chain_equality() {
    // Test: a = b ∧ b = c ∧ c = d ∧ P(a) ⊢ P(d)
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto c = make_constant("c");
    auto d = make_constant("d");
    auto eq1 = make_function_application("=", {a, b});
    auto eq2 = make_function_application("=", {b, c});
    auto eq3 = make_function_application("=", {c, d});
    auto P_a = make_function_application("P", {a});
    auto P_d = make_function_application("P", {d});
    
    auto hyp1 = make_and(eq1, eq2);
    auto hyp2 = make_and(eq3, P_a);
    auto hypothesis = make_and(hyp1, hyp2);
    
    run_benchmark("Chain Equality: a = b ∧ b = c ∧ c = d ∧ P(a) ⊢ P(d)", P_d, {hypothesis});
}

void test_complex_equality() {
    // Test: f(g(a)) = h(b) ∧ g(a) = c ∧ P(f(c)) ⊢ P(h(b))
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto c = make_constant("c");
    auto g_a = make_function_application("g", {a});
    auto f_g_a = make_function_application("f", {g_a});
    auto h_b = make_function_application("h", {b});
    auto f_c = make_function_application("f", {c});
    
    auto eq1 = make_function_application("=", {f_g_a, h_b});  // f(g(a)) = h(b)
    auto eq2 = make_function_application("=", {g_a, c});      // g(a) = c
    auto P_f_c = make_function_application("P", {f_c});       // P(f(c))
    auto P_h_b = make_function_application("P", {h_b});       // P(h(b)) - goal
    
    auto hyp1 = make_and(eq1, eq2);
    auto hypothesis = make_and(hyp1, P_f_c);
    
    run_benchmark("Complex Equality: f(g(a)) = h(b) ∧ g(a) = c ∧ P(f(c)) ⊢ P(h(b))", 
                  P_h_b, {hypothesis});
}

void test_symmetric_equality() {
    // Test: P(a) ∧ a = b ⊢ P(b) (symmetry test)
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto P_a = make_function_application("P", {a});
    auto P_b = make_function_application("P", {b});
    auto equality = make_function_application("=", {a, b});
    auto hypothesis = make_and(P_a, equality);
    
    run_benchmark("Symmetric Equality: P(a) ∧ a = b ⊢ P(b)", P_b, {hypothesis});
}

int main() {
    std::cout << "===== Knuth-Bendix Optimization for Paramodulation =====" << std::endl;
    std::cout << "Comparing resolution approaches on equality-heavy problems.\n" << std::endl;
    
    try {
        test_simple_equality();
        test_symmetric_equality();
        test_transitivity(); 
        test_function_equality();
        test_chain_equality();
        test_complex_equality();
        
        std::cout << "\n===== Summary =====" << std::endl;
        std::cout << "• Basic Resolution: Expected to fail on equality problems" << std::endl;
        std::cout << "• Standard Paramodulation: The established approach for equality reasoning" << std::endl;
        std::cout << "• KB-Optimized Paramodulation: Uses KB preprocessing to simplify equality sets" << std::endl;
        std::cout << "\nLook for cases where KB preprocessing reduces iterations or improves performance!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}