// src/proof/goal_manager.hpp
#pragma once

#include "proof_state.hpp"
#include "tactic.hpp"
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace theorem_prover {

/**
 * Manages goal decomposition and recombination for complex proof goals
 * 
 * Tracks subgoals that have been proven and can recombine them to 
 * prove the parent goal. This supports structured incremental proofs
 * by allowing divide-and-conquer reasoning strategies.
 */
class GoalManager {
public:
    GoalManager();
    
    /**
     * Decomposes a goal into simpler subgoals
     * 
     * Currently handles:
     * - Conjunctive goals (A ∧ B) -> A, B
     * 
     * @param context The proof context
     * @param state The current proof state
     * @return A vector of new states, each with a simpler subgoal
     */
    std::vector<ProofStatePtr> decompose_goal(
        ProofContext& context, 
        const ProofStatePtr& state);
    
    /**
     * Registers a proven subgoal
     * 
     * @param goal The goal that was proven
     * @param state The proof state containing the proof
     */
    void register_proven_subgoal(
        const TermDBPtr& goal, 
        const ProofStatePtr& state);
        
    /**
     * Attempts to recombine proven subgoals to prove the parent goal
     * 
     * @param context The proof context
     * @param parent_state The original state with the parent goal
     * @param parent_goal The parent goal to prove
     * @return A new state with the parent goal proven, or nullptr if not possible
     */
    ProofStatePtr try_recombine(
        ProofContext& context,
        const ProofStatePtr& parent_state,
        const TermDBPtr& parent_goal);
        
    /**
     * Checks if all subgoals for a goal have been proven
     * 
     * @param goal The parent goal
     * @return True if all subgoals are proven, false otherwise
     */
    bool all_subgoals_proven(const TermDBPtr& goal) const;
    
    /**
     * Clears all proven subgoals and goal decompositions
     */
    void clear();
    
private:
    // Maps from term string representation to the state that proves it
    std::unordered_map<std::string, ProofStatePtr> proven_subgoals_;
    
    // Maps from parent goal string to a vector of subgoal strings
    // preserving the left-right structure for conjunctions
    std::unordered_map<std::string, std::vector<std::string>> goal_decompositions_;
    
    // Generate a canonical string representation of a term
    // Handles alpha-equivalence of terms
    std::string term_key(const TermDBPtr& term) const;
    
    // Helper functions
    
    /**
     * Decomposes a conjunctive goal (A ∧ B) into A and B
     */
    std::vector<ProofStatePtr> decompose_conjunctive_goal(
        ProofContext& context,
        const ProofStatePtr& state);
        
    /**
     * Attempts to recombine proven subgoals for a conjunctive goal
     * Preserves left-right identity during recombination
     */
    ProofStatePtr recombine_conjunctive_goal(
        ProofContext& context,
        const ProofStatePtr& parent_state,
        const TermDBPtr& parent_goal);
        
    /**
     * Gets the decomposition of a goal
     * 
     * @param goal The goal to decompose
     * @return A vector of subgoal string keys
     */
    std::vector<std::string> get_goal_decomposition(const TermDBPtr& goal);
    
    /**
     * Registers a new goal decomposition
     * 
     * @param parent_goal The parent goal
     * @param subgoals The subgoals it decomposes into
     * @param preserve_order Whether to preserve the order of subgoals
     */
    void register_goal_decomposition(
        const TermDBPtr& parent_goal,
        const std::vector<TermDBPtr>& subgoals,
        bool preserve_order = true);
    
    /**
     * To preserve ordering information when registering 
     * and recombining conjunctive goals
     */
    struct ConjunctStructure {
        TermDBPtr left;
        TermDBPtr right;
    };
    
    ConjunctStructure get_conjunct_structure(const TermDBPtr& conj_goal) const;
};

/**
 * Creates a tactic that decomposes goals using the GoalManager
 * 
 * @param goal_manager The goal manager to use
 * @return Tactic A tactic that decomposes goals
 */
Tactic decompose_goals(GoalManager& goal_manager);

/**
 * Creates a tactic that attempts to recombine proven subgoals
 * 
 * @param goal_manager The goal manager to use
 * @return Tactic A tactic that recombines subgoals
 */
Tactic recombine_subgoals(GoalManager& goal_manager);

/**
 * Creates a tactic that registers proven subgoals with the goal manager
 * 
 * @param goal_manager The goal manager to use
 * @return Tactic A tactic that registers proven subgoals
 */
Tactic register_proven_subgoal(GoalManager& goal_manager);

/**
 * Creates a tactic for goal-oriented proof search
 * @param goal_manager The goal manager to use
 * @param inner_tactic The tactic to apply to each subgoal
 * @return Tactic A goal-oriented proof search tactic
 */
Tactic goal_oriented_search(GoalManager& goal_manager, const Tactic& inner_tactic);

} // namespace theorem_prover