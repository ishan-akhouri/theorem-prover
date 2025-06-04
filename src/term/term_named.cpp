#include "term_named.hpp"
#include "../utils/hash.hpp"
#include "../utils/gensym.hpp"
#include <functional>
#include <algorithm>
#include <stdexcept>

namespace theorem_prover
{

    // NameContext implementation
    void NameContext::push(const std::string &name)
    {
        names_.push_back(name);
    }

    void NameContext::pop()
    {
        if (!names_.empty())
        {
            names_.pop_back();
        }
    }

    std::string NameContext::name_for_index(std::size_t index) const
    {
        if (index < names_.size())
        {
            return names_[names_.size() - 1 - index];
        }
        // If not found, generate a placeholder name
        return "free_" + std::to_string(index - names_.size());
    }

    int NameContext::index_for_name(const std::string &name) const
    {
        auto it = std::find(names_.rbegin(), names_.rend(), name);
        if (it != names_.rend())
        {
            return std::distance(names_.rbegin(), it);
        }
        return -1; // Not found
    }

    bool NameContext::contains(const std::string &name) const
    {
        return std::find(names_.begin(), names_.end(), name) != names_.end();
    }

    // VariableNamed implementation
    VariableNamed::VariableNamed(const std::string &name) : name_(name) {}

    bool VariableNamed::equals(const TermNamed &other) const
    {
        return equal_cast<VariableNamed>(*this, other,
                                         [](const VariableNamed &self, const VariableNamed &other)
                                         {
                                             return self.name_ == other.name_;
                                         });
    }

    std::size_t VariableNamed::hash() const
    {
        return std::hash<std::string>()(name_);
    }

    TermNamedPtr VariableNamed::clone() const
    {
        auto result = std::make_shared<VariableNamed>(name_);
        if (type_)
        {
            result->set_type(type_);
        }
        return result;
    }

    // ConstantNamed implementation
    ConstantNamed::ConstantNamed(const std::string &name) : name_(name) {}

    bool ConstantNamed::equals(const TermNamed &other) const
    {
        return equal_cast<ConstantNamed>(*this, other,
                                         [](const ConstantNamed &self, const ConstantNamed &other)
                                         {
                                             return self.name_ == other.name_;
                                         });
    }

    std::size_t ConstantNamed::hash() const
    {
        return std::hash<std::string>()(name_);
    }

    TermNamedPtr ConstantNamed::clone() const
    {
        auto result = std::make_shared<ConstantNamed>(name_);
        if (type_)
        {
            result->set_type(type_);
        }
        return result;
    }

    // FunctionApplicationNamed implementation
    FunctionApplicationNamed::FunctionApplicationNamed(
        const std::string &function_name,
        const std::vector<TermNamedPtr> &arguments)
        : function_name_(function_name), arguments_(arguments) {}

    bool FunctionApplicationNamed::equals(const TermNamed &other) const
    {
        return equal_cast<FunctionApplicationNamed>(*this, other,
                                                    [](const FunctionApplicationNamed &self, const FunctionApplicationNamed &other)
                                                    {
                                                        if (self.function_name_ != other.function_name_)
                                                            return false;
                                                        if (self.arguments_.size() != other.arguments_.size())
                                                            return false;

                                                        for (size_t i = 0; i < self.arguments_.size(); ++i)
                                                        {
                                                            if (!(*self.arguments_[i] == *other.arguments_[i]))
                                                            {
                                                                return false;
                                                            }
                                                        }
                                                        return true;
                                                    });
    }

    std::size_t FunctionApplicationNamed::hash() const
    {
        std::size_t seed = std::hash<std::string>()(function_name_);
        for (const auto &arg : arguments_)
        {
            hash_combine(seed, arg->hash());
        }
        return seed;
    }

    TermNamedPtr FunctionApplicationNamed::clone() const
    {
        std::vector<TermNamedPtr> cloned_args;
        cloned_args.reserve(arguments_.size());
        for (const auto &arg : arguments_)
        {
            cloned_args.push_back(arg->clone());
        }

        auto result = std::make_shared<FunctionApplicationNamed>(function_name_, cloned_args);
        if (type_)
        {
            result->set_type(type_);
        }
        return result;
    }

    // ForallNamed implementation
    ForallNamed::ForallNamed(const std::string &variable_name, TermNamedPtr body)
        : variable_name_(variable_name), body_(body) {}

    bool ForallNamed::equals(const TermNamed &other) const
    {
        // For alpha equivalence, we'd need to check that the bodies are equivalent up to
        // variable renaming. For this implementation, we just check structural equality.
        return equal_cast<ForallNamed>(*this, other,
                                       [](const ForallNamed &self, const ForallNamed &other)
                                       {
                                           // Here we're checking strict equality, not alpha-equivalence
                                           return self.variable_name_ == other.variable_name_ &&
                                                  (*self.body_ == *other.body_);
                                       });
    }
    // TermDB uses De Bruijn indices, so structural equality coincides with α-equivalence.
    // ∀x.P(x) == ∀y.P(y) if P(x) and P(y) bind identically.

    std::size_t ForallNamed::hash() const
    {
        std::size_t seed = std::hash<std::string>()(variable_name_);
        hash_combine(seed, body_->hash());
        return seed;
    }

    TermNamedPtr ForallNamed::clone() const
    {
        return std::make_shared<ForallNamed>(variable_name_, body_->clone());
    }

    // ExistsNamed implementation
    ExistsNamed::ExistsNamed(const std::string &variable_name, TermNamedPtr body)
        : variable_name_(variable_name), body_(body) {}

    bool ExistsNamed::equals(const TermNamed &other) const
    {
        return equal_cast<ExistsNamed>(*this, other,
                                       [](const ExistsNamed &self, const ExistsNamed &other)
                                       {
                                           return self.variable_name_ == other.variable_name_ &&
                                                  (*self.body_ == *other.body_);
                                       });
    }

    std::size_t ExistsNamed::hash() const
    {
        std::size_t seed = std::hash<std::string>()(variable_name_);
        hash_combine(seed, body_->hash());
        return seed;
    }

    TermNamedPtr ExistsNamed::clone() const
    {
        return std::make_shared<ExistsNamed>(variable_name_, body_->clone());
    }

    // AndNamed implementation
    AndNamed::AndNamed(TermNamedPtr left, TermNamedPtr right)
        : left_(left), right_(right) {}

    bool AndNamed::equals(const TermNamed &other) const
    {
        return equal_cast<AndNamed>(*this, other,
                                    [](const AndNamed &self, const AndNamed &other)
                                    {
                                        return (*self.left_ == *other.left_) &&
                                               (*self.right_ == *other.right_);
                                    });
    }

    std::size_t AndNamed::hash() const
    {
        std::size_t seed = 0x345678; // Some arbitrary value for And
        hash_combine(seed, left_->hash());
        hash_combine(seed, right_->hash());
        return seed;
    }

    TermNamedPtr AndNamed::clone() const
    {
        return std::make_shared<AndNamed>(left_->clone(), right_->clone());
    }

    // OrNamed implementation
    OrNamed::OrNamed(TermNamedPtr left, TermNamedPtr right)
        : left_(left), right_(right) {}

    bool OrNamed::equals(const TermNamed &other) const
    {
        return equal_cast<OrNamed>(*this, other,
                                   [](const OrNamed &self, const OrNamed &other)
                                   {
                                       return (*self.left_ == *other.left_) &&
                                              (*self.right_ == *other.right_);
                                   });
    }

    std::size_t OrNamed::hash() const
    {
        std::size_t seed = 0x456789; // Some arbitrary value for Or
        hash_combine(seed, left_->hash());
        hash_combine(seed, right_->hash());
        return seed;
    }

    TermNamedPtr OrNamed::clone() const
    {
        return std::make_shared<OrNamed>(left_->clone(), right_->clone());
    }

    // NotNamed implementation
    NotNamed::NotNamed(TermNamedPtr body) : body_(body) {}

    bool NotNamed::equals(const TermNamed &other) const
    {
        return equal_cast<NotNamed>(*this, other,
                                    [](const NotNamed &self, const NotNamed &other)
                                    {
                                        return (*self.body_ == *other.body_);
                                    });
    }

    std::size_t NotNamed::hash() const
    {
        std::size_t seed = 0x567890; // Some arbitrary value for Not
        hash_combine(seed, body_->hash());
        return seed;
    }

    TermNamedPtr NotNamed::clone() const
    {
        return std::make_shared<NotNamed>(body_->clone());
    }

    // ImpliesNamed implementation
    ImpliesNamed::ImpliesNamed(TermNamedPtr antecedent, TermNamedPtr consequent)
        : antecedent_(antecedent), consequent_(consequent) {}

    bool ImpliesNamed::equals(const TermNamed &other) const
    {
        return equal_cast<ImpliesNamed>(*this, other,
                                        [](const ImpliesNamed &self, const ImpliesNamed &other)
                                        {
                                            return (*self.antecedent_ == *other.antecedent_) &&
                                                   (*self.consequent_ == *other.consequent_);
                                        });
    }

    std::size_t ImpliesNamed::hash() const
    {
        std::size_t seed = 0x678901; // Some arbitrary value for Implies
        hash_combine(seed, antecedent_->hash());
        hash_combine(seed, consequent_->hash());
        return seed;
    }

    TermNamedPtr ImpliesNamed::clone() const
    {
        return std::make_shared<ImpliesNamed>(antecedent_->clone(), consequent_->clone());
    }

    // Factory functions

    TermNamedPtr make_named_variable(const std::string &name)
    {
        return std::make_shared<VariableNamed>(name);
    }

    TermNamedPtr make_named_constant(const std::string &name)
    {
        return std::make_shared<ConstantNamed>(name);
    }

    TermNamedPtr make_named_function_application(
        const std::string &function_name,
        const std::vector<TermNamedPtr> &arguments)
    {
        return std::make_shared<FunctionApplicationNamed>(function_name, arguments);
    }

    TermNamedPtr make_named_forall(const std::string &variable_name, TermNamedPtr body)
    {
        return std::make_shared<ForallNamed>(variable_name, body);
    }

    TermNamedPtr make_named_exists(const std::string &variable_name, TermNamedPtr body)
    {
        return std::make_shared<ExistsNamed>(variable_name, body);
    }

    TermNamedPtr make_named_and(TermNamedPtr left, TermNamedPtr right)
    {
        return std::make_shared<AndNamed>(left, right);
    }

    TermNamedPtr make_named_or(TermNamedPtr left, TermNamedPtr right)
    {
        return std::make_shared<OrNamed>(left, right);
    }

    TermNamedPtr make_named_not(TermNamedPtr body)
    {
        return std::make_shared<NotNamed>(body);
    }

    TermNamedPtr make_named_implies(TermNamedPtr antecedent, TermNamedPtr consequent)
    {
        return std::make_shared<ImpliesNamed>(antecedent, consequent);
    }

    // Conversion functions

    // Named to De Bruijn conversion
    TermDBPtr to_db_helper(const TermNamedPtr &term, NameContext &context)
    {
        switch (term->kind())
        {
        case TermNamed::TermKind::VARIABLE:
        {
            auto var = std::dynamic_pointer_cast<VariableNamed>(term);
            int index = context.index_for_name(var->name());
            if (index >= 0)
            {
                // Bound variable
                return make_variable(static_cast<size_t>(index));
            }
            else
            {
                // Free variable (treated as a constant)
                return make_variable(context.current_depth());
            }
        }
        case TermNamed::TermKind::CONSTANT:
        {
            auto constant = std::dynamic_pointer_cast<ConstantNamed>(term);
            return make_constant(constant->name());
        }
        case TermNamed::TermKind::FUNCTION_APPLICATION:
        {
            auto app = std::dynamic_pointer_cast<FunctionApplicationNamed>(term);
            std::vector<TermDBPtr> db_args;
            db_args.reserve(app->arguments().size());
            for (const auto &arg : app->arguments())
            {
                db_args.push_back(to_db_helper(arg, context));
            }
            return make_function_application(app->function_name(), db_args);
        }
        case TermNamed::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallNamed>(term);
            context.push(forall->variable_name());
            auto body_db = to_db_helper(forall->body(), context);
            context.pop();
            return make_forall(forall->variable_name(), body_db);
        }
        case TermNamed::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsNamed>(term);
            context.push(exists->variable_name());
            auto body_db = to_db_helper(exists->body(), context);
            context.pop();
            return make_exists(exists->variable_name(), body_db);
        }
        case TermNamed::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndNamed>(term);
            auto left_db = to_db_helper(and_term->left(), context);
            auto right_db = to_db_helper(and_term->right(), context);
            return make_and(left_db, right_db);
        }
        case TermNamed::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrNamed>(term);
            auto left_db = to_db_helper(or_term->left(), context);
            auto right_db = to_db_helper(or_term->right(), context);
            return make_or(left_db, right_db);
        }
        case TermNamed::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotNamed>(term);
            auto body_db = to_db_helper(not_term->body(), context);
            return make_not(body_db);
        }
        case TermNamed::TermKind::IMPLIES:
        {
            auto implies = std::dynamic_pointer_cast<ImpliesNamed>(term);
            auto antecedent_db = to_db_helper(implies->antecedent(), context);
            auto consequent_db = to_db_helper(implies->consequent(), context);
            return make_implies(antecedent_db, consequent_db);
        }
        default:
            throw std::runtime_error("Unsupported term kind in to_db conversion");
        }
    }

    TermDBPtr to_db(const TermNamedPtr &term)
    {
        NameContext context;
        return to_db_helper(term, context);
    }

    // De Bruijn to Named conversion
    TermNamedPtr to_named_helper(const TermDBPtr &term, NameContext &context)
    {
        switch (term->kind())
        {
        case TermDB::TermKind::VARIABLE:
        {
            auto var = std::dynamic_pointer_cast<VariableDB>(term);
            std::string name = context.name_for_index(var->index());
            return make_named_variable(name);
        }
        case TermDB::TermKind::CONSTANT:
        {
            auto constant = std::dynamic_pointer_cast<ConstantDB>(term);
            return make_named_constant(constant->symbol());
        }
        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            std::vector<TermNamedPtr> named_args;
            named_args.reserve(app->arguments().size());
            for (const auto &arg : app->arguments())
            {
                named_args.push_back(to_named_helper(arg, context));
            }
            return make_named_function_application(app->symbol(), named_args);
        }
        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(term);

            // Generate a fresh variable name that doesn't shadow existing ones
            std::string var_name = forall->variable_hint();
            if (var_name.empty() || context.contains(var_name))
            {
                var_name = gensym("x");
            }

            context.push(var_name);
            auto body_named = to_named_helper(forall->body(), context);
            context.pop();

            return make_named_forall(var_name, body_named);
        }
        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(term);

            // Generate a fresh variable name that doesn't shadow existing ones
            std::string var_name = exists->variable_hint();
            if (var_name.empty() || context.contains(var_name))
            {
                var_name = gensym("x");
            }

            context.push(var_name);
            auto body_named = to_named_helper(exists->body(), context);
            context.pop();

            return make_named_exists(var_name, body_named);
        }
        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(term);
            auto left_named = to_named_helper(and_term->left(), context);
            auto right_named = to_named_helper(and_term->right(), context);
            return make_named_and(left_named, right_named);
        }
        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(term);
            auto left_named = to_named_helper(or_term->left(), context);
            auto right_named = to_named_helper(or_term->right(), context);
            return make_named_or(left_named, right_named);
        }
        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(term);
            auto body_named = to_named_helper(not_term->body(), context);
            return make_named_not(body_named);
        }
        case TermDB::TermKind::IMPLIES:
        {
            auto implies = std::dynamic_pointer_cast<ImpliesDB>(term);
            auto antecedent_named = to_named_helper(implies->antecedent(), context);
            auto consequent_named = to_named_helper(implies->consequent(), context);
            return make_named_implies(antecedent_named, consequent_named);
        }
        default:
            throw std::runtime_error("Unsupported term kind in to_named conversion");
        }
    }

    TermNamedPtr to_named(const TermDBPtr &term)
    {
        NameContext context;
        return to_named_helper(term, context);
    }

} // namespace theorem_prover