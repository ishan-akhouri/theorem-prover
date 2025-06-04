#include "proof_state.hpp"
#include "../utils/hash.hpp"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iostream>

namespace theorem_prover
{

    // Hypothesis implementation
    Hypothesis::Hypothesis(const std::string &name, const TermDBPtr &formula)
        : name_(name), formula_(formula)
    {
        if (formula == nullptr)
        {
            throw std::invalid_argument("Hypothesis formula cannot be null");
        }
    }

    bool Hypothesis::equals(const Hypothesis &other) const
    {
        return name_ == other.name_ && *formula_ == *other.formula_;
    }

    std::size_t Hypothesis::hash() const
    {
        std::size_t h = std::hash<std::string>()(name_);
        hash_combine(h, formula_->hash());
        return h;
    }

    // ProofStep implementation
    ProofStep::ProofStep(const std::string &rule_name,
                         const std::vector<std::string> &premise_names,
                         const TermDBPtr &conclusion)
        : rule_name_(rule_name), premise_names_(premise_names), conclusion_(conclusion)
    {
        if (conclusion == nullptr)
        {
            throw std::invalid_argument("ProofStep conclusion cannot be null");
        }
    }

    // Factory method to create an initial proof state
    ProofStatePtr ProofState::create_initial(const TermDBPtr &goal)
    {
        if (goal == nullptr)
        {
            throw std::invalid_argument("Initial goal cannot be null");
        }

        return std::shared_ptr<ProofState>(new ProofState(
            nullptr,                   // No parent
            std::vector<Hypothesis>(), // No hypotheses
            goal,                      // Initial goal
            nullptr                    // No last step
            ));
    }

    // Factory method to create a derived proof state
    ProofStatePtr ProofState::create_from(
        const ProofStatePtr &parent,
        const std::string &rule_name,
        const std::vector<std::string> &premise_names,
        const std::vector<Hypothesis> &new_hypotheses,
        const TermDBPtr &new_goal)
    {

        if (parent == nullptr)
        {
            throw std::invalid_argument("Parent state cannot be null");
        }

        if (new_goal == nullptr)
        {
            throw std::invalid_argument("New goal cannot be null");
        }

        // Combine parent hypotheses with new ones
        std::vector<Hypothesis> all_hypotheses = parent->hypotheses_;
        all_hypotheses.insert(all_hypotheses.end(), new_hypotheses.begin(), new_hypotheses.end());

        // Create a proof step to record this transition
        auto step = std::make_unique<ProofStep>(
            rule_name,
            premise_names,
            new_goal);

        auto new_state = std::shared_ptr<ProofState>(new ProofState(
            parent,
            all_hypotheses,
            new_goal,
            std::move(step)));

        // Transfer metavariables from parent to new state
        for (const auto &[name, info] : parent->metavariables_)
        {
            if (info.instantiated)
            {
                new_state->add_metavariable(name, info.type);
                new_state->instantiate_metavariable(name, info.instantiation);
            }
            else
            {
                new_state->add_metavariable(name, info.type);
            }
        }

        return new_state;
    }

    // Private constructor
    ProofState::ProofState(
        const ProofStatePtr &parent,
        const std::vector<Hypothesis> &hypotheses,
        const TermDBPtr &goal,
        std::unique_ptr<ProofStep> last_step)
        : parent_(parent),
          hypotheses_(hypotheses),
          goal_(goal),
          last_step_(std::move(last_step)),
          depth_(parent ? parent->depth_ + 1 : 0)
    {

        calculate_hash();
    }

    // Calculate a hash value for this state
    void ProofState::calculate_hash()
    {
        hash_ = goal_->hash();

        // Incorporate hypotheses into the hash
        for (const auto &hyp : hypotheses_)
        {
            hash_combine(hash_, hyp.hash());
        }
    }

    // Mark a state as proved with a justification
    void ProofState::mark_as_proved(
        ProofCertification::Status status,
        const std::string &justification)
    {

        certification_.status = status;
        certification_.justification = justification;
    }

    // Find a hypothesis by name
    const Hypothesis *ProofState::find_hypothesis(const std::string &name) const
    {
        for (const auto &hyp : hypotheses_)
        {
            if (hyp.name() == name)
            {
                return &hyp;
            }
        }
        return nullptr;
    }

    // Check if this state proves the goal
    bool ProofState::is_proved() const
    {

        // Check if explicitly marked as proved
        if (certification_.is_proved())
        {
            return true;
        }

        // If we have uninstantiated metavariables, the state isn't fully proved yet
        if (has_uninstantiated_metavariables())
        {
            return false;
        }

        // Basic check: any hypothesis exactly matches the goal
        for (const auto &hyp : hypotheses_)
        {
            bool match = *hyp.formula() == *goal_;

            if (match)
            {
                return true;
            }
        }

        // Additional proof checks can be added here
        return false;
    }

    // Metavariable management
    void ProofState::add_metavariable(const std::string &name, const TypePtr &type)
    {
        if (metavariables_.find(name) != metavariables_.end())
        {
            throw std::invalid_argument("Metavariable already exists: " + name);
        }

        MetavariableInfo info;
        info.name = name;
        info.type = type;
        info.instantiated = false;

        metavariables_[name] = info;
    }

    bool ProofState::instantiate_metavariable(const std::string &name, const TermDBPtr &term)
    {
        auto it = metavariables_.find(name);
        if (it == metavariables_.end())
        {
            return false;
        }

        // TODO: Add type checking for the term

        it->second.instantiated = true;
        it->second.instantiation = term;
        return true;
    }

    bool ProofState::has_uninstantiated_metavariables() const
    {
        for (const auto &[name, info] : metavariables_)
        {
            if (!info.instantiated)
            {
                return true;
            }
        }
        return false;
    }

    const MetavariableInfo *ProofState::find_metavariable(const std::string &name) const
    {
        auto it = metavariables_.find(name);
        if (it != metavariables_.end())
        {
            return &it->second;
        }
        return nullptr;
    }
    // Check if two states are equal (extensionally)
    bool ProofState::equals(const ProofState &other) const
    {
        // Quick check using hash values
        if (hash_ != other.hash_)
        {
            return false;
        }

        // Check goals
        if (!(*goal_ == *other.goal_))
        {
            return false;
        }

        // Check hypotheses (ignoring order)
        if (hypotheses_.size() != other.hypotheses_.size())
        {
            return false;
        }

        // Create a copy to mark hypotheses as matched
        std::vector<bool> matched(other.hypotheses_.size(), false);

        for (const auto &hyp1 : hypotheses_)
        {
            bool found = false;
            for (size_t i = 0; i < other.hypotheses_.size(); ++i)
            {
                if (!matched[i] && hyp1.equals(other.hypotheses_[i]))
                {
                    matched[i] = true;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                return false;
            }
        }

        // Check metavariables
        if (metavariables_.size() != other.metavariables_.size())
        {
            return false;
        }

        for (const auto &[name, info] : metavariables_)
        {
            auto other_it = other.metavariables_.find(name);
            if (other_it == other.metavariables_.end())
            {
                return false;
            }

            const auto &other_info = other_it->second;
            // Check that types are the same
            if (!(*info.type == *other_info.type))
            {
                return false;
            }

            if (info.instantiated != other_info.instantiated)
            {
                return false;
            }

            if (info.instantiated && !(*info.instantiation == *other_info.instantiation))
            {
                return false;
            }
        }

        return true;
    }

    // Generate a proof trace from initial state to this state
    std::vector<const ProofStep *> ProofState::get_proof_trace() const
    {
        std::vector<const ProofStep *> trace;

        // Collect steps by traversing parents
        const ProofState *current = this;
        while (current != nullptr && current->last_step_ != nullptr)
        {
            trace.push_back(current->last_step_.get());
            current = current->parent_.get();
        }

        // Reverse to get chronological order
        std::reverse(trace.begin(), trace.end());
        return trace;
    }

    // ProofContext implementation
    ProofContext::ProofContext() {}

    // Create a new initial proof state
    ProofStatePtr ProofContext::create_initial_state(const TermDBPtr &goal)
    {
        auto state = ProofState::create_initial(goal);
        add_state(state);
        return state;
    }

    // Apply a rule to a state and get the resulting state
    ProofStatePtr ProofContext::apply_rule(
        const ProofStatePtr &state,
        const std::string &rule_name,
        const std::vector<std::string> &premise_names,
        const std::vector<Hypothesis> &new_hypotheses,
        const TermDBPtr &new_goal)
    {

        // Validate the rule application for soundness
        if (!validate_rule_application(rule_name, premise_names, new_goal))
        {
            throw std::runtime_error("Invalid rule application: " + rule_name);
        }

        // Create the new state
        auto new_state = ProofState::create_from(
            state,
            rule_name,
            premise_names,
            new_hypotheses,
            new_goal);

        // Check if this state already exists
        auto existing = find_state(new_state->hash());
        if (existing && existing->equals(*new_state))
        {
            return existing;
        }

        // Add the new state to the context
        add_state(new_state);
        return new_state;
    }

    // Validate a rule application (for soundness checking)
    bool ProofContext::validate_rule_application(
        const std::string &rule_name,
        const std::vector<std::string> &premise_names,
        const TermDBPtr &conclusion)
    {

        // TODO: Implement rule validation

        return true; // Always valid for now
    }

    // Add a state to the context
    void ProofContext::add_state(const ProofStatePtr &state)
    {
        states_.insert(state);
        state_map_[state->hash()] = state;
    }

    // Find a state by its hash value
    ProofStatePtr ProofContext::find_state(std::size_t hash_value) const
    {
        auto it = state_map_.find(hash_value);
        if (it != state_map_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    // Get all leaf states in the proof tree
    std::vector<ProofStatePtr> ProofContext::get_leaf_states() const
    {
        std::vector<ProofStatePtr> leaves;

        // Create a set of non-leaf states
        std::unordered_set<const ProofState *> non_leaves;
        for (const auto &state : states_)
        {
            if (state->parent())
            {
                non_leaves.insert(state->parent().get());
            }
        }

        // Collect states that are not parents of any other state
        for (const auto &state : states_)
        {
            if (non_leaves.find(state.get()) == non_leaves.end())
            {
                leaves.push_back(state);
            }
        }

        return leaves;
    }

    // Get all proved states
    std::vector<ProofStatePtr> ProofContext::get_proved_states() const
    {
        std::vector<ProofStatePtr> proved;

        for (const auto &state : states_)
        {
            if (state->is_proved())
            {
                proved.push_back(state);
            }
        }

        return proved;
    }

    // Comparison functions for STL containers
    bool operator==(const Hypothesis &lhs, const Hypothesis &rhs)
    {
        return lhs.equals(rhs);
    }

    bool operator==(const ProofState &lhs, const ProofState &rhs)
    {
        return lhs.equals(rhs);
    }

} // namespace theorem_prover

/**
 * Type checking and rule validation are not implemented in this iteration of the prover.
 * They will be implemented when we begin to deal with more difficult problems.
 */