#include "term_db.hpp"
#include "../utils/hash.hpp"
#include <set>
#include <functional>

namespace theorem_prover
{

    // VariableDB implementation
    VariableDB::VariableDB(std::size_t index) : index_(index) {}

    bool VariableDB::equals(const TermDB &other) const
    {
        return equal_cast<VariableDB>(*this, other,
                                      [](const VariableDB &self, const VariableDB &other)
                                      {
                                          return self.index_ == other.index_;
                                      });
    }

    std::size_t VariableDB::hash() const
    {
        return std::hash<std::size_t>()(index_);
    }

    TermDBPtr VariableDB::clone() const
    {
        return std::make_shared<VariableDB>(index_);
    }

    // ConstantDB implementation
    ConstantDB::ConstantDB(const std::string &symbol) : symbol_(symbol) {}

    bool ConstantDB::equals(const TermDB &other) const
    {
        return equal_cast<ConstantDB>(*this, other,
                                      [](const ConstantDB &self, const ConstantDB &other)
                                      {
                                          return self.symbol_ == other.symbol_;
                                      });
    }

    std::size_t ConstantDB::hash() const
    {
        return std::hash<std::string>()(symbol_);
    }

    TermDBPtr ConstantDB::clone() const
    {
        auto result = std::make_shared<ConstantDB>(symbol_);
        if (type_)
        {
            result->set_type(type_);
        }
        return result;
    }

    // FunctionApplicationDB implementation
    FunctionApplicationDB::FunctionApplicationDB(
        const std::string &symbol,
        const std::vector<TermDBPtr> &arguments)
        : symbol_(symbol), arguments_(arguments) {}

    bool FunctionApplicationDB::equals(const TermDB &other) const
    {
        return equal_cast<FunctionApplicationDB>(*this, other,
                                                 [](const FunctionApplicationDB &self, const FunctionApplicationDB &other)
                                                 {
                                                     if (self.symbol_ != other.symbol_)
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

    std::size_t FunctionApplicationDB::hash() const
    {
        std::size_t seed = std::hash<std::string>()(symbol_);
        for (const auto &arg : arguments_)
        {
            hash_combine(seed, arg->hash());
        }
        return seed;
    }

    TermDBPtr FunctionApplicationDB::clone() const
    {
        std::vector<TermDBPtr> cloned_args;
        cloned_args.reserve(arguments_.size());
        for (const auto &arg : arguments_)
        {
            cloned_args.push_back(arg->clone());
        }

        auto result = std::make_shared<FunctionApplicationDB>(symbol_, cloned_args);
        if (type_)
        {
            result->set_type(type_);
        }
        return result;
    }

    // ForallDB implementation
    ForallDB::ForallDB(const std::string &var_hint, TermDBPtr body)
        : var_hint_(var_hint), body_(body) {}

    bool ForallDB::equals(const TermDB &other) const
    {
        return equal_cast<ForallDB>(*this, other,
                                    [](const ForallDB &self, const ForallDB &other)
                                    {
                                        // Note: var_hint is just a hint, not part of equality
                                        return (*self.body_ == *other.body_);
                                    });
    }

    std::size_t ForallDB::hash() const
    {
        std::size_t seed = 0x123456; // Some arbitrary value for Forall
        hash_combine(seed, body_->hash());
        return seed;
    }

    TermDBPtr ForallDB::clone() const
    {
        return std::make_shared<ForallDB>(var_hint_, body_->clone());
    }

    // ExistsDB implementation
    ExistsDB::ExistsDB(const std::string &var_hint, TermDBPtr body)
        : var_hint_(var_hint), body_(body) {}

    bool ExistsDB::equals(const TermDB &other) const
    {
        return equal_cast<ExistsDB>(*this, other,
                                    [](const ExistsDB &self, const ExistsDB &other)
                                    {
                                        // Note: var_hint is just a hint, not part of equality
                                        return (*self.body_ == *other.body_);
                                    });
    }

    std::size_t ExistsDB::hash() const
    {
        std::size_t seed = 0x234567; // Some arbitrary value for Exists
        hash_combine(seed, body_->hash());
        return seed;
    }

    TermDBPtr ExistsDB::clone() const
    {
        return std::make_shared<ExistsDB>(var_hint_, body_->clone());
    }

    // AndDB implementation
    AndDB::AndDB(TermDBPtr left, TermDBPtr right)
        : left_(left), right_(right) {}

    bool AndDB::equals(const TermDB &other) const
    {
        return equal_cast<AndDB>(*this, other,
                                 [](const AndDB &self, const AndDB &other)
                                 {
                                     return (*self.left_ == *other.left_) &&
                                            (*self.right_ == *other.right_);
                                 });
    }

    std::size_t AndDB::hash() const
    {
        std::size_t seed = 0x345678; // Some arbitrary value for And
        hash_combine(seed, left_->hash());
        hash_combine(seed, right_->hash());
        return seed;
    }

    TermDBPtr AndDB::clone() const
    {
        return std::make_shared<AndDB>(left_->clone(), right_->clone());
    }

    // OrDB implementation
    OrDB::OrDB(TermDBPtr left, TermDBPtr right)
        : left_(left), right_(right) {}

    bool OrDB::equals(const TermDB &other) const
    {
        return equal_cast<OrDB>(*this, other,
                                [](const OrDB &self, const OrDB &other)
                                {
                                    return (*self.left_ == *other.left_) &&
                                           (*self.right_ == *other.right_);
                                });
    }

    std::size_t OrDB::hash() const
    {
        std::size_t seed = 0x456789; // Some arbitrary value for Or
        hash_combine(seed, left_->hash());
        hash_combine(seed, right_->hash());
        return seed;
    }

    TermDBPtr OrDB::clone() const
    {
        return std::make_shared<OrDB>(left_->clone(), right_->clone());
    }

    // NotDB implementation
    NotDB::NotDB(TermDBPtr body) : body_(body) {}

    bool NotDB::equals(const TermDB &other) const
    {
        return equal_cast<NotDB>(*this, other,
                                 [](const NotDB &self, const NotDB &other)
                                 {
                                     return (*self.body_ == *other.body_);
                                 });
    }

    std::size_t NotDB::hash() const
    {
        std::size_t seed = 0x567890; // Some arbitrary value for Not
        hash_combine(seed, body_->hash());
        return seed;
    }

    TermDBPtr NotDB::clone() const
    {
        return std::make_shared<NotDB>(body_->clone());
    }

    // ImpliesDB implementation
    ImpliesDB::ImpliesDB(TermDBPtr antecedent, TermDBPtr consequent)
        : antecedent_(antecedent), consequent_(consequent) {}

    bool ImpliesDB::equals(const TermDB &other) const
    {
        return equal_cast<ImpliesDB>(*this, other,
                                     [](const ImpliesDB &self, const ImpliesDB &other)
                                     {
                                         return (*self.antecedent_ == *other.antecedent_) &&
                                                (*self.consequent_ == *other.consequent_);
                                     });
    }

    std::size_t ImpliesDB::hash() const
    {
        std::size_t seed = 0x678901; // Some arbitrary value for Implies
        hash_combine(seed, antecedent_->hash());
        hash_combine(seed, consequent_->hash());
        return seed;
    }

    TermDBPtr ImpliesDB::clone() const
    {
        return std::make_shared<ImpliesDB>(antecedent_->clone(), consequent_->clone());
    }

    // Factory functions

    TermDBPtr make_variable(std::size_t index)
    {
        return std::make_shared<VariableDB>(index);
    }

    TermDBPtr make_constant(const std::string &symbol)
    {
        return std::make_shared<ConstantDB>(symbol);
    }

    TermDBPtr make_function_application(
        const std::string &symbol,
        const std::vector<TermDBPtr> &arguments)
    {
        return std::make_shared<FunctionApplicationDB>(symbol, arguments);
    }

    TermDBPtr make_forall(const std::string &var_hint, TermDBPtr body)
    {
        return std::make_shared<ForallDB>(var_hint, body);
    }

    TermDBPtr make_exists(const std::string &var_hint, TermDBPtr body)
    {
        return std::make_shared<ExistsDB>(var_hint, body);
    }

    TermDBPtr make_and(TermDBPtr left, TermDBPtr right)
    {
        return std::make_shared<AndDB>(left, right);
    }

    TermDBPtr make_or(TermDBPtr left, TermDBPtr right)
    {
        return std::make_shared<OrDB>(left, right);
    }

    TermDBPtr make_not(TermDBPtr body)
    {
        return std::make_shared<NotDB>(body);
    }

    TermDBPtr make_implies(TermDBPtr antecedent, TermDBPtr consequent)
    {
        return std::make_shared<ImpliesDB>(antecedent, consequent);
    }

    // Variable discovery utilities
    std::set<std::size_t> find_all_variables(const TermDBPtr &term, std::size_t depth)
    {
        std::set<std::size_t> variables;

        switch (term->kind())
        {
        case TermDB::TermKind::VARIABLE:
        {
            auto var = std::dynamic_pointer_cast<VariableDB>(term);
            if (var->index() >= depth)
            {
                // Free variable - adjust for current depth
                variables.insert(var->index() - depth);
            }
            break;
        }
        case TermDB::TermKind::FUNCTION_APPLICATION:
        {
            auto func = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
            for (const auto &arg : func->arguments())
            {
                auto arg_vars = find_all_variables(arg, depth);
                variables.insert(arg_vars.begin(), arg_vars.end());
            }
            break;
        }
        case TermDB::TermKind::FORALL:
        {
            auto forall = std::dynamic_pointer_cast<ForallDB>(term);
            auto body_vars = find_all_variables(forall->body(), depth + 1);
            variables.insert(body_vars.begin(), body_vars.end());
            break;
        }
        case TermDB::TermKind::EXISTS:
        {
            auto exists = std::dynamic_pointer_cast<ExistsDB>(term);
            auto body_vars = find_all_variables(exists->body(), depth + 1);
            variables.insert(body_vars.begin(), body_vars.end());
            break;
        }
        case TermDB::TermKind::AND:
        {
            auto and_term = std::dynamic_pointer_cast<AndDB>(term);
            auto left_vars = find_all_variables(and_term->left(), depth);
            auto right_vars = find_all_variables(and_term->right(), depth);
            variables.insert(left_vars.begin(), left_vars.end());
            variables.insert(right_vars.begin(), right_vars.end());
            break;
        }
        case TermDB::TermKind::OR:
        {
            auto or_term = std::dynamic_pointer_cast<OrDB>(term);
            auto left_vars = find_all_variables(or_term->left(), depth);
            auto right_vars = find_all_variables(or_term->right(), depth);
            variables.insert(left_vars.begin(), left_vars.end());
            variables.insert(right_vars.begin(), right_vars.end());
            break;
        }
        case TermDB::TermKind::NOT:
        {
            auto not_term = std::dynamic_pointer_cast<NotDB>(term);
            auto body_vars = find_all_variables(not_term->body(), depth);
            variables.insert(body_vars.begin(), body_vars.end());
            break;
        }
        case TermDB::TermKind::IMPLIES:
        {
            auto implies = std::dynamic_pointer_cast<ImpliesDB>(term);
            auto ant_vars = find_all_variables(implies->antecedent(), depth);
            auto cons_vars = find_all_variables(implies->consequent(), depth);
            variables.insert(ant_vars.begin(), ant_vars.end());
            variables.insert(cons_vars.begin(), cons_vars.end());
            break;
        }
        case TermDB::TermKind::CONSTANT:
            // Constants have no variables
            break;
        }

        return variables;
    }

    std::size_t get_max_variable_index(const TermDBPtr &term, std::size_t depth)
    {
        auto variables = find_all_variables(term, depth);
        if (variables.empty())
        {
            return 0;
        }
        return *variables.rbegin(); // Maximum element
    }

    bool is_equality(const TermDBPtr &term)
    {
        if (term->kind() != TermDB::TermKind::FUNCTION_APPLICATION)
        {
            return false;
        }

        auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
        return func_app->symbol() == "=" && func_app->arguments().size() == 2;
    }

    std::pair<TermDBPtr, TermDBPtr> get_equality_sides(const TermDBPtr &term)
    {
        if (!is_equality(term))
        {
            throw std::runtime_error("Term is not an equality");
        }

        auto func_app = std::dynamic_pointer_cast<FunctionApplicationDB>(term);
        return {func_app->arguments()[0], func_app->arguments()[1]};
    }

} // namespace theorem_prover