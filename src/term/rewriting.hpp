#pragma once

#include "term_db.hpp"
#include "ordering.hpp"
#include <memory>
#include <vector>
#include <unordered_set>
#include <optional>

namespace theorem_prover
{

    /**
     * @brief A rewrite rule represents an oriented equation l → r
     *
     * Invariant: l >_ord r in the term ordering (oriented correctly)
     * Used for both demodulation and as input to Knuth-Bendix completion.
     */
    class TermRewriteRule
    {
    public:
        /**
         * @brief Construct a rewrite rule l → r
         * @param lhs Left-hand side (should be greater than rhs in ordering)
         * @param rhs Right-hand side
         * @param name Optional name for the rule
         */
        TermRewriteRule(const TermDBPtr &lhs, const TermDBPtr &rhs,
                        const std::string &name = "");

        const TermDBPtr &lhs() const { return lhs_; }
        const TermDBPtr &rhs() const { return rhs_; }
        const std::string &name() const { return name_; }

        /**
         * @brief Check if the rule is properly oriented with respect to ordering
         * @param ordering Term ordering to check against
         * @return true if lhs > rhs in the ordering
         */
        bool is_oriented(const TermOrdering &ordering) const;

        /**
         * @brief Get the rule with sides potentially swapped to ensure proper orientation
         * @param ordering Term ordering to use for orientation
         * @return TermRewriteRule that is properly oriented, or nullopt if terms are equivalent
         */
        std::optional<TermRewriteRule> orient(const TermOrdering &ordering) const;

        /**
         * @brief Check equality of rules (ignoring names)
         */
        bool equals(const TermRewriteRule &other) const;

        /**
         * @brief String representation of the rule
         */
        std::string to_string() const;

    private:
        TermDBPtr lhs_;
        TermDBPtr rhs_;
        std::string name_;
    };

    /**
     * @brief Position in a term for rewriting
     *
     * Represents a path from the root to a subterm position.
     * Empty position represents the root.
     */
    class Position
    {
    public:
        Position() = default;
        explicit Position(const std::vector<size_t> &path) : path_(path) {}

        const std::vector<size_t> &path() const { return path_; }
        bool is_root() const { return path_.empty(); }
        size_t depth() const { return path_.size(); }

        /**
         * @brief Extend position by descending into argument i
         */
        Position descend(size_t i) const
        {
            std::vector<size_t> new_path = path_;
            new_path.push_back(i);
            return Position(new_path);
        }

        /**
         * @brief Check if this position is a prefix of another (this position is above other)
         */
        bool is_prefix_of(const Position &other) const;

        /**
         * @brief String representation
         */
        std::string to_string() const;

        bool operator==(const Position &other) const
        {
            return path_ == other.path_;
        }

    private:
        std::vector<size_t> path_;
    };

    /**
     * @brief Result of a rewrite operation
     */
    struct RewriteResult
    {
        bool success;
        TermDBPtr result;
        Position position;
        std::string rule_name;

        RewriteResult() : success(false) {}
        RewriteResult(const TermDBPtr &result, const Position &pos, const std::string &rule)
            : success(true), result(result), position(pos), rule_name(rule) {}

        static RewriteResult failure()
        {
            return RewriteResult();
        }

        static RewriteResult success_at(const TermDBPtr &result, const Position &pos,
                                        const std::string &rule)
        {
            return RewriteResult(result, pos, rule);
        }
    };

    /**
     * @brief Term rewriting system
     *
     * Manages a set of rewrite rules and provides operations for:
     * - Single-step rewriting (apply one rule at one position)
     * - Multi-step rewriting (rewrite to normal form)
     * - Position-specific rewriting
     * - Rule management and orientation
     */
    class RewriteSystem
    {
    public:
        /**
         * @brief Construct empty rewrite system with given ordering
         * @param ordering Term ordering for rule orientation and termination
         */
        explicit RewriteSystem(std::shared_ptr<TermOrdering> ordering);

        /**
         * @brief Add a rewrite rule (will be oriented automatically)
         * @param lhs Left-hand side
         * @param rhs Right-hand side
         * @param name Optional name for the rule
         * @return true if rule was added successfully
         */
        bool add_rule(const TermDBPtr &lhs, const TermDBPtr &rhs,
                      const std::string &name = "");

        /**
         * @brief Add a pre-oriented rewrite rule
         * @param rule The rewrite rule (must be properly oriented)
         * @return true if rule was added successfully
         */
        bool add_rule(const TermRewriteRule &rule);

        /**
         * @brief Remove a rule by name
         * @param name Name of the rule to remove
         * @return true if a rule was removed
         */
        bool remove_rule(const std::string &name);

        /**
         * @brief Get all rules in the system
         */
        const std::vector<TermRewriteRule> &rules() const { return rules_; }

        /**
         * @brief Clear all rules
         */
        void clear() { rules_.clear(); }

        /**
         * @brief Apply one rewrite step at any position in the term
         * @param term Term to rewrite
         * @return RewriteResult indicating success/failure and details
         */
        RewriteResult rewrite_step(const TermDBPtr &term) const;

        /**
         * @brief Apply one rewrite step at a specific position
         * @param term Term to rewrite
         * @param position Position where to attempt rewriting
         * @return RewriteResult indicating success/failure and details
         */
        RewriteResult rewrite_at(const TermDBPtr &term, const Position &position) const;

        /**
         * @brief Rewrite term to normal form (no further rewrites possible)
         * @param term Term to normalize
         * @param max_steps Maximum number of rewrite steps (prevents infinite loops)
         * @return Normalized term, or original term if max_steps exceeded
         */
        TermDBPtr normalize(const TermDBPtr &term, size_t max_steps = 1000) const;

        /**
         * @brief Check if a term is in normal form
         * @param term Term to check
         * @return true if no rewrite rules apply to any position in term
         */
        bool is_normal_form(const TermDBPtr &term) const;

        /**
         * @brief Get subterm at given position
         * @param term Root term
         * @param position Position of desired subterm
         * @return Subterm at position, or nullptr if position is invalid
         */
        static TermDBPtr subterm_at(const TermDBPtr &term, const Position &position);

        /**
         * @brief Replace subterm at given position
         * @param term Root term
         * @param position Position where to replace
         * @param replacement New subterm
         * @return New term with replacement, or nullptr if position is invalid
         */
        static TermDBPtr replace_at(const TermDBPtr &term, const Position &position,
                                    const TermDBPtr &replacement);

        /**
         * @brief Find all positions where a rewrite rule could apply
         * @param term Term to analyze
         * @param rule Rule to check applicability
         * @return Vector of positions where rule could be applied
         */
        std::vector<Position> find_redex_positions(const TermDBPtr &term,
                                                   const TermRewriteRule &rule) const;

        /**
         * @brief Check if two terms are joinable (have a common reduct)
         * @param term1 First term
         * @param term2 Second term
         * @param max_steps Maximum steps for normalization
         * @return true if normalize(term1) == normalize(term2)
         */
        bool joinable(const TermDBPtr &term1, const TermDBPtr &term2,
                      size_t max_steps = 1000) const;

    private:
        std::shared_ptr<TermOrdering> ordering_;
        std::vector<TermRewriteRule> rules_;

        /**
         * @brief Try to apply a specific rule at the root of a term
         * @param term Term to match against rule lhs
         * @param rule Rule to apply
         * @return Result term if successful, nullptr otherwise
         */
        TermDBPtr try_apply_rule(const TermDBPtr &term, const TermRewriteRule &rule) const;

        /**
         * @brief Generate fresh rule name
         */
        std::string generate_rule_name() const;
    };

    /**
     * @brief Equation class for representing unoriented equations
     *
     * Used as input to rewrite systems and Knuth-Bendix completion.
     * Can be converted to oriented rewrite rules.
     */
    class Equation
    {
    public:
        Equation(const TermDBPtr &lhs, const TermDBPtr &rhs, const std::string &name = "")
            : lhs_(lhs), rhs_(rhs), name_(name) {}

        const TermDBPtr &lhs() const { return lhs_; }
        const TermDBPtr &rhs() const { return rhs_; }
        const std::string &name() const { return name_; }

        /**
         * @brief Convert equation to oriented rewrite rule
         * @param ordering Term ordering for orientation
         * @return TermRewriteRule if terms can be oriented, nullopt if equivalent
         */
        std::optional<TermRewriteRule> orient(const TermOrdering &ordering) const;

        /**
         * @brief Check if equation is orientable (terms are not equivalent)
         * @param ordering Term ordering to check
         * @return true if lhs and rhs are not equivalent in the ordering
         */
        bool is_orientable(const TermOrdering &ordering) const;

        std::string to_string() const;

    private:
        TermDBPtr lhs_;
        TermDBPtr rhs_;
        std::string name_;
    };

    // Factory functions
    std::shared_ptr<RewriteSystem> make_rewrite_system(std::shared_ptr<TermOrdering> ordering);

} // namespace theorem_prover