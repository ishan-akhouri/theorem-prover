#pragma once

#include "../proof/proof_state.hpp"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace theorem_prover
{

    /**
     * @brief Represents a violation of constraints during rule application
     *
     * This class provides information about why a rule application failed,
     * including type mismatches, occurs check failures, and other constraint
     * violations that might occur during proof construction.
     */
    class ConstraintViolation
    {
    public:
        enum class ViolationType
        {
            TYPE_MISMATCH,               // Types don't match or can't be unified
            OCCURS_CHECK_FAILURE,        // Circular type dependency detected
            UNINSTANTIATED_METAVARIABLE, // Required metavariable not instantiated
            UNKNOWN_METAVARIABLE,        // Reference to undefined metavariable
            TYPE_INFERENCE_FAILURE,      // Couldn't infer type of a term
            RULE_PATTERN_MISMATCH,       // Term doesn't match expected rule pattern
            INVALID_HYPOTHESIS,          // Referenced hypothesis doesn't exist
            INVALID_RULE_APPLICATION     // Generic rule application failure
        };

        /**
         * @brief Construct a new Constraint Violation object
         *
         * @param type The type of violation
         * @param message Detailed error message
         * @param expected Expected type (for type mismatches)
         * @param actual Actual type encountered (for type mismatches)
         */
        ConstraintViolation(
            ViolationType type,
            const std::string &message,
            const TypePtr &expected = nullptr,
            const TypePtr &actual = nullptr)
            : type_(type), message_(message), expected_(expected), actual_(actual) {}

        // Accessors
        ViolationType type() const { return type_; }
        std::string message() const { return message_; }
        const TypePtr &expected_type() const { return expected_; }
        const TypePtr &actual_type() const { return actual_; }

    private:
        ViolationType type_;
        std::string message_;
        TypePtr expected_;
        TypePtr actual_;
    };

    /**
     * @brief Context for rule application with dynamic parameters
     *
     * This struct holds additional parameters that might be needed
     * during rule application but aren't part of the rule's core identity.
     */
    struct RuleApplicationContext
    {
        // Term substitution parameters
        TermDBPtr substitution_term;

        // Position parameters
        std::size_t position = 0;

        // Direction flags
        bool left_to_right = true;

        // Hypothesis selectors
        std::string hypothesis_name;

        // Term construction
        TermDBPtr additional_term;

        // Variable naming
        std::string variable_name = "x";
    };

    /**
     * @brief Base class for all proof rules
     *
     * A proof rule represents a single logical inference step that
     * transforms a proof state. Rules are immutable and configured
     * at construction time.
     */
    class ProofRule
    {
    public:
        /**
         * @brief Default constructor
         */
        ProofRule() = default;

        /**
         * @brief Virtual destructor
         */
        virtual ~ProofRule() = default;

        /**
         * @brief Check if the rule is applicable to the given state
         *
         * @param state The proof state to check
         * @param app_context Optional application context for dynamic parameters
         * @return true if the rule can be applied, false otherwise
         */
        virtual bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const = 0;

        /**
         * @brief Apply the rule to a proof state
         *
         * @param context The proof context to register new states
         * @param state The state to transform
         * @param violation Optional output parameter for constraint violations
         * @param app_context Optional application context for dynamic parameters
         * @return std::vector<ProofStatePtr> Resulting states (empty if application failed)
         */
        virtual std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const = 0;

        /**
         * @brief Get the rule's name
         *
         * @return std::string Name of the rule
         */
        virtual std::string name() const = 0;

        /**
         * @brief Get a description of the rule
         *
         * @return std::string Description of the rule's function
         */
        virtual std::string description() const = 0;

        // Delete copy operations to prevent unintended sharing of rule instances
        ProofRule(const ProofRule &) = delete;
        ProofRule &operator=(const ProofRule &) = delete;

        // Allow move operations
        ProofRule(ProofRule &&) = default;
        ProofRule &operator=(ProofRule &&) = default;
    };

    using ProofRulePtr = std::shared_ptr<ProofRule>;

    /**
     * @brief Modus Ponens rule: from P and P→Q, derive Q
     */
    class ModusPonensRule : public ProofRule
    {
    public:
        /**
         * @brief Construct a new Modus Ponens Rule
         *
         * @param antecedent_hyp_name Name of the hypothesis containing the antecedent P
         * @param implication_hyp_name Name of the hypothesis containing the implication P→Q
         */
        ModusPonensRule(
            const std::string &antecedent_hyp_name,
            const std::string &implication_hyp_name);

        bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::string name() const override;
        std::string description() const override;

    private:
        /**
         * @brief Validate that premises match the rule pattern
         *
         * @param antecedent The P term
         * @param implication The P→Q term
         * @return true if valid application
         */
        bool validate_pattern(
            const TermDBPtr &antecedent,
            const TermDBPtr &implication) const;

        const std::string antecedent_hyp_name_;
        const std::string implication_hyp_name_;
    };

    /**
     * @brief And-Introduction rule: from P and Q, derive P∧Q
     */
    class AndIntroRule : public ProofRule
    {
    public:
        /**
         * @brief Construct a new And Intro Rule
         *
         * @param left_hyp_name Name of the hypothesis containing the left conjunct
         * @param right_hyp_name Name of the hypothesis containing the right conjunct
         */
        AndIntroRule(
            const std::string &left_hyp_name,
            const std::string &right_hyp_name);

        bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::string name() const override;
        std::string description() const override;

    private:
        const std::string left_hyp_name_;
        const std::string right_hyp_name_;
    };

    /**
     * @brief And-Elimination rule: from P∧Q, derive P or Q
     */
    class AndElimRule : public ProofRule
    {
    public:
        /**
         * @brief Construct a new And Elim Rule
         *
         * @param conjunction_hyp_name Name of the hypothesis containing the conjunction
         * @param extract_left Whether to extract the left (true) or right (false) conjunct
         */
        AndElimRule(
            const std::string &conjunction_hyp_name,
            bool extract_left);

        bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::string name() const override;
        std::string description() const override;

        /**
         * @brief Factory method for creating a left conjunct extractor
         *
         * @param conjunction_hyp_name Name of the hypothesis containing the conjunction
         * @return std::shared_ptr<AndElimRule> Rule that extracts the left conjunct
         */
        static std::shared_ptr<AndElimRule> create_left(const std::string &conjunction_hyp_name)
        {
            return std::make_shared<AndElimRule>(conjunction_hyp_name, true);
        }

        /**
         * @brief Factory method for creating a right conjunct extractor
         *
         * @param conjunction_hyp_name Name of the hypothesis containing the conjunction
         * @return std::shared_ptr<AndElimRule> Rule that extracts the right conjunct
         */
        static std::shared_ptr<AndElimRule> create_right(const std::string &conjunction_hyp_name)
        {
            return std::make_shared<AndElimRule>(conjunction_hyp_name, false);
        }

    private:
        const std::string conjunction_hyp_name_;
        const bool extract_left_; // Whether to extract the left (P) or right (Q) component
    };

    /**
     * @brief Or-Introduction rule: from P, derive P∨Q
     */
    class OrIntroRule : public ProofRule
    {
    public:
        /**
         * @brief Construct a new Or Intro Rule
         *
         * @param premise_hyp_name Name of the hypothesis to introduce into the disjunction
         * @param additional_term The other term to add to the disjunction
         * @param premise_on_left Whether to place the premise on the left (true) or right (false)
         */
        OrIntroRule(
            const std::string &premise_hyp_name,
            const TermDBPtr &additional_term,
            bool premise_on_left);

        bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::string name() const override;
        std::string description() const override;

        /**
         * @brief Factory method for creating a left disjunct introducer
         *
         * @param premise_hyp_name Name of the hypothesis to use as the left disjunct
         * @param right_term Term to use as the right disjunct
         * @return std::shared_ptr<OrIntroRule> Rule that places the premise on the left
         */
        static std::shared_ptr<OrIntroRule> create_left(
            const std::string &premise_hyp_name,
            const TermDBPtr &right_term)
        {
            return std::make_shared<OrIntroRule>(premise_hyp_name, right_term, true);
        }

        /**
         * @brief Factory method for creating a right disjunct introducer
         *
         * @param premise_hyp_name Name of the hypothesis to use as the right disjunct
         * @param left_term Term to use as the left disjunct
         * @return std::shared_ptr<OrIntroRule> Rule that places the premise on the right
         */
        static std::shared_ptr<OrIntroRule> create_right(
            const std::string &premise_hyp_name,
            const TermDBPtr &left_term)
        {
            return std::make_shared<OrIntroRule>(premise_hyp_name, left_term, false);
        }

    private:
        const std::string premise_hyp_name_;
        const TermDBPtr additional_term_;
        const bool premise_on_left_; // Whether to place premise on left or right side
    };

    /**
     * @brief Implication-Introduction rule: to prove P→Q, assume P and prove Q
     */
    class ImpliesIntroRule : public ProofRule
    {
    public:
        /**
         * @brief Construct a new Implies Intro Rule
         */
        ImpliesIntroRule();

        bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::string name() const override;
        std::string description() const override;
    };

    /**
     * @brief Universal Quantifier Introduction: to prove ∀x.P(x), prove P(x) for fresh x
     */
    class ForallIntroRule : public ProofRule
    {
    public:
        /**
         * @brief Construct a new Forall Intro Rule
         *
         * @param variable_hint Optional hint for the variable name
         */
        explicit ForallIntroRule(const std::string &variable_hint = "x");

        bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::string name() const override;
        std::string description() const override;

    private:
        const std::string variable_hint_;
    };

    /**
     * @brief Universal Quantifier Elimination: from ∀x.P(x), derive P(t) for any term t
     */
    class ForallElimRule : public ProofRule
    {
    public:
        /**
         * @brief Construct a new Forall Elim Rule
         *
         * @param forall_hyp_name Name of the hypothesis containing the universal quantifier
         * @param substitution_term Term to substitute for the quantified variable
         */
        ForallElimRule(
            const std::string &forall_hyp_name,
            const TermDBPtr &substitution_term);

        bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::string name() const override;
        std::string description() const override;

    private:
        const std::string forall_hyp_name_;
        const TermDBPtr substitution_term_; // Term to substitute for the quantified variable
    };

    /**
     * @brief Existential Quantifier Introduction: from P(t), derive ∃x.P(x)
     */
    class ExistsIntroRule : public ProofRule
    {
    public:
        /**
         * @brief Construct a new Exists Intro Rule
         *
         * @param witness_hyp_name Name of the hypothesis containing the witness
         * @param variable_hint Optional hint for the variable name
         */
        ExistsIntroRule(
            const std::string &witness_hyp_name,
            const std::string &variable_hint = "x");

        bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::string name() const override;
        std::string description() const override;

    private:
        const std::string witness_hyp_name_; // Hypothesis containing the witness term
        const std::string variable_hint_;    // Hint for the variable name
    };

    /**
     * @brief Assumption rule: add a formula as a hypothesis
     */
    class AssumptionRule : public ProofRule
    {
    public:
        /**
         * @brief Construct a new Assumption Rule
         *
         * @param formula Formula to add as a hypothesis
         * @param name Optional name for the hypothesis
         */
        AssumptionRule(
            const TermDBPtr &formula,
            const std::string &name = "");

        bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::string name() const override;
        std::string description() const override;

    private:
        const TermDBPtr formula_; // Formula to assume
        const std::string name_;  // Name for the hypothesis
    };

    /**
     * @brief Cut rule: introduce a lemma and prove it separately
     */
    class CutRule : public ProofRule
    {
    public:
        /**
         * @brief Construct a new Cut Rule
         *
         * @param lemma Formula to prove as a subgoal
         * @param lemma_name Name to give the lemma when proved
         */
        CutRule(
            const TermDBPtr &lemma,
            const std::string &lemma_name = "lemma");

        bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::string name() const override;
        std::string description() const override;

    private:
        const TermDBPtr lemma_;        // Formula to prove as lemma
        const std::string lemma_name_; // Name for the lemma hypothesis
    };

    /**
     * @brief Contradiction rule: from a contradiction, derive anything
     */
    class ContradictionRule : public ProofRule
    {
    public:
        /**
         * @brief Construct a new Contradiction Rule
         *
         * @param formula_hyp_name Name of the hypothesis containing a formula
         * @param negation_hyp_name Name of the hypothesis containing its negation
         */
        ContradictionRule(
            const std::string &formula_hyp_name,
            const std::string &negation_hyp_name);

        bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::string name() const override;
        std::string description() const override;

    private:
        const std::string formula_hyp_name_;
        const std::string negation_hyp_name_;
    };

    /**
     * @brief Rewrite rule: rewrite a term or formula using an equality
     */
    class RewriteRule : public ProofRule
    {
    public:
        /**
         * @brief Construct a new Rewrite Rule
         *
         * @param equality_hyp_name Name of the hypothesis containing the equality
         * @param left_to_right Whether to rewrite left-to-right (true) or right-to-left (false)
         */
        RewriteRule(
            const std::string &equality_hyp_name,
            bool left_to_right = true);

        bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::string name() const override;
        std::string description() const override;

        /**
         * @brief Factory method for creating a left-to-right rewriter
         *
         * @param equality_hyp_name Name of the hypothesis containing the equality
         * @return std::shared_ptr<RewriteRule> Rule that rewrites left to right
         */
        static std::shared_ptr<RewriteRule> create_left_to_right(const std::string &equality_hyp_name)
        {
            return std::make_shared<RewriteRule>(equality_hyp_name, true);
        }

        /**
         * @brief Factory method for creating a right-to-left rewriter
         *
         * @param equality_hyp_name Name of the hypothesis containing the equality
         * @return std::shared_ptr<RewriteRule> Rule that rewrites right to left
         */
        static std::shared_ptr<RewriteRule> create_right_to_left(const std::string &equality_hyp_name)
        {
            return std::make_shared<RewriteRule>(equality_hyp_name, false);
        }

    private:
        const std::string equality_hyp_name_; // Hypothesis containing the equality
        const bool left_to_right_;            // Direction of rewriting
    };

    /**
     * Existential Elimination: from ∃x.P(x), introduce P(c) with a fresh constant c
     */
    class ExistsElimRule : public ProofRule
    {
    public:
        /**
         * @brief Construct a new Exists Elim Rule
         *
         * @param exists_hyp_name Name of the hypothesis containing the existential quantifier
         */
        ExistsElimRule(const std::string &exists_hyp_name);

        bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::string name() const override;
        std::string description() const override;

    private:
        const std::string exists_hyp_name_; // Hypothesis containing the existential quantifier
    };

    /**
     * Quantifier Negation: transform between dual quantifiers with negation
     * Implements ¬(∃x.P(x)) ⟷ ∀x.¬P(x) and ¬(∀x.P(x)) ⟷ ∃x.¬P(x)
     */
    class QuantifierNegationRule : public ProofRule
    {
    public:
        /**
         * @brief Construct a new Quantifier Negation Rule
         *
         * @param hyp_name Name of the hypothesis containing the quantified formula
         * @param direction Direction of transformation:
         *                  true = ¬(Qx.P) → Q̅x.¬P
         *                  false = Q̅x.¬P → ¬(Qx.P)
         *                  Where Q is the quantifier and Q̅ is its dual
         */
        QuantifierNegationRule(const std::string &hyp_name, bool direction = true);

        bool is_applicable(
            const ProofStatePtr &state,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::vector<ProofStatePtr> apply(
            ProofContext &context,
            const ProofStatePtr &state,
            std::optional<ConstraintViolation> &violation,
            const std::optional<RuleApplicationContext> &app_context = std::nullopt) const override;

        std::string name() const override;
        std::string description() const override;

        /**
         * @brief Factory method for creating a rule to transform ¬(Qx.P) to Q̅x.¬P
         *
         * @param hyp_name Name of the hypothesis containing the negated quantified formula
         * @return std::shared_ptr<QuantifierNegationRule> Rule that transforms negation inward
         */
        static std::shared_ptr<QuantifierNegationRule> create_negation_inward(const std::string &hyp_name)
        {
            return std::make_shared<QuantifierNegationRule>(hyp_name, true);
        }

        /**
         * @brief Factory method for creating a rule to transform Q̅x.¬P to ¬(Qx.P)
         *
         * @param hyp_name Name of the hypothesis containing the quantified negated formula
         * @return std::shared_ptr<QuantifierNegationRule> Rule that transforms negation outward
         */
        static std::shared_ptr<QuantifierNegationRule> create_negation_outward(const std::string &hyp_name)
        {
            return std::make_shared<QuantifierNegationRule>(hyp_name, false);
        }

    private:
        const std::string hyp_name_; // Hypothesis containing the formula to transform
        const bool direction_;       // Direction of transformation (inward or outward)
    };

    // Factory functions for creating rule instances
    std::shared_ptr<ExistsElimRule> make_exists_elim(const std::string &exists_hyp_name);
    std::shared_ptr<QuantifierNegationRule> make_quantifier_negation(
        const std::string &hyp_name, bool direction = true);

} // namespace theorem_prover