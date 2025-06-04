#include "type.hpp"
#include <sstream>
#include <stdexcept>

namespace theorem_prover
{

    // BaseType implementation
    BaseType::BaseType(const std::string &name) : name_(name) {}

    bool BaseType::equals(const Type &other) const
    {
        return equal_cast<BaseType>(*this, other,
                                    [](const BaseType &self, const BaseType &other)
                                    {
                                        return self.name_ == other.name_;
                                    });
    }

    std::size_t BaseType::hash() const
    {
        return std::hash<std::string>()(name_);
    }

    TypePtr BaseType::clone() const
    {
        return std::make_shared<BaseType>(name_);
    }

    std::string BaseType::to_string() const
    {
        return name_;
    }

    // VariableType implementation
    VariableType::VariableType(const std::string &name) : name_(name) {}

    bool VariableType::equals(const Type &other) const
    {
        return equal_cast<VariableType>(*this, other,
                                        [](const VariableType &self, const VariableType &other)
                                        {
                                            return self.name_ == other.name_;
                                        });
    }

    std::size_t VariableType::hash() const
    {
        return std::hash<std::string>()(name_);
    }

    TypePtr VariableType::clone() const
    {
        return std::make_shared<VariableType>(name_);
    }

    std::string VariableType::to_string() const
    {
        return name_;
    }

    // FunctionType implementation
    FunctionType::FunctionType(const TypePtr &argument_type, const TypePtr &return_type)
        : return_type_(return_type)
    {
        argument_types_.push_back(argument_type);
    }

    FunctionType::FunctionType(const std::vector<TypePtr> &argument_types, const TypePtr &return_type)
        : argument_types_(argument_types), return_type_(return_type) {}

    bool FunctionType::equals(const Type &other) const
    {
        return equal_cast<FunctionType>(*this, other,
                                        [](const FunctionType &self, const FunctionType &other)
                                        {
                                            if (self.argument_types_.size() != other.argument_types_.size())
                                            {
                                                return false;
                                            }

                                            for (size_t i = 0; i < self.argument_types_.size(); ++i)
                                            {
                                                if (!(*self.argument_types_[i] == *other.argument_types_[i]))
                                                {
                                                    return false;
                                                }
                                            }

                                            return (*self.return_type_ == *other.return_type_);
                                        });
    }

    std::size_t FunctionType::hash() const
    {
        std::size_t seed = 0x12345678;
        for (const auto &arg_type : argument_types_)
        {
            hash_combine(seed, arg_type->hash());
        }
        hash_combine(seed, return_type_->hash());
        return seed;
    }

    TypePtr FunctionType::clone() const
    {
        std::vector<TypePtr> cloned_args;
        cloned_args.reserve(argument_types_.size());
        for (const auto &arg_type : argument_types_)
        {
            cloned_args.push_back(arg_type->clone());
        }
        return std::make_shared<FunctionType>(cloned_args, return_type_->clone());
    }

    std::string FunctionType::to_string() const
    {
        std::ostringstream oss;

        if (argument_types_.size() == 1)
        {
            // Single argument function
            if (argument_types_[0]->kind() == TypeKind::FUNCTION_TYPE)
            {
                oss << "(" << argument_types_[0]->to_string() << ")";
            }
            else
            {
                oss << argument_types_[0]->to_string();
            }
        }
        else
        {
            // Multi-argument function
            oss << "(";
            for (size_t i = 0; i < argument_types_.size(); ++i)
            {
                if (i > 0)
                    oss << ", ";
                oss << argument_types_[i]->to_string();
            }
            oss << ")";
        }

        oss << " -> " << return_type_->to_string();
        return oss.str();
    }

    const TypePtr &FunctionType::argument_type() const
    {
        if (argument_types_.empty())
        {
            throw std::runtime_error("Function type has no arguments");
        }
        return argument_types_[0];
    }

    // ProductType implementation
    ProductType::ProductType(const std::vector<TypePtr> &component_types)
        : component_types_(component_types) {}

    bool ProductType::equals(const Type &other) const
    {
        return equal_cast<ProductType>(*this, other,
                                       [](const ProductType &self, const ProductType &other)
                                       {
                                           if (self.component_types_.size() != other.component_types_.size())
                                           {
                                               return false;
                                           }

                                           for (size_t i = 0; i < self.component_types_.size(); ++i)
                                           {
                                               if (!(*self.component_types_[i] == *other.component_types_[i]))
                                               {
                                                   return false;
                                               }
                                           }

                                           return true;
                                       });
    }

    std::size_t ProductType::hash() const
    {
        std::size_t seed = 0x23456789;
        for (const auto &component : component_types_)
        {
            hash_combine(seed, component->hash());
        }
        return seed;
    }

    TypePtr ProductType::clone() const
    {
        std::vector<TypePtr> cloned_components;
        cloned_components.reserve(component_types_.size());
        for (const auto &component : component_types_)
        {
            cloned_components.push_back(component->clone());
        }
        return std::make_shared<ProductType>(cloned_components);
    }

    std::string ProductType::to_string() const
    {
        std::ostringstream oss;
        oss << "(";
        for (size_t i = 0; i < component_types_.size(); ++i)
        {
            if (i > 0)
                oss << " × ";
            oss << component_types_[i]->to_string();
        }
        oss << ")";
        return oss.str();
    }

    // RecordType implementation
    RecordType::RecordType(const FieldMap &fields) : fields_(fields) {}

    bool RecordType::equals(const Type &other) const
    {
        return equal_cast<RecordType>(*this, other,
                                      [](const RecordType &self, const RecordType &other)
                                      {
                                          if (self.fields_.size() != other.fields_.size())
                                          {
                                              return false;
                                          }

                                          for (const auto &[name, type] : self.fields_)
                                          {
                                              auto it = other.fields_.find(name);
                                              if (it == other.fields_.end() || !(*type == *it->second))
                                              {
                                                  return false;
                                              }
                                          }

                                          return true;
                                      });
    }

    std::size_t RecordType::hash() const
    {
        std::size_t seed = 0x34567890;
        for (const auto &[name, type] : fields_)
        {
            hash_combine(seed, std::hash<std::string>()(name));
            hash_combine(seed, type->hash());
        }
        return seed;
    }

    TypePtr RecordType::clone() const
    {
        FieldMap cloned_fields;
        for (const auto &[name, type] : fields_)
        {
            cloned_fields[name] = type->clone();
        }
        return std::make_shared<RecordType>(cloned_fields);
    }

    std::string RecordType::to_string() const
    {
        std::ostringstream oss;
        oss << "{";
        bool first = true;
        for (const auto &[name, type] : fields_)
        {
            if (!first)
                oss << ", ";
            oss << name << ": " << type->to_string();
            first = false;
        }
        oss << "}";
        return oss.str();
    }

    bool RecordType::has_field(const std::string &name) const
    {
        return fields_.find(name) != fields_.end();
    }

    TypePtr RecordType::field_type(const std::string &name) const
    {
        auto it = fields_.find(name);
        if (it == fields_.end())
        {
            throw std::runtime_error("Record type does not have field: " + name);
        }
        return it->second;
    }

    // SumType implementation
    SumType::SumType(const std::vector<Constructor> &constructors)
        : constructors_(constructors) {}

    bool SumType::equals(const Type &other) const
    {
        return equal_cast<SumType>(*this, other,
                                   [](const SumType &self, const SumType &other)
                                   {
                                       if (self.constructors_.size() != other.constructors_.size())
                                       {
                                           return false;
                                       }

                                       for (size_t i = 0; i < self.constructors_.size(); ++i)
                                       {
                                           const auto &[self_name, self_types] = self.constructors_[i];
                                           const auto &[other_name, other_types] = other.constructors_[i];

                                           if (self_name != other_name || self_types.size() != other_types.size())
                                           {
                                               return false;
                                           }

                                           for (size_t j = 0; j < self_types.size(); ++j)
                                           {
                                               if (!(*self_types[j] == *other_types[j]))
                                               {
                                                   return false;
                                               }
                                           }
                                       }

                                       return true;
                                   });
    }

    std::size_t SumType::hash() const
    {
        std::size_t seed = 0x45678901;
        for (const auto &[name, types] : constructors_)
        {
            hash_combine(seed, std::hash<std::string>()(name));
            for (const auto &type : types)
            {
                hash_combine(seed, type->hash());
            }
        }
        return seed;
    }

    TypePtr SumType::clone() const
    {
        std::vector<Constructor> cloned_constructors;
        cloned_constructors.reserve(constructors_.size());

        for (const auto &[name, types] : constructors_)
        {
            std::vector<TypePtr> cloned_types;
            cloned_types.reserve(types.size());

            for (const auto &type : types)
            {
                cloned_types.push_back(type->clone());
            }

            cloned_constructors.emplace_back(name, cloned_types);
        }

        return std::make_shared<SumType>(cloned_constructors);
    }

    std::string SumType::to_string() const
    {
        std::ostringstream oss;
        for (size_t i = 0; i < constructors_.size(); ++i)
        {
            const auto &[name, types] = constructors_[i];
            if (i > 0)
                oss << " | ";
            oss << name;

            if (!types.empty())
            {
                oss << " of ";
                for (size_t j = 0; j < types.size(); ++j)
                {
                    if (j > 0)
                        oss << " × ";
                    oss << types[j]->to_string();
                }
            }
        }
        return oss.str();
    }

    // PropType implementation
    PropType::PropType() {}

    bool PropType::equals(const Type &other) const
    {
        return other.kind() == TypeKind::PROP_TYPE;
    }

    std::size_t PropType::hash() const
    {
        return 0x56789012; // Fixed hash for PropType
    }

    TypePtr PropType::clone() const
    {
        return std::make_shared<PropType>();
    }

    std::string PropType::to_string() const
    {
        return "Prop";
    }

    // Factory functions
    TypePtr make_base_type(const std::string &name)
    {
        return std::make_shared<BaseType>(name);
    }

    TypePtr make_variable_type(const std::string &name)
    {
        return std::make_shared<VariableType>(name);
    }

    TypePtr make_function_type(const TypePtr &argument_type, const TypePtr &return_type)
    {
        return std::make_shared<FunctionType>(argument_type, return_type);
    }

    TypePtr make_function_type(const std::vector<TypePtr> &argument_types, const TypePtr &return_type)
    {
        return std::make_shared<FunctionType>(argument_types, return_type);
    }

    TypePtr make_product_type(const std::vector<TypePtr> &component_types)
    {
        return std::make_shared<ProductType>(component_types);
    }

    TypePtr make_record_type(const RecordType::FieldMap &fields)
    {
        return std::make_shared<RecordType>(fields);
    }

    TypePtr make_sum_type(const std::vector<SumType::Constructor> &constructors)
    {
        return std::make_shared<SumType>(constructors);
    }

    TypePtr make_prop_type()
    {
        return std::make_shared<PropType>();
    }

    // Common built-in types
    TypePtr int_type()
    {
        static auto instance = make_base_type("Int");
        return instance;
    }

    TypePtr real_type()
    {
        static auto instance = make_base_type("Real");
        return instance;
    }

    TypePtr bool_type()
    {
        static auto instance = make_base_type("Bool");
        return instance;
    }

    TypePtr prop_type()
    {
        static auto instance = make_prop_type();
        return instance;
    }

    TypePtr string_type()
    {
        static auto instance = make_base_type("String");
        return instance;
    }

    // TypeChecker implementation
    bool TypeChecker::check_type(const TermDBPtr &term, TypePtr &inferred_type)
    {
        // This is a placeholder for the actual type checking logic
        // Actual implementation would recursively traverse the term structure
        // and apply typing rules to infer and check types

        // For now, we'll just return a dummy result
        // The real implementation would be more complex
        inferred_type = make_base_type("Unknown");
        return true;
    }

    bool TypeChecker::unify(const TypePtr &type1, const TypePtr &type2,
                            std::unordered_map<std::string, TypePtr> &substitution)
    {
        // Base case: if types are already equal
        if (*type1 == *type2)
        {
            return true;
        }

        // Case 1: type1 is a variable
        if (type1->kind() == Type::TypeKind::VARIABLE_TYPE)
        {
            auto var_type = std::dynamic_pointer_cast<VariableType>(type1);
            auto var_name = var_type->name();

            // Check if this variable is already in the substitution
            auto it = substitution.find(var_name);
            if (it != substitution.end())
            {
                // Recursive unification with the existing substitution
                return unify(it->second, type2, substitution);
            }

            // TODO: Occurs check to prevent circular substitutions

            // Add to substitution
            substitution[var_name] = type2;
            return true;
        }

        // Case 2: type2 is a variable
        if (type2->kind() == Type::TypeKind::VARIABLE_TYPE)
        {
            auto var_type = std::dynamic_pointer_cast<VariableType>(type2);
            auto var_name = var_type->name();

            // Check if this variable is already in the substitution
            auto it = substitution.find(var_name);
            if (it != substitution.end())
            {
                // Recursive unification with the existing substitution
                return unify(type1, it->second, substitution);
            }

            // TODO: Occurs check to prevent circular substitutions

            // Add to substitution
            substitution[var_name] = type1;
            return true;
        }

        // Case 3: both are function types
        if (type1->kind() == Type::TypeKind::FUNCTION_TYPE &&
            type2->kind() == Type::TypeKind::FUNCTION_TYPE)
        {
            auto func1 = std::dynamic_pointer_cast<FunctionType>(type1);
            auto func2 = std::dynamic_pointer_cast<FunctionType>(type2);

            // Must have same number of arguments
            if (func1->argument_types().size() != func2->argument_types().size())
            {
                return false;
            }

            // Unify argument types
            for (size_t i = 0; i < func1->argument_types().size(); ++i)
            {
                if (!unify(func1->argument_types()[i], func2->argument_types()[i], substitution))
                {
                    return false;
                }
            }

            // Unify return types
            return unify(func1->return_type(), func2->return_type(), substitution);
        }

        // Case 4: both are product types
        if (type1->kind() == Type::TypeKind::PRODUCT_TYPE &&
            type2->kind() == Type::TypeKind::PRODUCT_TYPE)
        {
            auto prod1 = std::dynamic_pointer_cast<ProductType>(type1);
            auto prod2 = std::dynamic_pointer_cast<ProductType>(type2);

            // Must have same number of components
            if (prod1->component_types().size() != prod2->component_types().size())
            {
                return false;
            }

            // Unify component types
            for (size_t i = 0; i < prod1->component_types().size(); ++i)
            {
                if (!unify(prod1->component_types()[i], prod2->component_types()[i], substitution))
                {
                    return false;
                }
            }

            return true;
        }

        // Case 5: both are record types
        if (type1->kind() == Type::TypeKind::RECORD_TYPE &&
            type2->kind() == Type::TypeKind::RECORD_TYPE)
        {
            auto rec1 = std::dynamic_pointer_cast<RecordType>(type1);
            auto rec2 = std::dynamic_pointer_cast<RecordType>(type2);

            // Must have same field names
            if (rec1->fields().size() != rec2->fields().size())
            {
                return false;
            }

            // Unify field types
            for (const auto &[name, type] : rec1->fields())
            {
                if (!rec2->has_field(name))
                {
                    return false;
                }

                if (!unify(type, rec2->field_type(name), substitution))
                {
                    return false;
                }
            }

            return true;
        }

        // Case 6: both are sum types
        if (type1->kind() == Type::TypeKind::SUM_TYPE &&
            type2->kind() == Type::TypeKind::SUM_TYPE)
        {
            auto sum1 = std::dynamic_pointer_cast<SumType>(type1);
            auto sum2 = std::dynamic_pointer_cast<SumType>(type2);

            // Must have same number of constructors
            if (sum1->constructors().size() != sum2->constructors().size())
            {
                return false;
            }

            // Unify constructor types
            for (size_t i = 0; i < sum1->constructors().size(); ++i)
            {
                const auto &[name1, types1] = sum1->constructors()[i];
                const auto &[name2, types2] = sum2->constructors()[i];

                // Constructor names must match
                if (name1 != name2)
                {
                    return false;
                }

                // Must have same number of arguments
                if (types1.size() != types2.size())
                {
                    return false;
                }

                // Unify argument types
                for (size_t j = 0; j < types1.size(); ++j)
                {
                    if (!unify(types1[j], types2[j], substitution))
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        // Types don't match
        return false;
    }

    TypePtr TypeChecker::apply_substitution(const TypePtr &type,
                                            const std::unordered_map<std::string, TypePtr> &substitution)
    {
        // Case 1: Variable type
        if (type->kind() == Type::TypeKind::VARIABLE_TYPE)
        {
            auto var_type = std::dynamic_pointer_cast<VariableType>(type);
            auto var_name = var_type->name();

            auto it = substitution.find(var_name);
            if (it != substitution.end())
            {
                // Apply substitution recursively to handle chained substitutions
                return apply_substitution(it->second, substitution);
            }

            // No substitution for this variable
            return type;
        }

        // Case 2: Function type
        if (type->kind() == Type::TypeKind::FUNCTION_TYPE)
        {
            auto func_type = std::dynamic_pointer_cast<FunctionType>(type);

            std::vector<TypePtr> subst_args;
            subst_args.reserve(func_type->argument_types().size());

            for (const auto &arg_type : func_type->argument_types())
            {
                subst_args.push_back(apply_substitution(arg_type, substitution));
            }

            auto subst_return = apply_substitution(func_type->return_type(), substitution);

            return make_function_type(subst_args, subst_return);
        }

        // Case 3: Product type
        if (type->kind() == Type::TypeKind::PRODUCT_TYPE)
        {
            auto prod_type = std::dynamic_pointer_cast<ProductType>(type);

            std::vector<TypePtr> subst_components;
            subst_components.reserve(prod_type->component_types().size());

            for (const auto &component : prod_type->component_types())
            {
                subst_components.push_back(apply_substitution(component, substitution));
            }

            return make_product_type(subst_components);
        }

        // Case 4: Record type
        if (type->kind() == Type::TypeKind::RECORD_TYPE)
        {
            auto rec_type = std::dynamic_pointer_cast<RecordType>(type);

            RecordType::FieldMap subst_fields;
            for (const auto &[name, field_type] : rec_type->fields())
            {
                subst_fields[name] = apply_substitution(field_type, substitution);
            }

            return make_record_type(subst_fields);
        }

        // Case 5: Sum type
        if (type->kind() == Type::TypeKind::SUM_TYPE)
        {
            auto sum_type = std::dynamic_pointer_cast<SumType>(type);

            std::vector<SumType::Constructor> subst_constructors;
            subst_constructors.reserve(sum_type->constructors().size());

            for (const auto &[name, types] : sum_type->constructors())
            {
                std::vector<TypePtr> subst_types;
                subst_types.reserve(types.size());

                for (const auto &type : types)
                {
                    subst_types.push_back(apply_substitution(type, substitution));
                }

                subst_constructors.emplace_back(name, subst_types);
            }

            return make_sum_type(subst_constructors);
        }

        // Default case: base types, prop type, etc. (no substitution needed)
        return type;
    }

} // namespace theorem_prover