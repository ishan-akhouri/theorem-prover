# Theorem Prover

A high-performance automated theorem prover for first-order logic with automated reasoning capabilities.

## Overview

This project implements a theorem prover that combines interactive proof construction with powerful automated tactics. It features a robust architecture for representing first-order logic formulas, efficient proof state management, and composable proof strategies through a tactic framework. Recent additions include efficient proof search through resolution and paramodulation, and sophisticated equality handling via Knuth-Bendix completion.

## Core Algorithms

- **Resolution** - Complete implementation of the resolution principle for first-order logic with clause indexing and optimization strategies. Provides efficient proof search for satisfiability problems with sophisticated clause selection heuristics.

- **Unification** - Robinson's unification algorithm for first-order terms with occurs check and efficient substitution composition. Handles complex unification problems with proper variable scoping and substitution propagation.

- **Paramodulation for Equality Reasoning** - Advanced paramodulation implementation for handling equality in first-order logic. Features strategic term selection, orientation controls, and integration with the resolution framework for complete equality reasoning.

- **Knuth-Bendix Completion** - Full implementation of the Knuth-Bendix completion algorithm for term rewriting systems. Includes critical pair computation, termination ordering, and sophisticated completion strategies with configurable resource limits.


## Features

- **Dual term representation** - Both De Bruijn indexed terms (for efficient internal manipulation) and named terms (for readable user interaction)
- **Rich type system** - Support for base types, function types, product types, records, and algebraic data types
- **First-order logic** - Complete implementation of first-order logic with quantifiers, logical connectives, and equality reasoning
- **Substitution engine** - Sophisticated term substitution with proper handling of variable capture
- **Resolution-based theorem proving** - Complete resolution method with clause indexing and optimization
- **Unification with occurs check** - Robinson's algorithm with efficiency optimizations
- **Paramodulation framework** - Complete equality reasoning with strategic term orientation
- **Knuth-Bendix completion** - Term rewriting system completion with critical pair computation
- **Proof state management** - Immutable proof states with hypotheses, goals, and proof history tracking
- **Logical inference rules** - Implementation of standard natural deduction rules
- **Tactic framework** - Composable proof strategies via tactic combinators
- **Goal management** - Intelligent handling of complex goals through decomposition and recombination
- **Fast automated proof search** - Optimized algorithms for efficient automated reasoning

## Installation

### Prerequisites

- CMake (3.14 or higher)
- C++17 compatible compiler
- Boost library (for testing)

### Building with CMake

```bash
# Clone the repository
git clone https://github.com/ishan-akhouri/theorem-prover.git
cd theorem-prover

# Create build directory
mkdir build && cd build

# Generate build files
cmake ..

# Build
cmake --build .
```

### Building directly with compilers

#### Using g++

```bash
g++ -std=c++17 -I./src -o theorem_prover src/**/*.cpp tests/*.cpp
```

#### Using clang++

```bash
clang++ -std=c++17 -I./src -o theorem_prover src/**/*.cpp tests/*.cpp
```

## Project Structure

```
.
├── CMakeLists.txt
├── LICENSE
├── project_structure.txt
├── README.md
├── src
│   ├── completion
│   │   ├── critical_pairs.cpp
│   │   ├── critical_pairs.hpp
│   │   ├── knuth_bendix.cpp
│   │   └── knuth_bendix.hpp
│   ├── proof
│   │   ├── goal_manager.cpp
│   │   ├── goal_manager.hpp
│   │   ├── proof_state.cpp
│   │   ├── proof_state.hpp
│   │   ├── tactic.cpp
│   │   └── tactic.hpp
│   ├── resolution
│   │   ├── clause.cpp
│   │   ├── clause.hpp
│   │   ├── cnf_converter.cpp
│   │   ├── cnf_converter.hpp
│   │   ├── indexing.cpp
│   │   ├── indexing.hpp
│   │   ├── resolution_prover.cpp
│   │   └── resolution_prover.hpp
│   ├── rule
│   │   ├── proof_rule.cpp
│   │   └── proof_rule.hpp
│   ├── term
│   │   ├── ordering.cpp
│   │   ├── ordering.hpp
│   │   ├── rewriting.cpp
│   │   ├── rewriting.hpp
│   │   ├── substitution.cpp
│   │   ├── substitution.hpp
│   │   ├── term_db.cpp
│   │   ├── term_db.hpp
│   │   ├── term_named.cpp
│   │   ├── term_named.hpp
│   │   ├── unification.cpp
│   │   └── unification.hpp
│   ├── type
│   │   ├── type.cpp
│   │   └── type.hpp
│   └── utils
│       ├── gensym.hpp
│       └── hash.hpp
└── tests
    ├── test_challenging_benchmark.cpp
    ├── test_clause.cpp
    ├── test_cnf_converter.cpp
    ├── test_core_architecture.cpp
    ├── test_critical_pairs.cpp
    ├── test_goal_manager.cpp
    ├── test_indexing_performance.cpp
    ├── test_kb_resolution_benchmark.cpp
    ├── test_knuth_bendix.cpp
    ├── test_ordering.cpp
    ├── test_paramodulation.cpp
    ├── test_proof_rule.cpp
    ├── test_proof_state.cpp
    ├── test_resolution_comparison.cpp
    ├── test_resolution_prover.cpp
    ├── test_rewriting.cpp
    ├── test_substitution.cpp
    ├── test_subsumption.cpp
    ├── test_tactic.cpp
    ├── test_term_conversion_roundtrip.cpp
    ├── test_type.cpp
    ├── test_unification.cpp
    └── test_variable_standardization.cpp
```

## Usage

The theorem prover can be used both as a library in other projects and as a standalone tool for proving theorems.

### Basic Example

```cpp
#include "term/term_named.hpp"
#include "proof/proof_state.hpp"
#include "proof/tactic.hpp"

using namespace theorem_prover;

int main() {
    // Create formula: ∀x. P(x) → Q(x)
    auto forall_impl = make_named_forall("x", 
        make_named_implies(
            make_named_function_application("P", {make_named_variable("x")}),
            make_named_function_application("Q", {make_named_variable("x")})
        )
    );
    
    // Create formula: P(f(y))
    auto p_fy = make_named_function_application("P", {
        make_named_function_application("f", {make_named_variable("y")})
    });
    
    // Convert to De Bruijn indexed terms
    auto forall_impl_db = to_db(forall_impl);
    auto p_fy_db = to_db(p_fy);
    
    // Set up proof context with goal Q(f(y))
    auto q_fy = make_named_function_application("Q", {
        make_named_function_application("f", {make_named_variable("y")})
    });
    auto goal = to_db(q_fy);
    
    ProofContext context;
    auto initial_state = context.create_initial_state(goal);
    
    // Add hypotheses
    auto with_hypotheses = context.apply_rule(
        initial_state,
        "add_hypotheses",
        {},
        {Hypothesis("forall_rule", forall_impl_db), Hypothesis("p_fy", p_fy_db)},
        goal
    );
    
    // Define and apply tactics
    auto mp_with_forall = sequence({
        form_useful_conjunctions(),
        match_mp_antecedent(),
        direct_proof()
    });
    
    auto result = mp_with_forall(context, with_hypotheses, std::nullopt);
    
    // Check if proof succeeded
    if (!result.empty() && result[0]->is_proved()) {
        std::cout << "Theorem proved successfully!" << std::endl;
    }
    
    return 0;
}
```
### Advanced Example: Using Resolution and Paramodulation
```cpp
#include "resolution/resolution_prover.hpp"
#include "resolution/cnf_converter.hpp"
#include "term/unification.hpp"
#include "completion/knuth_bendix.hpp"

using namespace theorem_prover;

int main() {
    // Example: Equality reasoning with paramodulation
    // Problem: f(a) = b ∧ P(f(a)) ⊢ P(b)
    
    // Create clauses
    auto eq_clause = make_clause({make_equality(make_function("f", {make_constant("a")}), 
                                                make_constant("b"))});
    auto premise_clause = make_clause({make_predicate("P", {make_function("f", {make_constant("a")})})});
    auto goal_clause = make_clause({make_negation(make_predicate("P", {make_constant("b")}))});
    
    // Set up resolution prover with paramodulation
    ResolutionProver prover;
    prover.add_clause(eq_clause);
    prover.add_clause(premise_clause);
    prover.add_clause(goal_clause);
    
    // Enable paramodulation for equality reasoning
    prover.enable_paramodulation(true);
    
    // Run automated proof search
    auto result = prover.prove();
    
    if (result.is_contradiction()) {
        std::cout << "Proof found via resolution + paramodulation!" << std::endl;
        std::cout << "Steps: " << result.steps() << std::endl;
        std::cout << "Clauses generated: " << result.clause_count() << std::endl;
    }
    
    // Alternative: Use Knuth-Bendix preprocessing
    KnuthBendixCompletion kb;
    kb.add_equation(make_equation(make_function("f", {make_constant("a")}), 
                                  make_constant("b")));
    
    auto completion_result = kb.complete();
    if (completion_result.success) {
        std::cout << "KB completion succeeded with " 
                  << completion_result.rules.size() << " rules" << std::endl;
        
        // Use completed system for more efficient reasoning
        prover.set_rewrite_system(completion_result.rules);
        auto kb_result = prover.prove();
        
        std::cout << "KB-optimized proof: " << kb_result.steps() << " steps" << std::endl;
    }
    
    return 0;
}
```
## Testing and Benchmark Results

The project includes a comprehensive test suite that verifies core functionality and demonstrates performance on standard theorem proving benchmarks.

To run the tests:

```bash
cd build
ctest
```

### Benchmark Results

These benchmarks were conducted on a 2024 fanless macbook air (M3, 16 GB unified memory, MacOS Sequoia).

```
====================================================================================
| BENCHMARK SUMMARY                                                                |
====================================================================================
| Theorem                                   | Success | Time (ms) | States | Steps |
|-------------------------------------------|---------|-----------|--------|-------|
| ∀x. P(x) → Q(x), P(f(y)) ⊢ Q(f(y))        | ✓       | 0.251     | 5      | 4     |
| ∀x. P(x) ∧ Q(x) → R(x), P(a), Q(a) ⊢ R(a) | ✓       | 0.206     | 7      | 6     |
| ∃x. P(x) ∨ Q(x) ⊢ ∃x. Q(x) ∨ P(x)         | ✓       | 0.086     | 4      | 3     |
| ¬(∃x. P(x)) ⊢ ∀x. ¬P(x)                   | ✓       | 0.040     | 3      | 2     |
====================================================================================

========================================================================================================================================================
| COMPARISON: Resolution vs Paramodulation vs KB-Optimized Paramodulation                                                                              |
========================================================================================================================================================
| Problem                          | Basic Resolution                  | Standard Paramodulation              | KB-Optimized Paramodulation            |
|----------------------------------|-----------------------------------|--------------------------------------|----------------------------------------|
| Simple Equality                  | SATURATED (1.0ms, 3 iter, 3 cls)  | PROVED (0.0ms, 2 iter, 5 cls)        | PROVED (0.0ms, 1 iter, 5 cls)          |
| Symmetric Equality               | SATURATED (0.0ms, 3 iter, 3 cls)  | PROVED (0.0ms, 1 iter, 5 cls)        | PROVED (0.0ms, 1 iter, 5 cls)          |
| Transitivity                     | SATURATED (0.0ms, 3 iter, 3 cls)  | PROVED (0.0ms, 1 iter, 7 cls)        | PROVED (1.0ms, 2 iter, 13 cls)         |
| Function Equality                | SATURATED (0.0ms, 3 iter, 3 cls)  | PROVED (0.0ms, 2 iter, 5 cls)        | PROVED (0.0ms, 1 iter, 5 cls)          |
| Chain Equality                   | SATURATED (0.0ms, 5 iter, 5 cls)  | PROVED (1.0ms, 5 iter, 16 cls)       | SATURATED (3.0ms, 13 iter, 13 cls)     |
| Complex Equality                 | SATURATED (0.0ms, 4 iter, 4 cls)  | PROVED (0.0ms, 3 iter, 11 cls)       | PROVED (0.0ms, 1 iter, 7 cls)          |
| Long Equality Chain (7 steps)    | SATURATED (0.9ms, 9 iter, 9 cls)  | PROVED (14.5ms, 9 iter, 50 cls)      | SATURATED (67.5ms, 57 iter, 57 cls)    |
| Recursive Function Definitions   | SATURATED (0.1ms, 4 iter, 4 cls)  | PROVED (2.2ms, 3 iter, 25 cls)       | PROVED (10040.6ms, 3 iter, 25 cls)     |
| Deep Nested Structures           | SATURATED (0.1ms, 5 iter, 5 cls)  | PROVED (1.4ms, 4 iter, 18 cls)       | PROVED (1.5ms, 5 iter, 17 cls)         |
| Associativity-like Problem       | SATURATED (0.1ms, 3 iter, 3 cls)  | SATURATED (0.3ms, 4 iter, 4 cls)     | SATURATED (0.5ms, 4 iter, 4 cls)       |
| Large Equality System (12 vars)  | SATURATED (0.5ms, 16 iter, 16 cls)| PROVED (54.9ms, 16 iter, 161 cls)    | PROVED (2.0ms, 1 iter, 23 cls)         |
| Mixed Complexity Web             | SATURATED (0.1ms, 7 iter, 7 cls)  | PROVED (54.7ms, 6 iter, 184 cls)     | SATURATED (4677.1ms, 777 iter, 777 cls)|
========================================================================================================================================================
```
## Key Components

### Type System

The type system supports a rich hierarchy of types including base types, function types, product types, records, and sum types. It also provides type checking and unification capabilities.

### Term Representation

Terms can be represented both with De Bruijn indices (which simplify substitution and alpha-equivalence) and with named variables (which are more readable). Conversion functions allow seamless transitions between the two representations.

### Proof State Management

The proof state management system keeps track of the current hypotheses, goal, and proof history. It ensures immutability of proof states for reliable backtracking and proof exploration.

### Tactics Framework

The tactics framework provides high-level proof strategies that can be composed using combinators like `sequence`, `repeat`, `first`, and `orelse`. This allows for the creation of sophisticated proof automation.

### Resolution Method

Complete implementation of the resolution principle with clause indexing, CNF conversion, and optimized clause selection strategies for efficient automated theorem proving.

### Unification Algorithm

Robinson's unification algorithm with occurs check, providing the foundation for resolution and paramodulation with proper variable handling and substitution composition.

### Paramodulation System

Advanced equality reasoning through paramodulation with strategic term orientation and integration with resolution for complete first-order theorem proving.

### Knuth-Bendix Completion

Term rewriting system completion with critical pair computation and termination ordering, demonstrating characteristic structural sensitivity in automated reasoning.


## Contributing

This project will not accepting external contributions.
Please do not submit pull requests or open issues requesting new features.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Documentation
Linked are the Medium articles I wrote exploring the implementation, results, design motives, and more.
[Core Architecture of a Theorem Prover](https://medium.com/@theoreticalcs/core-architecture-of-a-theorem-prover-24a6ea5a0fa8)
[Fundamental Algorithms of a Theorem Prover](https://medium.com/@theoreticalcs/fundamental-algorithms-of-a-theorem-prover-ecdadb8a5d6d)
