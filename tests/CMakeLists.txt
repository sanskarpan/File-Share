find_package(GTest REQUIRED)

# Test sources
set(TEST_SOURCES
    test_peer.cpp
    test_file_manager.cpp
    test_protocol.cpp
    test_thread_pool.cpp
    test_performance.cpp
)

# Create test executable
add_executable(p2p-tests ${TEST_SOURCES})

target_link_libraries(p2p-tests
    p2p-core
    GTest::GTest
    GTest::Main
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Add tests to CTest
add_test(NAME unit-tests COMMAND p2p-tests)
add_test(NAME performance-tests COMMAND p2p-tests --gtest_filter="Performance*")

# Valgrind memory check (if available)
find_program(VALGRIND_PROGRAM valgrind)
if(VALGRIND_PROGRAM)
    add_test(NAME memory-check
        COMMAND ${VALGRIND_PROGRAM} 
        --tool=memcheck 
        --leak-check=full 
        --error-exitcode=1 
        ${CMAKE_BINARY_DIR}/tests/p2p-tests
    )
endif()

# Coverage target (if gcov available)
option(ENABLE_COVERAGE "Enable coverage reporting" OFF)
if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    
    find_program(GCOVR_PROGRAM gcovr)
    if(GCOVR_PROGRAM)
        add_custom_target(coverage
            COMMAND ${CMAKE_BINARY_DIR}/tests/p2p-tests
            COMMAND ${GCOVR_PROGRAM} --root ${CMAKE_SOURCE_DIR} --html --html-details -o coverage.html
            DEPENDS p2p-tests
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    endif()
endif()