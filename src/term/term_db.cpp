#include "term_db.hpp"
#include "../utils/hash.hpp"
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

} // namespace theorem_prover