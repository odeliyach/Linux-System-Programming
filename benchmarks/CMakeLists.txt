# Benchmark suite configuration
cmake_minimum_required(VERSION 3.14)

# Queue performance benchmark
add_executable(queue_benchmark
    queue_benchmark.c
)
target_link_libraries(queue_benchmark PRIVATE
    queue_lib
    common_utils
    Threads::Threads
)

# Add as a test (for CI integration)
add_test(NAME QueueBenchmark COMMAND queue_benchmark)
set_tests_properties(QueueBenchmark PROPERTIES
    LABELS "benchmark;performance"
    TIMEOUT 120
)
