#include "critical_pairs.hpp"
#include "../utils/gensym.hpp"
#include <algorithm>
#include <sstream>
#include <set>

namespace theorem_prover
{

    Equation CriticalPair::to_equation() const
    {
        std::string eq_name = "cp_" + rule1_name + "_" + rule2_name + "_" + position.to_string();
        return Equation(left, right, eq_name);
    }

    std::string CriticalPair::to_string() const
    {
        std::ostringstream oss;
        oss << "CP(" << rule1_name << "," << rule2_name << "@" << position.to_string() << "): ";
        oss << left->hash() << " =?= " << right->hash();
        return oss.str();
    }

    std::vector<CriticalPair> CriticalPairComputer::compute_critical_pairs(
        const TermRewriteRule &rule1,
        const TermRewriteRule &rule2)
    {

        std::vector<CriticalPair> pairs;

        // Rename variables to avoid conflicts
        auto renamed_rule1 = rename_rule_variables(rule1, 0);
        auto renamed_rule2 = rename_rule_variables(rule2, 1000);

        // Direction 1: rule1 overlaps with rule2
        auto overlaps_1_2 = find_overlap_positions(renamed_rule1, renamed_rule2);
        for (const auto &[position, unifier] : overlaps_1_2)
        {
            auto unified_rule1_rhs = SubstitutionEngine::substitute(renamed_rule1.rhs(), unifier);
            auto unified_rule2_lhs = SubstitutionEngine::substitute(renamed_rule2.lhs(), unifier);
            auto unified_rule2_rhs = SubstitutionEngine::substitute(renamed_rule2.rhs(), unifier);

            auto left_term = RewriteSystem::replace_at(unified_rule2_lhs, position, unified_rule1_rhs);
            auto right_term = unified_rule2_rhs;

            if (left_term && !(*left_term == *right_term))
            {
                pairs.emplace_back(left_term, right_term,
                                   renamed_rule1.name(), renamed_rule2.name(),
                                   position, unifier);
            }
        }

        // Direction 2: rule2 overlaps with rule1 (if rules are different)
        if (!(renamed_rule1.name() == renamed_rule2.name()))
        {
            auto overlaps_2_1 = find_overlap_positions(renamed_rule2, renamed_rule1);
            for (const auto &[position, unifier] : overlaps_2_1)
            {
                auto unified_rule2_rhs = SubstitutionEngine::substitute(renamed_rule2.rhs(), unifier);
                auto unified_rule1_lhs = SubstitutionEngine::substitute(renamed_rule1.lhs(), unifier);
                auto unified_rule1_rhs = SubstitutionEngine::substitute(renamed_rule1.rhs(), unifier);

                auto left_term = RewriteSystem::replace_at(unified_rule1_lhs, position, unified_rule2_rhs);
                auto right_term = unified_rule1_rhs;

                if (left_term && !(*left_term == *right_term))
                {
                    pairs.emplace_back(left_term, right_term,
                                       renamed_rule2.name(), renamed_rule1.name(),
                                       position, unifier);
                }
            }
        }

        return pairs;
    }

    std::vector<CriticalPair> CriticalPairComputer::compute_self_critical_pairs(
        const TermRewriteRule &rule)
    {

        std::vector<CriticalPair> pairs;

        // Rename variables to create two copies
        auto rule_copy1 = rename_rule_variables(rule, 0);
        auto rule_copy2 = rename_rule_variables(rule, 1000);

        // For self-overlapping, we need BOTH directions since it's the same rule
        // Direction 1: rule_copy1 overlaps with rule_copy2
        auto overlaps_1_2 = find_overlap_positions(rule_copy1, rule_copy2);
        for (const auto &[position, unifier] : overlaps_1_2)
        {
            // Skip root position for self-overlaps (would be trivial)
            if (position.is_root())
                continue;

            auto unified_rule1_rhs = SubstitutionEngine::substitute(rule_copy1.rhs(), unifier);
            auto unified_rule2_lhs = SubstitutionEngine::substitute(rule_copy2.lhs(), unifier);
            auto unified_rule2_rhs = SubstitutionEngine::substitute(rule_copy2.rhs(), unifier);

            auto left_term = RewriteSystem::replace_at(unified_rule2_lhs, position, unified_rule1_rhs);
            auto right_term = unified_rule2_rhs;

            if (left_term && !(*left_term == *right_term))
            {
                pairs.emplace_back(left_term, right_term,
                                   rule_copy1.name(), rule_copy2.name(),
                                   position, unifier);
            }
        }

        // Direction 2: rule_copy2 overlaps with rule_copy1
        auto overlaps_2_1 = find_overlap_positions(rule_copy2, rule_copy1);
        for (const auto &[position, unifier] : overlaps_2_1)
        {
            if (position.is_root())
                continue;

            auto unified_rule2_rhs = SubstitutionEngine::substitute(rule_copy2.rhs(), unifier);
            auto unified_rule1_lhs = SubstitutionEngine::substitute(rule_copy1.lhs(), unifier);
            auto unified_rule1_rhs = SubstitutionEngine::substitute(rule_copy1.rhs(), unifier);

            auto left_term = RewriteSystem::replace_at(unified_rule1_lhs, position, unified_rule2_rhs);
            auto right_term = unified_rule1_rhs;

            if (left_term && !(*left_term == *right_term))
            {
                pairs.emplace_back(left_term, right_term,
                                   rule_copy2.name(), rule_copy1.name(),
                                   position, unifier);
            }
        }

        return pairs;
    }

    std::vector<CriticalPair> CriticalPairComputer::compute_all_critical_pairs(
        const std::vector<TermRewriteRule> &rules)
    {

        std::vector<CriticalPair> all_pairs;

        // Compute pairs between distinct rules
        for (std::size_t i = 0; i < rules.size(); ++i)
        {
            for (std::size_t j = i + 1; j < rules.size(); ++j)
            {
                auto pairs_ij = compute_critical_pairs(rules[i], rules[j]);
                all_pairs.insert(all_pairs.end(), pairs_ij.begin(), pairs_ij.end());

                auto pairs_ji = compute_critical_pairs(rules[j], rules[i]);
                all_pairs.insert(all_pairs.end(), pairs_ji.begin(), pairs_ji.end());
            }

            // Compute self-overlaps
            auto self_pairs = compute_self_critical_pairs(rules[i]);
            all_pairs.insert(all_pairs.end(), self_pairs.begin(), self_pairs.end());
        }

        return all_pairs;
    }

    std::vector<std::pair<Position, SubstitutionMap>>
    CriticalPairComputer::find_overlap_positions(const TermRewriteRule &rule1,
                                                 const TermRewriteRule &rule2)
    {

        std::vector<std::pair<Position, SubstitutionMap>> overlaps;

        // Find all non-variable positions in rule2's lhs
        auto positions = find_non_variable_positions(rule2.lhs());

        for (const auto &pos : positions)
        {
            auto unifier = try_unify_at_position(rule1.lhs(), rule2.lhs(), pos);
            if (unifier)
            {
                overlaps.emplace_back(pos, *unifier);
            }
        }

        return overlaps;
    }

    std::optional<SubstitutionMap>
    CriticalPairComputer::try_unify_at_position(const TermDBPtr &term1,
                                                const TermDBPtr &term2,
                                                const Position &position)
    {

        auto subterm = RewriteSystem::subterm_at(term2, position);
        if (!subterm)
        {
            return std::nullopt;
        }

        auto unif_result = Unifier::unify(term1, subterm);
        if (unif_result.success)
        {
            return unif_result.substitution;
        }

        return std::nullopt;
    }

    TermRewriteRule CriticalPairComputer::rename_rule_variables(const TermRewriteRule &rule,
                                                                std::size_t offset)
    {

        SubstitutionMap renaming;

        // Find all variables in the rule (at depth 0, since these are top-level terms)
        std::set<std::size_t> lhs_vars = find_all_variables(rule.lhs(), 0);
        std::set<std::size_t> rhs_vars = find_all_variables(rule.rhs(), 0);

        std::set<std::size_t> all_vars;
        all_vars.insert(lhs_vars.begin(), lhs_vars.end());
        all_vars.insert(rhs_vars.begin(), rhs_vars.end());

        // Create renaming map: old_var -> new_var (old_var + offset)
        for (auto var : all_vars)
        {
            renaming[var] = make_variable(var + offset);
        }

        // Apply renaming (depth 0 since these are top-level terms)
        auto new_lhs = SubstitutionEngine::substitute(rule.lhs(), renaming, 0);
        auto new_rhs = SubstitutionEngine::substitute(rule.rhs(), renaming, 0);

        return TermRewriteRule(new_lhs, new_rhs, rule.name() + "_renamed");
    }

    std::vector<Position> CriticalPairComputer::find_non_variable_positions(const TermDBPtr &term)
    {
        std::vector<Position> positions;

        // Add root position - critical pairs can occur even with variables
        positions.push_back(Position());

        // Recursively find positions in subterms
        switch (term->kind())
        {
        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto func = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            for (std::size_t i = 0; i < func->arguments().size(); ++i)
            {
                auto sub_positions = find_non_variable_positions(func->arguments()[i]);
                for (const auto &sub_pos : sub_positions)
                {
                    // Create new position by extending current index with subposition
                    std::vector<std::size_t> new_path = {i};
                    for (std::size_t path_elem : sub_pos.path())
                    {
                        new_path.push_back(path_elem);
                    }
                    positions.push_back(Position(new_path));
                }
            }
            break;
        }

        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(term);

            // Left subterm positions
            auto left_positions = find_non_variable_positions(and_term->left());
            for (const auto &pos : left_positions)
            {
                std::vector<std::size_t> new_path = {0};
                for (std::size_t path_elem : pos.path())
                {
                    new_path.push_back(path_elem);
                }
                positions.push_back(Position(new_path));
            }

            // Right subterm positions
            auto right_positions = find_non_variable_positions(and_term->right());
            for (const auto &pos : right_positions)
            {
                std::vector<std::size_t> new_path = {1};
                for (std::size_t path_elem : pos.path())
                {
                    new_path.push_back(path_elem);
                }
                positions.push_back(Position(new_path));
            }
            break;
        }

        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(term);

            // Left subterm positions
            auto left_positions = find_non_variable_positions(or_term->left());
            for (const auto &pos : left_positions)
            {
                std::vector<std::size_t> new_path = {0};
                for (std::size_t path_elem : pos.path())
                {
                    new_path.push_back(path_elem);
                }
                positions.push_back(Position(new_path));
            }

            // Right subterm positions
            auto right_positions = find_non_variable_positions(or_term->right());
            for (const auto &pos : right_positions)
            {
                std::vector<std::size_t> new_path = {1};
                for (std::size_t path_elem : pos.path())
                {
                    new_path.push_back(path_elem);
                }
                positions.push_back(Position(new_path));
            }
            break;
        }

        default:
            break;
        }

        return positions;
    }

} // namespace theorem_prover