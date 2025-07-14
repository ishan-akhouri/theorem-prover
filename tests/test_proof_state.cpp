#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include "../src/proof/proof_state.hpp"
#include "../src/term/term_db.hpp"

using namespace theorem_prover;

// Utility for printing test results
#define TEST(name)                                   \
    std::cout << "Running test: " << name << "... "; \
    try                                              \
    {

#define END_TEST                                          \
    std::cout << "PASSED" << std::endl;                   \
    }                                                     \
    catch (const std::exception &e)                       \
    {                                                     \
        std::cout << "FAILED: " << e.what() << std::endl; \
        return false;                                     \
    }

// Create a simple propositional formula for testing
TermDBPtr create_proposition(const std::string &symbol)
{
    return make_constant(symbol);
}

// Create an implication formula: A -> B
TermDBPtr create_implication(const std::string &a, const std::string &b)
{
    auto A = create_proposition(a);
    auto B = create_proposition(b);
    return make_implies(A, B);
}

// Test creating initial proof states
bool test_initial_state_creation()
{
    TEST("Create initial state")
    auto A = create_proposition("A");
    auto state = ProofState::create_initial(A);

    assert(state->hypotheses().empty());
    assert(*state->goal() == *A);
    assert(state->parent() == nullptr);
    assert(state->last_step() == nullptr);
    assert(!state->is_proved());
    assert(state->depth() == 0);
    END_TEST

    return true;
}

// Test state derivation and hypothesis handling
bool test_state_derivation()
{
    TEST("Derive new state with hypotheses")
    auto A = create_proposition("A");
    auto B = create_proposition("B");

    auto initial = ProofState::create_initial(B);

    // Add a hypothesis A
    std::vector<Hypothesis> new_hyps = {Hypothesis("h1", A)};
    auto derived = ProofState::create_from(
        initial,
        "assume",
        {"h1"},
        new_hyps,
        B);

    assert(derived->hypotheses().size() == 1);
    assert(*derived->hypotheses()[0].formula() == *A);
    assert(derived->hypotheses()[0].name() == "h1");
    assert(*derived->goal() == *B);
    assert(derived->parent() == initial);
    assert(derived->last_step() != nullptr);
    assert(derived->last_step()->rule_name() == "assume");
    assert(!derived->is_proved());
    assert(derived->depth() == 1);
    END_TEST

    TEST("Find hypothesis by name")
    auto A = create_proposition("A");
    auto B = create_proposition("B");

    auto initial = ProofState::create_initial(B);

    // Add hypotheses A and B
    std::vector<Hypothesis> new_hyps = {
        Hypothesis("h1", A),
        Hypothesis("h2", B)};

    auto derived = ProofState::create_from(
        initial,
        "assume",
        {"h1", "h2"},
        new_hyps,
        B);

    const Hypothesis *h1 = derived->find_hypothesis("h1");
    const Hypothesis *h2 = derived->find_hypothesis("h2");
    const Hypothesis *h3 = derived->find_hypothesis("h3");

    assert(h1 != nullptr);
    assert(h2 != nullptr);
    assert(h3 == nullptr);
    assert(*h1->formula() == *A);
    assert(*h2->formula() == *B);
    END_TEST

    return true;
}

// Test proof status and certification
bool test_proof_status()
{
    TEST("Basic proof detection")
    auto A = create_proposition("A");

    auto initial = ProofState::create_initial(A);

    // Add hypothesis A (matching the goal)
    std::vector<Hypothesis> new_hyps = {Hypothesis("h1", A)};
    auto derived = ProofState::create_from(
        initial,
        "assume",
        {"h1"},
        new_hyps,
        A);

    // The state should be automatically detected as proved
    assert(derived->is_proved());
    END_TEST

    TEST("Explicit proof certification")
    auto A = create_proposition("A");
    auto B = create_proposition("B");

    auto initial = ProofState::create_initial(B);

    // Add hypothesis A (not matching the goal)
    std::vector<Hypothesis> new_hyps = {Hypothesis("h1", A)};
    auto derived = ProofState::create_from(
        initial,
        "assume",
        {"h1"},
        new_hyps,
        B);

    // Initially not proved
    assert(!derived->is_proved());

    // Explicitly mark as proved
    derived->mark_as_proved(
        ProofCertification::Status::PROVED_BY_RULE,
        "Special rule applied");

    // Now it should be proved
    assert(derived->is_proved());
    assert(derived->certification().status == ProofCertification::Status::PROVED_BY_RULE);
    assert(derived->certification().justification == "Special rule applied");
    END_TEST

    return true;
}

// Test metavariable handling
bool test_metavariables()
{
    TEST("Add and find metavariables")
    auto A = create_proposition("A");
    auto state = ProofState::create_initial(A);

    // Add a metavariable
    auto int_type = make_base_type("Int");
    state->add_metavariable("X", int_type);

    // Find the metavariable
    const MetavariableInfo *info = state->find_metavariable("X");
    assert(info != nullptr);
    assert(info->name == "X");
    assert(*info->type == *int_type);
    assert(!info->instantiated);

    // Non-existent metavariable
    assert(state->find_metavariable("Y") == nullptr);
    END_TEST

    TEST("Instantiate metavariables")
    auto A = create_proposition("A");
    auto state = ProofState::create_initial(A);

    // Add a metavariable
    auto int_type = make_base_type("Int");
    state->add_metavariable("X", int_type);

    // Initially has uninstantiated metavariables
    assert(state->has_uninstantiated_metavariables());

    // Instantiate the metavariable
    auto term = create_proposition("42");
    bool result = state->instantiate_metavariable("X", term);
    assert(result);

    // Now it should be instantiated
    assert(!state->has_uninstantiated_metavariables());

    // Check the instantiation
    const MetavariableInfo *info = state->find_metavariable("X");
    assert(info != nullptr);
    assert(info->instantiated);
    assert(*info->instantiation == *term);
    END_TEST

    TEST("Proof with uninstantiated metavariables")
    auto A = create_proposition("A");
    auto state = ProofState::create_initial(A);

    // Add a metavariable
    auto int_type = make_base_type("Int");
    state->add_metavariable("X", int_type);

    // Add hypothesis A (matching the goal)
    std::vector<Hypothesis> new_hyps = {Hypothesis("h1", A)};
    auto derived = ProofState::create_from(
        state,
        "assume",
        {"h1"},
        new_hyps,
        A);

    // The state shouldn't be proved due to uninstantiated metavariables
    assert(!derived->is_proved());

    // Mark as pending instantiation
    derived->mark_as_proved(
        ProofCertification::Status::PENDING_INSTANTIATION,
        "Proof complete but metavariables remain");

    // Still not fully proved
    assert(!derived->is_proved());

    // Instantiate the metavariable
    auto term = create_proposition("42");
    bool result = derived->instantiate_metavariable("X", term);
    assert(result);

    // Now it can be proved
    derived->mark_as_proved(
        ProofCertification::Status::PROVED_BY_RULE,
        "All metavariables instantiated");
    assert(derived->is_proved());
    END_TEST

    return true;
}

// Test proof trace generation
bool test_proof_trace()
{
    TEST("Generate proof trace")
    auto A = create_proposition("A");
    auto B = create_proposition("B");
    auto C = create_proposition("C");

    // Create a chain of proof states: A -> B -> C
    auto state1 = ProofState::create_initial(A);

    std::vector<Hypothesis> hyps1 = {Hypothesis("h1", A)};
    auto state2 = ProofState::create_from(state1, "rule1", {"h1"}, hyps1, B);

    std::vector<Hypothesis> hyps2 = {Hypothesis("h2", B)};
    auto state3 = ProofState::create_from(state2, "rule2", {"h2"}, hyps2, C);

    // Get the proof trace
    auto trace = state3->get_proof_trace();

    // Check the trace
    assert(trace.size() == 2);
    assert(trace[0]->rule_name() == "rule1");
    assert(trace[1]->rule_name() == "rule2");
    assert(*trace[0]->conclusion() == *B);
    assert(*trace[1]->conclusion() == *C);
    END_TEST

    return true;
}

// Test proof context functionalities
bool test_proof_context()
{
    TEST("Context state management")
    ProofContext context;

    auto A = create_proposition("A");
    auto B = create_proposition("B");

    // Create an initial state
    auto state1 = context.create_initial_state(A);
    assert(context.count() == 1);

    // Apply a rule
    std::vector<Hypothesis> hyps = {Hypothesis("h1", A)};
    auto state2 = context.apply_rule(state1, "rule1", {"h1"}, hyps, B);
    assert(context.count() == 2);

    // Find a state by hash
    auto found = context.find_state(state2->hash());
    assert(found == state2);

    // Non-existent state
    assert(context.find_state(12345) == nullptr);
    END_TEST

    TEST("Context leaf states")
    ProofContext context;

    auto A = create_proposition("A");
    auto B = create_proposition("B");
    auto C = create_proposition("C");

    // Create a tree of states:
    //      A
    //     / \
        //    B   C
    auto root = context.create_initial_state(A);

    std::vector<Hypothesis> hyps1 = {Hypothesis("h1", A)};
    auto leaf1 = context.apply_rule(root, "rule1", {"h1"}, hyps1, B);

    std::vector<Hypothesis> hyps2 = {Hypothesis("h2", A)};
    auto leaf2 = context.apply_rule(root, "rule2", {"h2"}, hyps2, C);

    // Get leaf states
    auto leaves = context.get_leaf_states();

    // Check the leaves
    assert(leaves.size() == 2);
    assert(std::find(leaves.begin(), leaves.end(), leaf1) != leaves.end());
    assert(std::find(leaves.begin(), leaves.end(), leaf2) != leaves.end());
    END_TEST

    TEST("Context proved states")
    ProofContext context;

    auto A = create_proposition("A");
    auto B = create_proposition("B");

    // Create a state that's not proved
    auto state1 = context.create_initial_state(A);

    // Create a state that's proved
    auto state2 = context.create_initial_state(B);
    std::vector<Hypothesis> hyps = {Hypothesis("h1", B)};
    auto proved = context.apply_rule(state2, "rule1", {"h1"}, hyps, B);

    // Get proved states
    auto proved_states = context.get_proved_states();

    // Check the proved states
    assert(proved_states.size() == 1);
    assert(proved_states[0] == proved);
    END_TEST

    return true;
}

// Test state equality and hashing
bool test_state_equality()
{
    TEST("State equality - identical states")
    auto A = create_proposition("A");
    auto B = create_proposition("B");

    auto state1 = ProofState::create_initial(A);
    std::vector<Hypothesis> hyps1 = {Hypothesis("h1", B)};
    auto derived1 = ProofState::create_from(state1, "rule1", {"h1"}, hyps1, A);

    auto state2 = ProofState::create_initial(A);
    std::vector<Hypothesis> hyps2 = {Hypothesis("h1", B)};
    auto derived2 = ProofState::create_from(state2, "rule1", {"h1"}, hyps2, A);

    assert(derived1->equals(*derived2));
    assert(derived1->hash() == derived2->hash());
    END_TEST

    TEST("State equality - different goals")
    auto A = create_proposition("A");
    auto B = create_proposition("B");

    auto state1 = ProofState::create_initial(A);
    auto state2 = ProofState::create_initial(B);

    assert(!state1->equals(*state2));
    END_TEST

    TEST("State equality - different hypotheses")
    auto A = create_proposition("A");
    auto B = create_proposition("B");
    auto C = create_proposition("C");

    auto state1 = ProofState::create_initial(A);
    std::vector<Hypothesis> hyps1 = {Hypothesis("h1", B)};
    auto derived1 = ProofState::create_from(state1, "rule1", {"h1"}, hyps1, A);

    auto state2 = ProofState::create_initial(A);
    std::vector<Hypothesis> hyps2 = {Hypothesis("h1", C)};
    auto derived2 = ProofState::create_from(state2, "rule1", {"h1"}, hyps2, A);

    assert(!derived1->equals(*derived2));
    END_TEST

    TEST("State equality - different metavariables")
    auto A = create_proposition("A");
    auto int_type = make_base_type("Int");
    auto bool_type = make_base_type("Bool");

    auto state1 = ProofState::create_initial(A);
    state1->add_metavariable("X", int_type);

    auto state2 = ProofState::create_initial(A);
    state2->add_metavariable("X", bool_type);

    assert(!state1->equals(*state2));
    END_TEST

    return true;
}

// Main test runner
int main()
{
    bool all_passed = true;

    std::cout << "===== Running Proof State Tests =====" << std::endl;

    all_passed &= test_initial_state_creation();
    all_passed &= test_state_derivation();
    all_passed &= test_proof_status();
    all_passed &= test_metavariables();
    all_passed &= test_proof_trace();
    all_passed &= test_proof_context();
    all_passed &= test_state_equality();

    if (all_passed)
    {
        std::cout << "===== All tests passed! =====" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "===== Some tests failed! =====" << std::endl;
        return 1;
    }
}