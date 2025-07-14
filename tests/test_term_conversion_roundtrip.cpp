#include "../src/term/term_db.hpp"
#include "../src/term/term_named.hpp"
#include "../src/term/substitution.hpp"
#include <cassert>
#include <iostream>

using namespace theorem_prover;

// Add this function to test_term_conversion_roundtrip.cpp
void debug_term_structure(const TermDBPtr &term, int depth = 0)
{
    std::string indent(depth * 2, ' ');

    switch (term->kind())
    {
    case TermDB::TermKind::VARIABLE:
    {
        auto var = std::dynamic_pointer_cast<VariableDB>(term);
        std::cout << indent << "Variable(index=" << var->index() << ")\n";
        break;
    }
    case TermDB::TermKind::CONSTANT:
    {
        auto constant = std::dynamic_pointer_cast<ConstantDB>(term);
        std::cout << indent << "Constant('" << constant->symbol() << "')\n";
        break;
    }
    case TermDB::TermKind::FUNCTION_APPLICATION:
    {
        auto app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
        std::cout << indent << "FunctionApplication('" << app->symbol() << "', [\n";
        for (const auto &arg : app->arguments())
        {
            debug_term_structure(arg, depth + 1);
        }
        std::cout << indent << "])\n";
        break;
    }
    case TermDB::TermKind::FORALL:
    {
        auto forall = std::dynamic_pointer_cast<ForallDB>(term);
        std::cout << indent << "Forall('" << forall->variable_hint() << "', \n";
        debug_term_structure(forall->body(), depth + 1);
        std::cout << indent << ")\n";
        break;
    }
    case TermDB::TermKind::EXISTS:
    {
        auto exists = std::dynamic_pointer_cast<ExistsDB>(term);
        std::cout << indent << "Exists('" << exists->variable_hint() << "', \n";
        debug_term_structure(exists->body(), depth + 1);
        std::cout << indent << ")\n";
        break;
    }
    case TermDB::TermKind::AND:
    {
        auto and_term = std::dynamic_pointer_cast<AndDB>(term);
        std::cout << indent << "And(\n";
        debug_term_structure(and_term->left(), depth + 1);
        debug_term_structure(and_term->right(), depth + 1);
        std::cout << indent << ")\n";
        break;
    }
    case TermDB::TermKind::OR:
    {
        auto or_term = std::dynamic_pointer_cast<OrDB>(term);
        std::cout << indent << "Or(\n";
        debug_term_structure(or_term->left(), depth + 1);
        debug_term_structure(or_term->right(), depth + 1);
        std::cout << indent << ")\n";
        break;
    }
    case TermDB::TermKind::NOT:
    {
        auto not_term = std::dynamic_pointer_cast<NotDB>(term);
        std::cout << indent << "Not(\n";
        debug_term_structure(not_term->body(), depth + 1);
        std::cout << indent << ")\n";
        break;
    }
    case TermDB::TermKind::IMPLIES:
    {
        auto implies = std::dynamic_pointer_cast<ImpliesDB>(term);
        std::cout << indent << "Implies(\n";
        debug_term_structure(implies->antecedent(), depth + 1);
        debug_term_structure(implies->consequent(), depth + 1);
        std::cout << indent << ")\n";
        break;
    }
    default:
        std::cout << indent << "Unknown term type\n";
        break;
    }
}

void test_roundtrip_conversion()
{
    // Test that substitution works correctly on terms converted from named to De Bruijn

    // Create a named term: ∀x. ∃y. P(x, y, z)
    auto x_named = make_named_variable("x");
    auto y_named = make_named_variable("y");
    auto z_named = make_named_variable("z");

    std::vector<TermNamedPtr> args = {x_named, y_named, z_named};
    auto p_named = make_named_function_application("P", args);

    auto exists_named = make_named_exists("y", p_named);
    auto forall_named = make_named_forall("x", exists_named);

    std::cout << "Original named term: ∀x. ∃y. P(x, y, z)\n";

    // Convert to De Bruijn
    auto db_term = to_db(forall_named);

    std::cout << "Converted to De Bruijn:\n";
    debug_term_structure(db_term);

    // Substitute z with a constant
    SubstitutionMap subst;
    // z should have index 2 in the De Bruijn representation
    subst[0] = make_constant("Z");

    std::cout << "Substituting index 2 with constant 'Z'\n";

    auto result = SubstitutionEngine::substitute(db_term, subst);

    std::cout << "After substitution:\n";
    debug_term_structure(result);

    // Convert back to named for easier verification
    auto result_named = to_named(result);

    // Verify that only z was substituted
    // The result should be: ∀x. ∃y. P(x, y, Z)
    auto x_verify = make_named_variable("x");
    auto y_verify = make_named_variable("y");
    auto z_verify = make_named_constant("Z");

    std::vector<TermNamedPtr> verify_args = {x_verify, y_verify, z_verify};
    auto p_verify = make_named_function_application("P", verify_args);

    auto exists_verify = make_named_exists("y", p_verify);
    auto forall_verify = make_named_forall("x", exists_verify);

    bool equals = *result_named == *forall_verify;
    std::cout << "Result equals expected: " << (equals ? "yes" : "no") << "\n";

    assert(*result_named == *forall_verify);
}

int main()
{
    std::cout << "Running term conversion roundtrip tests...\n";
    test_roundtrip_conversion();
    std::cout << "All term conversion roundtrip tests passed!\n";
    return 0;
}