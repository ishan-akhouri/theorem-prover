# Theorem Prover

A high-performance interactive theorem prover for first-order logic with automated reasoning capabilities.

## Overview

This project implements a theorem prover that combines interactive proof construction with powerful automated tactics. It features a robust architecture for representing first-order logic formulas, efficient proof state management, and composable proof strategies through a tactic framework.

## Features

- **Dual term representation** - Both De Bruijn indexed terms (for efficient internal manipulation) and named terms (for readable user interaction)
- **Rich type system** - Support for base types, function types, product types, records, and algebraic data types
- **First-order logic** - Complete implementation of first-order logic with quantifiers, logical connectives, and equality reasoning
- **Substitution engine** - Sophisticated term substitution with proper handling of variable capture
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
├── README.md
├── scripts
├── src
│   ├── proof              # Proof state management and tactics
│   │   ├── goal_manager.cpp
│   │   ├── goal_manager.hpp
│   │   ├── proof_state.cpp
│   │   ├── proof_state.hpp
│   │   ├── tactic.cpp
│   │   └── tactic.hpp
│   ├── rule               # Logical inference rules
│   │   ├── proof_rule.cpp
│   │   └── proof_rule.hpp
│   ├── term               # Term representation
│   │   ├── substitution.cpp
│   │   ├── substitution.hpp
│   │   ├── term_db.cpp
│   │   ├── term_db.hpp
│   │   ├── term_named.cpp
│   │   └── term_named.hpp
│   ├── type               # Type system
│   │   ├── type.cpp
│   │   └── type.hpp
│   └── utils              # Utility functions
│       ├── gensym.hpp
│       └── hash.hpp
└── tests                  # Test suite
    ├── test_core_architecture.cpp
    ├── test_goal_manager.cpp
    ├── test_proof_rule.cpp
    ├── test_proof_state.cpp
    ├── test_substitution.cpp
    ├── test_tactic.cpp
    ├── test_term_conversion_roundtrip.cpp
    └── test_type.cpp
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

## Contributing

This project will not accepting external contributions.
Please do not submit pull requests or open issues requesting new features.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Documentation

See the full architecture and evaluation writeup here: [Core Architecture of a Theorem Prover](https://medium.com/@theoreticalcs/core-architecture-of-a-theorem-prover-24a6ea5a0fa8)
