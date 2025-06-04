#pragma once

#include <memory>
#include <string>
#include <vector>
#include "term_db.hpp"

namespace theorem_prover
{

    /**
     * Base class for human-readable named terms
     *
     * This representation uses named variables instead of De Bruijn indices
     * for better readability in user interfaces.
     */
    class TermNamed
    {
    public:
        enum class TermKind
        {
            VARIABLE,
            CONSTANT,
            FUNCTION_APPLICATION,
            FORALL,
            EXISTS,
            AND,
            OR,
            NOT,
            IMPLIES
        };

        virtual ~TermNamed() = default;

        // Core operations
        virtual TermKind kind() const = 0;
        virtual bool equals(const TermNamed &other) const = 0;
        virtual std::size_t hash() const = 0;
        virtual std::shared_ptr<TermNamed> clone() const = 0;

        // Optional type annotation (for future extension)
        virtual TypePtr type() const { return nullptr; }
        virtual void set_type(TypePtr type) {}

        // Utility for structural equality
        bool operator==(const TermNamed &other) const
        {
            return equals(other);
        }

        // Helper for implementing equals() in derived classes
        template <typename T>
        static bool equal_cast(const TermNamed &self, const TermNamed &other,
                               std::function<bool(const T &, const T &)> cmp)
        {
            if (self.kind() != other.kind())
                return false;
            const T *other_cast = dynamic_cast<const T *>(&other);
            if (!other_cast)
                return false;
            return cmp(static_cast<const T &>(self), *other_cast);
        }
    };

    // Explicit enum mappings between named and De Bruijn representations
    inline TermDB::TermKind to_db_kind(TermNamed::TermKind kind)
    {
        switch (kind)
        {
        case TermNamed::TermKind::VARIABLE:
            return TermDB::TermKind::VARIABLE;
        case TermNamed::TermKind::CONSTANT:
            return TermDB::TermKind::CONSTANT;
        case TermNamed::TermKind::FUNCTION_APPLICATION:
            return TermDB::TermKind::FUNCTION_APPLICATION;
        case TermNamed::TermKind::FORALL:
            return TermDB::TermKind::FORALL;
        case TermNamed::TermKind::EXISTS:
            return TermDB::TermKind::EXISTS;
        case TermNamed::TermKind::AND:
            return TermDB::TermKind::AND;
        case TermNamed::TermKind::OR:
            return TermDB::TermKind::OR;
        case TermNamed::TermKind::NOT:
            return TermDB::TermKind::NOT;
        case TermNamed::TermKind::IMPLIES:
            return TermDB::TermKind::IMPLIES;
        default:
            return TermDB::TermKind::UNKNOWN;
        }
    }

    inline TermNamed::TermKind to_named_kind(TermDB::TermKind kind)
    {
        switch (kind)
        {
        case TermDB::TermKind::VARIABLE:
            return TermNamed::TermKind::VARIABLE;
        case TermDB::TermKind::CONSTANT:
            return TermNamed::TermKind::CONSTANT;
        case TermDB::TermKind::FUNCTION_APPLICATION:
            return TermNamed::TermKind::FUNCTION_APPLICATION;
        case TermDB::TermKind::FORALL:
            return TermNamed::TermKind::FORALL;
        case TermDB::TermKind::EXISTS:
            return TermNamed::TermKind::EXISTS;
        case TermDB::TermKind::AND:
            return TermNamed::TermKind::AND;
        case TermDB::TermKind::OR:
            return TermNamed::TermKind::OR;
        case TermDB::TermKind::NOT:
            return TermNamed::TermKind::NOT;
        case TermDB::TermKind::IMPLIES:
            return TermNamed::TermKind::IMPLIES;
        default:
            // Handle cases like ABSTRACTION, LET, etc. that don't have named equivalents yet
            throw std::runtime_error("No named equivalent for DB kind: " +
                                     std::to_string(static_cast<int>(kind)));
        }
    }

    using TermNamedPtr = std::shared_ptr<TermNamed>;

    /**
     * Named variable (x, y, z, etc.)
     */
    class VariableNamed : public TermNamed
    {
    public:
        explicit VariableNamed(const std::string &name);

        TermKind kind() const override { return TermKind::VARIABLE; }
        bool equals(const TermNamed &other) const override;
        std::size_t hash() const override;
        TermNamedPtr clone() const override;

        const std::string &name() const { return name_; }

        // Type accessors
        TypePtr type() const override { return type_; }
        void set_type(TypePtr type) override { type_ = type; }

    private:
        std::string name_;
        TypePtr type_;
    };

    /**
     * Constant symbol (true, false, constants, etc.)
     */
    class ConstantNamed : public TermNamed
    {
    public:
        explicit ConstantNamed(const std::string &name);

        TermKind kind() const override { return TermKind::CONSTANT; }
        bool equals(const TermNamed &other) const override;
        std::size_t hash() const override;
        TermNamedPtr clone() const override;

        const std::string &name() const { return name_; }

        // Type accessors
        TypePtr type() const override { return type_; }
        void set_type(TypePtr type) override { type_ = type; }

    private:
        std::string name_;
        TypePtr type_;
    };

    /**
     * Function application (f(x, y, ...))
     */
    class FunctionApplicationNamed : public TermNamed
    {
    public:
        FunctionApplicationNamed(const std::string &function_name,
                                 const std::vector<TermNamedPtr> &arguments);

        TermKind kind() const override { return TermKind::FUNCTION_APPLICATION; }
        bool equals(const TermNamed &other) const override;
        std::size_t hash() const override;
        TermNamedPtr clone() const override;

        const std::string &function_name() const { return function_name_; }
        const std::vector<TermNamedPtr> &arguments() const { return arguments_; }

        // Type accessors
        TypePtr type() const override { return type_; }
        void set_type(TypePtr type) override { type_ = type; }

    private:
        std::string function_name_;
        std::vector<TermNamedPtr> arguments_;
        TypePtr type_;
    };

    /**
     * Universal quantification (∀x. φ)
     */
    class ForallNamed : public TermNamed
    {
    public:
        ForallNamed(const std::string &variable_name, TermNamedPtr body);

        TermKind kind() const override { return TermKind::FORALL; }
        bool equals(const TermNamed &other) const override;
        std::size_t hash() const override;
        TermNamedPtr clone() const override;

        const std::string &variable_name() const { return variable_name_; }
        const TermNamedPtr &body() const { return body_; }

    private:
        std::string variable_name_;
        TermNamedPtr body_;
    };

    /**
     * Existential quantification (∃x. φ)
     */
    class ExistsNamed : public TermNamed
    {
    public:
        ExistsNamed(const std::string &variable_name, TermNamedPtr body);

        TermKind kind() const override { return TermKind::EXISTS; }
        bool equals(const TermNamed &other) const override;
        std::size_t hash() const override;
        TermNamedPtr clone() const override;

        const std::string &variable_name() const { return variable_name_; }
        const TermNamedPtr &body() const { return body_; }

    private:
        std::string variable_name_;
        TermNamedPtr body_;
    };

    /**
     * Logical conjunction (φ ∧ ψ)
     */
    class AndNamed : public TermNamed
    {
    public:
        AndNamed(TermNamedPtr left, TermNamedPtr right);

        TermKind kind() const override { return TermKind::AND; }
        bool equals(const TermNamed &other) const override;
        std::size_t hash() const override;
        TermNamedPtr clone() const override;

        const TermNamedPtr &left() const { return left_; }
        const TermNamedPtr &right() const { return right_; }

    private:
        TermNamedPtr left_;
        TermNamedPtr right_;
    };

    /**
     * Logical disjunction (φ ∨ ψ)
     */
    class OrNamed : public TermNamed
    {
    public:
        OrNamed(TermNamedPtr left, TermNamedPtr right);

        TermKind kind() const override { return TermKind::OR; }
        bool equals(const TermNamed &other) const override;
        std::size_t hash() const override;
        TermNamedPtr clone() const override;

        const TermNamedPtr &left() const { return left_; }
        const TermNamedPtr &right() const { return right_; }

    private:
        TermNamedPtr left_;
        TermNamedPtr right_;
    };

    /**
     * Logical negation (¬φ)
     */
    class NotNamed : public TermNamed
    {
    public:
        explicit NotNamed(TermNamedPtr body);

        TermKind kind() const override { return TermKind::NOT; }
        bool equals(const TermNamed &other) const override;
        std::size_t hash() const override;
        TermNamedPtr clone() const override;

        const TermNamedPtr &body() const { return body_; }

    private:
        TermNamedPtr body_;
    };

    /**
     * Logical implication (φ → ψ)
     */
    class ImpliesNamed : public TermNamed
    {
    public:
        ImpliesNamed(TermNamedPtr antecedent, TermNamedPtr consequent);

        TermKind kind() const override { return TermKind::IMPLIES; }
        bool equals(const TermNamed &other) const override;
        std::size_t hash() const override;
        TermNamedPtr clone() const override;

        const TermNamedPtr &antecedent() const { return antecedent_; }
        const TermNamedPtr &consequent() const { return consequent_; }

    private:
        TermNamedPtr antecedent_;
        TermNamedPtr consequent_;
    };

    // Factory functions for creating named terms
    TermNamedPtr make_named_variable(const std::string &name);
    TermNamedPtr make_named_constant(const std::string &name);
    TermNamedPtr make_named_function_application(
        const std::string &function_name,
        const std::vector<TermNamedPtr> &arguments);
    TermNamedPtr make_named_forall(const std::string &variable_name, TermNamedPtr body);
    TermNamedPtr make_named_exists(const std::string &variable_name, TermNamedPtr body);
    TermNamedPtr make_named_and(TermNamedPtr left, TermNamedPtr right);
    TermNamedPtr make_named_or(TermNamedPtr left, TermNamedPtr right);
    TermNamedPtr make_named_not(TermNamedPtr body);
    TermNamedPtr make_named_implies(TermNamedPtr antecedent, TermNamedPtr consequent);

    // Conversion functions between named and De Bruijn representations
    TermDBPtr to_db(const TermNamedPtr &term);
    TermNamedPtr to_named(const TermDBPtr &term);

    /**
     * Helper class for tracking variable names during conversion
     */
    class NameContext
    {
    public:
        NameContext() = default;

        // Push a variable name onto the context (for entering a binder)
        void push(const std::string &name);

        // Pop a variable name from the context (for exiting a binder)
        void pop();

        // Get the name for a De Bruijn index (0 = innermost)
        std::string name_for_index(std::size_t index) const;

        // Get the De Bruijn index for a name (-1 if not found)
        int index_for_name(const std::string &name) const;

        // Check if a name exists in the context
        bool contains(const std::string &name) const;

        std::size_t current_depth() const
        {
            return names_.size();
        }

    private:
        std::vector<std::string> names_;
    };

} // namespace theorem_prover