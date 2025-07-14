#include <iostream>
#include <cassert>
#include "../src/term/term_db.hpp"
#include "../src/resolution/clause.hpp"

using namespace theorem_prover;

void print_term_details(const TermDBPtr& term, const std::string& label) {
    std::cout << "  " << label << ": ";
    if (term->kind() == TermDB::TermKind::FUNCTION_APPLICATION) {
        auto func = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
        std::cout << func->symbol() << "(";
        for (size_t i = 0; i < func->arguments().size(); ++i) {
            if (i > 0) std::cout << ",";
            auto arg = func->arguments()[i];
            if (arg->kind() == TermDB::TermKind::CONSTANT) {
                auto const_arg = std::dynamic_pointer_cast<ConstantDB>(arg);
                std::cout << const_arg->symbol();
            } else if (arg->kind() == TermDB::TermKind::FUNCTION_APPLICATION) {
                auto func_arg = std::dynamic_pointer_cast<FunctionApplicationDB>(arg);
                std::cout << func_arg->symbol() << "(...)";
            } else {
                std::cout << "var" << arg->hash();
            }
        }
        std::cout << ")";
    } else if (term->kind() == TermDB::TermKind::CONSTANT) {
        auto constant = std::dynamic_pointer_cast<ConstantDB>(term);
        std::cout << constant->symbol();
    } else if (term->kind() == TermDB::TermKind::VARIABLE) {
        auto var = std::dynamic_pointer_cast<VariableDB>(term);
        std::cout << "X" << var->index();
    }
    std::cout << std::endl;
}

void test_basic_paramodulation() {
    std::cout << "=== Test 1: Basic Paramodulation f(a) = b, P(f(a)) ⊢ P(b) ===" << std::endl;
    
    // Create terms
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto f_a = make_function_application("f", {a});
    auto P_f_a = make_function_application("P", {f_a});
    
    print_term_details(f_a, "f(a)");
    print_term_details(P_f_a, "P(f(a))");
    
    // Create equality: f(a) = b
    auto equality = make_function_application("=", {f_a, b});
    print_term_details(equality, "Equality");
    
    // Create clauses
    auto eq_clause = std::make_shared<Clause>(std::vector<Literal>{
        Literal(equality, true)
    });
    
    auto target_clause = std::make_shared<Clause>(std::vector<Literal>{
        Literal(P_f_a, true)
    });
    
    std::cout << "Applying paramodulation..." << std::endl;
    
    // Test paramodulation
    auto paramod_results = ResolutionWithParamodulation::try_paramodulation(eq_clause, target_clause);
    
    std::cout << "Results: " << paramod_results.size() << " clauses generated" << std::endl;
    
    if (!paramod_results.empty()) {
        auto result_clause = paramod_results[0];
        std::cout << "Generated clause has " << result_clause->literals().size() << " literals" << std::endl;
        
        for (size_t i = 0; i < result_clause->literals().size(); ++i) {
            const auto& lit = result_clause->literals()[i];
            std::cout << "  Literal " << i << ": ";
            print_term_details(lit.atom(), (lit.is_positive() ? "+" : "-"));
        }
        
        // Verify the result should be P(b)
        if (result_clause->literals().size() == 1) {
            const auto& result_lit = result_clause->literals()[0];
            if (result_lit.is_positive() && 
                result_lit.atom()->kind() == TermDB::TermKind::FUNCTION_APPLICATION) {
                auto func = std::dynamic_pointer_cast<FunctionApplicationDB>(result_lit.atom());
                if (func->symbol() == "P" && func->arguments().size() == 1) {
                    auto arg = func->arguments()[0];
                    if (arg->kind() == TermDB::TermKind::CONSTANT) {
                        auto const_arg = std::dynamic_pointer_cast<ConstantDB>(arg);
                        if (const_arg->symbol() == "b") {
                            std::cout << "✓ Correctly derived P(b)" << std::endl;
                        }
                    }
                }
            }
        }
    }
    
    std::cout << std::endl;
}

void test_variable_paramodulation() {
    std::cout << "=== Test 2: Variable Paramodulation f(X) = g(X), P(f(a)) ⊢ P(g(a)) ===" << std::endl;
    
    // Create terms
    auto a = make_constant("a");
    auto X = make_variable(0);
    auto f_X = make_function_application("f", {X});
    auto g_X = make_function_application("g", {X});
    auto f_a = make_function_application("f", {a});
    auto P_f_a = make_function_application("P", {f_a});
    
    print_term_details(f_X, "f(X)");
    print_term_details(g_X, "g(X)");
    print_term_details(P_f_a, "P(f(a))");
    
    // Create equality: f(X) = g(X)
    auto equality = make_function_application("=", {f_X, g_X});
    print_term_details(equality, "Equality");
    
    // Create clauses
    auto eq_clause = std::make_shared<Clause>(std::vector<Literal>{
        Literal(equality, true)
    });
    
    auto target_clause = std::make_shared<Clause>(std::vector<Literal>{
        Literal(P_f_a, true)
    });
    
    std::cout << "Applying paramodulation..." << std::endl;
    
    // Test paramodulation
    auto paramod_results = ResolutionWithParamodulation::try_paramodulation(eq_clause, target_clause);
    
    std::cout << "Results: " << paramod_results.size() << " clauses generated" << std::endl;
    
    if (!paramod_results.empty()) {
        auto result_clause = paramod_results[0];
        std::cout << "Generated clause has " << result_clause->literals().size() << " literals" << std::endl;
        
        for (size_t i = 0; i < result_clause->literals().size(); ++i) {
            const auto& lit = result_clause->literals()[i];
            std::cout << "  Literal " << i << ": ";
            print_term_details(lit.atom(), (lit.is_positive() ? "+" : "-"));
        }
    }
    
    std::cout << std::endl;
}

void test_multiple_positions() {
    std::cout << "=== Test 3: Multiple Positions f(a) = b, Q(f(a), f(a)) ⊢ ? ===" << std::endl;
    
    // Create terms
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto f_a = make_function_application("f", {a});
    auto Q_f_a_f_a = make_function_application("Q", {f_a, f_a});
    
    print_term_details(Q_f_a_f_a, "Q(f(a), f(a))");
    
    // Create equality: f(a) = b
    auto equality = make_function_application("=", {f_a, b});
    
    // Create clauses
    auto eq_clause = std::make_shared<Clause>(std::vector<Literal>{
        Literal(equality, true)
    });
    
    auto target_clause = std::make_shared<Clause>(std::vector<Literal>{
        Literal(Q_f_a_f_a, true)
    });
    
    std::cout << "Applying paramodulation..." << std::endl;
    
    // Test paramodulation
    auto paramod_results = ResolutionWithParamodulation::try_paramodulation(eq_clause, target_clause);
    
    std::cout << "Results: " << paramod_results.size() << " clauses generated" << std::endl;
    std::cout << "(Should generate Q(b, f(a)) and Q(f(a), b))" << std::endl;
    
    for (size_t j = 0; j < paramod_results.size(); ++j) {
        std::cout << "Result " << j << ":" << std::endl;
        auto result_clause = paramod_results[j];
        for (size_t i = 0; i < result_clause->literals().size(); ++i) {
            const auto& lit = result_clause->literals()[i];
            std::cout << "  Literal " << i << ": ";
            print_term_details(lit.atom(), (lit.is_positive() ? "+" : "-"));
        }
    }
    
    std::cout << std::endl;
}

int main() {
    std::cout << "===== Detailed Paramodulation Tests =====" << std::endl;
    
    try {
        test_basic_paramodulation();
        test_variable_paramodulation();
        test_multiple_positions();
        
        std::cout << "===== All Tests Completed! =====" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}