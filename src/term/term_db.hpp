#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace theorem_prover
{

    // Forward declaration for type system integration
    class Type;
    using TypePtr = std::shared_ptr<Type>;

    // Forward declare TermDB class
    class TermDB;

    // Define shared pointer type early
    using TermDBPtr = std::shared_ptr<TermDB>;

    /**
     * Base class for De Bruijn indexed terms
     *
     * This representation uses De Bruijn indices for bound variables,
     * which simplifies alpha-equivalence checking and substitution.
     */
    class TermDB
    {
    public:
        enum class TermKind
        {
            // Core terms
            VARIABLE,
            CONSTANT,
            FUNCTION_APPLICATION,

            // Logical connectives
            AND,
            OR,
            NOT,
            IMPLIES,

            // Quantifiers
            FORALL,
            EXISTS,

            // Reserved for future extensions
            ABSTRACTION, // Lambda abstraction (for HOL)
            LET,         // Let bindings
            ITE,         // If-then-else
            MATCH,       // Pattern matching

            // Extensibility
            UNKNOWN // For extensions
        };

        virtual ~TermDB() = default;

        // Core operations
        virtual TermKind kind() const = 0;
        virtual bool equals(const TermDB &other) const = 0;
        virtual std::size_t hash() const = 0;
        virtual TermDBPtr clone() const = 0;

        // Optional type annotation
        virtual TypePtr type() const { return nullptr; }
        virtual void set_type(TypePtr type) {}

        // Utility for structural equality
        bool operator==(const TermDB &other) const
        {
            return equals(other);
        }

        // Helper for implementing equals() in derived classes
        template <typename T>
        static bool equal_cast(const TermDB &self, const TermDB &other,
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

    using TermDBPtr = std::shared_ptr<TermDB>;

    /**
     * De Bruijn variable refers to a bound variable by its binding depth
     * Index 0 refers to the innermost binder, 1 to the next one out, etc.
     */
    class VariableDB : public TermDB
    {
    public:
        explicit VariableDB(std::size_t index);

        TermKind kind() const override { return TermKind::VARIABLE; }
        bool equals(const TermDB &other) const override;
        std::size_t hash() const override;
        TermDBPtr clone() const override;

        std::size_t index() const { return index_; }

    private:
        std::size_t index_; // De Bruijn index
        TypePtr type_;
    };

    /**
     * Constant symbol (like a free variable or named constant)
     */
    class ConstantDB : public TermDB
    {
    public:
        explicit ConstantDB(const std::string &symbol);

        TermKind kind() const override { return TermKind::CONSTANT; }
        bool equals(const TermDB &other) const override;
        std::size_t hash() const override;
        TermDBPtr clone() const override;

        const std::string &symbol() const { return symbol_; }

        // Type accessors
        TypePtr type() const override { return type_; }
        void set_type(TypePtr type) override { type_ = type; }

    private:
        std::string symbol_;
        TypePtr type_;
    };

    /**
     * Function application (f t1 t2 ... tn)
     */
    class FunctionApplicationDB : public TermDB
    {
    public:
        FunctionApplicationDB(const std::string &symbol,
                              const std::vector<TermDBPtr> &arguments);

        TermKind kind() const override { return TermKind::FUNCTION_APPLICATION; }
        bool equals(const TermDB &other) const override;
        std::size_t hash() const override;
        TermDBPtr clone() const override;

        const std::string &symbol() const { return symbol_; }
        const std::vector<TermDBPtr> &arguments() const { return arguments_; }

        // Type accessors
        TypePtr type() const override { return type_; }
        void set_type(TypePtr type) override { type_ = type; }

    private:
        std::string symbol_;
        std::vector<TermDBPtr> arguments_;
        TypePtr type_;
    };

    /**
     * Universal quantification (∀x.φ)
     */
    class ForallDB : public TermDB
    {
    public:
        ForallDB(const std::string &var_hint, TermDBPtr body);

        TermKind kind() const override { return TermKind::FORALL; }
        bool equals(const TermDB &other) const override;
        std::size_t hash() const override;
        TermDBPtr clone() const override;

        const std::string &variable_hint() const { return var_hint_; }
        const TermDBPtr &body() const { return body_; }

    private:
        std::string var_hint_; // Optional hint for pretty-printing
        TermDBPtr body_;
    };

    /**
     * Existential quantification (∃x.φ)
     */
    class ExistsDB : public TermDB
    {
    public:
        ExistsDB(const std::string &var_hint, TermDBPtr body);

        TermKind kind() const override { return TermKind::EXISTS; }
        bool equals(const TermDB &other) const override;
        std::size_t hash() const override;
        TermDBPtr clone() const override;

        const std::string &variable_hint() const { return var_hint_; }
        const TermDBPtr &body() const { return body_; }

    private:
        std::string var_hint_; // Optional hint for pretty-printing
        TermDBPtr body_;
    };

    /**
     * Logical conjunction (φ ∧ ψ)
     */
    class AndDB : public TermDB
    {
    public:
        AndDB(TermDBPtr left, TermDBPtr right);

        TermKind kind() const override { return TermKind::AND; }
        bool equals(const TermDB &other) const override;
        std::size_t hash() const override;
        TermDBPtr clone() const override;

        const TermDBPtr &left() const { return left_; }
        const TermDBPtr &right() const { return right_; }

    private:
        TermDBPtr left_;
        TermDBPtr right_;
    };

    /**
     * Logical disjunction (φ ∨ ψ)
     */
    class OrDB : public TermDB
    {
    public:
        OrDB(TermDBPtr left, TermDBPtr right);

        TermKind kind() const override { return TermKind::OR; }
        bool equals(const TermDB &other) const override;
        std::size_t hash() const override;
        TermDBPtr clone() const override;

        const TermDBPtr &left() const { return left_; }
        const TermDBPtr &right() const { return right_; }

    private:
        TermDBPtr left_;
        TermDBPtr right_;
    };

    /**
     * Logical negation (¬φ)
     */
    class NotDB : public TermDB
    {
    public:
        explicit NotDB(TermDBPtr body);

        TermKind kind() const override { return TermKind::NOT; }
        bool equals(const TermDB &other) const override;
        std::size_t hash() const override;
        TermDBPtr clone() const override;

        const TermDBPtr &body() const { return body_; }

    private:
        TermDBPtr body_;
    };

    /**
     * Logical implication (φ → ψ)
     */
    class ImpliesDB : public TermDB
    {
    public:
        ImpliesDB(TermDBPtr antecedent, TermDBPtr consequent);

        TermKind kind() const override { return TermKind::IMPLIES; }
        bool equals(const TermDB &other) const override;
        std::size_t hash() const override;
        TermDBPtr clone() const override;

        const TermDBPtr &antecedent() const { return antecedent_; }
        const TermDBPtr &consequent() const { return consequent_; }

    private:
        TermDBPtr antecedent_;
        TermDBPtr consequent_;
    };

    // Factory functions for creating terms

    TermDBPtr make_variable(std::size_t index);
    TermDBPtr make_constant(const std::string &symbol);
    TermDBPtr make_function_application(const std::string &symbol,
                                        const std::vector<TermDBPtr> &arguments);
    TermDBPtr make_forall(const std::string &var_hint, TermDBPtr body);
    TermDBPtr make_exists(const std::string &var_hint, TermDBPtr body);
    TermDBPtr make_and(TermDBPtr left, TermDBPtr right);
    TermDBPtr make_or(TermDBPtr left, TermDBPtr right);
    TermDBPtr make_not(TermDBPtr body);
    TermDBPtr make_implies(TermDBPtr antecedent, TermDBPtr consequent);

} // namespace theorem_prover