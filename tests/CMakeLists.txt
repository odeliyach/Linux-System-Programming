# Test suite configuration
cmake_minimum_required(VERSION 3.14)

# Queue tests
add_executable(queue_test_runner
    queue/test_queue.c
)
target_link_libraries(queue_test_runner PRIVATE
    queue_lib
    common_utils
    Threads::Threads
)

# Shell tests (integration tests)
add_executable(shell_test_runner
    shell/test_shell.c
)
target_link_libraries(shell_test_runner PRIVATE
    shell_lib
    common_utils
)

# Integration tests
add_executable(integration_test_runner
    integration/test_integration.c
)
target_link_libraries(integration_test_runner PRIVATE
    shell_lib
    queue_lib
    common_utils
    Threads::Threads
)

# Register tests with CTest
add_test(NAME QueueTests COMMAND queue_test_runner)
add_test(NAME ShellTests COMMAND shell_test_runner)
add_test(NAME IntegrationTests COMMAND integration_test_runner)

# Set test properties
set_tests_properties(QueueTests PROPERTIES
    TIMEOUT 30
    LABELS "unit;queue"
)

set_tests_properties(ShellTests PROPERTIES
    TIMEOUT 30
    LABELS "integration;shell"
)

set_tests_properties(IntegrationTests PROPERTIES
    TIMEOUT 60
    LABELS "integration"
)
