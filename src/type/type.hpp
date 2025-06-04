#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "../term/term_db.hpp"
#include "../utils/hash.hpp"

namespace theorem_prover
{

    // Forward declarations
    class Type;
    using TypePtr = std::shared_ptr<Type>;

    /**
     * Base class for the type system
     *
     * This provides a common interface for all types in the system,
     * supporting both simple types and compound types.
     */
    class Type
    {
    public:
        enum class TypeKind
        {
            // Basic types
            BASE_TYPE,     // Primitive types like Int, Bool, etc.
            VARIABLE_TYPE, // Type variables for polymorphism (like 'α', 'β' in '∀α. List α → List α')

            // Compound types
            FUNCTION_TYPE, // Functions from one type to another
            PRODUCT_TYPE,  // Tuples/products of types
            RECORD_TYPE,   // Named record types
            SUM_TYPE,      // Algebraic data types / tagged unions

            // Special types
            PROP_TYPE,      // Propositions (logical formulas)
            UNIVERSAL_TYPE, // Universal/top type (⊤)
            VOID_TYPE,      // Empty/bottom type

            // For extensions
            UNKNOWN
        };

        virtual ~Type() = default;

        // Core operations
        virtual TypeKind kind() const = 0;
        virtual bool equals(const Type &other) const = 0;
        virtual std::size_t hash() const = 0;
        virtual TypePtr clone() const = 0;
        virtual std::string to_string() const = 0;

        // Utility for structural equality
        bool operator==(const Type &other) const
        {
            return equals(other);
        }

        // Helper for implementing equals() in derived classes
        template <typename T>
        static bool equal_cast(const Type &self, const Type &other,
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

    /**
     * Base types like Int, Bool, etc.
     */
    class BaseType : public Type
    {
    public:
        explicit BaseType(const std::string &name);

        TypeKind kind() const override { return TypeKind::BASE_TYPE; }
        bool equals(const Type &other) const override;
        std::size_t hash() const override;
        TypePtr clone() const override;
        std::string to_string() const override;

        const std::string &name() const { return name_; }

    private:
        std::string name_;
    };

    /**
     * Type variables for polymorphic types
     */
    class VariableType : public Type
    {
    public:
        explicit VariableType(const std::string &name);

        TypeKind kind() const override { return TypeKind::VARIABLE_TYPE; }
        bool equals(const Type &other) const override;
        std::size_t hash() const override;
        TypePtr clone() const override;
        std::string to_string() const override;

        const std::string &name() const { return name_; }

    private:
        std::string name_;
    };

    /**
     * Function types (argument_type -> return_type)
     */
    class FunctionType : public Type
    {
    public:
        FunctionType(const TypePtr &argument_type, const TypePtr &return_type);
        FunctionType(const std::vector<TypePtr> &argument_types, const TypePtr &return_type);

        TypeKind kind() const override { return TypeKind::FUNCTION_TYPE; }
        bool equals(const Type &other) const override;
        std::size_t hash() const override;
        TypePtr clone() const override;
        std::string to_string() const override;

        // For single-argument functions
        const TypePtr &argument_type() const;
        const TypePtr &return_type() const { return return_type_; }

        // For multi-argument functions
        const std::vector<TypePtr> &argument_types() const { return argument_types_; }

        // Check if this is a multi-argument function
        bool is_multi_argument() const { return argument_types_.size() > 1; }

    private:
        std::vector<TypePtr> argument_types_;
        TypePtr return_type_;
    };

    /**
     * Product types (tuples of types)
     */
    class ProductType : public Type
    {
    public:
        explicit ProductType(const std::vector<TypePtr> &component_types);

        TypeKind kind() const override { return TypeKind::PRODUCT_TYPE; }
        bool equals(const Type &other) const override;
        std::size_t hash() const override;
        TypePtr clone() const override;
        std::string to_string() const override;

        const std::vector<TypePtr> &component_types() const { return component_types_; }
        std::size_t arity() const { return component_types_.size(); }

    private:
        std::vector<TypePtr> component_types_;
    };

    /**
     * Record types with named fields
     */
    class RecordType : public Type
    {
    public:
        using FieldMap = std::unordered_map<std::string, TypePtr>;

        explicit RecordType(const FieldMap &fields);

        TypeKind kind() const override { return TypeKind::RECORD_TYPE; }
        bool equals(const Type &other) const override;
        std::size_t hash() const override;
        TypePtr clone() const override;
        std::string to_string() const override;

        const FieldMap &fields() const { return fields_; }
        bool has_field(const std::string &name) const;
        TypePtr field_type(const std::string &name) const;

    private:
        FieldMap fields_;
    };

    /**
     * Sum types (tagged unions / algebraic data types)
     */
    class SumType : public Type
    {
    public:
        using Constructor = std::pair<std::string, std::vector<TypePtr>>;

        explicit SumType(const std::vector<Constructor> &constructors);

        TypeKind kind() const override { return TypeKind::SUM_TYPE; }
        bool equals(const Type &other) const override;
        std::size_t hash() const override;
        TypePtr clone() const override;
        std::string to_string() const override;

        const std::vector<Constructor> &constructors() const { return constructors_; }

    private:
        std::vector<Constructor> constructors_;
    };

    /**
     * Proposition type (type of logical formulas)
     */
    class PropType : public Type
    {
    public:
        PropType();

        TypeKind kind() const override { return TypeKind::PROP_TYPE; }
        bool equals(const Type &other) const override;
        std::size_t hash() const override;
        TypePtr clone() const override;
        std::string to_string() const override;
    };

    // Type factory functions
    TypePtr make_base_type(const std::string &name);
    TypePtr make_variable_type(const std::string &name);
    TypePtr make_function_type(const TypePtr &argument_type, const TypePtr &return_type);
    TypePtr make_function_type(const std::vector<TypePtr> &argument_types, const TypePtr &return_type);
    TypePtr make_product_type(const std::vector<TypePtr> &component_types);
    TypePtr make_record_type(const RecordType::FieldMap &fields);
    TypePtr make_sum_type(const std::vector<SumType::Constructor> &constructors);
    TypePtr make_prop_type();

    // Common built-in types
    TypePtr int_type();    // Integer type
    TypePtr real_type();   // Real number type
    TypePtr bool_type();   // Boolean type for data-level booleans (true/false in computational language)
    TypePtr prop_type();   // Proposition type for logical formulas
    TypePtr string_type(); // String type

    /**
     * Type checking and unification
     */
    class TypeChecker
    {
    public:
        // Check if a term has a valid type
        static bool check_type(const TermDBPtr &term, TypePtr &inferred_type);

        // Type unification
        static bool unify(const TypePtr &type1, const TypePtr &type2,
                          std::unordered_map<std::string, TypePtr> &substitution);

        // Apply a type substitution to a type
        static TypePtr apply_substitution(const TypePtr &type,
                                          const std::unordered_map<std::string, TypePtr> &substitution);
    };

} // namespace theorem_prover