#include <iostream>
#include <cassert>
#include <vector>
#include <memory>
#include "../src/completion/critical_pairs.hpp"
#include "../src/term/term_db.hpp"
#include "../src/term/rewriting.hpp"
#include "../src/term/ordering.hpp"

using namespace theorem_prover;

// Helper function to create a simple LPO ordering
std::shared_ptr<TermOrdering> create_test_ordering() {
    return std::make_shared<LexicographicPathOrdering>();
}

// Helper function to print test results
void print_test_result(const std::string& test_name, bool passed) {
    std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << test_name << std::endl;
}

// Test 1: Basic critical pair computation between two simple rules
void test_basic_critical_pairs() {
    std::cout << "\n=== Test 1: Basic Critical Pairs ===" << std::endl;
    
    // Create terms for rules:
    // Rule 1: f(g(x)) → x  
    // Rule 2: g(a) → b
    auto x = make_variable(0);
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto g_x = make_function_application("g", {x});
    auto f_g_x = make_function_application("f", {g_x});
    auto g_a = make_function_application("g", {a});
    
    auto ordering = create_test_ordering();
    
    // Create rules
    TermRewriteRule rule1(f_g_x, x, "rule1");
    TermRewriteRule rule2(g_a, b, "rule2");
    
    std::cout << "Rule 1: " << rule1.to_string() << std::endl;
    std::cout << "Rule 2: " << rule2.to_string() << std::endl;
    
    // Compute critical pairs
    auto critical_pairs = CriticalPairComputer::compute_critical_pairs(rule1, rule2);
    
    std::cout << "Found " << critical_pairs.size() << " critical pairs:" << std::endl;
    for (const auto& cp : critical_pairs) {
        std::cout << "  " << cp.to_string() << std::endl;
    }
    
    // Should find one critical pair: f(b) =?= a
    // This occurs when g(x) in rule1 unifies with g(a) in rule2
    bool found_expected_pair = false;
    for (const auto& cp : critical_pairs) {
        // Check if this is the expected critical pair
        if (cp.position.path() == std::vector<size_t>{0}) { // Position [0] in f(g(x))
            found_expected_pair = true;
            break;
        }
    }
    
    print_test_result("Basic critical pairs computation", found_expected_pair);
}

// Test 2: Self-overlapping critical pairs
void test_self_critical_pairs() {
    std::cout << "\n=== Test 2: Self-Overlapping Critical Pairs ===" << std::endl;
    
    // Better example: f(f(x, y), z) → f(x, f(y, z)) (associativity)
    // This should create self-overlaps
    auto x = make_variable(0);
    auto y = make_variable(1);
    auto z = make_variable(2);
    
    auto f_x_y = make_function_application("f", {x, y});
    auto f_y_z = make_function_application("f", {y, z});
    auto f_f_x_y_z = make_function_application("f", {f_x_y, z});
    auto f_x_f_y_z = make_function_application("f", {x, f_y_z});
    
    TermRewriteRule rule(f_f_x_y_z, f_x_f_y_z, "assoc_rule");
    
    std::cout << "Rule: " << rule.to_string() << std::endl;
    
    // Compute self-critical pairs
    auto self_pairs = CriticalPairComputer::compute_self_critical_pairs(rule);
    
    std::cout << "Found " << self_pairs.size() << " self-critical pairs:" << std::endl;
    for (const auto& cp : self_pairs) {
        std::cout << "  " << cp.to_string() << std::endl;
    }
    
    // Should find critical pairs for associativity
    bool found_self_overlap = self_pairs.size() > 0;
    
    print_test_result("Self-overlapping critical pairs", found_self_overlap);
}

// Test 3: Critical pairs with associativity
void test_associativity_critical_pairs() {
    std::cout << "\n=== Test 3: Associativity Critical Pairs ===" << std::endl;
    
    // Create associativity rules:
    // Rule 1: +(+(x,y),z) → +(x,+(y,z))
    // Rule 2: +(x,+(y,z)) → +(+(x,y),z)
    auto x = make_variable(0);
    auto y = make_variable(1);
    auto z = make_variable(2);
    
    auto plus_x_y = make_function_application("+", {x, y});
    auto plus_y_z = make_function_application("+", {y, z});
    auto plus_plus_x_y_z = make_function_application("+", {plus_x_y, z});
    auto plus_x_plus_y_z = make_function_application("+", {x, plus_y_z});
    
    TermRewriteRule rule1(plus_plus_x_y_z, plus_x_plus_y_z, "assoc_left");
    TermRewriteRule rule2(plus_x_plus_y_z, plus_plus_x_y_z, "assoc_right");
    
    std::cout << "Rule 1: " << rule1.to_string() << std::endl;
    std::cout << "Rule 2: " << rule2.to_string() << std::endl;
    
    // Compute critical pairs between the rules
    auto critical_pairs = CriticalPairComputer::compute_critical_pairs(rule1, rule2);
    
    std::cout << "Found " << critical_pairs.size() << " critical pairs:" << std::endl;
    for (const auto& cp : critical_pairs) {
        std::cout << "  " << cp.to_string() << std::endl;
    }
    
    // Should find critical pairs due to overlapping associativity
    bool has_critical_pairs = critical_pairs.size() > 0;
    
    print_test_result("Associativity critical pairs", has_critical_pairs);
}

// Test 4: Critical pairs with commutativity
void test_commutativity_critical_pairs() {
    std::cout << "\n=== Test 4: Commutativity Critical Pairs ===" << std::endl;
    
    // Create commutativity rule and another rule:
    // Rule 1: +(x,y) → +(y,x)  (commutativity)
    // Rule 2: +(0,x) → x       (left identity)
    auto x = make_variable(0);
    auto y = make_variable(1);
    auto zero = make_constant("0");
    
    auto plus_x_y = make_function_application("+", {x, y});
    auto plus_y_x = make_function_application("+", {y, x});
    auto plus_zero_x = make_function_application("+", {zero, x});
    
    TermRewriteRule rule1(plus_x_y, plus_y_x, "comm");
    TermRewriteRule rule2(plus_zero_x, x, "left_id");
    
    std::cout << "Rule 1 (commutativity): " << rule1.to_string() << std::endl;
    std::cout << "Rule 2 (left identity): " << rule2.to_string() << std::endl;
    
    // Compute critical pairs
    auto critical_pairs = CriticalPairComputer::compute_critical_pairs(rule1, rule2);
    
    std::cout << "Found " << critical_pairs.size() << " critical pairs:" << std::endl;
    for (const auto& cp : critical_pairs) {
        std::cout << "  " << cp.to_string() << std::endl;
    }
    
    // Should find critical pairs when commutativity interacts with identity
    bool has_critical_pairs = critical_pairs.size() > 0;
    
    print_test_result("Commutativity critical pairs", has_critical_pairs);
}

// Test 5: All critical pairs computation
void test_all_critical_pairs() {
    std::cout << "\n=== Test 5: All Critical Pairs Computation ===" << std::endl;
    
    // Create a small set of rules
    auto x = make_variable(0);
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto f_x = make_function_application("f", {x});
    auto g_x = make_function_application("g", {x});
    auto f_a = make_function_application("f", {a});
    
    std::vector<TermRewriteRule> rules = {
        TermRewriteRule(f_x, g_x, "rule1"),
        TermRewriteRule(f_a, b, "rule2"),
        TermRewriteRule(g_x, x, "rule3")
    };
    
    std::cout << "Rules:" << std::endl;
    for (const auto& rule : rules) {
        std::cout << "  " << rule.to_string() << std::endl;
    }
    
    // Compute all critical pairs
    auto all_pairs = CriticalPairComputer::compute_all_critical_pairs(rules);
    
    std::cout << "Found " << all_pairs.size() << " total critical pairs:" << std::endl;
    for (const auto& cp : all_pairs) {
        std::cout << "  " << cp.to_string() << std::endl;
    }
    
    // Should find various critical pairs between the rules
    bool has_multiple_pairs = all_pairs.size() > 1;
    
    print_test_result("All critical pairs computation", has_multiple_pairs);
}

// Test 6: Critical pair to equation conversion
void test_critical_pair_to_equation() {
    std::cout << "\n=== Test 6: Critical Pair to Equation Conversion ===" << std::endl;
    
    // Create a simple critical pair
    auto x = make_variable(0);
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto f_x = make_function_application("f", {x});
    auto g_a = make_function_application("g", {a});
    
    TermRewriteRule rule1(f_x, a, "rule1");
    TermRewriteRule rule2(g_a, b, "rule2");
    
    auto critical_pairs = CriticalPairComputer::compute_critical_pairs(rule1, rule2);
    
    if (!critical_pairs.empty()) {
        auto cp = critical_pairs[0];
        auto equation = cp.to_equation();
        
        std::cout << "Critical pair: " << cp.to_string() << std::endl;
        std::cout << "As equation: " << equation.to_string() << std::endl;
        
        // Check that equation has the same left and right terms
        bool conversion_correct = (*equation.lhs() == *cp.left) && (*equation.rhs() == *cp.right);
        
        print_test_result("Critical pair to equation conversion", conversion_correct);
    } else {
        print_test_result("Critical pair to equation conversion", false);
        std::cout << "  No critical pairs found to test conversion" << std::endl;
    }
}

// Test 7: Position finding in terms
void test_position_finding() {
    std::cout << "\n=== Test 7: Position Finding ===" << std::endl;
    
    // Create a nested term: f(g(h(x)))
    auto x = make_variable(0);
    auto h_x = make_function_application("h", {x});
    auto g_h_x = make_function_application("g", {h_x});
    auto f_g_h_x = make_function_application("f", {g_h_x});
    
    // Use the private method via public interface
    // We'll test this indirectly through critical pair computation
    auto y = make_variable(1);
    auto g_y = make_function_application("g", {y});
    auto rule = TermRewriteRule(g_y, y, "test_rule");
    
    // Create a rule that should match at position [0] in f(g(h(x)))
    TermRewriteRule target_rule(f_g_h_x, x, "target_rule");
    
    auto critical_pairs = CriticalPairComputer::compute_critical_pairs(rule, target_rule);
    
    std::cout << "Target term: f(g(h(x)))" << std::endl;
    std::cout << "Rule: g(y) → y" << std::endl;
    std::cout << "Found " << critical_pairs.size() << " critical pairs" << std::endl;
    
    // Should find a critical pair at position [0] where g(h(x)) matches g(y)
    bool found_position_match = false;
    for (const auto& cp : critical_pairs) {
        if (cp.position.path() == std::vector<size_t>{0}) {
            found_position_match = true;
            std::cout << "  Found match at position " << cp.position.to_string() << std::endl;
            break;
        }
    }
    
    print_test_result("Position finding in terms", found_position_match);
}
// Add this simple test at the beginning of main()
void test_simple_debug() {
    std::cout << "\n=== Debug: Simple Term Creation ===" << std::endl;
    
    // Create simple terms
    auto x = make_variable(0);
    auto y = make_variable(1);
    auto a = make_constant("a");
    auto b = make_constant("b");
    
    std::cout << "Variable x: " << x->hash() << std::endl;
    std::cout << "Variable y: " << y->hash() << std::endl;
    std::cout << "Constant a: " << a->hash() << std::endl;
    std::cout << "Constant b: " << b->hash() << std::endl;
    
    // Test variable finding
    auto vars_x = find_all_variables(x, 0);
    auto vars_a = find_all_variables(a, 0);
    std::cout << "Variables in x: " << vars_x.size() << std::endl;
    std::cout << "Variables in a: " << vars_a.size() << std::endl;
    
    // Test simple unification
    auto unif_result = Unifier::unify(x, a);
    std::cout << "Unify x with a: " << (unif_result.success ? "SUCCESS" : "FAIL") << std::endl;
    
    if (unif_result.success) {
        std::cout << "Substitution size: " << unif_result.substitution.size() << std::endl;
    }
}
void test_very_simple() {
    std::cout << "\n=== Very Simple Test ===" << std::endl;
    
    // Even simpler: x → a vs x → b
    auto x = make_variable(0);
    auto a = make_constant("a");
    auto b = make_constant("b");
    
    TermRewriteRule rule1(x, a, "rule1");
    TermRewriteRule rule2(x, b, "rule2");
    
    std::cout << "Rule1: x → a" << std::endl;
    std::cout << "Rule2: x → b" << std::endl;
    
    auto critical_pairs = CriticalPairComputer::compute_critical_pairs(rule1, rule2);
    std::cout << "Found " << critical_pairs.size() << " critical pairs" << std::endl;
    
    // Should find critical pair: a =?= b (when x is unified)
    print_test_result("Very simple critical pairs", critical_pairs.size() > 0);
}

int main() {
    std::cout << "===== Critical Pairs Tests =====" << std::endl;
    
    try {
        test_very_simple();
        test_simple_debug();
        test_basic_critical_pairs();
        test_self_critical_pairs();
        test_associativity_critical_pairs();
        test_commutativity_critical_pairs();
        test_all_critical_pairs();
        test_critical_pair_to_equation();
        test_position_finding();
        
        std::cout << "\n===== All Critical Pairs Tests Completed! =====" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Exception during testing: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}