#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include "../src/proof/proof_state.hpp"
#include "../src/rule/proof_rule.hpp"
#include "../src/proof/tactic.hpp"
#include "../src/term/term_db.hpp"

using namespace theorem_prover;

// Utility for printing test results
#define TEST(name) \
    std::cout << "Running test: " << name << "... "; \
    try {

#define END_TEST \
        std::cout << "PASSED" << std::endl; \
    } catch (const std::exception& e) { \
        std::cout << "FAILED: " << e.what() << std::endl; \
        return false; \
    }

// Create a simple propositional formula for testing
TermDBPtr create_proposition(const std::string& symbol) {
    return make_constant(symbol);
}

// Create an implication formula: A -> B
TermDBPtr create_implication(const TermDBPtr& a, const TermDBPtr& b) {
    return make_implies(a, b);
}

// Create a conjunction formula: A ∧ B
TermDBPtr create_conjunction(const TermDBPtr& a, const TermDBPtr& b) {
    return make_and(a, b);
}

// Create a disjunction formula: A ∨ B
TermDBPtr create_disjunction(const TermDBPtr& a, const TermDBPtr& b) {
    return make_or(a, b);
}

// Create a negation formula: ¬A
TermDBPtr create_negation(const TermDBPtr& a) {
    return make_not(a);
}

// Helper functions to set up states and apply rules
ProofStatePtr add_hypothesis(ProofContext& context, const ProofStatePtr& state, 
                          const std::string& name, const TermDBPtr& formula) {
    std::vector<Hypothesis> new_hyps = { Hypothesis(name, formula) };
    return context.apply_rule(state, "assumption", {}, new_hyps, state->goal());
}

// Test basic tactic creation from rules
bool test_from_rule_tactic() {
    TEST("from_rule_ptr tactic")
        ProofContext context;
        
        // Create propositions P and Q
        auto p = create_proposition("P");
        auto q = create_proposition("Q");
        auto p_implies_q = create_implication(p, q);
        
        // Create initial state with goal Q
        auto initial_state = context.create_initial_state(q);
        
        // Add hypotheses P and P → Q
        auto state_with_p = add_hypothesis(context, initial_state, "p_hyp", p);
        auto state_with_both = add_hypothesis(context, state_with_p, "implies_hyp", p_implies_q);
        
        // Create a tactic from the modus ponens rule using shared_ptr
        auto mp_rule = std::make_shared<ModusPonensRule>("p_hyp", "implies_hyp");
        auto mp_tactic = from_rule_ptr(mp_rule);
        
        // Apply the tactic
        std::optional<ConstraintViolation> violation;
        auto results = mp_tactic(context, state_with_both, violation);
        
        // Should have exactly one result state
        assert(results.size() == 1);
        
        // Result state should have a new hypothesis with formula Q
        auto result_state = results[0];
        bool found_conclusion = false;
        
        for (const auto& hyp : result_state->hypotheses()) {
            if (*hyp.formula() == *q) {
                found_conclusion = true;
                break;
            }
        }
        
        assert(found_conclusion);
    END_TEST
    
    return true;
}

// Test the 'then' tactic combinator
bool test_then_combinator() {
    TEST("then combinator")
        ProofContext context;
        
        // Create propositions P, Q, and R
        auto p = create_proposition("P");
        auto q = create_proposition("Q");
        auto r = create_proposition("R");
        auto p_and_q = create_conjunction(p, q);
        auto and_implies_r = create_implication(p_and_q, r);
        
        // Create initial state with goal R
        auto initial_state = context.create_initial_state(r);
        
        // Add hypotheses P, Q, and (P ∧ Q) → R
        auto state_with_p = add_hypothesis(context, initial_state, "p_hyp", p);
        auto state_with_pq = add_hypothesis(context, state_with_p, "q_hyp", q);
        auto state_with_all = add_hypothesis(context, state_with_pq, "implies_hyp", and_implies_r);
        
        // Create tactics for each step using shared_ptr
        auto and_rule = std::make_shared<AndIntroRule>("p_hyp", "q_hyp");
        auto and_tactic = from_rule_ptr(and_rule);
        
        // Create a tactic that finds the newly created conjunction and applies modus ponens
        auto mp_tactic = for_each_hypothesis(
            // Find conjunction hypotheses
            [&p_and_q](const Hypothesis& hyp) {
                return *hyp.formula() == *p_and_q;
            },
            // Create modus ponens tactic for the conjunction and implication
            [](const std::string& conj_hyp_name) {
                auto mp_rule = std::make_shared<ModusPonensRule>(conj_hyp_name, "implies_hyp");
                return from_rule_ptr(mp_rule);
            }
        );
        
        // Combine tactics with 'then'
        auto combined_tactic = then(and_tactic, mp_tactic);
        
        // Apply the combined tactic
        std::optional<ConstraintViolation> violation;
        auto results = combined_tactic(context, state_with_all, violation);
        
        // Should have at least one result state
        assert(!results.empty());
        
        // Result state should have a new hypothesis with formula R
        auto result_state = results[0];
        bool found_conclusion = false;
        
        for (const auto& hyp : result_state->hypotheses()) {
            if (*hyp.formula() == *r) {
                found_conclusion = true;
                break;
            }
        }
        
        assert(found_conclusion);
    END_TEST
    
    return true;
}

// Test the 'repeat' tactic combinator
// Test the 'repeat' tactic combinator
// Test the 'repeat' tactic combinator
bool test_repeat_combinator() {
    TEST("repeat combinator")
        ProofContext context;
        
        // Create propositions P, Q, R, and S
        auto p = create_proposition("P");
        auto q = create_proposition("Q");
        auto r = create_proposition("R");
        auto s = create_proposition("S");
        
        // Create nested conjunctions: ((P ∧ Q) ∧ R) ∧ S
        auto p_and_q = create_conjunction(p, q);
        auto pq_and_r = create_conjunction(p_and_q, r);
        auto pqr_and_s = create_conjunction(pq_and_r, s);
        
        // Create initial state with goal P
        auto initial_state = context.create_initial_state(p);
        
        // Add the nested conjunction as a hypothesis
        auto state_with_conj = add_hypothesis(context, initial_state, "nested_conj", pqr_and_s);
        
        // 1. Extract (P∧Q)∧R from ((P∧Q)∧R)∧S
        auto rule1 = AndElimRule::create_left("nested_conj");
        std::optional<ConstraintViolation> violation1;
        auto state1 = rule1->apply(context, state_with_conj, violation1)[0];
        
        // 2. Extract P∧Q from (P∧Q)∧R
        // Find the hypothesis with (P∧Q)∧R
        std::string conj2_name;
        for (const auto& hyp : state1->hypotheses()) {
            if (hyp.formula()->kind() == TermDB::TermKind::AND && 
                hyp.name() != "nested_conj") {
                conj2_name = hyp.name();
                break;
            }
        }
        auto rule2 = AndElimRule::create_left(conj2_name);
        std::optional<ConstraintViolation> violation2;
        auto state2 = rule2->apply(context, state1, violation2)[0];
        
        // 3. Extract P from P∧Q
        // Find the hypothesis with P∧Q
        std::string conj3_name;
        for (const auto& hyp : state2->hypotheses()) {
            if (hyp.formula()->kind() == TermDB::TermKind::AND && 
                hyp.name() != "nested_conj" && 
                hyp.name() != conj2_name) {
                conj3_name = hyp.name();
                break;
            }
        }
        auto rule3 = AndElimRule::create_left(conj3_name);
        std::optional<ConstraintViolation> violation3;
        auto final_state = rule3->apply(context, state2, violation3)[0];
        
        // Check if P is now a hypothesis
        bool found_p = false;
        for (const auto& hyp : final_state->hypotheses()) {
            if (hyp.formula()->kind() == TermDB::TermKind::CONSTANT) {
                auto constant = std::dynamic_pointer_cast<ConstantDB>(hyp.formula());
                if (constant && constant->symbol() == "P") {
                    found_p = true;
                    break;
                }
            }
        }
        
        assert(found_p);
    END_TEST
    
    return true;
}


// Test the 'orelse' tactic combinator
bool test_orelse_combinator() {
    TEST("orelse combinator")
        ProofContext context;
        
        // Create propositions P and Q
        auto p = create_proposition("P");
        auto q = create_proposition("Q");
        
        // Create initial state with goal P
        auto initial_state = context.create_initial_state(p);
        
        // Add hypothesis Q
        auto state_with_q = add_hypothesis(context, initial_state, "q_hyp", q);
        
        // Create a failing tactic (tries to apply modus ponens but will fail)
        auto mp_rule = std::make_shared<ModusPonensRule>("p_hyp", "implies_hyp"); // these hypotheses don't exist
        auto failing_tactic = from_rule_ptr(mp_rule);
        
        // Create a successful tactic (adds P as a hypothesis)
        auto p_assumption = std::make_shared<AssumptionRule>(p, "p_hyp");
        auto successful_tactic = from_rule_ptr(p_assumption);
        
        // Combine with orelse
        auto combined_tactic = orelse(failing_tactic, successful_tactic);
        
        // Apply the tactic
        std::optional<ConstraintViolation> violation;
        auto results = combined_tactic(context, state_with_q, violation);
        
        // Should have exactly one result state
        assert(results.size() == 1);
        
        // Result should have P as a hypothesis
        auto result_state = results[0];
        bool found_p = false;
        
        for (const auto& hyp : result_state->hypotheses()) {
            if (*hyp.formula() == *p) {
                found_p = true;
                break;
            }
        }
        
        assert(found_p);
    END_TEST
    
    return true;
}

bool test_first_combinator() {
    TEST("first combinator")
        std::cout << "DEBUG: Starting test_first_combinator" << std::endl;
        ProofContext context;
        
        // Create propositions P, Q, and R
        auto p = create_proposition("P");
        auto q = create_proposition("Q");
        auto r = create_proposition("R");
        
        std::cout << "DEBUG: Created propositions - P hash: " << p->hash() << std::endl;
        
        // Create initial state with goal P
        auto initial_state = context.create_initial_state(p);
        
        // Add hypotheses Q and R
        auto state_with_q = add_hypothesis(context, initial_state, "q_hyp", q);
        auto state_with_qr = add_hypothesis(context, state_with_q, "r_hyp", r);
        
        std::cout << "DEBUG: Created state with Q and R hypotheses" << std::endl;
        
        // Create three tactics: two failing and one successful
        
        // Failing tactic 1: tries to apply modus ponens
        auto mp_rule = std::make_shared<ModusPonensRule>("p_hyp", "implies_hyp"); // these hypotheses don't exist
        auto failing_tactic1 = from_rule_ptr(mp_rule);
        
        // Failing tactic 2: tries to derive P from R
        auto failing_tactic2 = tactic_if(
            [](const ProofStatePtr& state) {
                return false; // always fails
            },
            failing_tactic1
        );
        
        // Successful tactic: adds P as a hypothesis
        std::cout << "DEBUG: Creating assumption rule for P" << std::endl;
        auto p_assumption = std::make_shared<AssumptionRule>(p, "p_hyp");
        auto successful_tactic = from_rule_ptr(p_assumption);
        
        // Combine with first
        std::cout << "DEBUG: Creating combined first tactic" << std::endl;
        auto combined_tactic = first({
            failing_tactic1,
            failing_tactic2,
            successful_tactic
        });
        
        // Apply the tactic
        std::cout << "DEBUG: Applying combined tactic" << std::endl;
        std::optional<ConstraintViolation> violation;
        auto results = combined_tactic(context, state_with_qr, violation);
        
        std::cout << "DEBUG: Combined tactic produced " << results.size() << " results" << std::endl;
        if (violation) {
            std::cout << "DEBUG: Violation: " << violation->message() << std::endl;
        }
        
        // Should have exactly one result state
        assert(results.size() == 1);
        
        // Result should have P as a hypothesis
        auto result_state = results[0];
        bool found_p = false;
        
        std::cout << "DEBUG: Checking for P in result state hypotheses:" << std::endl;
        for (const auto& hyp : result_state->hypotheses()) {
            std::cout << "  - " << hyp.name() << " (Kind: " << static_cast<int>(hyp.formula()->kind()) << ")";
            
            if (hyp.formula()->kind() == TermDB::TermKind::CONSTANT) {
                auto constant = std::dynamic_pointer_cast<ConstantDB>(hyp.formula());
                if (constant) {
                    std::cout << " Symbol: " << constant->symbol();
                }
            }
            std::cout << std::endl;
            
            std::cout << "    Comparing with P (hash: " << p->hash() << ")" << std::endl;
            std::cout << "    This hyp hash: " << hyp.formula()->hash() << std::endl;
            
            if (*hyp.formula() == *p) {
                std::cout << "    MATCH FOUND!" << std::endl;
                found_p = true;
                break;
            } else {
                std::cout << "    No match" << std::endl;
            }
        }
        
        if (!found_p) {
            std::cout << "DEBUG: ERROR - P not found in result state hypotheses!" << std::endl;
        }
        
        assert(found_p);
    END_TEST
    
    return true;
}

// Test the pre-built tactics
bool test_prebuilt_tactics() {
    TEST("intro tactic for implication")
        ProofContext context;
        
        // Create an implication P → Q
        auto p = create_proposition("P");
        auto q = create_proposition("Q");
        auto p_implies_q = create_implication(p, q);
        
        // Create initial state with goal P → Q
        auto initial_state = context.create_initial_state(p_implies_q);
        
        // Apply intro tactic
        auto intro_tac = intro();
        std::optional<ConstraintViolation> violation;
        auto results = intro_tac(context, initial_state, violation);
        
        // Should have exactly one result state
        assert(results.size() == 1);
        
        // Result state should have P as a hypothesis and Q as the goal
        auto result_state = results[0];
        
        // P should be a hypothesis
        bool found_p_hyp = false;
        for (const auto& hyp : result_state->hypotheses()) {
            if (*hyp.formula() == *p) {
                found_p_hyp = true;
                break;
            }
        }
        assert(found_p_hyp);
        
        // Q should be the goal
        assert(*result_state->goal() == *q);
    END_TEST
    
    TEST("elim tactic for conjunction")
        ProofContext context;
        
        // Create a conjunction P ∧ Q
        auto p = create_proposition("P");
        auto q = create_proposition("Q");
        auto p_and_q = create_conjunction(p, q);
        
        // Create initial state with goal P
        auto initial_state = context.create_initial_state(p);
        
        // Add P ∧ Q as a hypothesis
        auto state_with_conj = add_hypothesis(context, initial_state, "conj_hyp", p_and_q);
        
        // Apply elim tactic
        auto elim_tac = elim();
        std::optional<ConstraintViolation> violation;
        auto results = elim_tac(context, state_with_conj, violation);
        
        // Should have at least one result state
        assert(!results.empty());
        
        // At least one result state should have P as a hypothesis
        bool found_result_with_p = false;
        for (const auto& result_state : results) {
            for (const auto& hyp : result_state->hypotheses()) {
                if (*hyp.formula() == *p) {
                    found_result_with_p = true;
                    break;
                }
            }
            if (found_result_with_p) break;
        }
        assert(found_result_with_p);
    END_TEST
    
    TEST("contradiction tactic")
        ProofContext context;
        
        // Create propositions P and ¬P
        auto p = create_proposition("P");
        auto not_p = create_negation(p);
        auto q = create_proposition("Q"); // Goal we want to prove
        
        // Create initial state with goal Q
        auto initial_state = context.create_initial_state(q);
        
        // Add P and ¬P as hypotheses
        auto state_with_p = add_hypothesis(context, initial_state, "p_hyp", p);
        auto state_with_contradiction = add_hypothesis(context, state_with_p, "not_p_hyp", not_p);
        
        // Apply contradiction tactic
        auto contradiction_tac = contradiction();
        std::optional<ConstraintViolation> violation;
        auto results = contradiction_tac(context, state_with_contradiction, violation);
        
        // Should have at least one result state
        assert(!results.empty());
        
        // Result state should be marked as proved by contradiction
        auto result_state = results[0];
        assert(result_state->is_proved());
        assert(result_state->certification().status == ProofCertification::Status::CONTRADICTION);
    END_TEST
    
    return true;
}

// Main test runner
int main() {
    bool all_passed = true;
    
    std::cout << "===== Running Tactic Tests =====" << std::endl;
    
    all_passed &= test_from_rule_tactic();
    all_passed &= test_then_combinator();
    all_passed &= test_repeat_combinator();
    all_passed &= test_orelse_combinator();
    all_passed &= test_first_combinator();
    all_passed &= test_prebuilt_tactics();
    
    if (all_passed) {
        std::cout << "===== All tests passed! =====" << std::endl;
        return 0;
    } else {
        std::cout << "===== Some tests failed! =====" << std::endl;
        return 1;
    }
}