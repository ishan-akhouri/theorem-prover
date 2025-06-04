#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include "../term/term_db.hpp"
#include "../type/type.hpp"

namespace theorem_prover
{

    // Forward declarations
    class ProofState;
    using ProofStatePtr = std::shared_ptr<ProofState>;

    /**
     * Represents a named assumption or fact in a proof
     */
    class Hypothesis
    {
    public:
        Hypothesis(const std::string &name, const TermDBPtr &formula);

        const std::string &name() const { return name_; }
        const TermDBPtr &formula() const { return formula_; }

        bool equals(const Hypothesis &other) const;
        std::size_t hash() const;

    private:
        std::string name_;
        TermDBPtr formula_;
    };

    /**
     * Records a step in the proof history
     */
    class ProofStep
    {
    public:
        ProofStep(const std::string &rule_name,
                  const std::vector<std::string> &premise_names,
                  const TermDBPtr &conclusion);

        const std::string &rule_name() const { return rule_name_; }
        const std::vector<std::string> &premise_names() const { return premise_names_; }
        const TermDBPtr &conclusion() const { return conclusion_; }

    private:
        std::string rule_name_;                  // Name of the rule applied
        std::vector<std::string> premise_names_; // Names of hypotheses used
        TermDBPtr conclusion_;                   // Resulting formula
    };

    /**
     * Records the proof status and justification
     */
    struct ProofCertification
    {
        enum class Status
        {
            UNPROVEN,             // Default state
            PROVED_BY_RULE,       // Explicitly proved by a rule application
            TAUTOLOGY,            // Proven because it's a tautology
            CONTRADICTION,        // Proven because the hypotheses contain a contradiction
            PENDING_INSTANTIATION // Proved modulo instantiation of metavariables
        };

        Status status = Status::UNPROVEN;
        std::string justification = "";

        // Helper methods
        bool is_proved() const
        {
            return status != Status::UNPROVEN &&
                   status != Status::PENDING_INSTANTIATION;
        }
    };

    /**
     * Information about a metavariable in a proof
     */
    struct MetavariableInfo
    {
        std::string name;
        TypePtr type;
        bool instantiated = false;
        TermDBPtr instantiation = nullptr;
    };

    /**
     * Immutable proof state representation
     *
     * Maintains a set of hypotheses and a goal formula,
     * with a history of steps that led to this state.
     */
    class ProofState
    {
    public:
        // Create an initial proof state with a goal
        static ProofStatePtr create_initial(const TermDBPtr &goal);

        // Create a new proof state from an existing one
        static ProofStatePtr create_from(
            const ProofStatePtr &parent,
            const std::string &rule_name,
            const std::vector<std::string> &premise_names,
            const std::vector<Hypothesis> &new_hypotheses,
            const TermDBPtr &new_goal);

        // Access methods
        const std::vector<Hypothesis> &hypotheses() const { return hypotheses_; }
        const TermDBPtr &goal() const { return goal_; }
        const ProofStatePtr &parent() const { return parent_; }
        const ProofStep *last_step() const { return last_step_.get(); }
        const ProofCertification &certification() const { return certification_; }

        // Get a hypothesis by name
        const Hypothesis *find_hypothesis(const std::string &name) const;

        // Mark a state as proved with a justification
        void mark_as_proved(
            ProofCertification::Status status = ProofCertification::Status::PROVED_BY_RULE,
            const std::string &justification = "");

        // Check if this state proves the goal
        bool is_proved() const;

        // Metavariable management
        void add_metavariable(const std::string &name, const TypePtr &type);
        bool instantiate_metavariable(const std::string &name, const TermDBPtr &term);
        bool has_uninstantiated_metavariables() const;
        const MetavariableInfo *find_metavariable(const std::string &name) const;

        // Get a unique identifier for this state
        std::size_t hash() const { return hash_; }

        // Check if two states are equal (extensionally)
        bool equals(const ProofState &other) const;

        // Generate a proof trace from initial state to this state
        std::vector<const ProofStep *> get_proof_trace() const;

        // Get the depth of this state in the proof tree
        std::size_t depth() const { return depth_; }

    private:
        // Private constructor, use factory methods instead
        ProofState(
            const ProofStatePtr &parent,
            const std::vector<Hypothesis> &hypotheses,
            const TermDBPtr &goal,
            std::unique_ptr<ProofStep> last_step);

        // Calculate hash value for this state
        void calculate_hash();

        ProofStatePtr parent_;                                            // Parent state (null for initial state)
        std::vector<Hypothesis> hypotheses_;                              // Current hypotheses
        TermDBPtr goal_;                                                  // Current goal
        std::unique_ptr<ProofStep> last_step_;                            // Last proof step applied
        std::size_t hash_;                                                // Hash value for quick identity checks
        std::size_t depth_;                                               // Depth in the proof tree
        ProofCertification certification_;                                // Proof status and justification
        std::unordered_map<std::string, MetavariableInfo> metavariables_; // Metavariables in this state
    };

    /**
     * Context for managing proof states
     *
     * Stores proof states and provides methods for manipulating them.
     */
    class ProofContext
    {
    public:
        ProofContext();

        // Create a new initial proof state
        ProofStatePtr create_initial_state(const TermDBPtr &goal);

        // Apply a rule to a state and get the resulting state
        ProofStatePtr apply_rule(
            const ProofStatePtr &state,
            const std::string &rule_name,
            const std::vector<std::string> &premise_names,
            const std::vector<Hypothesis> &new_hypotheses,
            const TermDBPtr &new_goal);

        // Validate a rule application (for soundness checking)
        bool validate_rule_application(
            const std::string &rule_name,
            const std::vector<std::string> &premise_names,
            const TermDBPtr &conclusion);

        // Get all states in the context
        const std::unordered_set<ProofStatePtr> &states() const { return states_; }

        // Get the total number of states
        std::size_t count() const { return states_.size(); }

        // Find a state by its hash value
        ProofStatePtr find_state(std::size_t hash_value) const;

        // Get all leaf states in the proof tree
        std::vector<ProofStatePtr> get_leaf_states() const;

        // Get all proved states
        std::vector<ProofStatePtr> get_proved_states() const;

    private:
        // Add a state to the context
        void add_state(const ProofStatePtr &state);

        // All proof states in this context
        std::unordered_set<ProofStatePtr> states_;

        // Maps from hash values to states for quick lookup
        std::unordered_map<std::size_t, ProofStatePtr> state_map_;
    };

    // Comparison functions for STL containers
    bool operator==(const Hypothesis &lhs, const Hypothesis &rhs);
    bool operator==(const ProofState &lhs, const ProofState &rhs);

    // Hash functions for STL containers
    struct HypothesisHash
    {
        std::size_t operator()(const Hypothesis &h) const
        {
            return h.hash();
        }
    };

    struct ProofStateHash
    {
        std::size_t operator()(const ProofStatePtr &p) const
        {
            return p->hash();
        }
    };

    struct ProofStateEqual
    {
        bool operator()(const ProofStatePtr &p1, const ProofStatePtr &p2) const
        {
            return p1->equals(*p2);
        }
    };

} // namespace theorem_prover