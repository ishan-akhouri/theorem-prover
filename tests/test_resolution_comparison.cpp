#include <iostream>
#include <cassert>
#include <chrono>
#include "../src/term/term_db.hpp"
#include "../src/resolution/resolution_prover.hpp"
#include "../src/resolution/cnf_converter.hpp"

using namespace theorem_prover;

void test_equality_reasoning() {
    std::cout << "=== Testing Equality Reasoning: f(a) = b ∧ P(f(a)) ⊢ P(b) ===" << std::endl;
    
    // Create terms
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto f_a = make_function_application("f", {a});
    auto P_f_a = make_function_application("P", {f_a});
    auto P_b = make_function_application("P", {b});
    
    // Create equality: f(a) = b
    auto equality = make_function_application("=", {f_a, b});
    
    // Create hypotheses: f(a) = b ∧ P(f(a))
    auto hypothesis = make_and(equality, P_f_a);
    
    // Goal: P(b)
    auto goal = P_b;
    
    std::cout << "Hypotheses: f(a) = b ∧ P(f(a))" << std::endl;
    std::cout << "Goal: P(b)" << std::endl;
    
    // Test 1: Standard resolution (should fail or timeout)
    std::cout << "\n--- Test 1: Standard Resolution ---" << std::endl;
    ResolutionConfig standard_config;
    standard_config.use_paramodulation = false;
    standard_config.max_iterations = 1000;  // Limit iterations for faster test
    standard_config.max_time_ms = 5000.0;   // 5 second timeout
    ResolutionProver standard_prover(standard_config);
    
    auto start = std::chrono::high_resolution_clock::now();
    auto standard_result = standard_prover.prove(goal, {hypothesis});
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Result: ";
    switch (standard_result.status) {
        case ResolutionProofResult::Status::PROVED:
            std::cout << "PROVED"; break;
        case ResolutionProofResult::Status::DISPROVED:
            std::cout << "DISPROVED"; break;
        case ResolutionProofResult::Status::TIMEOUT:
            std::cout << "TIMEOUT"; break;
        case ResolutionProofResult::Status::SATURATED:
            std::cout << "SATURATED"; break;
        default:
            std::cout << "UNKNOWN"; break;
    }
    std::cout << std::endl;
    std::cout << "Time: " << duration.count() << " ms" << std::endl;
    std::cout << "Iterations: " << standard_result.iterations << std::endl;
    std::cout << "Final clauses: " << standard_result.final_clauses.size() << std::endl;
    
    // Test 2: Resolution with paramodulation (should succeed)
    std::cout << "\n--- Test 2: Resolution with Paramodulation ---" << std::endl;
    ResolutionConfig paramod_config;
    paramod_config.use_paramodulation = true;
    paramod_config.max_iterations = 1000;
    paramod_config.max_time_ms = 5000.0;
    ResolutionProver paramod_prover(paramod_config);
    
    start = std::chrono::high_resolution_clock::now();
    auto paramod_result = paramod_prover.prove(goal, {hypothesis});
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Result: ";
    switch (paramod_result.status) {
        case ResolutionProofResult::Status::PROVED:
            std::cout << "PROVED"; break;
        case ResolutionProofResult::Status::DISPROVED:
            std::cout << "DISPROVED"; break;
        case ResolutionProofResult::Status::TIMEOUT:
            std::cout << "TIMEOUT"; break;
        case ResolutionProofResult::Status::SATURATED:
            std::cout << "SATURATED"; break;
        default:
            std::cout << "UNKNOWN"; break;
    }
    std::cout << std::endl;
    std::cout << "Time: " << duration.count() << " ms" << std::endl;
    std::cout << "Iterations: " << paramod_result.iterations << std::endl;
    std::cout << "Final clauses: " << paramod_result.final_clauses.size() << std::endl;
    
    if (paramod_result.status == ResolutionProofResult::Status::PROVED) {
        std::cout << "✓ Successfully proved with paramodulation!" << std::endl;
    }
    
    std::cout << std::endl;
}

void test_simple_equality() {
    std::cout << "=== Testing Simple Equality: a = b ∧ P(a) ⊢ P(b) ===" << std::endl;
    
    // Create terms
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto P_a = make_function_application("P", {a});
    auto P_b = make_function_application("P", {b});
    
    // Create equality: a = b
    auto equality = make_function_application("=", {a, b});
    
    // Create hypotheses: a = b ∧ P(a)
    auto hypothesis = make_and(equality, P_a);
    
    // Goal: P(b)
    auto goal = P_b;
    
    std::cout << "Hypotheses: a = b ∧ P(a)" << std::endl;
    std::cout << "Goal: P(b)" << std::endl;
    
    // Test with paramodulation
    ResolutionConfig paramod_config;
    paramod_config.use_paramodulation = true;
    paramod_config.max_iterations = 500;
    paramod_config.max_time_ms = 3000.0;
    ResolutionProver paramod_prover(paramod_config);
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = paramod_prover.prove(goal, {hypothesis});
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Result: ";
    switch (result.status) {
        case ResolutionProofResult::Status::PROVED:
            std::cout << "PROVED"; break;
        case ResolutionProofResult::Status::DISPROVED:
            std::cout << "DISPROVED"; break;
        case ResolutionProofResult::Status::TIMEOUT:
            std::cout << "TIMEOUT"; break;
        case ResolutionProofResult::Status::SATURATED:
            std::cout << "SATURATED"; break;
        default:
            std::cout << "UNKNOWN"; break;
    }
    std::cout << std::endl;
    std::cout << "Time: " << duration.count() << " ms" << std::endl;
    std::cout << "Iterations: " << result.iterations << std::endl;
    
    if (result.status == ResolutionProofResult::Status::PROVED) {
        std::cout << "✓ Successfully proved simple equality!" << std::endl;
    }
    
    std::cout << std::endl;
}

void test_transitivity() {
    std::cout << "=== Testing Transitivity: a = b ∧ b = c ⊢ a = c ===" << std::endl;
    
    // Create terms
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto c = make_constant("c");
    
    // Create equalities
    auto eq1 = make_function_application("=", {a, b});  // a = b
    auto eq2 = make_function_application("=", {b, c});  // b = c
    auto eq3 = make_function_application("=", {a, c});  // a = c (goal)
    
    auto hypothesis = make_and(eq1, eq2);
    auto goal = eq3;
    
    std::cout << "Hypotheses: a = b ∧ b = c" << std::endl;
    std::cout << "Goal: a = c" << std::endl;
    
    // Test with paramodulation
    ResolutionConfig paramod_config;
    paramod_config.use_paramodulation = true;
    paramod_config.max_iterations = 500;
    paramod_config.max_time_ms = 3000.0;
    ResolutionProver paramod_prover(paramod_config);
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = paramod_prover.prove(goal, {hypothesis});
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Result: ";
    switch (result.status) {
        case ResolutionProofResult::Status::PROVED:
            std::cout << "PROVED"; break;
        case ResolutionProofResult::Status::DISPROVED:
            std::cout << "DISPROVED"; break;
        case ResolutionProofResult::Status::TIMEOUT:
            std::cout << "TIMEOUT"; break;
        case ResolutionProofResult::Status::SATURATED:
            std::cout << "SATURATED"; break;
        default:
            std::cout << "UNKNOWN"; break;
    }
    std::cout << std::endl;
    std::cout << "Time: " << duration.count() << " ms" << std::endl;
    std::cout << "Iterations: " << result.iterations << std::endl;
    
    if (result.status == ResolutionProofResult::Status::PROVED) {
        std::cout << "✓ Successfully proved transitivity!" << std::endl;
    }
    
    std::cout << std::endl;
}

int main() {
    std::cout << "===== Resolution vs Paramodulation Comparison =====" << std::endl;
    
    try {
        test_simple_equality();
        test_equality_reasoning();
        test_transitivity();
        
        std::cout << "===== All Tests Completed! =====" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}