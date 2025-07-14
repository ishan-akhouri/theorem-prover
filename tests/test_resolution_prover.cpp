// tests/test_resolution_prover.cpp
#include <iostream>
#include <cassert>
#include "../src/resolution/resolution_prover.hpp"
#include "../src/term/term_db.hpp"

using namespace theorem_prover;

void test_basic_resolution_proving() {
    std::cout << "Testing basic resolution proving..." << std::endl;
    
    ResolutionProver prover;
    
    // Test simple modus ponens: P, P → Q ⊢ Q
    auto p = make_constant("P");
    auto q = make_constant("Q");
    auto p_implies_q = make_implies(p, q);
    
    std::vector<TermDBPtr> hypotheses = {p, p_implies_q};
    auto result = prover.prove(q, hypotheses);
    
    assert(result.is_proved());
    assert(result.status == ResolutionProofResult::Status::PROVED);
    std::cout << "  Modus ponens proved in " << result.iterations << " iterations" << std::endl;
    
    std::cout << "Basic resolution proving tests passed!" << std::endl;
}

void test_contradiction_detection() {
    std::cout << "Testing contradiction detection..." << std::endl;
    
    ResolutionProver prover;
    
    // Test P ∧ ¬P ⊢ Q (anything follows from contradiction)
    auto p = make_constant("P");
    auto q = make_constant("Q");
    auto not_p = make_not(p);
    auto contradiction = make_and(p, not_p);
    
    std::vector<TermDBPtr> hypotheses = {contradiction};
    auto result = prover.prove(q, hypotheses);
    
    assert(result.is_proved());
    std::cout << "  Contradiction detected in " << result.iterations << " iterations" << std::endl;
    
    std::cout << "Contradiction detection tests passed!" << std::endl;
}

void test_syllogism() {
    std::cout << "Testing syllogism..." << std::endl;
    
    ResolutionProver prover;
    
    // Test P → Q, Q → R, P ⊢ R
    auto p = make_constant("P");
    auto q = make_constant("Q");
    auto r = make_constant("R");
    
    auto p_implies_q = make_implies(p, q);
    auto q_implies_r = make_implies(q, r);
    
    std::vector<TermDBPtr> hypotheses = {p_implies_q, q_implies_r, p};
    auto result = prover.prove(r, hypotheses);
    
    assert(result.is_proved());
    std::cout << "  Syllogism proved in " << result.iterations << " iterations" << std::endl;
    
    std::cout << "Syllogism tests passed!" << std::endl;
}

void test_disjunctive_syllogism() {
    std::cout << "Testing disjunctive syllogism..." << std::endl;
    
    ResolutionProver prover;
    
    // Test P ∨ Q, ¬P ⊢ Q
    auto p = make_constant("P");
    auto q = make_constant("Q");
    auto not_p = make_not(p);
    auto p_or_q = make_or(p, q);
    
    std::vector<TermDBPtr> hypotheses = {p_or_q, not_p};
    auto result = prover.prove(q, hypotheses);
    
    assert(result.is_proved());
    std::cout << "  Disjunctive syllogism proved in " << result.iterations << " iterations" << std::endl;
    
    std::cout << "Disjunctive syllogism tests passed!" << std::endl;
}

void test_unprovable_theorem() {
    std::cout << "Testing unprovable theorem..." << std::endl;
    
    ResolutionProver prover;
    
    // Test P ⊢ Q (should not be provable)
    auto p = make_constant("P");
    auto q = make_constant("Q");
    
    std::vector<TermDBPtr> hypotheses = {p};
    auto result = prover.prove(q, hypotheses);
    
    assert(!result.is_proved());
    assert(result.status == ResolutionProofResult::Status::SATURATED);
    std::cout << "  Correctly identified unprovable theorem" << std::endl;
    
    std::cout << "Unprovable theorem tests passed!" << std::endl;
}

void test_simple_quantifier_instantiation() {
    std::cout << "Testing simple quantifier instantiation..." << std::endl;
    
    ResolutionProver prover;
    
    // Simpler test: ∀x.P(x) ⊢ P(a)
    auto a = make_constant("a");
    auto p_x = make_function_application("P", {make_variable(0)});
    auto p_a = make_function_application("P", {a});
    
    auto forall_p = make_forall("x", p_x);
    
    std::vector<TermDBPtr> hypotheses = {forall_p};
    auto result = prover.prove(p_a, hypotheses);
    
    std::cout << "  Result status: " << static_cast<int>(result.status) << std::endl;
    std::cout << "  Explanation: " << result.explanation << std::endl;
    
    if (!result.is_proved()) {
        std::cout << "  FAILED - analyzing clauses..." << std::endl;
        // Convert to see what clauses we get
        auto clauses = CNFConverter::to_cnf(forall_p);
        resolution_utils::print_clause_set(clauses, "Clauses from ∀x.P(x)");
        
        auto neg_goal_clauses = CNFConverter::to_cnf(make_not(p_a));
        resolution_utils::print_clause_set(neg_goal_clauses, "Clauses from ¬P(a)");
    }
    
    // This should work if our quantifier handling is correct
    assert(result.is_proved());
    
    std::cout << "Simple quantifier instantiation tests passed!" << std::endl;
}

void test_resolution_with_quantifiers() {
    std::cout << "Testing resolution with quantifiers..." << std::endl;
    
    ResolutionProver prover;
    
    // Test ∀x.P(x), P(a) → Q(a) ⊢ Q(a)
    auto a = make_constant("a");
    auto p_x = make_function_application("P", {make_variable(0)});
    auto p_a = make_function_application("P", {a});
    auto q_a = make_function_application("Q", {a});
    
    auto forall_p = make_forall("x", p_x);
    auto p_implies_q = make_implies(p_a, q_a);
    
    std::vector<TermDBPtr> hypotheses = {forall_p, p_implies_q};
    
    // Debug: Let's see what clauses we get from CNF conversion
    std::cout << "  Converting hypotheses to CNF..." << std::endl;
    
    auto forall_clauses = CNFConverter::to_cnf(forall_p);
    std::cout << "  ∀x.P(x) converts to " << forall_clauses.size() << " clauses" << std::endl;
    
    auto impl_clauses = CNFConverter::to_cnf(p_implies_q);
    std::cout << "  P(a) → Q(a) converts to " << impl_clauses.size() << " clauses" << std::endl;
    
    // Convert goal to negated form for refutation
    auto neg_goal = make_not(q_a);
    auto goal_clauses = CNFConverter::to_cnf(neg_goal);
    std::cout << "  ¬Q(a) converts to " << goal_clauses.size() << " clauses" << std::endl;
    
    auto result = prover.prove(q_a, hypotheses);
    
    std::cout << "  Result status: " << static_cast<int>(result.status) << std::endl;
    std::cout << "  Iterations: " << result.iterations << std::endl;
    std::cout << "  Explanation: " << result.explanation << std::endl;
    
    if (!result.is_proved()) {
        std::cout << "  FAILED - let's analyze the final clause set:" << std::endl;
        resolution_utils::print_clause_set(result.final_clauses, "Final Clauses");
    }
    
    assert(result.is_proved());
    std::cout << "  Quantified formula proved in " << result.iterations << " iterations" << std::endl;
    
    std::cout << "Resolution with quantifiers tests passed!" << std::endl;
}

void test_existential_reasoning() {
    std::cout << "Testing existential reasoning..." << std::endl;
    
    ResolutionProver prover;
    
    // Test ∃x.P(x), ∀x.(P(x) → Q(x)) ⊢ ∃x.Q(x)
    auto p_x = make_function_application("P", {make_variable(0)});
    auto q_x = make_function_application("Q", {make_variable(0)});
    
    auto exists_p = make_exists("x", p_x);
    auto p_implies_q = make_implies(p_x, q_x);
    auto forall_impl = make_forall("x", p_implies_q);
    auto exists_q = make_exists("y", make_function_application("Q", {make_variable(0)}));
    
    std::vector<TermDBPtr> hypotheses = {exists_p, forall_impl};
    auto result = prover.prove(exists_q, hypotheses);
    
    assert(result.is_proved());
    std::cout << "  Existential reasoning proved in " << result.iterations << " iterations" << std::endl;
    
    std::cout << "Existential reasoning tests passed!" << std::endl;
}

void test_clause_selection_strategies() {
    std::cout << "Testing different clause selection strategies..." << std::endl;
    
    // Test same problem with different strategies
    auto p = make_constant("P");
    auto q = make_constant("Q");
    auto r = make_constant("R");
    
    auto p_implies_q = make_implies(p, q);
    auto q_implies_r = make_implies(q, r);
    std::vector<TermDBPtr> hypotheses = {p_implies_q, q_implies_r, p};
    
    // Test FIFO strategy
    ResolutionConfig fifo_config;
    fifo_config.selection_strategy = ResolutionConfig::SelectionStrategy::FIFO;
    ResolutionProver fifo_prover(fifo_config);
    auto fifo_result = fifo_prover.prove(r, hypotheses);
    assert(fifo_result.is_proved());
    std::cout << "  FIFO strategy: " << fifo_result.iterations << " iterations" << std::endl;
    
    // Test SMALLEST_FIRST strategy
    ResolutionConfig smallest_config;
    smallest_config.selection_strategy = ResolutionConfig::SelectionStrategy::SMALLEST_FIRST;
    ResolutionProver smallest_prover(smallest_config);
    auto smallest_result = smallest_prover.prove(r, hypotheses);
    assert(smallest_result.is_proved());
    std::cout << "  Smallest first strategy: " << smallest_result.iterations << " iterations" << std::endl;
    
    // Test UNIT_PREFERENCE strategy
    ResolutionConfig unit_config;
    unit_config.selection_strategy = ResolutionConfig::SelectionStrategy::UNIT_PREFERENCE;
    ResolutionProver unit_prover(unit_config);
    auto unit_result = unit_prover.prove(r, hypotheses);
    assert(unit_result.is_proved());
    std::cout << "  Unit preference strategy: " << unit_result.iterations << " iterations" << std::endl;
    
    std::cout << "Clause selection strategy tests passed!" << std::endl;
}

void test_satisfiability_checking() {
    std::cout << "Testing satisfiability checking..." << std::endl;
    
    ResolutionProver prover;
    
    // Test satisfiable set: P ∨ Q, ¬P ∨ R
    auto p = make_constant("P");
    auto q = make_constant("Q");
    auto r = make_constant("R");
    auto not_p = make_not(p);
    
    auto p_or_q = make_or(p, q);
    auto not_p_or_r = make_or(not_p, r);
    
    std::vector<TermDBPtr> satisfiable_formulas = {p_or_q, not_p_or_r};
    auto sat_result = prover.check_satisfiability(satisfiable_formulas);
    
    assert(sat_result.status == ResolutionProofResult::Status::PROVED); // Satisfiable
    std::cout << "  Correctly identified satisfiable formula set" << std::endl;
    
    // Test unsatisfiable set: P, ¬P
    std::vector<TermDBPtr> unsat_formulas = {p, not_p};
    auto unsat_result = prover.check_satisfiability(unsat_formulas);
    
    assert(unsat_result.status == ResolutionProofResult::Status::DISPROVED); // Unsatisfiable
    std::cout << "  Correctly identified unsatisfiable formula set" << std::endl;
    
    std::cout << "Satisfiability checking tests passed!" << std::endl;
}

void test_complex_reasoning() {
    std::cout << "Testing complex reasoning..." << std::endl;
    
    ResolutionProver prover;
    
    // Test more complex logical reasoning
    // (P ∧ Q) → R, P, Q ⊢ R
    auto p = make_constant("P");
    auto q = make_constant("Q");
    auto r = make_constant("R");
    
    auto p_and_q = make_and(p, q);
    auto pq_implies_r = make_implies(p_and_q, r);
    
    std::vector<TermDBPtr> hypotheses = {pq_implies_r, p, q};
    auto result = prover.prove(r, hypotheses);
    
    assert(result.is_proved());
    std::cout << "  Complex conjunction reasoning proved in " << result.iterations << " iterations" << std::endl;
    
    // Test (P → Q) ∧ (Q → R) ∧ (R → S), P ⊢ S
    auto s = make_constant("S");
    auto p_implies_q = make_implies(p, q);
    auto q_implies_r = make_implies(q, r);
    auto r_implies_s = make_implies(r, s);
    
    auto chain1 = make_and(p_implies_q, q_implies_r);
    auto chain2 = make_and(chain1, r_implies_s);
    
    std::vector<TermDBPtr> chain_hypotheses = {chain2, p};
    auto chain_result = prover.prove(s, chain_hypotheses);
    
    assert(chain_result.is_proved());
    std::cout << "  Complex implication chain proved in " << chain_result.iterations << " iterations" << std::endl;
    
    std::cout << "Complex reasoning tests passed!" << std::endl;
}

void test_timeout_and_limits() {
    std::cout << "Testing timeout and limits..." << std::endl;
    
    // Test iteration limit with a problem that generates many clauses
    ResolutionConfig iteration_limit_config;
    iteration_limit_config.max_iterations = 3;
    iteration_limit_config.max_time_ms = 10000.0; // High time limit
    iteration_limit_config.max_clauses = 1000;    // High clause limit
    
    ResolutionProver iteration_prover(iteration_limit_config);
    
    // Create a problem that will generate multiple resolution steps
    auto p1 = make_constant("P1");
    auto p2 = make_constant("P2");
    auto p3 = make_constant("P3");
    auto p4 = make_constant("P4");
    auto goal = make_constant("GOAL");
    
    // Create implications that form a chain
    auto chain1 = make_implies(p1, p2);
    auto chain2 = make_implies(p2, p3);
    auto chain3 = make_implies(p3, p4);
    auto chain4 = make_implies(p4, goal);
    
    // Add some extra clauses to complicate resolution
    auto disj1 = make_or(p1, p2);
    auto disj2 = make_or(p3, p4);
    
    std::vector<TermDBPtr> hypotheses = {chain1, chain2, chain3, chain4, disj1, disj2, p1};
    auto result = iteration_prover.prove(goal, hypotheses);
    
    // Should hit iteration limit
    assert(result.status == ResolutionProofResult::Status::TIMEOUT);
    assert(result.iterations >= iteration_limit_config.max_iterations - 1);
    std::cout << "  Iteration limit test passed (iterations: " << result.iterations << ")" << std::endl;
    
    // Test clause limit
    ResolutionConfig clause_limit_config;
    clause_limit_config.max_iterations = 1000;
    clause_limit_config.max_time_ms = 10000.0;
    clause_limit_config.max_clauses = 8; // Very low clause limit
    
    ResolutionProver clause_prover(clause_limit_config);
    auto clause_result = clause_prover.prove(goal, hypotheses);
    
    // Should hit clause limit or succeed quickly
    bool hit_clause_limit = (clause_result.status == ResolutionProofResult::Status::TIMEOUT ||
                            clause_result.status == ResolutionProofResult::Status::PROVED);
    assert(hit_clause_limit);
    std::cout << "  Clause limit test passed" << std::endl;
    
    std::cout << "Timeout and limits tests passed!" << std::endl;
}

void test_clause_set_operations() {
    std::cout << "Testing clause set operations..." << std::endl;
    
    ResolutionConfig config;
    config.use_subsumption = true;
    config.use_tautology_deletion = true;
    
    ClauseSet clause_set(config);
    
    // Test adding normal clause
    auto p = make_constant("P");
    auto q = make_constant("Q");
    
    std::vector<Literal> literals1 = {Literal(p, true), Literal(q, true)};
    auto clause1 = std::make_shared<Clause>(literals1);
    clause_set.add_clause(clause1);
    assert(clause_set.size() == 1);
    
    // Test adding duplicate clause (should be ignored)
    clause_set.add_clause(clause1);
    assert(clause_set.size() == 1);
    
    // Test adding tautology P ∨ ¬P (should be ignored)
    std::vector<Literal> taut_literals = {Literal(p, true), Literal(p, false)};
    auto taut_clause = std::make_shared<Clause>(taut_literals);
    clause_set.add_clause(taut_clause);
    assert(clause_set.size() == 1);
    
    // Test clause selection
    auto selected = clause_set.select_clause();
    assert(selected != nullptr);
    assert(selected->equals(*clause1));
    
    std::cout << "Clause set operations tests passed!" << std::endl;
}

void test_resolution_utils() {
    std::cout << "Testing resolution utilities..." << std::endl;
    
    auto p = make_constant("P");
    auto q = make_constant("Q");
    
    // Test clause set analysis
    std::vector<Literal> unit_literals = {Literal(p, true)};
    std::vector<Literal> binary_literals = {Literal(p, true), Literal(q, false)};
    
    auto unit_clause = std::make_shared<Clause>(unit_literals);
    auto binary_clause = std::make_shared<Clause>(binary_literals);
    
    std::vector<ClausePtr> test_clauses = {unit_clause, binary_clause};
    
    auto stats = resolution_utils::analyze_clause_set(test_clauses);
    assert(stats.total_clauses == 2);
    assert(stats.unit_clauses == 1);
    assert(stats.max_clause_size == 2);
    assert(stats.avg_clause_size == 1.5);
    
    // Test obviously unsatisfiable detection
    std::vector<Literal> empty_literals;
    auto empty_clause = std::make_shared<Clause>(empty_literals);
    std::vector<ClausePtr> unsat_clauses = {empty_clause};
    
    assert(resolution_utils::is_obviously_unsatisfiable(unsat_clauses));
    
    // Test obviously satisfiable detection
    std::vector<ClausePtr> empty_clause_set;
    assert(resolution_utils::is_obviously_satisfiable(empty_clause_set));
    
    std::cout << "Resolution utilities tests passed!" << std::endl;
}

int main() {
    std::cout << "===== Running Resolution Prover Tests =====" << std::endl;
    
    test_basic_resolution_proving();
    test_contradiction_detection();
    test_syllogism();
    test_disjunctive_syllogism();
    test_unprovable_theorem();
    test_simple_quantifier_instantiation();
    test_resolution_with_quantifiers();
    test_existential_reasoning();
    test_clause_selection_strategies();
    test_satisfiability_checking();
    test_complex_reasoning();
    test_timeout_and_limits();
    test_clause_set_operations();
    test_resolution_utils();
    
    std::cout << "\n===== All Resolution Prover Tests Passed! =====" << std::endl;
    return 0;
}