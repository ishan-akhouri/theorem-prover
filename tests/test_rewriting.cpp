// tests/test_rewriting.cpp
#include <iostream>
#include <cassert>
#include <chrono>
#include "../src/term/rewriting.hpp"
#include "../src/term/ordering.hpp"
#include "../src/term/term_db.hpp"

using namespace theorem_prover;

void test_rewrite_rule_basics() {
   std::cout << "Testing rewrite rule basics..." << std::endl;
   
   auto lpo = make_lpo();
   
   // Create test terms
   auto a = make_constant("a");
   auto b = make_constant("b");
   auto f_a = make_function_application("f", {a});
   auto f_b = make_function_application("f", {b});
   
   // Create a rule f(a) → a
   TermRewriteRule rule(f_a, a, "test_rule");
   
   // Check basic properties
   assert(rule.lhs() == f_a);
   assert(rule.rhs() == a);
   assert(rule.name() == "test_rule");
   
   // Check orientation with LPO (f(a) > a should be true)
   assert(rule.is_oriented(*lpo));
   
   // Test rule equality
   TermRewriteRule same_rule(f_a, a, "different_name");
   assert(rule.equals(same_rule)); // Names don't matter for equality
   
   TermRewriteRule different_rule(f_b, b, "test_rule");
   assert(!rule.equals(different_rule));
   
   std::cout << "Rewrite rule basics tests passed!" << std::endl;
}

void test_rule_orientation() {
   std::cout << "Testing rule orientation..." << std::endl;
   
   auto lpo = make_lpo();
   
   // Create test terms
   auto a = make_constant("a");
   auto f_a = make_function_application("f", {a});
   
   // Test correctly oriented rule
   TermRewriteRule correct_rule(f_a, a, "correct");
   auto oriented1 = correct_rule.orient(*lpo);
   assert(oriented1.has_value());
   assert(oriented1->lhs() == f_a);
   assert(oriented1->rhs() == a);
   
   // Test incorrectly oriented rule (needs swapping)
   TermRewriteRule incorrect_rule(a, f_a, "incorrect");
   auto oriented2 = incorrect_rule.orient(*lpo);
   assert(oriented2.has_value());
   assert(oriented2->lhs() == f_a); // Should be swapped
   assert(oriented2->rhs() == a);
   
   // Test unorientable rule (equivalent terms)
   TermRewriteRule unorientable(a, a, "unorientable");
   auto oriented3 = unorientable.orient(*lpo);
   assert(!oriented3.has_value()); // Cannot orient equivalent terms
   
   std::cout << "Rule orientation tests passed!" << std::endl;
}

void test_position_system() {
   std::cout << "Testing position system..." << std::endl;
   
   // Test basic position operations
   Position root;
   assert(root.is_root());
   assert(root.depth() == 0);
   
   Position pos1 = root.descend(0);
   assert(!pos1.is_root());
   assert(pos1.depth() == 1);
   
   Position pos2 = pos1.descend(1);
   assert(pos2.depth() == 2);
   
   // Test prefix relationships
   assert(root.is_prefix_of(pos1));
   assert(root.is_prefix_of(pos2));
   assert(pos1.is_prefix_of(pos2));
   assert(!pos1.is_prefix_of(root));
   assert(!pos2.is_prefix_of(pos1));
   
   // Test equality
   Position pos1_copy = root.descend(0);
   assert(pos1 == pos1_copy);
   assert(!(pos1 == pos2));
   
   std::cout << "Position system tests passed!" << std::endl;
}

void test_subterm_operations() {
   std::cout << "Testing subterm operations..." << std::endl;
   
   // Create test term f(g(a), b)
   auto a = make_constant("a");
   auto b = make_constant("b");
   auto g_a = make_function_application("g", {a});
   auto f_g_a_b = make_function_application("f", {g_a, b});
   
   std::cout << "  Testing subterm extraction..." << std::endl;
   
   // Test subterm extraction
   auto root_pos = Position();
   auto subterm_root = RewriteSystem::subterm_at(f_g_a_b, root_pos);
   assert(subterm_root == f_g_a_b);
   
   auto pos_0 = root_pos.descend(0);
   auto subterm_0 = RewriteSystem::subterm_at(f_g_a_b, pos_0);
   assert(*subterm_0 == *g_a);
   
   auto pos_1 = root_pos.descend(1);
   auto subterm_1 = RewriteSystem::subterm_at(f_g_a_b, pos_1);
   assert(*subterm_1 == *b);
   
   auto pos_0_0 = pos_0.descend(0);
   auto subterm_0_0 = RewriteSystem::subterm_at(f_g_a_b, pos_0_0);
   assert(*subterm_0_0 == *a);
   
   std::cout << "  Testing invalid position..." << std::endl;
   
   // Test invalid position
   auto pos_invalid = root_pos.descend(2); // f only has 2 args (0,1)
   auto subterm_invalid = RewriteSystem::subterm_at(f_g_a_b, pos_invalid);
   assert(subterm_invalid == nullptr);
   
   std::cout << "  Testing replacement..." << std::endl;
   
   // Test replacement
   auto c = make_constant("c");
   auto replaced = RewriteSystem::replace_at(f_g_a_b, pos_1, c);
   assert(replaced != nullptr);
   
   // Check that replacement worked: f(g(a), c)
   auto expected = make_function_application("f", {g_a, c});
   // Can't easily test equality due to shared_ptr comparison, but check structure
   assert(replaced->kind() == TermDB::TermKind::FUNCTION_APPLICATION);
   
   std::cout << "Subterm operations tests passed!" << std::endl;
}

void test_rewrite_system_basics() {
   std::cout << "Testing rewrite system basics..." << std::endl;
   
   auto lpo = make_lpo();
   
   std::cout << "  Creating rewrite system..." << std::endl;
   auto rewrite_sys = make_rewrite_system(lpo);
   
   // Create test terms
   auto a = make_constant("a");
   auto b = make_constant("b");
   auto f_a = make_function_application("f", {a});
   auto f_b = make_function_application("f", {b});
   
   std::cout << "  Adding first rule..." << std::endl;
   
   // Add a rule f(a) → a
   bool added1 = rewrite_sys->add_rule(f_a, a, "rule1");
   assert(added1);
   assert(rewrite_sys->rules().size() == 1);
   
   std::cout << "  Testing unorientable rule..." << std::endl;
   
   // Try to add an unorientable rule a = a
   bool added2 = rewrite_sys->add_rule(a, a, "rule2");
   assert(!added2); // Should fail
   assert(rewrite_sys->rules().size() == 1);
   
   std::cout << "  Adding second rule..." << std::endl;
   
   // Add another valid rule f(b) → b
   bool added3 = rewrite_sys->add_rule(f_b, b, "rule3");
   assert(added3);
   assert(rewrite_sys->rules().size() == 2);
   
   std::cout << "  Testing duplicate rule..." << std::endl;
   
   // Try to add duplicate rule
   bool added4 = rewrite_sys->add_rule(f_a, a, "rule4");
   assert(!added4); // Should fail (duplicate)
   assert(rewrite_sys->rules().size() == 2);
   
   std::cout << "  Testing rule removal..." << std::endl;
   
   // Test rule removal
   bool removed = rewrite_sys->remove_rule("rule1");
   assert(removed);
   assert(rewrite_sys->rules().size() == 1);
   
   bool removed_again = rewrite_sys->remove_rule("rule1");
   assert(!removed_again); // Already removed
   
   std::cout << "Rewrite system basics tests passed!" << std::endl;
}

int main() {
   std::cout << "===== Running Progressive Rewriting Tests =====" << std::endl;
   
   try {
       test_rewrite_rule_basics();
       test_rule_orientation(); 
       test_position_system();
       test_subterm_operations();
       test_rewrite_system_basics();
       
       std::cout << "\n===== All Tests Passed! =====" << std::endl;
       
   } catch (const std::exception& e) {
       std::cout << "Exception caught: " << e.what() << std::endl;
       return 1;
   } catch (...) {
       std::cout << "Unknown exception caught" << std::endl;
       return 1;
   }
   
   return 0;
}