#include <iostream>
#include <cassert>
#include <memory>
#include <string>
#include "../src/type/type.hpp"

using namespace theorem_prover;

// Utility for printing test results
#define TEST(name)                                   \
    std::cout << "Running test: " << name << "... "; \
    try                                              \
    {

#define END_TEST                                          \
    std::cout << "PASSED" << std::endl;                   \
    }                                                     \
    catch (const std::exception &e)                       \
    {                                                     \
        std::cout << "FAILED: " << e.what() << std::endl; \
        return false;                                     \
    }

// Test equality and hash functions for types
bool test_type_equality_and_hash()
{
    TEST("Type equality - identical base types")
    auto type1 = make_base_type("Int");
    auto type2 = make_base_type("Int");
    assert(*type1 == *type2);
    assert(type1->hash() == type2->hash());
    END_TEST

    TEST("Type equality - different base types")
    auto type1 = make_base_type("Int");
    auto type2 = make_base_type("Bool");
    assert(!(*type1 == *type2));
    // Hash codes may collide, so no assertion about different hashes
    END_TEST

    TEST("Type equality - function types")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");
    auto func1 = make_function_type(int_t, bool_t);
    auto func2 = make_function_type(int_t, bool_t);
    auto func3 = make_function_type(bool_t, int_t);

    assert(*func1 == *func2);
    assert(func1->hash() == func2->hash());
    assert(!(*func1 == *func3));
    END_TEST

    TEST("Type equality - product types")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");
    auto prod1 = make_product_type({int_t, bool_t});
    auto prod2 = make_product_type({int_t, bool_t});
    auto prod3 = make_product_type({bool_t, int_t});

    assert(*prod1 == *prod2);
    assert(prod1->hash() == prod2->hash());
    assert(!(*prod1 == *prod3));
    END_TEST

    TEST("Type equality - record types")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");
    RecordType::FieldMap fields1 = {{"x", int_t}, {"y", bool_t}};
    RecordType::FieldMap fields2 = {{"x", int_t}, {"y", bool_t}};
    RecordType::FieldMap fields3 = {{"a", int_t}, {"b", bool_t}};

    auto rec1 = make_record_type(fields1);
    auto rec2 = make_record_type(fields2);
    auto rec3 = make_record_type(fields3);

    assert(*rec1 == *rec2);
    assert(rec1->hash() == rec2->hash());
    assert(!(*rec1 == *rec3));
    END_TEST

    TEST("Type equality - sum types")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");

    std::vector<SumType::Constructor> constructors1 = {
        {"Just", {int_t}},
        {"Nothing", {}}};

    std::vector<SumType::Constructor> constructors2 = {
        {"Just", {int_t}},
        {"Nothing", {}}};

    std::vector<SumType::Constructor> constructors3 = {
        {"Some", {bool_t}},
        {"None", {}}};

    auto sum1 = make_sum_type(constructors1);
    auto sum2 = make_sum_type(constructors2);
    auto sum3 = make_sum_type(constructors3);

    assert(*sum1 == *sum2);
    assert(sum1->hash() == sum2->hash());
    assert(!(*sum1 == *sum3));
    END_TEST

    return true;
}

// Test type unification with simple function types
bool test_unification_simple_function_types()
{
    TEST("Unification - identical function types")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");
    auto func1 = make_function_type(int_t, bool_t);
    auto func2 = make_function_type(int_t, bool_t);

    std::unordered_map<std::string, TypePtr> substitution;
    bool result = TypeChecker::unify(func1, func2, substitution);

    assert(result);
    assert(substitution.empty()); // No substitutions needed
    END_TEST

    TEST("Unification - different function types")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");
    auto string_t = make_base_type("String");

    auto func1 = make_function_type(int_t, bool_t);
    auto func2 = make_function_type(int_t, string_t);

    std::unordered_map<std::string, TypePtr> substitution;
    bool result = TypeChecker::unify(func1, func2, substitution);

    assert(!result); // Unification should fail
    END_TEST

    TEST("Unification - multi-argument function types")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");

    auto func1 = make_function_type({int_t, bool_t}, int_t);
    auto func2 = make_function_type({int_t, bool_t}, int_t);
    auto func3 = make_function_type({int_t}, int_t);

    std::unordered_map<std::string, TypePtr> substitution1;
    bool result1 = TypeChecker::unify(func1, func2, substitution1);
    assert(result1);

    std::unordered_map<std::string, TypePtr> substitution2;
    bool result2 = TypeChecker::unify(func1, func3, substitution2);
    assert(!result2); // Different arity, should fail
    END_TEST

    return true;
}

// Test unification with type variables
bool test_unification_with_type_variables()
{
    TEST("Unification - variable with concrete type")
    auto int_t = make_base_type("Int");
    auto alpha = make_variable_type("alpha");

    std::unordered_map<std::string, TypePtr> substitution;
    bool result = TypeChecker::unify(alpha, int_t, substitution);

    assert(result);
    assert(substitution.size() == 1);
    assert(substitution.find("alpha") != substitution.end());
    assert(*substitution["alpha"] == *int_t);
    END_TEST

    TEST("Unification - function with variable")
    auto int_t = make_base_type("Int");
    auto alpha = make_variable_type("alpha");
    auto beta = make_variable_type("beta");

    auto func1 = make_function_type(alpha, beta);
    auto func2 = make_function_type(int_t, beta);

    std::unordered_map<std::string, TypePtr> substitution;
    bool result = TypeChecker::unify(func1, func2, substitution);

    assert(result);
    assert(substitution.size() == 1);
    assert(substitution.find("alpha") != substitution.end());
    assert(*substitution["alpha"] == *int_t);
    END_TEST

    TEST("Unification - variables on both sides")
    auto alpha = make_variable_type("alpha");
    auto beta = make_variable_type("beta");

    std::unordered_map<std::string, TypePtr> substitution;
    bool result = TypeChecker::unify(alpha, beta, substitution);

    assert(result);
    assert(substitution.size() == 1);
    // Either alpha -> beta or beta -> alpha, both are valid
    END_TEST

    TEST("Unification - complex function with variables")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");
    auto alpha = make_variable_type("alpha");
    auto beta = make_variable_type("beta");

    auto func1 = make_function_type({alpha, beta}, int_t);
    auto func2 = make_function_type({int_t, bool_t}, alpha);

    std::unordered_map<std::string, TypePtr> substitution;
    bool result = TypeChecker::unify(func1, func2, substitution);

    assert(result);
    assert(substitution.size() == 2);
    assert(*substitution["alpha"] == *int_t);
    assert(*substitution["beta"] == *bool_t);

    // Apply substitution to alpha and check result
    auto alpha_after_subst = TypeChecker::apply_substitution(alpha, substitution);
    assert(*alpha_after_subst == *int_t);
    END_TEST

    return true;
}

// Test substitution application
bool test_substitution_application()
{
    TEST("Substitution - apply to variable")
    auto int_t = make_base_type("Int");
    auto alpha = make_variable_type("alpha");

    std::unordered_map<std::string, TypePtr> substitution = {
        {"alpha", int_t}};

    auto result = TypeChecker::apply_substitution(alpha, substitution);
    assert(*result == *int_t);
    END_TEST

    TEST("Substitution - apply to function type")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");
    auto alpha = make_variable_type("alpha");
    auto beta = make_variable_type("beta");

    auto func = make_function_type(alpha, beta);
    std::unordered_map<std::string, TypePtr> substitution = {
        {"alpha", int_t},
        {"beta", bool_t}};

    auto result = TypeChecker::apply_substitution(func, substitution);
    auto expected = make_function_type(int_t, bool_t);

    assert(*result == *expected);
    END_TEST

    TEST("Substitution - apply to product type")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");
    auto alpha = make_variable_type("alpha");
    auto beta = make_variable_type("beta");

    auto prod = make_product_type({alpha, beta});
    std::unordered_map<std::string, TypePtr> substitution = {
        {"alpha", int_t},
        {"beta", bool_t}};

    auto result = TypeChecker::apply_substitution(prod, substitution);
    auto expected = make_product_type({int_t, bool_t});

    assert(*result == *expected);
    END_TEST

    TEST("Substitution - chained substitutions")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");
    auto alpha = make_variable_type("alpha");
    auto beta = make_variable_type("beta");
    auto gamma = make_variable_type("gamma");

    std::unordered_map<std::string, TypePtr> substitution = {
        {"alpha", beta},
        {"beta", gamma},
        {"gamma", int_t}};

    auto result = TypeChecker::apply_substitution(alpha, substitution);
    assert(*result == *int_t);
    END_TEST

    return true;
}

// Test record type mismatch
bool test_record_mismatch()
{
    TEST("Record mismatch - different field names")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");

    RecordType::FieldMap fields1 = {{"x", int_t}, {"y", bool_t}};
    RecordType::FieldMap fields2 = {{"a", int_t}, {"b", bool_t}};

    auto rec1 = make_record_type(fields1);
    auto rec2 = make_record_type(fields2);

    std::unordered_map<std::string, TypePtr> substitution;
    bool result = TypeChecker::unify(rec1, rec2, substitution);

    assert(!result); // Unification should fail
    END_TEST

    TEST("Record mismatch - different number of fields")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");

    RecordType::FieldMap fields1 = {{"x", int_t}, {"y", bool_t}};
    RecordType::FieldMap fields2 = {{"x", int_t}};

    auto rec1 = make_record_type(fields1);
    auto rec2 = make_record_type(fields2);

    std::unordered_map<std::string, TypePtr> substitution;
    bool result = TypeChecker::unify(rec1, rec2, substitution);

    assert(!result); // Unification should fail
    END_TEST

    TEST("Record mismatch - different field types")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");
    auto string_t = make_base_type("String");

    RecordType::FieldMap fields1 = {{"x", int_t}, {"y", bool_t}};
    RecordType::FieldMap fields2 = {{"x", int_t}, {"y", string_t}};

    auto rec1 = make_record_type(fields1);
    auto rec2 = make_record_type(fields2);

    std::unordered_map<std::string, TypePtr> substitution;
    bool result = TypeChecker::unify(rec1, rec2, substitution);

    assert(!result); // Unification should fail
    END_TEST

    TEST("Record with variables - successful unification")
    auto int_t = make_base_type("Int");
    auto alpha = make_variable_type("alpha");

    RecordType::FieldMap fields1 = {{"x", int_t}, {"y", alpha}};
    RecordType::FieldMap fields2 = {{"x", int_t}, {"y", int_t}};

    auto rec1 = make_record_type(fields1);
    auto rec2 = make_record_type(fields2);

    std::unordered_map<std::string, TypePtr> substitution;
    bool result = TypeChecker::unify(rec1, rec2, substitution);

    assert(result);
    assert(substitution.size() == 1);
    assert(*substitution["alpha"] == *int_t);
    END_TEST

    return true;
}

// Test sum type mismatch
bool test_sum_type_mismatch()
{
    TEST("Sum type mismatch - different constructor names")
    auto int_t = make_base_type("Int");

    std::vector<SumType::Constructor> constructors1 = {
        {"Just", {int_t}},
        {"Nothing", {}}};

    std::vector<SumType::Constructor> constructors2 = {
        {"Some", {int_t}},
        {"None", {}}};

    auto sum1 = make_sum_type(constructors1);
    auto sum2 = make_sum_type(constructors2);

    std::unordered_map<std::string, TypePtr> substitution;
    bool result = TypeChecker::unify(sum1, sum2, substitution);

    assert(!result); // Unification should fail
    END_TEST

    TEST("Sum type mismatch - different number of constructors")
    auto int_t = make_base_type("Int");

    std::vector<SumType::Constructor> constructors1 = {
        {"Just", {int_t}},
        {"Nothing", {}}};

    std::vector<SumType::Constructor> constructors2 = {
        {"Just", {int_t}}};

    auto sum1 = make_sum_type(constructors1);
    auto sum2 = make_sum_type(constructors2);

    std::unordered_map<std::string, TypePtr> substitution;
    bool result = TypeChecker::unify(sum1, sum2, substitution);

    assert(!result); // Unification should fail
    END_TEST

    TEST("Sum type mismatch - different constructor argument types")
    auto int_t = make_base_type("Int");
    auto bool_t = make_base_type("Bool");

    std::vector<SumType::Constructor> constructors1 = {
        {"Just", {int_t}},
        {"Nothing", {}}};

    std::vector<SumType::Constructor> constructors2 = {
        {"Just", {bool_t}},
        {"Nothing", {}}};

    auto sum1 = make_sum_type(constructors1);
    auto sum2 = make_sum_type(constructors2);

    std::unordered_map<std::string, TypePtr> substitution;
    bool result = TypeChecker::unify(sum1, sum2, substitution);

    assert(!result); // Unification should fail
    END_TEST

    TEST("Sum type with variables - successful unification")
    auto int_t = make_base_type("Int");
    auto alpha = make_variable_type("alpha");

    std::vector<SumType::Constructor> constructors1 = {
        {"Just", {alpha}},
        {"Nothing", {}}};

    std::vector<SumType::Constructor> constructors2 = {
        {"Just", {int_t}},
        {"Nothing", {}}};

    auto sum1 = make_sum_type(constructors1);
    auto sum2 = make_sum_type(constructors2);

    std::unordered_map<std::string, TypePtr> substitution;
    bool result = TypeChecker::unify(sum1, sum2, substitution);

    assert(result);
    assert(substitution.size() == 1);
    assert(*substitution["alpha"] == *int_t);
    END_TEST

    return true;
}

// Additional tests for built-in types
bool test_builtin_types()
{
    TEST("Built-in types - identity")
    auto int1 = int_type();
    auto int2 = int_type();
    assert(int1 == int2); // Should be the same instance (static)

    auto bool1 = bool_type();
    auto bool2 = bool_type();
    assert(bool1 == bool2); // Should be the same instance (static)

    auto prop1 = prop_type();
    auto prop2 = prop_type();
    assert(prop1 == prop2); // Should be the same instance (static)
    END_TEST

    TEST("Built-in types - prop vs bool")
    auto bool_t = bool_type();
    auto prop_t = prop_type();
    assert(!(*bool_t == *prop_t)); // Should be different types
    END_TEST

    return true;
}

// Main test runner
int main()
{
    bool all_passed = true;

    std::cout << "===== Running Type System Tests =====" << std::endl;

    all_passed &= test_type_equality_and_hash();
    all_passed &= test_unification_simple_function_types();
    all_passed &= test_unification_with_type_variables();
    all_passed &= test_substitution_application();
    all_passed &= test_record_mismatch();
    all_passed &= test_sum_type_mismatch();
    all_passed &= test_builtin_types();

    if (all_passed)
    {
        std::cout << "===== All tests passed! =====" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "===== Some tests failed! =====" << std::endl;
        return 1;
    }
}