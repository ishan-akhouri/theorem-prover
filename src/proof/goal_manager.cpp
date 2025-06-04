#include "goal_manager.hpp"
#include <iostream>
#include <sstream>

namespace theorem_prover
{

    GoalManager::GoalManager() {}

    std::vector<ProofStatePtr> GoalManager::decompose_goal(
        ProofContext &context,
        const ProofStatePtr &state)
    {

        auto goal = state->goal();

        // Handle different goal types
        if (goal->kind() == TermDB::TermKind::AND)
        {
            return decompose_conjunctive_goal(context, state);
        }

        // If no decomposition is possible, return the original state
        return {state};
    }

    void GoalManager::register_proven_subgoal(
        const TermDBPtr &goal,
        const ProofStatePtr &state)
    {

        std::string goal_key = term_key(goal);
        proven_subgoals_[goal_key] = state;
    }

    ProofStatePtr GoalManager::try_recombine(
        ProofContext &context,
        const ProofStatePtr &parent_state,
        const TermDBPtr &parent_goal)
    {

        // Get the goal type
        auto goal_kind = parent_goal->kind();

        // Handle different goal types
        if (goal_kind == TermDB::TermKind::AND)
        {

            return recombine_conjunctive_goal(context, parent_state, parent_goal);
        }

        // If no recombination is possible, return nullptr

        return nullptr;
    }

    bool GoalManager::all_subgoals_proven(const TermDBPtr &goal) const
    {
        std::string goal_key = term_key(goal);

        // Check if this goal is directly proven
        if (proven_subgoals_.find(goal_key) != proven_subgoals_.end())
        {

            return true;
        }

        // Check if it's decomposed and all subgoals are proven
        auto decomp_it = goal_decompositions_.find(goal_key);
        if (decomp_it == goal_decompositions_.end())
        {

            return false; // Not decomposed
        }

        const auto &subgoal_keys = decomp_it->second;

        for (const auto &subgoal_key : subgoal_keys)
        {
            if (proven_subgoals_.find(subgoal_key) == proven_subgoals_.end())
            {

                return false; // At least one subgoal not proven
            }
        }

        return true; // All subgoals proven
    }

    void GoalManager::clear()
    {
        proven_subgoals_.clear();
        goal_decompositions_.clear();
    }

    // Generate a canonical string representation of a term
    // Handles alpha-equivalence of terms
    std::string GoalManager::term_key(const TermDBPtr &term) const
    {

        std::ostringstream oss;

        // Different handling based on term kind
        switch (term->kind())
        {
        case TermDB::TermKind::CONSTANT:
        {
            auto constant = std::dynamic_pointer_cast<ConstantDB>(term);
            oss << "CONST:" << constant->symbol();
            break;
        }
        case TermDB::TermKind::VARIABLE:
        {
            auto variable = std::dynamic_pointer_cast<VariableDB>(term);
            oss << "VAR:" << variable->index();
            break;
        }
        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            oss << "FUNC:" << func_app->symbol() << "(";
            const auto &args = func_app->arguments();
            for (size_t i = 0; i < args.size(); ++i)
            {
                if (i > 0)
                    oss << ",";
                oss << term_key(args[i]);
            }
            oss << ")";
            break;
        }
        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(term);
            oss << "AND:(" << term_key(and_term->left()) << ";" << term_key(and_term->right()) << ")";
            break;
        }
        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(term);
            oss << "OR:(" << term_key(or_term->left()) << ";" << term_key(or_term->right()) << ")";
            break;
        }
        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(term);
            oss << "NOT:(" << term_key(not_term->body()) << ")";
            break;
        }
        case TermDB::TermKind::IMPLIES:
        {
            auto implies = std::dynamic_pointer_cast<ImpliesDB>(term);
            oss << "IMPLIES:(" << term_key(implies->antecedent()) << "→" << term_key(implies->consequent()) << ")";
            break;
        }
        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(term);
            oss << "FORALL:(" << forall->variable_hint() << "." << term_key(forall->body()) << ")";
            break;
        }
        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(term);
            oss << "EXISTS:(" << exists->variable_hint() << "." << term_key(exists->body()) << ")";
            break;
        }
        default:
            oss << "UNKNOWN:" << term->hash();
        }

        return oss.str();
    }

    // Private helper functions

    std::vector<ProofStatePtr> GoalManager::decompose_conjunctive_goal(
        ProofContext &context,
        const ProofStatePtr &state)
    {

        auto goal = state->goal();

        // Ensure goal is a conjunction
        if (goal->kind() != TermDB::TermKind::AND)
        {
            return {state};
        }

        // Get the conjunct structure
        auto conj_structure = get_conjunct_structure(goal);
        auto left_goal = conj_structure.left;
        auto right_goal = conj_structure.right;

        std::string goal_key = term_key(goal);
        if (goal_decompositions_.find(goal_key) != goal_decompositions_.end())
        {

            // The goal is already decomposed, but we still need to create states for the subgoals
            std::vector<ProofStatePtr> result_states;

            // Create a state for the left subgoal
            auto left_state = context.apply_rule(
                state,
                "and_left_goal",
                {},       // No premise names
                {},       // No new hypotheses
                left_goal // New goal is the left component
            );
            result_states.push_back(left_state);

            // Create a state for the right subgoal
            auto right_state = context.apply_rule(
                state,
                "and_right_goal",
                {},        // No premise names
                {},        // No new hypotheses
                right_goal // New goal is the right component
            );
            result_states.push_back(right_state);

            return result_states;
        }

        // Register the decomposition with order preservation
        register_goal_decomposition(goal, {left_goal, right_goal}, true);

        // Create states for each subgoal
        std::vector<ProofStatePtr> result_states;

        // Create a state for the left subgoal
        auto left_state = context.apply_rule(
            state,
            "and_left_goal",
            {},       // No premise names
            {},       // No new hypotheses
            left_goal // New goal is the left component
        );
        result_states.push_back(left_state);

        // Create a state for the right subgoal
        auto right_state = context.apply_rule(
            state,
            "and_right_goal",
            {},        // No premise names
            {},        // No new hypotheses
            right_goal // New goal is the right component
        );
        result_states.push_back(right_state);

        return result_states;
    }

    ProofStatePtr GoalManager::recombine_conjunctive_goal(
        ProofContext &context,
        const ProofStatePtr &parent_state,
        const TermDBPtr &parent_goal)
    {

        // Ensure parent goal is a conjunction
        if (parent_goal->kind() != TermDB::TermKind::AND)
        {
            return nullptr;
        }

        // Get the conjunct structure to preserve left-right identity
        auto conj_structure = get_conjunct_structure(parent_goal);
        auto left_goal = conj_structure.left;
        auto right_goal = conj_structure.right;

        // Get the key values
        std::string left_key = term_key(left_goal);
        std::string right_key = term_key(right_goal);

        // Check if both subgoals are proven
        auto left_it = proven_subgoals_.find(left_key);
        auto right_it = proven_subgoals_.find(right_key);

        if (left_it == proven_subgoals_.end() || right_it == proven_subgoals_.end())
        {
            return nullptr; // At least one subgoal not proven
        }

        const auto &left_state = left_it->second;
        const auto &right_state = right_it->second;

        // Find the hypotheses that prove the subgoals
        std::string left_hyp_name;
        std::string right_hyp_name;

        // Find hypothesis proving left subgoal
        for (const auto &hyp : left_state->hypotheses())
        {
            if (*hyp.formula() == *left_goal)
            {
                left_hyp_name = hyp.name();

                break;
            }
        }

        // Find hypothesis proving right subgoal
        for (const auto &hyp : right_state->hypotheses())
        {
            if (*hyp.formula() == *right_goal)
            {
                right_hyp_name = hyp.name();

                break;
            }
        }

        // If no hypotheses found, return nullptr
        if (left_hyp_name.empty() || right_hyp_name.empty())
        {

            return nullptr;
        }

        // Merge all hypotheses from both states
        std::vector<Hypothesis> merged_hypotheses;

        // Add hypotheses from parent state
        for (const auto &hyp : parent_state->hypotheses())
        {
            merged_hypotheses.push_back(hyp);
        }

        // Add hypotheses from left state (avoiding duplicates)
        for (const auto &hyp : left_state->hypotheses())
        {
            bool already_present = false;
            for (const auto &existing : merged_hypotheses)
            {
                if (*existing.formula() == *hyp.formula())
                {
                    already_present = true;
                    break;
                }
            }
            if (!already_present)
            {
                merged_hypotheses.push_back(hyp);
            }
        }

        // Add hypotheses from right state (avoiding duplicates)
        for (const auto &hyp : right_state->hypotheses())
        {
            bool already_present = false;
            for (const auto &existing : merged_hypotheses)
            {
                if (*existing.formula() == *hyp.formula())
                {
                    already_present = true;
                    break;
                }
            }
            if (!already_present)
            {
                merged_hypotheses.push_back(hyp);
            }
        }

        // Create a state with all the merged hypotheses
        auto merged_state = context.apply_rule(
            parent_state,
            "merge_states",
            {},                // No premise names
            merged_hypotheses, // All merged hypotheses
            parent_goal        // Original goal
        );

        // Find the left and right hypotheses in the merged state
        std::string merged_left_hyp;
        std::string merged_right_hyp;

        for (const auto &hyp : merged_state->hypotheses())
        {
            if (*hyp.formula() == *left_goal)
            {
                merged_left_hyp = hyp.name();
            }
            else if (*hyp.formula() == *right_goal)
            {
                merged_right_hyp = hyp.name();
            }
        }
        // If no merged hypotheses found, return nullptr
        if (merged_left_hyp.empty() || merged_right_hyp.empty())
        {

            return nullptr;
        }

        // Apply AND-introduction to form the conjunction
        auto and_intro_rule = std::make_shared<AndIntroRule>(
            merged_left_hyp, merged_right_hyp);

        std::optional<ConstraintViolation> and_violation;
        auto and_results = and_intro_rule->apply(context, merged_state, and_violation);

        // Check if AND-introduction was successfu
        if (and_results.empty())
        {
            if (and_violation)
            {
                std::cout << "ERROR: AndIntroRule failed: " << and_violation->message() << std::endl;
            }
            else
            {
                std::cout << "ERROR: AndIntroRule failed with no violation" << std::endl;
            }
            return nullptr;
        }

        // Get the final state
        auto final_state = and_results[0];

        // Create a hypothesis name that we can predict
        std::string conj_hyp_name = "and_intro_result";

        // Create a new state with an explicit hypothesis for the conjunction
        auto named_state = context.apply_rule(
            final_state,
            "name_conjunction",
            {},                                       // No premise names
            {Hypothesis(conj_hyp_name, parent_goal)}, // Add the conjunction with known name
            parent_goal                               // Same goal
        );

        // Create a state that marks the proof as complete
        auto completed_state = context.apply_rule(
            named_state,
            "direct_proof",
            {conj_hyp_name}, // Using the conjunction hypothesis as premise
            {},              // No new hypotheses
            parent_goal      // Original goal
        );

        // Mark as proven
        completed_state->mark_as_proved(
            ProofCertification::Status::PROVED_BY_RULE,
            "Proof completed by recombining subgoals");

        // Register this proven goal
        register_proven_subgoal(parent_goal, completed_state);

        return completed_state;
    }

    std::vector<std::string> GoalManager::get_goal_decomposition(const TermDBPtr &goal)
    {
        std::string goal_key = term_key(goal);
        auto it = goal_decompositions_.find(goal_key);

        if (it != goal_decompositions_.end())
        {
            return it->second;
        }

        // If no decomposition is registered, create one based on the goal structure
        if (goal->kind() == TermDB::TermKind::AND)
        {
            auto conj_structure = get_conjunct_structure(goal);
            auto left_goal = conj_structure.left;
            auto right_goal = conj_structure.right;

            register_goal_decomposition(goal, {left_goal, right_goal}, true);
            return goal_decompositions_[goal_key];
        }

        // If no decomposition is possible, return empty vector
        return {};
    }

    void GoalManager::register_goal_decomposition(
        const TermDBPtr &parent_goal,
        const std::vector<TermDBPtr> &subgoals,
        bool preserve_order)
    {

        std::string parent_key = term_key(parent_goal);

        // Skip if the goal has already been decomposed
        if (goal_decompositions_.find(parent_key) != goal_decompositions_.end())
        {
            return;
        }

        std::vector<std::string> subgoal_keys;

        for (const auto &subgoal : subgoals)
        {
            subgoal_keys.push_back(term_key(subgoal));
        }

        goal_decompositions_[parent_key] = subgoal_keys;
    }

    GoalManager::ConjunctStructure GoalManager::get_conjunct_structure(const TermDBPtr &conj_goal) const
    {
        // Preserve left-right ordering
        if (conj_goal->kind() == TermDB::TermKind::AND)
        {
            auto and_goal = std::dynamic_pointer_cast<AndDB>(conj_goal);
            return {and_goal->left(), and_goal->right()};
        }

        // Default to empty structure if not a conjunction
        return {nullptr, nullptr};
    }

    // Tactics that use the GoalManager

    Tactic decompose_goals(GoalManager &goal_manager)
    {
        return [&goal_manager](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            if (state->is_proved())
            {
                return {state}; // Already proven, no need to decompose
            }

            return goal_manager.decompose_goal(context, state);
        };
    }

    Tactic recombine_subgoals(GoalManager &goal_manager)
    {
        return [&goal_manager](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            if (state->is_proved())
            {
                return {state}; // Already proven, no need to recombine
            }

            auto result = goal_manager.try_recombine(context, state, state->goal());
            if (result)
            {
                return {result};
            }

            // If recombination failed, return the original state
            return {state};
        };
    }

    Tactic register_proven_subgoal(GoalManager &goal_manager)
    {
        return [&goal_manager](
                   ProofContext &context,
                   const ProofStatePtr &state,
                   std::optional<ConstraintViolation> &violation) -> std::vector<ProofStatePtr>
        {
            // First check if the state is already explicitly marked as proven
            if (state->is_proved())
            {

                goal_manager.register_proven_subgoal(state->goal(), state);
                return {state};
            }

            // Next check if any hypothesis directly matches the goal
            for (const auto &hyp : state->hypotheses())
            {
                bool match = *hyp.formula() == *state->goal();

                if (match)
                {

                    // Create an explicitly proven state
                    auto new_state = context.apply_rule(
                        state,
                        "direct_proof",
                        {hyp.name()},
                        {},
                        state->goal());

                    new_state->mark_as_proved(
                        ProofCertification::Status::PROVED_BY_RULE,
                        "Direct proof from hypothesis " + hyp.name());

                    goal_manager.register_proven_subgoal(state->goal(), new_state);

                    return {new_state};
                }
            }

            // State is not proven yet
            return {state};
        };
    }

    Tactic goal_oriented_search(GoalManager &goal_manager, const Tactic &inner_tactic)
    {
        // Optimized tactic sequencing
        return sequence({

            register_proven_subgoal(goal_manager),

            try_tactic(inner_tactic),

            try_tactic(direct_proof()),

            recombine_subgoals(goal_manager),

            decompose_goals(goal_manager)});
    }

} // namespace theorem_prover