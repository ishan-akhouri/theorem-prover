#include "clause.hpp"
#include "../utils/gensym.hpp"
#include "../utils/hash.hpp"
#include "../term/term_db.hpp"
#include "../term/substitution.hpp"
#include "../term/rewriting.hpp"
#include "../term/unification.hpp"
#include <set>
#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace theorem_prover
{

    Literal::Literal(const TermDBPtr &atom, bool positive)
        : atom_(atom), positive_(positive) {}

    Literal Literal::negate() const
    {
        return Literal(atom_, !positive_);
    }

    bool Literal::is_complementary(const Literal &other) const
    {
        return positive_ != other.positive_ && *atom_ == *other.atom_;
    }

    bool Literal::equals(const Literal &other) const
    {
        return positive_ == other.positive_ && *atom_ == *other.atom_;
    }

    std::size_t Literal::hash() const
    {
        std::size_t h = atom_->hash();
        hash_combine(h, positive_);
        return h;
    }

    std::string Literal::to_string() const
    {
        std::string result = positive_ ? "" : "¬";

        // Add basic term printing
        switch (atom_->kind())
        {
        case TermDB::TermKind::CONSTANT:
        {
            auto constant = std::dynamic_pointer_cast<ConstantDB>(atom_);
            result += constant->symbol();
            break;
        }
        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto func = std::dynamic_pointer_cast<FunctionApplicationDB>(atom_);
            result += func->symbol() + "(";
            for (size_t i = 0; i < func->arguments().size(); ++i)
            {
                if (i > 0)
                    result += ",";

                // Simple argument printing
                auto arg = func->arguments()[i];
                if (arg->kind() == TermDB::TermKind::CONSTANT)
                {
                    auto const_arg = std::dynamic_pointer_cast<ConstantDB>(arg);
                    result += const_arg->symbol();
                }
                else if (arg->kind() == TermDB::TermKind::VARIABLE)
                {
                    auto var_arg = std::dynamic_pointer_cast<VariableDB>(arg);
                    result += "X" + std::to_string(var_arg->index());
                }
                else
                {
                    result += "?";
                }
            }
            result += ")";
            break;
        }
        case TermDB::TermKind::VARIABLE:
        {
            auto variable = std::dynamic_pointer_cast<VariableDB>(atom_);
            result += "X" + std::to_string(variable->index());
            break;
        }
        default:
            result += "atom";
        }

        return result;
    }

    // Clause implementation
    Clause::Clause(const std::vector<Literal> &literals)
        : literals_(literals), hash_computed_(false) {}

    Clause::Clause(const std::vector<LiteralPtr> &literals)
        : hash_computed_(false)
    {
        for (const auto &lit_ptr : literals)
        {
            literals_.push_back(*lit_ptr);
        }
    }

    bool Clause::is_tautology() const
    {
        for (std::size_t i = 0; i < literals_.size(); ++i)
        {
            for (std::size_t j = i + 1; j < literals_.size(); ++j)
            {
                if (literals_[i].is_complementary(literals_[j]))
                {
                    return true;
                }
            }
        }
        return false;
    }

    Clause Clause::simplify() const
    {
        if (is_tautology())
        {
            return Clause(); // Return empty clause for tautologies
        }

        // Remove duplicates
        std::vector<Literal> unique_literals;
        for (const auto &lit : literals_)
        {
            bool found = false;
            for (const auto &existing : unique_literals)
            {
                if (lit.equals(existing))
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                unique_literals.push_back(lit);
            }
        }

        return Clause(unique_literals);
    }

    Clause Clause::substitute(const SubstitutionMap &subst) const
    {
        std::vector<Literal> new_literals;
        new_literals.reserve(literals_.size());

        for (const auto &lit : literals_)
        {
            auto new_atom = SubstitutionEngine::substitute(lit.atom(), subst);
            new_literals.emplace_back(new_atom, lit.is_positive());
        }

        return Clause(new_literals);
    }

    Clause Clause::rename_variables(std::size_t offset) const
    {
        SubstitutionMap renaming;

        // Find all variables in all literals
        std::set<std::size_t> all_variables;
        for (const auto &lit : literals_)
        {
            auto lit_vars = find_all_variables(lit.atom());
            all_variables.insert(lit_vars.begin(), lit_vars.end());
        }

        // Create renaming substitution: old_var -> (old_var + offset)
        for (std::size_t var_idx : all_variables)
        {
            renaming[var_idx] = make_variable(var_idx + offset);
        }

        // Apply renaming
        return substitute(renaming);
    }

    bool Clause::equals(const Clause &other) const
    {
        if (literals_.size() != other.literals_.size())
        {
            return false;
        }

        // Check if all literals match (order independent)
        for (const auto &lit1 : literals_)
        {
            bool found = false;
            for (const auto &lit2 : other.literals_)
            {
                if (lit1.equals(lit2))
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                return false;
            }
        }

        return true;
    }

    std::size_t Clause::hash() const
    {
        if (!hash_computed_)
        {
            compute_hash();
        }
        return hash_cache_;
    }

    void Clause::compute_hash() const
    {
        hash_cache_ = 0;
        for (const auto &lit : literals_)
        {
            hash_combine(hash_cache_, lit.hash());
        }
        hash_computed_ = true;
    }

    std::string Clause::to_string() const
    {
        if (literals_.empty())
        {
            return "□"; // Empty clause symbol
        }

        std::ostringstream oss;
        for (std::size_t i = 0; i < literals_.size(); ++i)
        {
            if (i > 0)
                oss << " ∨ ";
            oss << literals_[i].to_string();
        }
        return oss.str();
    }

    ResolutionResult ResolutionInference::resolve(const ClausePtr &clause1,
                                                  const ClausePtr &clause2)
    {
        // Try resolving on each pair of complementary literals
        for (std::size_t i = 0; i < clause1->literals().size(); ++i)
        {
            for (std::size_t j = 0; j < clause2->literals().size(); ++j)
            {
                auto result = resolve_on_literals(clause1, clause2, i, j);
                if (result.success)
                {
                    return result;
                }
            }
        }

        return ResolutionResult::make_failure("No resolvable literal pairs found");
    }

    ResolutionResult ResolutionInference::resolve_on_literals(const ClausePtr &clause1,
                                                              const ClausePtr &clause2,
                                                              std::size_t lit1_idx,
                                                              std::size_t lit2_idx)
    {
        const auto &lit1 = clause1->literals()[lit1_idx];
        const auto &lit2 = clause2->literals()[lit2_idx];

        // Literals must have opposite polarities to resolve
        if (lit1.is_positive() == lit2.is_positive())
        {
            return ResolutionResult::make_failure("Literals have same polarity");
        }

        // Try to unify the atoms
        auto unif_result = Unifier::unify(lit1.atom(), lit2.atom());
        if (!unif_result.success)
        {
            return ResolutionResult::make_failure("Atoms do not unify");
        }

        // Create resolvent by combining remaining literals
        std::vector<Literal> resolvent_literals;

        // Add literals from clause1 (except the resolved one)
        for (std::size_t i = 0; i < clause1->literals().size(); ++i)
        {
            if (i != lit1_idx)
            {
                auto lit = clause1->literals()[i];
                auto new_atom = SubstitutionEngine::substitute(lit.atom(), unif_result.substitution);
                resolvent_literals.emplace_back(new_atom, lit.is_positive());
            }
        }

        // Add literals from clause2 (except the resolved one)
        for (std::size_t j = 0; j < clause2->literals().size(); ++j)
        {
            if (j != lit2_idx)
            {
                auto lit = clause2->literals()[j];
                auto new_atom = SubstitutionEngine::substitute(lit.atom(), unif_result.substitution);
                resolvent_literals.emplace_back(new_atom, lit.is_positive());
            }
        }

        auto resolvent = std::make_shared<Clause>(resolvent_literals);
        resolvent = std::make_shared<Clause>(resolvent->simplify());

        return ResolutionResult::make_success(resolvent);
    }

    ClausePtr ResolutionInference::factor(const ClausePtr &clause)
    {
        std::vector<Literal> factored_literals;

        for (std::size_t i = 0; i < clause->literals().size(); ++i)
        {
            const auto &lit1 = clause->literals()[i];
            bool kept = false;

            // Try to unify with existing literals of same polarity
            for (std::size_t j = 0; j < factored_literals.size(); ++j)
            {
                const auto &lit2 = factored_literals[j];

                if (lit1.is_positive() == lit2.is_positive())
                {
                    auto unif_result = Unifier::unify(lit1.atom(), lit2.atom());
                    if (unif_result.success)
                    {
                        // Replace with unified version
                        auto new_atom = SubstitutionEngine::substitute(lit1.atom(), unif_result.substitution);
                        factored_literals[j] = Literal(new_atom, lit1.is_positive());
                        kept = true;
                        break;
                    }
                }
            }

            if (!kept)
            {
                factored_literals.push_back(lit1);
            }
        }

        return std::make_shared<Clause>(factored_literals);
    }

    std::size_t ResolutionInference::find_max_variable_index(const ClausePtr &clause1,
                                                             const ClausePtr &clause2)
    {
        // Would need to traverse terms to find maximum variable index
        // Simplified for now
        return 100; // Placeholder
    }

    bool Clause::subsumes(const Clause &other) const
    {
        return subsumes(std::make_shared<Clause>(*this),
                        std::make_shared<Clause>(other));
    }

    bool Clause::subsumes(const ClausePtr &c1, const ClausePtr &c2)
    {
        // Quick checks first
        if (!c1 || !c2)
            return false;
        if (c1->size() > c2->size())
            return false;
        if (c1->is_empty())
            return true; // Empty clause subsumes everything
        if (c2->is_empty())
            return false; // Nothing subsumes empty clause except empty

        // Try unification-based subsumption
        return try_all_literal_mappings(c1, c2);
    }

    bool Clause::try_all_literal_mappings(const ClausePtr &c1, const ClausePtr &c2)
    {
        std::vector<int> mapping(c1->size(), -1);
        std::vector<bool> used(c2->size(), false);

        return find_consistent_mapping(c1, c2, 0, mapping, used);
    }

    bool Clause::find_consistent_mapping(const ClausePtr &c1, const ClausePtr &c2,
                                         int lit_idx, std::vector<int> &mapping,
                                         std::vector<bool> &used)
    {
        // Base case: all literals in c1 have been mapped
        if (lit_idx == static_cast<int>(c1->size()))
        {
            return check_substitution_consistency(c1, c2, mapping);
        }

        const auto &lit1 = c1->literals()[lit_idx];

        // Try mapping lit1 to each unused literal in c2
        for (int i = 0; i < static_cast<int>(c2->size()); ++i)
        {
            if (used[i])
                continue;

            const auto &lit2 = c2->literals()[i];

            // Quick check: can these literals potentially unify?
            if (!can_unify_literals(lit1, lit2))
                continue;

            // Try this mapping
            mapping[lit_idx] = i;
            used[i] = true;

            if (find_consistent_mapping(c1, c2, lit_idx + 1, mapping, used))
            {
                return true;
            }

            // Backtrack
            used[i] = false;
        }

        return false;
    }

    bool Clause::check_substitution_consistency(const ClausePtr &c1, const ClausePtr &c2,
                                                const std::vector<int> &mapping)
    {
        SubstitutionMap global_substitution;

        // For each mapped pair of literals, check if they unify consistently
        for (size_t i = 0; i < mapping.size(); ++i)
        {
            const auto &lit1 = c1->literals()[i];
            const auto &lit2 = c2->literals()[mapping[i]];

            // Literals must have same polarity to unify
            if (lit1.is_positive() != lit2.is_positive())
            {
                return false;
            }

            // Try to unify the atoms
            auto unif_result = Unifier::unify(lit1.atom(), lit2.atom());
            if (!unif_result.success)
            {
                return false;
            }

            // Check if this unification is consistent with previous ones
            for (const auto &[var, term] : unif_result.substitution)
            {
                auto existing_it = global_substitution.find(var);
                if (existing_it != global_substitution.end())
                {
                    // Variable already has a binding - check consistency
                    if (!(*existing_it->second == *term))
                    {
                        return false; // Inconsistent bindings
                    }
                }
                else
                {
                    // New binding
                    global_substitution[var] = term;
                }
            }
        }

        // All unifications are consistent
        return true;
    }

    bool Clause::can_unify_literals(const Literal &lit1, const Literal &lit2)
    {
        // Literals must have same polarity
        if (lit1.is_positive() != lit2.is_positive())
        {
            return false;
        }

        // Quick unifiability check on atoms
        return Unifier::unifiable(lit1.atom(), lit2.atom());
    }

    // ParamodulationInference implementation
    ResolutionResult ParamodulationInference::paramodulate(const ClausePtr &equality_clause,
                                                           const ClausePtr &target_clause,
                                                           std::size_t eq_lit_idx,
                                                           std::size_t target_lit_idx,
                                                           const Position &position)
    {
        // Get the equality literal
        if (eq_lit_idx >= equality_clause->literals().size())
        {
            return ResolutionResult::make_failure("Invalid equality literal index");
        }

        const auto &eq_literal = equality_clause->literals()[eq_lit_idx];

        // Check if it's an equality
        if (!is_equality(eq_literal.atom()))
        {
            return ResolutionResult::make_failure("Literal is not an equality");
        }

        // Get the target literal
        if (target_lit_idx >= target_clause->literals().size())
        {
            return ResolutionResult::make_failure("Invalid target literal index");
        }

        const auto &target_literal = target_clause->literals()[target_lit_idx];

        // Get equality sides
        auto [left_side, right_side] = get_equality_sides(eq_literal.atom());

        // Get subterm at position in target literal
        auto subterm = RewriteSystem::subterm_at(target_literal.atom(), position);
        if (!subterm)
        {
            return ResolutionResult::make_failure("Invalid position in target literal");
        }

        // Try to unify subterm with left side of equality
        auto unif_result = Unifier::unify(left_side, subterm);
        if (!unif_result.success)
        {
            // Try the other direction (right side with subterm)
            unif_result = Unifier::unify(right_side, subterm);
            if (!unif_result.success)
            {
                return ResolutionResult::make_failure("Cannot unify equality with subterm");
            }
            // Swap sides if we unified with right side
            std::swap(left_side, right_side);
        }

        // Apply substitution to get the replacement term
        auto replacement = SubstitutionEngine::substitute(right_side, unif_result.substitution);

        // Apply the equality substitution to the target literal
        auto new_atom = apply_equality_at_position(target_literal.atom(), position,
                                                   subterm, replacement, unif_result.substitution);
        if (!new_atom)
        {
            return ResolutionResult::make_failure("Failed to apply equality substitution");
        }

        // Create new literal with the rewritten atom
        Literal new_target_literal(new_atom, target_literal.is_positive());

        // Build the paramodulant clause
        std::vector<Literal> paramodulant_literals;

        // Add all literals from equality clause except the equality literal (if positive)
        for (std::size_t i = 0; i < equality_clause->literals().size(); ++i)
        {
            if (i != eq_lit_idx || !eq_literal.is_positive())
            {
                auto lit = equality_clause->literals()[i];
                auto new_atom_subst = SubstitutionEngine::substitute(lit.atom(), unif_result.substitution);
                paramodulant_literals.emplace_back(new_atom_subst, lit.is_positive());
            }
        }

        // Add all literals from target clause except the target literal
        for (std::size_t i = 0; i < target_clause->literals().size(); ++i)
        {
            if (i != target_lit_idx)
            {
                auto lit = target_clause->literals()[i];
                auto new_atom_subst = SubstitutionEngine::substitute(lit.atom(), unif_result.substitution);
                paramodulant_literals.emplace_back(new_atom_subst, lit.is_positive());
            }
        }

        // Add the rewritten target literal
        paramodulant_literals.push_back(new_target_literal);

        auto paramodulant = std::make_shared<Clause>(paramodulant_literals);
        paramodulant = std::make_shared<Clause>(paramodulant->simplify());

        return ResolutionResult::make_success(paramodulant);
    }

    std::vector<std::tuple<std::size_t, Position, TermDBPtr>>
    ParamodulationInference::find_paramod_positions(const ClausePtr &clause)
    {
        std::vector<std::tuple<std::size_t, Position, TermDBPtr>> positions;

        for (std::size_t i = 0; i < clause->literals().size(); ++i)
        {
            const auto &literal = clause->literals()[i];

            // Find all subterm positions in this literal
            std::function<void(const TermDBPtr &, const Position &)> find_positions =
                [&](const TermDBPtr &term, const Position &pos)
            {
                positions.emplace_back(i, pos, term);

                // Recursively find positions in subterms
                switch (term->kind())
                {
                case TermDB::TermKind::FUNCTION_APPLICATION:
                {
                    auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
                    for (std::size_t j = 0; j < func_app->arguments().size(); ++j)
                    {
                        find_positions(func_app->arguments()[j], pos.descend(j));
                    }
                    break;
                }
                default:
                    // Variables and constants have no subterms
                    break;
                }
            };

            find_positions(literal.atom(), Position());
        }

        return positions;
    }

    TermDBPtr ParamodulationInference::apply_equality_at_position(const TermDBPtr &term,
                                                                  const Position &position,
                                                                  const TermDBPtr &from_term,
                                                                  const TermDBPtr &to_term,
                                                                  const SubstitutionMap &substitution)
    {
        // Apply substitution to the replacement term
        auto replacement = SubstitutionEngine::substitute(to_term, substitution);

        // Use the rewrite system's replace_at function
        return RewriteSystem::replace_at(term, position, replacement);
    }

    std::vector<ClausePtr> ResolutionWithParamodulation::resolve_with_paramodulation(
        const ClausePtr &clause1, const ClausePtr &clause2)
    {

        std::vector<ClausePtr> results;

        // Try standard resolution first
        auto resolution_results = try_resolution(clause1, clause2);
        results.insert(results.end(), resolution_results.begin(), resolution_results.end());

        // Try paramodulation if either clause has equalities
        if (has_equality_literals(clause1) || has_equality_literals(clause2))
        {
            auto paramod_results = try_paramodulation(clause1, clause2);
            results.insert(results.end(), paramod_results.begin(), paramod_results.end());
        }

        return results;
    }

    std::vector<ClausePtr> ResolutionWithParamodulation::try_resolution(
        const ClausePtr &clause1, const ClausePtr &clause2)
    {

        std::vector<ClausePtr> results;

        // Use existing resolution method
        auto resolution_result = ResolutionInference::resolve(clause1, clause2);
        if (resolution_result.success)
        {
            results.push_back(resolution_result.resolvent);
        }

        return results;
    }

    std::vector<ClausePtr> ResolutionWithParamodulation::try_paramodulation(
        const ClausePtr &clause1, const ClausePtr &clause2)
    {

        std::vector<ClausePtr> results;

        // Try clause1 as equality, clause2 as target
        if (has_equality_literals(clause1))
        {
            auto eq_indices = get_equality_literal_indices(clause1);
            auto target_positions = ParamodulationInference::find_paramod_positions(clause2);

            for (auto eq_idx : eq_indices)
            {
                // Only use positive equality literals for paramodulation
                if (clause1->literals()[eq_idx].is_positive())
                {
                    for (const auto &[target_lit_idx, position, subterm] : target_positions)
                    {
                        auto paramod_result = ParamodulationInference::paramodulate(
                            clause1, clause2, eq_idx, target_lit_idx, position);

                        if (paramod_result.success)
                        {
                            results.push_back(paramod_result.resolvent);
                        }
                    }
                }
            }
        }

        // Try clause2 as equality, clause1 as target
        if (has_equality_literals(clause2))
        {
            auto eq_indices = get_equality_literal_indices(clause2);
            auto target_positions = ParamodulationInference::find_paramod_positions(clause1);

            for (auto eq_idx : eq_indices)
            {
                // Only use positive equality literals for paramodulation
                if (clause2->literals()[eq_idx].is_positive())
                {
                    for (const auto &[target_lit_idx, position, subterm] : target_positions)
                    {
                        auto paramod_result = ParamodulationInference::paramodulate(
                            clause2, clause1, eq_idx, target_lit_idx, position);

                        if (paramod_result.success)
                        {
                            results.push_back(paramod_result.resolvent);
                        }
                    }
                }
            }
        }

        return results;
    }

    bool ResolutionWithParamodulation::has_equality_literals(const ClausePtr &clause)
    {
        for (const auto &literal : clause->literals())
        {
            if (is_equality(literal.atom()))
            {
                return true;
            }
        }
        return false;
    }

    std::vector<std::size_t> ResolutionWithParamodulation::get_equality_literal_indices(
        const ClausePtr &clause)
    {

        std::vector<std::size_t> indices;

        for (std::size_t i = 0; i < clause->literals().size(); ++i)
        {
            if (is_equality(clause->literals()[i].atom()))
            {
                indices.push_back(i);
            }
        }

        return indices;
    }
} // namespace theorem_prover