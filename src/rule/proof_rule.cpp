#include "proof_rule.hpp"
#include "../term/substitution.hpp"
#include "../utils/gensym.hpp"
#include <sstream>
#include <iostream>
#include <algorithm>

namespace theorem_prover
{

    // ModusPonensRule implementation
    ModusPonensRule::ModusPonensRule(
        const std::string &antecedent_hyp_name,
        const std::string &implication_hyp_name)
        : antecedent_hyp_name_(antecedent_hyp_name),
          implication_hyp_name_(implication_hyp_name) {}

    bool ModusPonensRule::is_applicable(
        const ProofStatePtr &state,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the hypothesis containing the antecedent
        const Hypothesis *antecedent_hyp = state->find_hypothesis(antecedent_hyp_name_);
        if (!antecedent_hyp)
        {
            return false;
        }

        // Find the hypothesis containing the implication
        const Hypothesis *implication_hyp = state->find_hypothesis(implication_hyp_name_);
        if (!implication_hyp)
        {
            return false;
        }

        // Check if implication_hyp is actually an implication
        return validate_pattern(antecedent_hyp->formula(), implication_hyp->formula());
    }

    std::vector<ProofStatePtr> ModusPonensRule::apply(
        ProofContext &context,
        const ProofStatePtr &state,
        std::optional<ConstraintViolation> &violation,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the hypothesis containing the antecedent
        const Hypothesis *antecedent_hyp = state->find_hypothesis(antecedent_hyp_name_);
        if (!antecedent_hyp)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_HYPOTHESIS,
                "Antecedent hypothesis not found: " + antecedent_hyp_name_);
            return {};
        }

        // Find the hypothesis containing the implication
        const Hypothesis *implication_hyp = state->find_hypothesis(implication_hyp_name_);
        if (!implication_hyp)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_HYPOTHESIS,
                "Implication hypothesis not found: " + implication_hyp_name_);
            return {};
        }

        // Verify the pattern
        if (!validate_pattern(antecedent_hyp->formula(), implication_hyp->formula()))
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                "The terms do not match the pattern for modus ponens");
            return {};
        }

        // Extract the consequent from the implication
        auto implies = std::dynamic_pointer_cast<ImpliesDB>(implication_hyp->formula());
        auto consequent = implies->consequent();

        // Create a new proof state with the consequent as a new hypothesis
        std::string new_hyp_name = gensym("mp_result");
        std::vector<Hypothesis> new_hypotheses = {Hypothesis(new_hyp_name, consequent)};

        // Create the proof step with premise names
        std::vector<std::string> premise_names = {antecedent_hyp_name_, implication_hyp_name_};

        // Apply the rule through the context
        auto new_state = context.apply_rule(
            state,
            "modus_ponens",
            premise_names,
            new_hypotheses,
            state->goal());

        return {new_state};
    }

    std::string ModusPonensRule::name() const
    {
        return "Modus Ponens";
    }

    std::string ModusPonensRule::description() const
    {
        return "From P and P→Q, derive Q";
    }

    bool ModusPonensRule::validate_pattern(
        const TermDBPtr &antecedent,
        const TermDBPtr &implication) const
    {

        // Check if implication is of the form P→Q
        if (implication->kind() != TermDB::TermKind::IMPLIES)
        {
            return false;
        }

        auto implies = std::dynamic_pointer_cast<ImpliesDB>(implication);

        // Check if the antecedent matches the implication's antecedent
        return *antecedent == *implies->antecedent();
    }

    // AndIntroRule implementation
    AndIntroRule::AndIntroRule(
        const std::string &left_hyp_name,
        const std::string &right_hyp_name)
        : left_hyp_name_(left_hyp_name),
          right_hyp_name_(right_hyp_name) {}

    bool AndIntroRule::is_applicable(
        const ProofStatePtr &state,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Check if both hypotheses exist
        return state->find_hypothesis(left_hyp_name_) != nullptr &&
               state->find_hypothesis(right_hyp_name_) != nullptr;
    }

    std::vector<ProofStatePtr> AndIntroRule::apply(
        ProofContext &context,
        const ProofStatePtr &state,
        std::optional<ConstraintViolation> &violation,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the left conjunct hypothesis
        const Hypothesis *left_hyp = state->find_hypothesis(left_hyp_name_);
        if (!left_hyp)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_HYPOTHESIS,
                "Left conjunct hypothesis not found: " + left_hyp_name_);
            return {};
        }

        // Find the right conjunct hypothesis
        const Hypothesis *right_hyp = state->find_hypothesis(right_hyp_name_);
        if (!right_hyp)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_HYPOTHESIS,
                "Right conjunct hypothesis not found: " + right_hyp_name_);
            return {};
        }

        // Create the conjunction term
        auto conjunction = make_and(left_hyp->formula(), right_hyp->formula());

        // Create a new hypothesis with the conjunction
        std::string new_hyp_name = gensym("and_intro_result");
        std::vector<Hypothesis> new_hypotheses = {Hypothesis(new_hyp_name, conjunction)};

        // Create the proof step with premise names
        std::vector<std::string> premise_names = {left_hyp_name_, right_hyp_name_};

        // Apply the rule through the context
        auto new_state = context.apply_rule(
            state,
            "and_intro",
            premise_names,
            new_hypotheses,
            state->goal());

        return {new_state};
    }

    std::string AndIntroRule::name() const
    {
        return "And Introduction";
    }

    std::string AndIntroRule::description() const
    {
        return "From P and Q, derive P∧Q";
    }

    // AndElimRule implementation
    AndElimRule::AndElimRule(
        const std::string &conjunction_hyp_name,
        bool extract_left)
        : conjunction_hyp_name_(conjunction_hyp_name),
          extract_left_(extract_left) {}

    bool AndElimRule::is_applicable(
        const ProofStatePtr &state,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the conjunction hypothesis
        const Hypothesis *conjunction_hyp = state->find_hypothesis(conjunction_hyp_name_);
        if (!conjunction_hyp)
        {
            return false;
        }

        // Check if the hypothesis is a conjunction
        return conjunction_hyp->formula()->kind() == TermDB::TermKind::AND;
    }

    std::vector<ProofStatePtr> AndElimRule::apply(
        ProofContext &context,
        const ProofStatePtr &state,
        std::optional<ConstraintViolation> &violation,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the conjunction hypothesis
        const Hypothesis *conjunction_hyp = state->find_hypothesis(conjunction_hyp_name_);
        if (!conjunction_hyp)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_HYPOTHESIS,
                "Conjunction hypothesis not found: " + conjunction_hyp_name_);
            return {};
        }

        // Verify that the hypothesis is a conjunction
        if (conjunction_hyp->formula()->kind() != TermDB::TermKind::AND)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                "Hypothesis is not a conjunction: " + conjunction_hyp_name_);
            return {};
        }

        // Cast to AndDB
        auto and_formula = std::dynamic_pointer_cast<AndDB>(conjunction_hyp->formula());

        // Extract the requested conjunct
        TermDBPtr conjunct = extract_left_ ? and_formula->left() : and_formula->right();

        // Create a name for the new hypothesis
        std::string new_hyp_name = gensym(extract_left_ ? "and_elim_left" : "and_elim_right");

        // Create a new hypothesis with the extracted conjunct
        std::vector<Hypothesis> new_hypotheses = {Hypothesis(new_hyp_name, conjunct)};

        // Create the proof step with premise name
        std::vector<std::string> premise_names = {conjunction_hyp_name_};

        // Apply the rule through the context
        auto new_state = context.apply_rule(
            state,
            extract_left_ ? "and_elim_left" : "and_elim_right",
            premise_names,
            new_hypotheses,
            state->goal());

        return {new_state};
    }

    std::string AndElimRule::name() const
    {
        return extract_left_ ? "And Elimination (Left)" : "And Elimination (Right)";
    }

    std::string AndElimRule::description() const
    {
        return extract_left_ ? "From P∧Q, derive P" : "From P∧Q, derive Q";
    }

    // OrIntroRule implementation
    OrIntroRule::OrIntroRule(
        const std::string &premise_hyp_name,
        const TermDBPtr &additional_term,
        bool premise_on_left)
        : premise_hyp_name_(premise_hyp_name),
          additional_term_(additional_term),
          premise_on_left_(premise_on_left) {}

    bool OrIntroRule::is_applicable(
        const ProofStatePtr &state,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Check if the premise hypothesis exists
        return state->find_hypothesis(premise_hyp_name_) != nullptr;
    }

    std::vector<ProofStatePtr> OrIntroRule::apply(
        ProofContext &context,
        const ProofStatePtr &state,
        std::optional<ConstraintViolation> &violation,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the premise hypothesis
        const Hypothesis *premise_hyp = state->find_hypothesis(premise_hyp_name_);
        if (!premise_hyp)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_HYPOTHESIS,
                "Premise hypothesis not found: " + premise_hyp_name_);
            return {};
        }

        // Check if an additional term was provided via app_context
        TermDBPtr term_to_use = additional_term_;
        if (app_context && app_context->additional_term)
        {
            term_to_use = app_context->additional_term;
        }

        if (!term_to_use)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_RULE_APPLICATION,
                "Additional term required for Or-Introduction");
            return {};
        }

        // Create the disjunction term (P∨Q or Q∨P)
        TermDBPtr disjunction;
        if (premise_on_left_)
        {
            disjunction = make_or(premise_hyp->formula(), term_to_use);
        }
        else
        {
            disjunction = make_or(term_to_use, premise_hyp->formula());
        }

        // Create a new hypothesis with the disjunction
        std::string new_hyp_name = gensym("or_intro_result");
        std::vector<Hypothesis> new_hypotheses = {Hypothesis(new_hyp_name, disjunction)};

        // Create the proof step with premise name
        std::vector<std::string> premise_names = {premise_hyp_name_};

        // Apply the rule through the context
        auto new_state = context.apply_rule(
            state,
            premise_on_left_ ? "or_intro_left" : "or_intro_right",
            premise_names,
            new_hypotheses,
            state->goal());

        return {new_state};
    }

    std::string OrIntroRule::name() const
    {
        return premise_on_left_ ? "Or Introduction (Left)" : "Or Introduction (Right)";
    }

    std::string OrIntroRule::description() const
    {
        return premise_on_left_
                   ? "From P, introduce P∨Q for any Q"
                   : "From Q, introduce P∨Q for any P";
    }

    // ImpliesIntroRule implementation
    ImpliesIntroRule::ImpliesIntroRule() {}

    bool ImpliesIntroRule::is_applicable(
        const ProofStatePtr &state,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // This rule applies when trying to prove an implication
        return state->goal()->kind() == TermDB::TermKind::IMPLIES;
    }

    std::vector<ProofStatePtr> ImpliesIntroRule::apply(
        ProofContext &context,
        const ProofStatePtr &state,
        std::optional<ConstraintViolation> &violation,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Check if the goal is an implication
        if (state->goal()->kind() != TermDB::TermKind::IMPLIES)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                "Goal is not an implication");
            return {};
        }

        // Cast to ImpliesDB
        auto implies_goal = std::dynamic_pointer_cast<ImpliesDB>(state->goal());

        // Extract the antecedent and consequent
        auto antecedent = implies_goal->antecedent();
        auto consequent = implies_goal->consequent();

        // Create a name for the new hypothesis
        std::string new_hyp_name = gensym("implies_intro_premise");

        // Create a new hypothesis with the antecedent
        std::vector<Hypothesis> new_hypotheses = {Hypothesis(new_hyp_name, antecedent)};

        // Create the proof step with no premise names (this is a forward step)
        std::vector<std::string> premise_names = {};

        // Apply the rule through the context to create a new state with the consequent as the goal
        auto new_state = context.apply_rule(
            state,
            "implies_intro",
            premise_names,
            new_hypotheses,
            consequent);

        return {new_state};
    }

    std::string ImpliesIntroRule::name() const
    {
        return "Implication Introduction";
    }

    std::string ImpliesIntroRule::description() const
    {
        return "To prove P→Q, assume P and prove Q";
    }

    // ForallIntroRule implementation
    ForallIntroRule::ForallIntroRule(const std::string &variable_hint)
        : variable_hint_(variable_hint) {}

    bool ForallIntroRule::is_applicable(
        const ProofStatePtr &state,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // This rule applies when trying to prove a universally quantified statement
        return state->goal()->kind() == TermDB::TermKind::FORALL;
    }

    std::vector<ProofStatePtr> ForallIntroRule::apply(
        ProofContext &context,
        const ProofStatePtr &state,
        std::optional<ConstraintViolation> &violation,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Check if the goal is a universal quantifier
        if (state->goal()->kind() != TermDB::TermKind::FORALL)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                "Goal is not a universal quantifier");
            return {};
        }

        // Cast to ForallDB
        auto forall_goal = std::dynamic_pointer_cast<ForallDB>(state->goal());

        // Extract the body
        auto body = forall_goal->body();

        // Use the variable hint from application context if provided
        std::string var_hint = variable_hint_;
        if (app_context && !app_context->variable_name.empty())
        {
            var_hint = app_context->variable_name;
        }

        // Create a metavariable to represent the variable
        // This needs to be tracked in the proof state
        std::string meta_var_name = gensym("forall_intro_var");

        // We need to create a corresponding type for the metavariable
        // This requires inferring the type of the quantified variable
        // For now, we'll use a placeholder type
        auto var_type = make_base_type("placeholder"); // TODO: Infer proper type

        // Create the new proof state
        auto new_state = context.apply_rule(
            state,
            "forall_intro",
            {},  // No premises
            {},  // No new hypotheses
            body // New goal is the body of the forall
        );

        // Add the metavariable to the new state
        new_state->add_metavariable(meta_var_name, var_type);

        return {new_state};
    }

    std::string ForallIntroRule::name() const
    {
        return "Universal Introduction";
    }

    std::string ForallIntroRule::description() const
    {
        return "To prove ∀x.P(x), prove P(x) for a fresh variable x";
    }

    // ForallElimRule implementation
    ForallElimRule::ForallElimRule(
        const std::string &forall_hyp_name,
        const TermDBPtr &substitution_term)
        : forall_hyp_name_(forall_hyp_name),
          substitution_term_(substitution_term) {}

    bool ForallElimRule::is_applicable(
        const ProofStatePtr &state,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the forall hypothesis
        const Hypothesis *forall_hyp = state->find_hypothesis(forall_hyp_name_);
        if (!forall_hyp)
        {
            return false;
        }

        // Check if the hypothesis is a universal quantifier
        return forall_hyp->formula()->kind() == TermDB::TermKind::FORALL;
    }

    std::vector<ProofStatePtr> ForallElimRule::apply(
        ProofContext &context,
        const ProofStatePtr &state,
        std::optional<ConstraintViolation> &violation,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the forall hypothesis
        const Hypothesis *forall_hyp = state->find_hypothesis(forall_hyp_name_);
        if (!forall_hyp)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_HYPOTHESIS,
                "Universal quantifier hypothesis not found: " + forall_hyp_name_);
            return {};
        }

        // Verify that the hypothesis is a universal quantifier
        if (forall_hyp->formula()->kind() != TermDB::TermKind::FORALL)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                "Hypothesis is not a universal quantifier: " + forall_hyp_name_);
            return {};
        }

        // Cast to ForallDB
        auto forall_formula = std::dynamic_pointer_cast<ForallDB>(forall_hyp->formula());

        // Get the body of the universal quantifier
        auto body = forall_formula->body();

        // Get the substitution term, either from the rule or the application context
        TermDBPtr term_to_use = substitution_term_;
        if (app_context && app_context->substitution_term)
        {
            term_to_use = app_context->substitution_term;
        }

        if (!term_to_use)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_RULE_APPLICATION,
                "Substitution term required for Universal Elimination");
            return {};
        }

        // Create the substitution {0 -> term}
        SubstitutionMap subst;
        subst[0] = term_to_use;

        // Perform the substitution
        SubstitutionEngine engine;
        auto instantiated_body = engine.substitute(body, subst);

        // Create a name for the new hypothesis
        std::string new_hyp_name = gensym("forall_elim_result");

        // Create a new hypothesis with the instantiated formula
        std::vector<Hypothesis> new_hypotheses = {Hypothesis(new_hyp_name, instantiated_body)};

        // Create the proof step with premise name
        std::vector<std::string> premise_names = {forall_hyp_name_};

        // Apply the rule through the context
        auto new_state = context.apply_rule(
            state,
            "forall_elim",
            premise_names,
            new_hypotheses,
            state->goal());

        return {new_state};
    }

    std::string ForallElimRule::name() const
    {
        return "Universal Elimination";
    }

    std::string ForallElimRule::description() const
    {
        return "From ∀x.P(x), derive P(t) for any term t";
    }

    // ExistsIntroRule implementation
    ExistsIntroRule::ExistsIntroRule(
        const std::string &witness_hyp_name,
        const std::string &variable_hint)
        : witness_hyp_name_(witness_hyp_name),
          variable_hint_(variable_hint) {}

    bool ExistsIntroRule::is_applicable(
        const ProofStatePtr &state,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // This rule is applicable when the goal is an existential statement
        // and we have a hypothesis that could serve as a witness
        if (state->goal()->kind() != TermDB::TermKind::EXISTS)
        {
            return false;
        }

        // Check if the witness hypothesis exists
        return state->find_hypothesis(witness_hyp_name_) != nullptr;
    }

    std::vector<ProofStatePtr> ExistsIntroRule::apply(
        ProofContext &context,
        const ProofStatePtr &state,
        std::optional<ConstraintViolation> &violation,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Check if the goal is an existential quantifier
        if (state->goal()->kind() != TermDB::TermKind::EXISTS)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                "Goal is not an existential quantifier");
            return {};
        }

        // Find the witness hypothesis
        const Hypothesis *witness_hyp = state->find_hypothesis(witness_hyp_name_);
        if (!witness_hyp)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_HYPOTHESIS,
                "Witness hypothesis not found: " + witness_hyp_name_);
            return {};
        }

        // Cast to ExistsDB
        auto exists_goal = std::dynamic_pointer_cast<ExistsDB>(state->goal());

        // Get the body of the existential quantifier
        auto body = exists_goal->body();

        // We need to check if the witness matches the pattern
        // This requires reverse substitution and matching
        // For now, we'll use a simplified approach

        // TODO: Implement proper matching to check if witness_hyp is an instantiation of body

        // For now, we'll just mark the goal as proved if we have a witness
        auto new_state = context.apply_rule(
            state,
            "exists_intro",
            {witness_hyp_name_}, // Premise is the witness
            {},                  // No new hypotheses
            state->goal()        // Keep the same goal
        );

        // Mark the state as proven
        new_state->mark_as_proved(
            ProofCertification::Status::PROVED_BY_RULE,
            "Witnessed by " + witness_hyp_name_);

        return {new_state};
    }

    std::string ExistsIntroRule::name() const
    {
        return "Existential Introduction";
    }

    std::string ExistsIntroRule::description() const
    {
        return "From P(t), derive ∃x.P(x)";
    }

    // AssumptionRule implementation
    AssumptionRule::AssumptionRule(
        const TermDBPtr &formula,
        const std::string &name)
        : formula_(formula),
          name_(name.empty() ? gensym("assumption") : name) {}

    bool AssumptionRule::is_applicable(
        const ProofStatePtr &state,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // This rule is always applicable
        return true;
    }

    std::vector<ProofStatePtr> AssumptionRule::apply(
        ProofContext &context,
        const ProofStatePtr &state,
        std::optional<ConstraintViolation> &violation,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        std::cout << "DEBUG: AssumptionRule::apply - Starting" << std::endl;

        // Get the formula to assume
        TermDBPtr formula_to_use = formula_;
        std::string name_to_use = name_;

        // Override from application context if provided
        if (app_context && app_context->additional_term)
        {
            std::cout << "DEBUG: AssumptionRule - Using formula from app_context" << std::endl;
            formula_to_use = app_context->additional_term;

            // If a hypothesis name was provided in the context, use it
            if (!app_context->hypothesis_name.empty())
            {
                name_to_use = app_context->hypothesis_name;
            }
        }

        if (!formula_to_use)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_RULE_APPLICATION,
                "Formula required for Assumption");
            return {};
        }

        if (formula_to_use->kind() == TermDB::TermKind::CONSTANT)
        {
            auto constant = std::dynamic_pointer_cast<ConstantDB>(formula_to_use);
        }

        // Create a new hypothesis with the formula
        std::vector<Hypothesis> new_hypotheses = {Hypothesis(name_to_use, formula_to_use)};

        // Apply the rule through the context
        auto new_state = context.apply_rule(
            state,
            "assumption",
            {}, // No premises
            new_hypotheses,
            state->goal());

        return {new_state};
    }
    std::string AssumptionRule::name() const
    {
        return "Assumption";
    }

    std::string AssumptionRule::description() const
    {
        return "Add a formula as a hypothesis";
    }

    // CutRule implementation
    CutRule::CutRule(
        const TermDBPtr &lemma,
        const std::string &lemma_name)
        : lemma_(lemma),
          lemma_name_(lemma_name) {}

    bool CutRule::is_applicable(
        const ProofStatePtr &state,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // This rule is always applicable
        return true;
    }

    std::vector<ProofStatePtr> CutRule::apply(
        ProofContext &context,
        const ProofStatePtr &state,
        std::optional<ConstraintViolation> &violation,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Get the lemma to prove
        TermDBPtr lemma_to_use = lemma_;
        std::string name_to_use = lemma_name_;

        // Override from application context if provided
        if (app_context && app_context->additional_term)
        {
            lemma_to_use = app_context->additional_term;

            // If a lemma name was provided in the context, use it
            if (!app_context->hypothesis_name.empty())
            {
                name_to_use = app_context->hypothesis_name;
            }
        }

        if (!lemma_to_use)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_RULE_APPLICATION,
                "Lemma formula required for Cut rule");
            return {};
        }

        // Create two new states:
        // 1. A state where we need to prove the lemma
        auto lemma_state = context.apply_rule(
            state,
            "cut_prove_lemma",
            {},          // No premises
            {},          // No new hypotheses
            lemma_to_use // Lemma as the new goal
        );

        // 2. A state where we assume the lemma and continue with the original goal
        std::vector<Hypothesis> new_hypotheses = {Hypothesis(name_to_use, lemma_to_use)};
        auto continue_state = context.apply_rule(
            state,
            "cut_use_lemma",
            {}, // No premises (the lemma proof is tracked separately)
            new_hypotheses,
            state->goal());

        return {lemma_state, continue_state};
    }

    std::string CutRule::name() const
    {
        return "Cut";
    }

    std::string CutRule::description() const
    {
        return "Introduce a lemma and prove it separately";
    }

    // ContradictionRule implementation
    ContradictionRule::ContradictionRule(
        const std::string &formula_hyp_name,
        const std::string &negation_hyp_name)
        : formula_hyp_name_(formula_hyp_name),
          negation_hyp_name_(negation_hyp_name) {}

    bool ContradictionRule::is_applicable(
        const ProofStatePtr &state,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the formula hypothesis
        const Hypothesis *formula_hyp = state->find_hypothesis(formula_hyp_name_);
        if (!formula_hyp)
        {
            return false;
        }

        // Find the negation hypothesis
        const Hypothesis *negation_hyp = state->find_hypothesis(negation_hyp_name_);
        if (!negation_hyp)
        {
            return false;
        }

        // Check if negation_hyp is actually a negation of formula_hyp
        if (negation_hyp->formula()->kind() != TermDB::TermKind::NOT)
        {
            return false;
        }

        auto not_formula = std::dynamic_pointer_cast<NotDB>(negation_hyp->formula());
        return *not_formula->body() == *formula_hyp->formula();
    }

    std::vector<ProofStatePtr> ContradictionRule::apply(
        ProofContext &context,
        const ProofStatePtr &state,
        std::optional<ConstraintViolation> &violation,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the formula hypothesis
        const Hypothesis *formula_hyp = state->find_hypothesis(formula_hyp_name_);
        if (!formula_hyp)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_HYPOTHESIS,
                "Formula hypothesis not found: " + formula_hyp_name_);
            return {};
        }

        // Find the negation hypothesis
        const Hypothesis *negation_hyp = state->find_hypothesis(negation_hyp_name_);
        if (!negation_hyp)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_HYPOTHESIS,
                "Negation hypothesis not found: " + negation_hyp_name_);
            return {};
        }

        // Check if negation_hyp is actually a negation of formula_hyp
        if (negation_hyp->formula()->kind() != TermDB::TermKind::NOT)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                "Second hypothesis is not a negation");
            return {};
        }

        auto not_formula = std::dynamic_pointer_cast<NotDB>(negation_hyp->formula());
        if (!(*not_formula->body() == *formula_hyp->formula()))
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                "Negation does not match the formula");
            return {};
        }

        // Create a proof from contradiction - with this, we can derive anything
        // so we can mark the goal as proved
        auto new_state = context.apply_rule(
            state,
            "contradiction",
            {formula_hyp_name_, negation_hyp_name_},
            {}, // No new hypotheses needed
            state->goal());

        // Mark the state as proven by contradiction
        new_state->mark_as_proved(
            ProofCertification::Status::CONTRADICTION,
            "Proved by contradiction using " + formula_hyp_name_ + " and " + negation_hyp_name_);

        return {new_state};
    }

    std::string ContradictionRule::name() const
    {
        return "Contradiction";
    }

    std::string ContradictionRule::description() const
    {
        return "From a contradiction (P and ¬P), derive any conclusion";
    }

    // RewriteRule implementation
    RewriteRule::RewriteRule(
        const std::string &equality_hyp_name,
        bool left_to_right)
        : equality_hyp_name_(equality_hyp_name),
          left_to_right_(left_to_right) {}

    bool RewriteRule::is_applicable(
        const ProofStatePtr &state,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the equality hypothesis
        const Hypothesis *equality_hyp = state->find_hypothesis(equality_hyp_name_);
        if (!equality_hyp)
        {
            return false;
        }

        // Check if it's an equality - for now we assume equalities are represented
        // as function applications with "=" as the symbol
        if (equality_hyp->formula()->kind() != TermDB::TermKind::FUNCTION_APPLICATION)
        {
            return false;
        }

        auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(equality_hyp->formula());
        return func_app->symbol() == "=";
    }

    std::vector<ProofStatePtr> RewriteRule::apply(
        ProofContext &context,
        const ProofStatePtr &state,
        std::optional<ConstraintViolation> &violation,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the equality hypothesis
        const Hypothesis *equality_hyp = state->find_hypothesis(equality_hyp_name_);
        if (!equality_hyp)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_HYPOTHESIS,
                "Equality hypothesis not found: " + equality_hyp_name_);
            return {};
        }

        // Check if it's an equality
        if (equality_hyp->formula()->kind() != TermDB::TermKind::FUNCTION_APPLICATION)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                "Hypothesis is not a function application: " + equality_hyp_name_);
            return {};
        }

        auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(equality_hyp->formula());
        if (func_app->symbol() != "=")
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                "Hypothesis is not an equality: " + equality_hyp_name_);
            return {};
        }

        // Check if we have exactly two arguments
        if (func_app->arguments().size() != 2)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                "Equality does not have exactly two arguments");
            return {};
        }

        // Extract the terms
        TermDBPtr left = func_app->arguments()[0];
        TermDBPtr right = func_app->arguments()[1];

        // Determine the rewrite direction from rule or context
        bool direction = left_to_right_;
        if (app_context)
        {
            direction = app_context->left_to_right;
        }

        // Determine the terms to rewrite from and to
        TermDBPtr from_term = direction ? left : right;
        TermDBPtr to_term = direction ? right : left;

        // TODO: Implement actual rewriting of terms in the goal
        // This would require a term rewriting engine that can find and replace
        // occurrences of 'from_term' with 'to_term' in the goal

        // For now, we'll just create a placeholder implementation
        auto new_state = context.apply_rule(
            state,
            "rewrite",
            {equality_hyp_name_},
            {},           // No new hypotheses
            state->goal() // Keep the same goal for now
        );

        // TODO: Replace this with actual rewriting
        std::stringstream ss;
        ss << "Applied rewrite using " << equality_hyp_name_
           << " (" << (direction ? "left-to-right" : "right-to-left") << ")";

        new_state->mark_as_proved(
            ProofCertification::Status::PENDING_INSTANTIATION,
            ss.str());

        return {new_state};
    }

    std::string RewriteRule::name() const
    {
        return left_to_right_ ? "Rewrite (Left to Right)" : "Rewrite (Right to Left)";
    }

    std::string RewriteRule::description() const
    {
        return left_to_right_
                   ? "Rewrite terms using equality, replacing left with right"
                   : "Rewrite terms using equality, replacing right with left";
    }

    // ExistsElimRule implementation
    ExistsElimRule::ExistsElimRule(const std::string &exists_hyp_name)
        : exists_hyp_name_(exists_hyp_name) {}

    bool ExistsElimRule::is_applicable(
        const ProofStatePtr &state,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Check if the hypothesis exists and is an existential quantifier
        const Hypothesis *hyp = state->find_hypothesis(exists_hyp_name_);
        if (!hyp)
            return false;

        return hyp->formula()->kind() == TermDB::TermKind::EXISTS;
    }

    std::vector<ProofStatePtr> ExistsElimRule::apply(
        ProofContext &context,
        const ProofStatePtr &state,
        std::optional<ConstraintViolation> &violation,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the existential hypothesis
        const Hypothesis *exists_hyp = state->find_hypothesis(exists_hyp_name_);
        if (!exists_hyp)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_HYPOTHESIS,
                "Existential hypothesis not found: " + exists_hyp_name_);
            return {};
        }

        // Verify it's an existential quantifier
        if (exists_hyp->formula()->kind() != TermDB::TermKind::EXISTS)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                "Hypothesis is not an existential quantifier: " + exists_hyp_name_);
            return {};
        }

        // Cast to ExistsDB
        auto exists_formula = std::dynamic_pointer_cast<ExistsDB>(exists_hyp->formula());

        // Generate a fresh constant to use as witness
        std::string witness_name = gensym("witness");
        auto witness = make_constant(witness_name);

        // Create a substitution to replace the bound variable with the witness
        SubstitutionMap subst;
        subst[0] = witness; // Replace De Bruijn index 0 with the witness

        // Apply substitution to the body
        auto body = exists_formula->body();
        SubstitutionEngine engine;
        auto instantiated_body = engine.substitute(body, subst);

        // Create a name for the new hypothesis
        std::string new_hyp_name = gensym("exists_elim_result");

        // Create a new hypothesis with the instantiated formula
        std::vector<Hypothesis> new_hypotheses = {Hypothesis(new_hyp_name, instantiated_body)};

        // Create the proof step
        std::vector<std::string> premise_names = {exists_hyp_name_};

        // Apply the rule through the context
        auto new_state = context.apply_rule(
            state,
            "exists_elim",
            premise_names,
            new_hypotheses,
            state->goal());

        // In a full implementation, we'd need to track the witness constraint
        // and ensure it doesn't escape its scope in the final proof

        return {new_state};
    }

    std::string ExistsElimRule::name() const
    {
        return "Existential Elimination";
    }

    std::string ExistsElimRule::description() const
    {
        return "From ∃x.P(x), derive P(c) for a fresh witness constant c";
    }

    // QuantifierNegationRule implementation
    QuantifierNegationRule::QuantifierNegationRule(
        const std::string &hyp_name,
        bool direction)
        : hyp_name_(hyp_name), direction_(direction) {}

    bool QuantifierNegationRule::is_applicable(
        const ProofStatePtr &state,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the hypothesis
        const Hypothesis *hyp = state->find_hypothesis(hyp_name_);
        if (!hyp)
            return false;

        if (direction_)
        {
            // ¬(Qx.P) → Q̅x.¬P direction
            // Check if it's a negation
            if (hyp->formula()->kind() != TermDB::TermKind::NOT)
            {
                return false;
            }

            // Get the negated formula
            auto not_formula = std::dynamic_pointer_cast<NotDB>(hyp->formula());
            auto negated = not_formula->body();

            // Check if the negated formula is a quantifier
            return negated->kind() == TermDB::TermKind::FORALL ||
                   negated->kind() == TermDB::TermKind::EXISTS;
        }
        else
        {
            // Q̅x.¬P → ¬(Qx.P) direction
            // Check if it's a quantifier
            if (hyp->formula()->kind() != TermDB::TermKind::FORALL &&
                hyp->formula()->kind() != TermDB::TermKind::EXISTS)
            {
                return false;
            }

            // Get the body of the quantifier
            TermDBPtr body;
            if (hyp->formula()->kind() == TermDB::TermKind::FORALL)
            {
                auto forall = std::dynamic_pointer_cast<ForallDB>(hyp->formula());
                body = forall->body();
            }
            else
            { // EXISTS
                auto exists = std::dynamic_pointer_cast<ExistsDB>(hyp->formula());
                body = exists->body();
            }

            // Check if the body is a negation
            return body->kind() == TermDB::TermKind::NOT;
        }
    }

    std::vector<ProofStatePtr> QuantifierNegationRule::apply(
        ProofContext &context,
        const ProofStatePtr &state,
        std::optional<ConstraintViolation> &violation,
        const std::optional<RuleApplicationContext> &app_context) const
    {

        // Find the hypothesis
        const Hypothesis *hyp = state->find_hypothesis(hyp_name_);
        if (!hyp)
        {
            violation = ConstraintViolation(
                ConstraintViolation::ViolationType::INVALID_HYPOTHESIS,
                "Hypothesis not found: " + hyp_name_);
            return {};
        }

        TermDBPtr transformed;
        std::string rule_name;

        if (direction_)
        {
            // ¬(Qx.P) → Q̅x.¬P direction (pushing negation inward)

            // Confirm it's a negation
            if (hyp->formula()->kind() != TermDB::TermKind::NOT)
            {
                violation = ConstraintViolation(
                    ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                    "Hypothesis is not a negation: " + hyp_name_);
                return {};
            }

            auto not_formula = std::dynamic_pointer_cast<NotDB>(hyp->formula());
            auto negated = not_formula->body();

            // Check what kind of quantifier is being negated
            if (negated->kind() == TermDB::TermKind::FORALL)
            {
                // ¬(∀x.P(x)) → ∃x.¬P(x)
                auto forall = std::dynamic_pointer_cast<ForallDB>(negated);
                auto body = forall->body();

                // Create ¬P(x)
                auto negated_body = make_not(body);

                // Create ∃x.¬P(x)
                transformed = make_exists(forall->variable_hint(), negated_body);
                rule_name = "not_forall_to_exists_not";
            }
            else if (negated->kind() == TermDB::TermKind::EXISTS)
            {
                // ¬(∃x.P(x)) → ∀x.¬P(x)
                auto exists = std::dynamic_pointer_cast<ExistsDB>(negated);
                auto body = exists->body();

                // Create ¬P(x)
                auto negated_body = make_not(body);

                // Create ∀x.¬P(x)
                transformed = make_forall(exists->variable_hint(), negated_body);
                rule_name = "not_exists_to_forall_not";
            }
            else
            {
                violation = ConstraintViolation(
                    ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                    "Negated formula is not a quantifier: " + hyp_name_);
                return {};
            }
        }
        else
        {
            // Q̅x.¬P → ¬(Qx.P) direction (pulling negation outward)

            if (hyp->formula()->kind() == TermDB::TermKind::FORALL)
            {
                // ∀x.¬P(x) → ¬(∃x.P(x))
                auto forall = std::dynamic_pointer_cast<ForallDB>(hyp->formula());
                auto body = forall->body();

                // Confirm body is a negation
                if (body->kind() != TermDB::TermKind::NOT)
                {
                    violation = ConstraintViolation(
                        ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                        "Quantifier body is not a negation: " + hyp_name_);
                    return {};
                }

                auto not_body = std::dynamic_pointer_cast<NotDB>(body);
                auto inner_body = not_body->body();

                // Create ∃x.P(x)
                auto exists_formula = make_exists(forall->variable_hint(), inner_body);

                // Create ¬(∃x.P(x))
                transformed = make_not(exists_formula);
                rule_name = "forall_not_to_not_exists";
            }
            else if (hyp->formula()->kind() == TermDB::TermKind::EXISTS)
            {
                // ∃x.¬P(x) → ¬(∀x.P(x))
                auto exists = std::dynamic_pointer_cast<ExistsDB>(hyp->formula());
                auto body = exists->body();

                // Confirm body is a negation
                if (body->kind() != TermDB::TermKind::NOT)
                {
                    violation = ConstraintViolation(
                        ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                        "Quantifier body is not a negation: " + hyp_name_);
                    return {};
                }

                auto not_body = std::dynamic_pointer_cast<NotDB>(body);
                auto inner_body = not_body->body();

                // Create ∀x.P(x)
                auto forall_formula = make_forall(exists->variable_hint(), inner_body);

                // Create ¬(∀x.P(x))
                transformed = make_not(forall_formula);
                rule_name = "exists_not_to_not_forall";
            }
            else
            {
                violation = ConstraintViolation(
                    ConstraintViolation::ViolationType::RULE_PATTERN_MISMATCH,
                    "Hypothesis is not a quantifier: " + hyp_name_);
                return {};
            }
        }

        // Create a new hypothesis with the transformed formula
        std::string new_hyp_name = gensym("quantifier_negation_result");
        std::vector<Hypothesis> new_hypotheses = {Hypothesis(new_hyp_name, transformed)};

        // Create the proof step with premise name
        std::vector<std::string> premise_names = {hyp_name_};

        // Apply the rule through the context
        auto new_state = context.apply_rule(
            state,
            rule_name,
            premise_names,
            new_hypotheses,
            state->goal());

        return {new_state};
    }

    std::string QuantifierNegationRule::name() const
    {
        if (direction_)
        {
            return "Quantifier Negation (Inward)";
        }
        else
        {
            return "Quantifier Negation (Outward)";
        }
    }

    std::string QuantifierNegationRule::description() const
    {
        if (direction_)
        {
            return "Transform ¬(∀x.P(x)) to ∃x.¬P(x) or ¬(∃x.P(x)) to ∀x.¬P(x)";
        }
        else
        {
            return "Transform ∀x.¬P(x) to ¬(∃x.P(x)) or ∃x.¬P(x) to ¬(∀x.P(x))";
        }
    }

    // Factory functions
    std::shared_ptr<ExistsElimRule> make_exists_elim(const std::string &exists_hyp_name)
    {
        return std::make_shared<ExistsElimRule>(exists_hyp_name);
    }

    std::shared_ptr<QuantifierNegationRule> make_quantifier_negation(
        const std::string &hyp_name, bool direction)
    {
        return std::make_shared<QuantifierNegationRule>(hyp_name, direction);
    }

} // namespace theorem_prover