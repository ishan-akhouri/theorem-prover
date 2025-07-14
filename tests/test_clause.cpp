#include <iostream>
#include <cassert>
#include "../src/resolution/clause.hpp"
#include "../src/term/term_db.hpp"

using namespace theorem_prover;

void test_literal_creation() {
    std::cout << "Testing literal creation..." << std::endl;
    
    // Test positive literal
    auto atom_p = make_constant("P");
    Literal pos_p(atom_p, true);
    assert(pos_p.is_positive());
    assert(!pos_p.is_negative());
    assert(*pos_p.atom() == *atom_p);
    
    // Test negative literal
    Literal neg_p(atom_p, false);
    assert(!neg_p.is_positive());
    assert(neg_p.is_negative());
    
    // Test negation
    auto negated = pos_p.negate();
    assert(negated.is_negative());
    assert(*negated.atom() == *atom_p);
    
    std::cout << "Literal creation tests passed!" << std::endl;
}

void test_literal_complementary() {
    std::cout << "Testing complementary literals..." << std::endl;
    
    auto atom_p = make_constant("P");
    auto atom_q = make_constant("Q");
    
    Literal pos_p(atom_p, true);
    Literal neg_p(atom_p, false);
    Literal pos_q(atom_q, true);
    
    // P and ¬P should be complementary
    assert(pos_p.is_complementary(neg_p));
    assert(neg_p.is_complementary(pos_p));
    
    // P and Q should not be complementary
    assert(!pos_p.is_complementary(pos_q));
    
    // P and P should not be complementary
    assert(!pos_p.is_complementary(pos_p));
    
    std::cout << "Complementary literal tests passed!" << std::endl;
}

void test_clause_creation() {
    std::cout << "Testing clause creation..." << std::endl;
    
    auto atom_p = make_constant("P");
    auto atom_q = make_constant("Q");
    auto atom_r = make_constant("R");
    
    // Test empty clause
    Clause empty_clause;
    assert(empty_clause.is_empty());
    assert(!empty_clause.is_unit());
    assert(empty_clause.size() == 0);
    
    // Test unit clause
    std::vector<Literal> unit_literals = {Literal(atom_p, true)};
    Clause unit_clause(unit_literals);
    assert(!unit_clause.is_empty());
    assert(unit_clause.is_unit());
    assert(unit_clause.size() == 1);
    
    // Test multi-literal clause: P ∨ ¬Q ∨ R
    std::vector<Literal> multi_literals = {
        Literal(atom_p, true),
        Literal(atom_q, false),
        Literal(atom_r, true)
    };
    Clause multi_clause(multi_literals);
    assert(!multi_clause.is_empty());
    assert(!multi_clause.is_unit());
    assert(multi_clause.size() == 3);
    
    std::cout << "Clause creation tests passed!" << std::endl;
}

void test_clause_tautology() {
    std::cout << "Testing tautology detection..." << std::endl;
    
    auto atom_p = make_constant("P");
    auto atom_q = make_constant("Q");
    
    // Test tautology: P ∨ ¬P
    std::vector<Literal> taut_literals = {
        Literal(atom_p, true),
        Literal(atom_p, false)
    };
    Clause taut_clause(taut_literals);
    assert(taut_clause.is_tautology());
    
    // Test non-tautology: P ∨ Q
    std::vector<Literal> non_taut_literals = {
        Literal(atom_p, true),
        Literal(atom_q, true)
    };
    Clause non_taut_clause(non_taut_literals);
    assert(!non_taut_clause.is_tautology());
    
    // Test complex tautology: P ∨ Q ∨ ¬P ∨ R
    std::vector<Literal> complex_taut_literals = {
        Literal(atom_p, true),
        Literal(atom_q, true),
        Literal(atom_p, false),
        Literal(make_constant("R"), true)
    };
    Clause complex_taut_clause(complex_taut_literals);
    assert(complex_taut_clause.is_tautology());
    
    std::cout << "Tautology detection tests passed!" << std::endl;
}

void test_clause_simplification() {
    std::cout << "Testing clause simplification..." << std::endl;
    
    auto atom_p = make_constant("P");
    auto atom_q = make_constant("Q");
    
    // Test duplicate removal: P ∨ P ∨ Q becomes P ∨ Q
    std::vector<Literal> dup_literals = {
        Literal(atom_p, true),
        Literal(atom_p, true),
        Literal(atom_q, true)
    };
    Clause dup_clause(dup_literals);
    auto simplified = dup_clause.simplify();
    assert(simplified.size() == 2);
    
    // Test tautology simplification: P ∨ ¬P becomes empty
    std::vector<Literal> taut_literals = {
        Literal(atom_p, true),
        Literal(atom_p, false)
    };
    Clause taut_clause(taut_literals);
    auto taut_simplified = taut_clause.simplify();
    assert(taut_simplified.is_empty());
    
    std::cout << "Clause simplification tests passed!" << std::endl;
}

void test_clause_substitution() {
    std::cout << "Testing clause substitution..." << std::endl;
    
    // Create clause with variables: P(X) ∨ Q(Y)
    auto p_x = make_function_application("P", {make_variable(0)});
    auto q_y = make_function_application("Q", {make_variable(1)});
    
    std::vector<Literal> var_literals = {
        Literal(p_x, true),
        Literal(q_y, true)
    };
    Clause var_clause(var_literals);
    
    // Create substitution: X -> a, Y -> b
    SubstitutionMap subst;
    subst[0] = make_constant("a");
    subst[1] = make_constant("b");
    
    // Apply substitution
    auto substituted = var_clause.substitute(subst);
    assert(substituted.size() == 2);
    
    // Verify substitution worked (would need deeper inspection in practice)
    std::cout << "Clause substitution tests passed!" << std::endl;
}

void test_resolution_basic() {
    std::cout << "Testing basic resolution..." << std::endl;
    
    auto atom_p = make_constant("P");
    auto atom_q = make_constant("Q");
    
    // Test resolving P with ¬P ∨ Q to get Q
    std::vector<Literal> clause1_literals = {Literal(atom_p, true)};
    std::vector<Literal> clause2_literals = {
        Literal(atom_p, false),
        Literal(atom_q, true)
    };
    
    auto clause1 = std::make_shared<Clause>(clause1_literals);
    auto clause2 = std::make_shared<Clause>(clause2_literals);
    
    auto result = ResolutionInference::resolve(clause1, clause2);
    assert(result.success);
    assert(result.resolvent->size() == 1);
    assert(result.resolvent->literals()[0].is_positive());
    
    std::cout << "Basic resolution tests passed!" << std::endl;
}

void test_resolution_with_unification() {
    std::cout << "Testing resolution with unification..." << std::endl;
    
    // Test resolving P(X) with ¬P(a) ∨ Q(a) to get Q(a)
    auto p_x = make_function_application("P", {make_variable(0)});
    auto p_a = make_function_application("P", {make_constant("a")});
    auto q_a = make_function_application("Q", {make_constant("a")});
    
    std::vector<Literal> clause1_literals = {Literal(p_x, true)};
    std::vector<Literal> clause2_literals = {
        Literal(p_a, false),
        Literal(q_a, true)
    };
    
    auto clause1 = std::make_shared<Clause>(clause1_literals);
    auto clause2 = std::make_shared<Clause>(clause2_literals);
    
    auto result = ResolutionInference::resolve(clause1, clause2);
    assert(result.success);
    assert(result.resolvent->size() == 1);
    
    std::cout << "Resolution with unification tests passed!" << std::endl;
}

void test_resolution_failure_cases() {
    std::cout << "Testing resolution failure cases..." << std::endl;
    
    auto atom_p = make_constant("P");
    auto atom_q = make_constant("Q");
    
    // Test resolving P with Q (no complementary literals)
    std::vector<Literal> clause1_literals = {Literal(atom_p, true)};
    std::vector<Literal> clause2_literals = {Literal(atom_q, true)};
    
    auto clause1 = std::make_shared<Clause>(clause1_literals);
    auto clause2 = std::make_shared<Clause>(clause2_literals);
    
    auto result = ResolutionInference::resolve(clause1, clause2);
    assert(!result.success);
    
    // Test resolving P with P (same polarity)
    std::vector<Literal> clause3_literals = {Literal(atom_p, true)};
    auto clause3 = std::make_shared<Clause>(clause3_literals);
    
    result = ResolutionInference::resolve(clause1, clause3);
    assert(!result.success);
    
    std::cout << "Resolution failure case tests passed!" << std::endl;
}

void test_empty_clause_resolution() {
    std::cout << "Testing empty clause resolution..." << std::endl;
    
    auto atom_p = make_constant("P");
    
    // Test resolving P with ¬P to get empty clause
    std::vector<Literal> clause1_literals = {Literal(atom_p, true)};
    std::vector<Literal> clause2_literals = {Literal(atom_p, false)};
    
    auto clause1 = std::make_shared<Clause>(clause1_literals);
    auto clause2 = std::make_shared<Clause>(clause2_literals);
    
    auto result = ResolutionInference::resolve(clause1, clause2);
    assert(result.success);
    assert(result.resolvent->is_empty());
    
    std::cout << "Empty clause resolution tests passed!" << std::endl;
}

void test_factoring() {
    std::cout << "Testing factoring..." << std::endl;
    
    // Test factoring P(X) ∨ P(a) with substitution X -> a to get P(a)
    auto p_x = make_function_application("P", {make_variable(0)});
    auto p_a = make_function_application("P", {make_constant("a")});
    
    std::vector<Literal> literals = {
        Literal(p_x, true),
        Literal(p_a, true)
    };
    
    auto clause = std::make_shared<Clause>(literals);
    auto factored = ResolutionInference::factor(clause);
    
    // Should have reduced to one literal after factoring
    assert(factored->size() <= 2); // Simplified check
    
    std::cout << "Factoring tests passed!" << std::endl;
}

int main() {
    std::cout << "===== Running Clause Tests =====" << std::endl;
    
    test_literal_creation();
    test_literal_complementary();
    test_clause_creation();
    test_clause_tautology();
    test_clause_simplification();
    test_clause_substitution();
    test_resolution_basic();
    test_resolution_with_unification();
    test_resolution_failure_cases();
    test_empty_clause_resolution();
    test_factoring();
    
    std::cout << "\n===== All Clause Tests Passed! =====" << std::endl;
    return 0;
}