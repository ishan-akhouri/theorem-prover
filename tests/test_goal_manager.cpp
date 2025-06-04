// tests/test_goal_manager.cpp
#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include "../src/proof/goal_manager.hpp"
#include "../src/proof/proof_state.hpp"
#include "../src/term/term_db.hpp"

using namespace theorem_prover;

// Helper function to check if a state proves its goal
bool is_proven(const ProofStatePtr& state) {
    return state && state->is_proved();
}

// Test case for simple decomposition and recombination of conjunctive goals
void test_conjunctive_goal() {
    std::cout << "\n=== Testing conjunctive goal decomposition and recombination ===\n" << std::endl;
    
    // Create a proof context and goal manager
    ProofContext context;
    GoalManager goal_manager;
    
    // Create constants
    auto a = make_constant("a");
    auto b = make_constant("b");
    
    // Create predicates P(a) and Q(b)
    auto p_a = make_function_application("P", {a});
    auto q_b = make_function_application("Q", {b});
    
    // Create conjunctive goal P(a) ∧ Q(b)
    auto conj_goal = make_and(p_a, q_b);
    
    // Create initial state with conjunctive goal
    auto initial_state = context.create_initial_state(conj_goal);
    
    std::cout << "1. Created initial state with goal P(a) ∧ Q(b)" << std::endl;
    
    // Decompose the goal
    auto subgoal_states = goal_manager.decompose_goal(context, initial_state);
    
    // Check that decomposition produced exactly 2 states
    assert(subgoal_states.size() == 2);
    std::cout << "2. Successfully decomposed goal into " << subgoal_states.size() << " subgoals" << std::endl;
    
    // Check the subgoals
    bool has_p_a = false;
    bool has_q_b = false;
    
    for (const auto& state : subgoal_states) {
        auto goal = state->goal();
        if (*goal == *p_a) {
            has_p_a = true;
            std::cout << "   Found subgoal P(a)" << std::endl;
        } else if (*goal == *q_b) {
            has_q_b = true;
            std::cout << "   Found subgoal Q(b)" << std::endl;
        }
    }
    
    assert(has_p_a && has_q_b);
    
    // Now prove the individual subgoals
    for (const auto& state : subgoal_states) {
        auto goal = state->goal();
        
        // Add the goal as a hypothesis to simulate proving it
        auto proven_state = context.apply_rule(
            state,
            "assume_goal", 
            {},                    // No premise names
            {Hypothesis("proven", goal)},  // Add goal as hypothesis
            goal                   // Keep the same goal
        );
        
        // Mark as proven
        proven_state->mark_as_proved(
            ProofCertification::Status::PROVED_BY_RULE,
            "Direct proof by assumption"
        );
        
        // Register the proven subgoal
        goal_manager.register_proven_subgoal(goal, proven_state);
        
        std::cout << "3. Proved and registered subgoal" << std::endl;
    }
    
    // Try recombining the subgoals
    auto recombined = goal_manager.try_recombine(context, initial_state, conj_goal);
    
    // Check that recombination succeeded
    assert(is_proven(recombined));
    std::cout << "4. Successfully recombined subgoals to prove the original goal" << std::endl;
    
    // Change the order of subgoals and ensure recombination still works
    goal_manager.clear();
    std::cout << "\n5. Cleared goal manager state. Testing with different subgoal order..." << std::endl;
    
    // Register in reverse order
    for (auto it = subgoal_states.rbegin(); it != subgoal_states.rend(); ++it) {
        auto state = *it;
        auto goal = state->goal();
        
        // Add the goal as a hypothesis to simulate proving it
        auto proven_state = context.apply_rule(
            state,
            "assume_goal", 
            {},                    // No premise names
            {Hypothesis("proven", goal)},  // Add goal as hypothesis
            goal                   // Keep the same goal
        );
        
        // Mark as proven
        proven_state->mark_as_proved(
            ProofCertification::Status::PROVED_BY_RULE,
            "Direct proof by assumption"
        );
        
        // Register the proven subgoal
        goal_manager.register_proven_subgoal(goal, proven_state);
        
        std::cout << "6. Proved and registered subgoal in reverse order" << std::endl;
    }
    
    // Try recombining the subgoals again
    auto recombined2 = goal_manager.try_recombine(context, initial_state, conj_goal);
    
    // Check that recombination succeeded
    assert(is_proven(recombined2));
    std::cout << "7. Successfully recombined subgoals (reverse registration order)" << std::endl;
    
    std::cout << "\nAll tests passed!" << std::endl;
}

// Test a more complex example with nested conjunctions
void test_nested_conjunctive_goal() {
    std::cout << "\n=== Testing nested conjunctive goal decomposition and recombination ===\n" << std::endl;
    
    // Create a proof context and goal manager
    ProofContext context;
    GoalManager goal_manager;
    
    // Create constants
    auto a = make_constant("a");
    auto b = make_constant("b");
    auto c = make_constant("c");
    
    // Create predicates P(a), Q(b), and R(c)
    auto p_a = make_function_application("P", {a});
    auto q_b = make_function_application("Q", {b});
    auto r_c = make_function_application("R", {c});
    
    // Create nested conjunctive goal (P(a) ∧ Q(b)) ∧ R(c)
    auto inner_conj = make_and(p_a, q_b);
    auto outer_conj = make_and(inner_conj, r_c);
    
    // Create initial state with nested conjunctive goal
    auto initial_state = context.create_initial_state(outer_conj);
    
    std::cout << "1. Created initial state with goal (P(a) ∧ Q(b)) ∧ R(c)" << std::endl;
    
    // Decompose the goal
    auto subgoal_states = goal_manager.decompose_goal(context, initial_state);
    
    // Check that decomposition produced exactly 2 states
    assert(subgoal_states.size() == 2);
    std::cout << "2. Successfully decomposed goal into " << subgoal_states.size() << " subgoals" << std::endl;
    
    // Find the inner conjunctive subgoal
    ProofStatePtr inner_conj_state = nullptr;
    ProofStatePtr r_c_state = nullptr;
    
    for (const auto& state : subgoal_states) {
        auto goal = state->goal();
        if (*goal == *inner_conj) {
            inner_conj_state = state;
            std::cout << "   Found subgoal (P(a) ∧ Q(b))" << std::endl;
        } else if (*goal == *r_c) {
            r_c_state = state;
            std::cout << "   Found subgoal R(c)" << std::endl;
        }
    }
    
    assert(inner_conj_state && r_c_state);
    
    // Decompose the inner conjunction
    auto inner_subgoal_states = goal_manager.decompose_goal(context, inner_conj_state);
    
    // Check that inner decomposition produced exactly 2 states
    assert(inner_subgoal_states.size() == 2);
    std::cout << "3. Successfully decomposed inner goal into " << inner_subgoal_states.size() << " subgoals" << std::endl;
    
    // Prove all the leaf subgoals
    std::vector<ProofStatePtr> all_leaf_states;
    all_leaf_states.insert(all_leaf_states.end(), inner_subgoal_states.begin(), inner_subgoal_states.end());
    all_leaf_states.push_back(r_c_state);
    
    for (const auto& state : all_leaf_states) {
        auto goal = state->goal();
        
        // Add the goal as a hypothesis to simulate proving it
        auto proven_state = context.apply_rule(
            state,
            "assume_goal", 
            {},                    // No premise names
            {Hypothesis("proven", goal)},  // Add goal as hypothesis
            goal                   // Keep the same goal
        );
        
        // Mark as proven
        proven_state->mark_as_proved(
            ProofCertification::Status::PROVED_BY_RULE,
            "Direct proof by assumption"
        );
        
        // Register the proven subgoal
        goal_manager.register_proven_subgoal(goal, proven_state);
        
        std::cout << "4. Proved and registered leaf subgoal" << std::endl;
    }
    
    // First try recombining the inner conjunction
    auto recombined_inner = goal_manager.try_recombine(context, inner_conj_state, inner_conj);
    
    // Check that inner recombination succeeded
    assert(is_proven(recombined_inner));
    std::cout << "5. Successfully recombined inner subgoals to prove (P(a) ∧ Q(b))" << std::endl;
    
    // Now try recombining the outer conjunction
    auto recombined_outer = goal_manager.try_recombine(context, initial_state, outer_conj);
    
    // Check that outer recombination succeeded
    assert(is_proven(recombined_outer));
    std::cout << "6. Successfully recombined all subgoals to prove the original goal" << std::endl;
    
    std::cout << "\nAll nested conjunction tests passed!" << std::endl;
}

int main() {
    std::cout << "===== Running Goal Manager Tests =====\n" << std::endl;
    
    test_conjunctive_goal();
    test_nested_conjunctive_goal();
    
    std::cout << "\n===== Goal Manager Tests Complete =====\n" << std::endl;
    return 0;
}