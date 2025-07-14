cmake_minimum_required(VERSION 3.10)
project(theorem_prover)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR})

# Source files
set(SOURCES
    src/term/term_db.cpp
    src/term/term_named.cpp
    src/term/substitution.cpp
    src/type/type.cpp
    src/proof/proof_state.cpp
    src/rule/proof_rule.cpp
    src/proof/tactic.cpp
    src/proof/goal_manager.cpp
    src/term/unification.cpp
    src/resolution/clause.cpp
    src/resolution/cnf_converter.cpp
    src/resolution/resolution_prover.cpp
    src/resolution/indexing.cpp
    src/term/ordering.cpp
    src/term/rewriting.cpp
    src/completion/critical_pairs.cpp
    src/completion/knuth_bendix.cpp
)

# Test executables
add_executable(test_substitution tests/test_substitution.cpp ${SOURCES})
add_executable(test_term_conversion_roundtrip tests/test_term_conversion_roundtrip.cpp ${SOURCES})
add_executable(test_type tests/test_type.cpp ${SOURCES})
add_executable(test_proof_state tests/test_proof_state.cpp ${SOURCES})
add_executable(test_proof_rule tests/test_proof_rule.cpp ${SOURCES})
add_executable(test_tactic tests/test_tactic.cpp ${SOURCES})
add_executable(test_core_architecture tests/test_core_architecture.cpp ${SOURCES})
add_executable(test_goal_manager tests/test_goal_manager.cpp ${SOURCES})
add_executable(test_unification tests/test_unification.cpp ${SOURCES})
add_executable(test_clause tests/test_clause.cpp ${SOURCES})
add_executable(test_cnf_converter tests/test_cnf_converter.cpp ${SOURCES})
add_executable(test_resolution_prover tests/test_resolution_prover.cpp ${SOURCES})
add_executable(test_variable_standardization tests/test_variable_standardization.cpp ${SOURCES})
add_executable(test_subsumption tests/test_subsumption.cpp ${SOURCES})
add_executable(test_indexing_performance tests/test_indexing_performance.cpp ${SOURCES})
add_executable(test_ordering tests/test_ordering.cpp ${SOURCES})
add_executable(test_rewriting tests/test_rewriting.cpp ${SOURCES})
add_executable(test_paramodulation tests/test_paramodulation.cpp ${SOURCES})
add_executable(test_resolution_comparison tests/test_resolution_comparison.cpp ${SOURCES})
add_executable(test_critical_pairs tests/test_critical_pairs.cpp ${SOURCES})
add_executable(test_knuth_bendix tests/test_knuth_bendix.cpp ${SOURCES})
add_executable(test_kb_resolution_benchmark tests/test_kb_resolution_benchmark.cpp ${SOURCES})
add_executable(test_challenging_benchmark tests/test_challenging_benchmark.cpp ${SOURCES})

# Tests
enable_testing()
add_test(NAME TestSubstitution COMMAND test_substitution)
add_test(NAME TestTermConversion COMMAND test_term_conversion_roundtrip)
add_test(NAME TestType COMMAND test_type)
add_test(NAME TestProofState COMMAND test_proof_state)
add_test(NAME TestProofRule COMMAND test_proof_rule)
add_test(NAME TestTactic COMMAND test_tactic)
add_test(NAME TestCoreArchitecture COMMAND test_core_architecture)