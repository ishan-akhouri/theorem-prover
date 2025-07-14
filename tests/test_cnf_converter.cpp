#include <iostream>
#include <cassert>
#include "../src/resolution/cnf_converter.hpp"
#include "../src/term/term_db.hpp"

using namespace theorem_prover;

void test_eliminate_implications() {
    std::cout << "Testing implication elimination..." << std::endl;
    
    // Test P → Q becomes ¬P ∨ Q
    auto p = make_constant("P");
    auto q = make_constant("Q");
    auto impl = make_implies(p, q);
    
    auto result = CNFConverter::eliminate_implications(impl);
    assert(result->kind() == TermDB::TermKind::OR);
    
    auto or_result = std::dynamic_pointer_cast<OrDB>(result);
    assert(or_result->left()->kind() == TermDB::TermKind::NOT);
    assert(*or_result->right() == *q);
    
    std::cout << "Implication elimination tests passed!" << std::endl;
}

void test_move_negations_inward() {
    std::cout << "Testing negation movement..." << std::endl;
    
    auto p = make_constant("P");
    auto q = make_constant("Q");
    
    // Test ¬¬P becomes P
    auto double_neg = make_not(make_not(p));
    auto result = CNFConverter::move_negations_inward(double_neg);
    assert(*result == *p);
    
    // Test ¬(P ∧ Q) becomes ¬P ∨ ¬Q
    auto and_term = make_and(p, q);
    auto neg_and = make_not(and_term);
    result = CNFConverter::move_negations_inward(neg_and);
    assert(result->kind() == TermDB::TermKind::OR);
    
    auto or_result = std::dynamic_pointer_cast<OrDB>(result);
    assert(or_result->left()->kind() == TermDB::TermKind::NOT);
    assert(or_result->right()->kind() == TermDB::TermKind::NOT);
    
    // Test ¬(P ∨ Q) becomes ¬P ∧ ¬Q
    auto or_term = make_or(p, q);
    auto neg_or = make_not(or_term);
    result = CNFConverter::move_negations_inward(neg_or);
    assert(result->kind() == TermDB::TermKind::AND);
    
    std::cout << "Negation movement tests passed!" << std::endl;
}

void test_quantifier_negation() {
    std::cout << "Testing quantifier negation..." << std::endl;
    
    // Test ¬∀x.P(x) becomes ∃x.¬P(x)
    auto p_x = make_function_application("P", {make_variable(0)});
    auto forall_p = make_forall("x", p_x);
    auto neg_forall = make_not(forall_p);
    
    auto result = CNFConverter::move_negations_inward(neg_forall);
    assert(result->kind() == TermDB::TermKind::EXISTS);
    
    auto exists_result = std::dynamic_pointer_cast<ExistsDB>(result);
    assert(exists_result->body()->kind() == TermDB::TermKind::NOT);
    
    // Test ¬∃x.P(x) becomes ∀x.¬P(x)
    auto exists_p = make_exists("x", p_x);
    auto neg_exists = make_not(exists_p);
    
    result = CNFConverter::move_negations_inward(neg_exists);
    assert(result->kind() == TermDB::TermKind::FORALL);
    
    std::cout << "Quantifier negation tests passed!" << std::endl;
}

void test_skolemization() {
    std::cout << "Testing Skolemization..." << std::endl;
    
    // Test ∃x.P(x) becomes P(sk0)
    auto p_x = make_function_application("P", {make_variable(0)});
    auto exists_p = make_exists("x", p_x);
    
    std::size_t skolem_counter = 0;
    std::vector<std::size_t> universal_vars;
    auto result = CNFConverter::skolemize(exists_p, universal_vars, skolem_counter);
    
    // Should be P(sk0) where sk0 is a Skolem constant
    assert(result->kind() == TermDB::TermKind::FUNCTION_APPLICATION);
    
    // Test ∀x.∃y.P(x,y) becomes ∀x.P(x,f(x))
    auto p_xy = make_function_application("P", {make_variable(1), make_variable(0)});
    auto exists_y = make_exists("y", p_xy);
    auto forall_exists = make_forall("x", exists_y);
    
    skolem_counter = 0;
    universal_vars.clear();
    result = CNFConverter::skolemize(forall_exists, universal_vars, skolem_counter);
    
    // Should be ∀x.P(x,f(x)) where f is a Skolem function
    assert(result->kind() == TermDB::TermKind::FORALL);
    
    std::cout << "Skolemization tests passed!" << std::endl;
}

void test_distribute_or_over_and() {
    std::cout << "Testing OR distribution over AND..." << std::endl;
    
    auto p = make_constant("P");
    auto q = make_constant("Q");
    auto r = make_constant("R");
    
    // Test P ∨ (Q ∧ R) becomes (P ∨ Q) ∧ (P ∨ R)
    auto q_and_r = make_and(q, r);
    auto p_or_qr = make_or(p, q_and_r);
    
    auto result = CNFConverter::distribute_or_over_and(p_or_qr);
    assert(result->kind() == TermDB::TermKind::AND);
    
    auto and_result = std::dynamic_pointer_cast<AndDB>(result);
    assert(and_result->left()->kind() == TermDB::TermKind::OR);
    assert(and_result->right()->kind() == TermDB::TermKind::OR);
    
    // Test (P ∧ Q) ∨ R becomes (P ∨ R) ∧ (Q ∨ R)
    auto p_and_q = make_and(p, q);
    auto pq_or_r = make_or(p_and_q, r);
    
    result = CNFConverter::distribute_or_over_and(pq_or_r);
    assert(result->kind() == TermDB::TermKind::AND);
    
    std::cout << "OR distribution tests passed!" << std::endl;
}

void test_extract_clauses() {
    std::cout << "Testing clause extraction..." << std::endl;
    
    auto p = make_constant("P");
    auto q = make_constant("Q");
    auto r = make_constant("R");
    
    // Test single clause: P ∨ Q
    auto p_or_q = make_or(p, q);
    auto clauses = CNFConverter::extract_clauses(p_or_q);
    assert(clauses.size() == 1);
    assert(clauses[0]->size() == 2);
    
    // Test multiple clauses: (P ∨ Q) ∧ (¬P ∨ R)
    auto neg_p = make_not(p);
    auto neg_p_or_r = make_or(neg_p, r);
    auto cnf_formula = make_and(p_or_q, neg_p_or_r);
    
    clauses = CNFConverter::extract_clauses(cnf_formula);
    assert(clauses.size() == 2);
    assert(clauses[0]->size() == 2);
    assert(clauses[1]->size() == 2);
    
    // Test unit clause: P
    clauses = CNFConverter::extract_clauses(p);
    assert(clauses.size() == 1);
    assert(clauses[0]->size() == 1);
    assert(clauses[0]->is_unit());
    
    std::cout << "Clause extraction tests passed!" << std::endl;
}

void test_full_cnf_conversion() {
    std::cout << "Testing full CNF conversion..." << std::endl;
    
    auto p = make_constant("P");
    auto q = make_constant("Q");
    auto r = make_constant("R");
    
    // Test (P → Q) ∧ (Q → R) ∧ P ⊢ R
    auto p_impl_q = make_implies(p, q);
    auto q_impl_r = make_implies(q, r);
    auto premise = make_and(make_and(p_impl_q, q_impl_r), p);
    
    auto clauses = CNFConverter::to_cnf(premise);
    
    // Should have multiple clauses after conversion
    assert(clauses.size() >= 3);
    
    // Verify no clause is a tautology
    for (const auto& clause : clauses) {
        assert(!clause->is_tautology());
    }
    
    std::cout << "Full CNF conversion tests passed!" << std::endl;
}

void test_cnf_with_quantifiers() {
    std::cout << "Testing CNF conversion with quantifiers..." << std::endl;
    
    // Test ∀x.(P(x) → Q(x))
    auto p_x = make_function_application("P", {make_variable(0)});
    auto q_x = make_function_application("Q", {make_variable(0)});
    auto impl = make_implies(p_x, q_x);
    auto forall_impl = make_forall("x", impl);
    
    auto clauses = CNFConverter::to_cnf(forall_impl);
    assert(clauses.size() >= 1);
    
    // Test ∃x.P(x) ∨ ∀y.Q(y)
    auto exists_p = make_exists("x", p_x);
    auto forall_q = make_forall("y", make_function_application("Q", {make_variable(0)}));
    auto complex_formula = make_or(exists_p, forall_q);
    
    clauses = CNFConverter::to_cnf(complex_formula);
    assert(clauses.size() >= 1);
    
    std::cout << "CNF conversion with quantifiers tests passed!" << std::endl;
}

void test_cnf_edge_cases() {
    std::cout << "Testing CNF conversion edge cases..." << std::endl;
    
    auto p = make_constant("P");
    
    // Test single constant
    auto clauses = CNFConverter::to_cnf(p);
    assert(clauses.size() == 1);
    assert(clauses[0]->is_unit());
    
    // Test negated constant
    auto neg_p = make_not(p);
    clauses = CNFConverter::to_cnf(neg_p);
    assert(clauses.size() == 1);
    assert(clauses[0]->is_unit());
    assert(clauses[0]->literals()[0].is_negative());
    
    // Test tautology: P ∨ ¬P
    auto tautology = make_or(p, neg_p);
    clauses = CNFConverter::to_cnf(tautology);
    
    // Tautologies might be eliminated or kept depending on implementation
    if (!clauses.empty()) {
        for (const auto& clause : clauses) {
            // If kept, should be detected as tautology
            if (clause->size() == 2) {
                assert(clause->is_tautology());
            }
        }
    }
    
    std::cout << "CNF conversion edge case tests passed!" << std::endl;
}

int main() {
    std::cout << "===== Running CNF Converter Tests =====" << std::endl;
    
    test_eliminate_implications();
    test_move_negations_inward();
    test_quantifier_negation();
    test_skolemization();
    test_distribute_or_over_and();
    test_extract_clauses();
    test_full_cnf_conversion();
    test_cnf_with_quantifiers();
    test_cnf_edge_cases();
    
    std::cout << "\n===== All CNF Converter Tests Passed! =====" << std::endl;
    return 0;
}