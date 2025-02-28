#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <functional>
#include <numeric>
#include <random>
#include "../include/relinx.hpp"        // Original implementation
#include "../include/relinx_self.hpp"   // New implementation

// Namespace aliases for clarity
namespace orig = nstd::relinx;
namespace self = nstd::relinx_self;

// Timing utility
class Timer {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    std::string operation_name;

public:
    Timer(const std::string& name) : operation_name(name) {
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    ~Timer() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        std::cout << std::left << std::setw(50) << operation_name 
                  << ": " << std::right << std::setw(10) << duration << " µs" << std::endl;
    }
};

// Generate a large dataset for testing
std::vector<int> generate_data(size_t size) {
    std::vector<int> data(size);
    std::iota(data.begin(), data.end(), 1); // Fill with 1 to size
    
    // Shuffle the data
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(data.begin(), data.end(), g);
    
    return data;
}

// Helper to prevent compiler from optimizing away results
volatile int sink = 0;
template <typename T>
void prevent_optimize(const T& value) {
    sink = static_cast<int>(value);
}

// Benchmark 1: Simple operations
void benchmark_simple_operations(const std::vector<int>& data, int iterations) {
    std::cout << "\n=== Benchmark 1: Simple Operations (" << iterations << " iterations) ===\n";
    
    // Original implementation
    {
        Timer timer("Original: from->where->select->to_vector");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->where([](int n) { return n % 2 == 0; })
                ->select([](int n) { return n * n; })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // Self implementation
    {
        Timer timer("Self-reference: from->where->select->to_vector");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->where([](int n) { return n % 2 == 0; })
                ->select([](int n) { return n * n; })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
}

// Benchmark 2: Complex chained operations
void benchmark_complex_operations(const std::vector<int>& data, int iterations) {
    std::cout << "\n=== Benchmark 2: Complex Chained Operations (" << iterations << " iterations) ===\n";
    
    // Original implementation
    {
        Timer timer("Original: Complex chain of operations");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->where([](int n) { return n % 3 == 0; })
                ->select([](int n) { return n * 2; })
                ->distinct()
                ->take(data.size() / 4)
                ->where([](int n) { return n > 100; })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // Self implementation
    {
        Timer timer("Self-reference: Complex chain of operations");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->where([](int n) { return n % 3 == 0; })
                ->select([](int n) { return n * 2; })
                ->distinct()
                ->take(data.size() / 4)
                ->where([](int n) { return n > 100; })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
}

// Benchmark 3: Testing lazy evaluation with tee
void benchmark_lazy_evaluation(const std::vector<int>& data, int iterations) {
    std::cout << "\n=== Benchmark 3: Lazy Evaluation (" << iterations << " iterations) ===\n";
    
    // Original implementation
    {
        Timer timer("Original: Lazy evaluation with tee");
        for (int i = 0; i < iterations; i++) {
            int counter = 0;
            auto result = orig::from(data)
                ->where([](int n) { return n % 5 == 0; })
                ->tee([&counter](int) { counter++; })
                ->select([](int n) { return n * n; })
                ->to_vector();
            prevent_optimize(result.size() + counter);
        }
    }
    
    // Self implementation
    {
        Timer timer("Self-reference: Lazy evaluation with tee");
        for (int i = 0; i < iterations; i++) {
            int counter = 0;
            auto result = self::from(data)
                ->where([](int n) { return n % 5 == 0; })
                ->tee([&counter](int) { counter++; })
                ->select([](int n) { return n * n; })
                ->to_vector();
            prevent_optimize(result.size() + counter);
        }
    }
}

// Benchmark 4: Aggregate operations
void benchmark_aggregate_operations(const std::vector<int>& data, int iterations) {
    std::cout << "\n=== Benchmark 4: Aggregate Operations (" << iterations << " iterations) ===\n";
    
    // Original implementation
    {
        Timer timer("Original: aggregate (sum)");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->where([](int n) { return n % 2 == 0; })
                ->aggregate([](int acc, int val) { return acc + val; });
            prevent_optimize(result);
        }
    }
    
    // Self implementation
    {
        Timer timer("Self-reference: aggregate (sum)");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->where([](int n) { return n % 2 == 0; })
                ->aggregate([](int acc, int val) { return acc + val; });
            prevent_optimize(result);
        }
    }
    
    // Original implementation with seed
    {
        Timer timer("Original: aggregate with seed");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->where([](int n) { return n % 2 == 0; })
                ->aggregate(1000, [](int acc, int val) { return acc + val; });
            prevent_optimize(result);
        }
    }
    
    // Self implementation with seed
    {
        Timer timer("Self-reference: aggregate with seed");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->where([](int n) { return n % 2 == 0; })
                ->aggregate(1000, [](int acc, int val) { return acc + val; });
            prevent_optimize(result);
        }
    }
}

// Benchmark 5: Memory/performance with large number of objects
void benchmark_memory_performance(int iterations) {
    std::cout << "\n=== Benchmark 5: Memory/Performance with Multiple Objects (" << iterations << " iterations) ===\n";
    
    std::vector<int> small_data = {1, 2, 3, 4, 5};
    
    // Original implementation
    {
        Timer timer("Original: Creating many objects");
        for (int i = 0; i < iterations; i++) {
            for (int j = 0; j < 100; j++) {
                auto obj = orig::from(small_data)
                    ->where([](int n) { return n > 0; });
                prevent_optimize(obj->count());
            }
        }
    }
    
    // Self implementation
    {
        Timer timer("Self-reference: Creating many objects");
        for (int i = 0; i < iterations; i++) {
            for (int j = 0; j < 100; j++) {
                auto obj = self::from(small_data)
                    ->where([](int n) { return n > 0; });
                prevent_optimize(obj->count());
            }
        }
    }
}

// Benchmark 6: First/Last/Element_at operations
void benchmark_first_last_element_operations(const std::vector<int>& data, int iterations) {
    std::cout << "\n=== Benchmark 6: First/Last/Element_at Operations (" << iterations << " iterations) ===\n";
    
    // Original implementation for first/first_or_default
    {
        Timer timer("Original: first/first_or_default");
        for (int i = 0; i < iterations; i++) {
            try {
                auto result = orig::from(data)
                    ->where([](int n) { return n % 7 == 0; })
                    ->first();
                prevent_optimize(result);
            } catch (...) {
                // Handle exception if no matching element
            }

            auto result2 = orig::from(data)
                ->where([](int n) { return n % 11 == 0; })
                ->first_or_default();
            prevent_optimize(result2);
        }
    }
    
    // Self implementation for first/first_or_default
    {
        Timer timer("Self-reference: first/first_or_default");
        for (int i = 0; i < iterations; i++) {
            try {
                auto result = self::from(data)
                    ->where([](int n) { return n % 7 == 0; })
                    ->first();
                prevent_optimize(result);
            } catch (...) {
                // Handle exception if no matching element
            }
            
            auto result2 = self::from(data)
                ->where([](int n) { return n % 11 == 0; })
                ->first_or_default();
            prevent_optimize(result2);
        }
    }
    
    // Original implementation for last/last_or_default
    {
        Timer timer("Original: last/last_or_default");
        for (int i = 0; i < iterations; i++) {
            try {
                auto result = orig::from(data)
                    ->where([](int n) { return n % 7 == 0; })
                    ->last();
                prevent_optimize(result);
            } catch (...) {
                // Handle exception if no matching element
            }
            
            auto result2 = orig::from(data)
                ->where([](int n) { return n % 11 == 0; })
                ->last_or_default();
            prevent_optimize(result2);
        }
    }
    
    // Self implementation for last/last_or_default
    {
        Timer timer("Self-reference: last/last_or_default");
        for (int i = 0; i < iterations; i++) {
            try {
                auto result = self::from(data)
                    ->where([](int n) { return n % 7 == 0; })
                    ->last();
                prevent_optimize(result);
            } catch (...) {
                // Handle exception if no matching element
            }
            
            auto result2 = self::from(data)
                ->where([](int n) { return n % 11 == 0; })
                ->last_or_default();
            prevent_optimize(result2);
        }
    }
    
    // Original implementation for element_at/element_at_or_default
    {
        Timer timer("Original: element_at/element_at_or_default");
        for (int i = 0; i < iterations; i++) {
            try {
                auto result = orig::from(data)
                    ->where([](int n) { return n % 3 == 0; })
                    ->element_at(5);
                prevent_optimize(result);
            } catch (...) {
                // Handle exception if index out of range
            }
            
            auto result2 = orig::from(data)
                ->where([](int n) { return n % 3 == 0; })
                ->element_at_or_default(50);
            prevent_optimize(result2);
        }
    }
    
    // Self implementation for element_at/element_at_or_default
    {
        Timer timer("Self-reference: element_at/element_at_or_default");
        for (int i = 0; i < iterations; i++) {
            try {
                auto result = self::from(data)
                    ->where([](int n) { return n % 3 == 0; })
                    ->element_at(5);
                prevent_optimize(result);
            } catch (...) {
                // Handle exception if index out of range
            }
            
            auto result2 = self::from(data)
                ->where([](int n) { return n % 3 == 0; })
                ->element_at_or_default(50);
            prevent_optimize(result2);
        }
    }
}

// Benchmark 7: Skip/Skip_while operations
void benchmark_skip_operations(const std::vector<int>& data, int iterations) {
    std::cout << "\n=== Benchmark 7: Skip/Skip_while Operations (" << iterations << " iterations) ===\n";
    
    // Original implementation for skip
    {
        Timer timer("Original: skip");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->skip(data.size() / 2)
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // Self implementation for skip
    {
        Timer timer("Self-reference: skip");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->skip(data.size() / 2)
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // Original implementation for skip_while
    {
        Timer timer("Original: skip_while");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->skip_while([](int n) { return n < 5000; })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // Self implementation for skip_while
    {
        Timer timer("Self-reference: skip_while");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->skip_while([](int n) { return n < 5000; })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
}

// Benchmark 8: Single/Single_or_default operations
void benchmark_single_operations(const std::vector<int>& data, int iterations) {
    std::cout << "\n=== Benchmark 8: Single/Single_or_default Operations (" << iterations << " iterations) ===\n";
    
    // Original implementation for single/single_or_default
    {
        Timer timer("Original: single/single_or_default");
        for (int i = 0; i < iterations; i++) {
            try {
                // Find exactly one element equal to 1000 (should be just one in shuffled data)
                auto result = orig::from(data)
                    ->where([](int n) { return n == 1000; })
                    ->single();
                prevent_optimize(result);
            } catch (...) {
                // Handle exception if no matching element or multiple elements
            }
            
            // Find element with specific criteria that might not exist
            auto result2 = orig::from(data)
                ->where([](int n) { return n == 20000; })  // Should not exist
                ->single_or_default();
            prevent_optimize(result2);
        }
    }
    
    // Self implementation for single/single_or_default
    {
        Timer timer("Self-reference: single/single_or_default");
        for (int i = 0; i < iterations; i++) {
            try {
                // Find exactly one element equal to 1000 (should be just one in shuffled data)
                auto result = self::from(data)
                    ->where([](int n) { return n == 1000; })
                    ->single();
                prevent_optimize(result);
            } catch (...) {
                // Handle exception if no matching element or multiple elements
            }
            
            // Find element with specific criteria that might not exist
            auto result2 = self::from(data)
                ->where([](int n) { return n == 20000; })  // Should not exist
                ->single_or_default();
            prevent_optimize(result2);
        }
    }
}

// Benchmark 9: Min/Max operations
void benchmark_min_max_operations(const std::vector<int>& data, int iterations) {
    std::cout << "\n=== Benchmark 9: Min/Max Operations (" << iterations << " iterations) ===\n";
    
    // Original implementation for min/max
    {
        Timer timer("Original: min/max");
        for (int i = 0; i < iterations; i++) {
            auto min_result = orig::from(data)
                ->min();
            prevent_optimize(min_result);
            
            auto max_result = orig::from(data)
                ->max();
            prevent_optimize(max_result);
        }
    }
    
    // Self implementation for min/max
    {
        Timer timer("Self-reference: min/max");
        for (int i = 0; i < iterations; i++) {
            auto min_result = self::from(data)
                ->min();
            prevent_optimize(min_result);
            
            auto max_result = self::from(data)
                ->max();
            prevent_optimize(max_result);
        }
    }
    
    // Original implementation for min/max with selector
    {
        Timer timer("Original: min/max with selector");
        for (int i = 0; i < iterations; i++) {
            auto min_result = orig::from(data)
                ->min([](int n) { return n * n; });
            prevent_optimize(min_result);
            
            auto max_result = orig::from(data)
                ->max([](int n) { return n * n; });
            prevent_optimize(max_result);
        }
    }
    
    // Self implementation for min/max with selector
    {
        Timer timer("Self-reference: min/max with selector");
        for (int i = 0; i < iterations; i++) {
            auto min_result = self::from(data)
                ->min([](int n) { return n * n; });
            prevent_optimize(min_result);
            
            auto max_result = self::from(data)
                ->max([](int n) { return n * n; });
            prevent_optimize(max_result);
        }
    }
}

// Benchmark 10: Group_by operations
void benchmark_group_by_operations(const std::vector<int>& data, int iterations) {
    std::cout << "\n=== Benchmark 10: Group_by Operations (" << iterations << " iterations) ===\n";
    
    // Original implementation for group_by
    {
        Timer timer("Original: group_by");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->group_by([](int n) { return n % 5; })  // Group by remainder when divided by 5
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // Self implementation for group_by
    {
        Timer timer("Self-reference: group_by");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->group_by([](int n) { return n % 5; })  // Group by remainder when divided by 5
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
}

// Benchmark 11: Reverse operations
void benchmark_reverse_operations(const std::vector<int>& data, int iterations) {
    std::cout << "\n=== Benchmark 11: Reverse Operations (" << iterations << " iterations) ===\n";
    
    // Original implementation for reverse
    {
        Timer timer("Original: reverse");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->reverse()
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // Self implementation for reverse
    {
        Timer timer("Self-reference: reverse");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->reverse()
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
}

// Benchmark 12: Order_by operations
void benchmark_orderby_operations(const std::vector<int>& data, int iterations) {
    std::cout << "\n=== Benchmark 12: Order_by Operations (" << iterations << " iterations) ===\n";
    
    // Simple sorting
    {
        Timer timer("Original: order_by");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->order_by()
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    {
        Timer timer("Self-reference: order_by");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->order_by()
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // With selector function
    {
        Timer timer("Original: order_by with selector");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->order_by([](int n) { return std::abs(n - 5000); })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    {
        Timer timer("Self-reference: order_by with selector");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->order_by([](int n) { return std::abs(n - 5000); })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // Descending order
    {
        Timer timer("Original: order_by_descending");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->order_by_descending()
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    {
        Timer timer("Self-reference: order_by_descending");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->order_by_descending()
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // Chained ordering
    {
        Timer timer("Original: order_by->then_by");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->order_by([](int n) { return n % 10; })
                ->then_by([](int n) { return n; })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    {
        Timer timer("Self-reference: order_by->then_by");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->order_by([](int n) { return n % 10; })
                ->then_by([](int n) { return n; })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
}

// Benchmark 13: to_map operations
void benchmark_to_map_operations(const std::vector<int>& data, int iterations) {
    std::cout << "\n=== Benchmark 13: to_map Operations (" << iterations << " iterations) ===\n";
    
    // Basic to_map
    {
        Timer timer("Original: to_map");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->where([](int n) { return n % 5 == 0; })
                ->to_map([](int n) { return n; }, [](int n) { return n * 2; });
            prevent_optimize(result.size());
        }
    }
    
    {
        Timer timer("Self-reference: to_map");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->where([](int n) { return n % 5 == 0; })
                ->to_map([](int n) { return n; }, [](int n) { return n * 2; });
            prevent_optimize(result.size());
        }
    }
    
    // to_multimap
    {
        Timer timer("Original: to_multimap");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->where([](int n) { return n < 500; })
                ->to_multimap([](int n) { return n % 10; });
            prevent_optimize(result.size());
        }
    }
    
    {
        Timer timer("Self-reference: to_multimap");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->where([](int n) { return n < 500; })
                ->to_multimap([](int n) { return n % 10; });
            prevent_optimize(result.size());
        }
    }
}

// Benchmark 14: Select operations with different return types
void benchmark_select_operations(const std::vector<int>& data, int iterations) {
    std::cout << "\n=== Benchmark 14: Select Operations (" << iterations << " iterations) ===\n";
    
    // Simple select with same return type
    {
        Timer timer("Original: select (same type)");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->select([](int n) { return n * 2; })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    {
        Timer timer("Self-reference: select (same type)");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->select([](int n) { return n * 2; })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // Select with different return type (int to string)
    {
        Timer timer("Original: select (type conversion to string)");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->select([](int n) { return std::to_string(n); })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    {
        Timer timer("Self-reference: select (type conversion to string)");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->select([](int n) { return std::to_string(n); })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // Select with complex type (struct/class)
    {
        Timer timer("Original: select (to complex type)");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->select([](int n) { 
                    struct Result { int value; bool even; };
                    return Result{n, n % 2 == 0}; 
                })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    {
        Timer timer("Self-reference: select (to complex type)");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->select([](int n) { 
                    struct Result { int value; bool even; };
                    return Result{n, n % 2 == 0}; 
                })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // Chained selects
    {
        Timer timer("Original: chained selects");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->select([](int n) { return n * n; })
                ->select([](int n) { return std::to_string(n); })
                ->select([](const std::string& s) { return "Number: " + s; })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    {
        Timer timer("Self-reference: chained selects");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->select([](int n) { return n * n; })
                ->select([](int n) { return std::to_string(n); })
                ->select([](const std::string& s) { return "Number: " + s; })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // Select with where filter before
    {
        Timer timer("Original: where->select");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->where([](int n) { return n % 3 == 0; })
                ->select([](int n) { return n * n; })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    {
        Timer timer("Self-reference: where->select");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->where([](int n) { return n % 3 == 0; })
                ->select([](int n) { return n * n; })
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
}

// Benchmark 15: Additional methods
void benchmark_additional_methods(const std::vector<int>& data, int iterations) {
    std::cout << "\n=== Benchmark 15: Additional Methods (" << iterations << " iterations) ===\n";
    
    // sequence_equal
    {
        Timer timer("Original: sequence_equal");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->take(100)
                ->sequence_equal(data);
            prevent_optimize(static_cast<int>(result));
        }
    }
    
    {
        Timer timer("Self-reference: sequence_equal");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->take(100)
                ->sequence_equal(data);
            prevent_optimize(static_cast<int>(result));
        }
    }
    
    // sum
    {
        Timer timer("Original: sum");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->where([](int n) { return n % 2 == 0; })
                ->sum();
            prevent_optimize(result);
        }
    }
    
    {
        Timer timer("Self-reference: sum");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->where([](int n) { return n % 2 == 0; })
                ->sum();
            prevent_optimize(result);
        }
    }
    
    // sum with selector
    {
        Timer timer("Original: sum with selector");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->sum([](int n) { return n * 2; });
            prevent_optimize(result);
        }
    }
    
    {
        Timer timer("Self-reference: sum with selector");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->sum([](int n) { return n * 2; });
            prevent_optimize(result);
        }
    }
    
    // default_if_empty
    {
        Timer timer("Original: default_if_empty");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(std::vector<int>{})
                ->default_if_empty(42)
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    {
        Timer timer("Self-reference: default_if_empty");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(std::vector<int>{})
                ->default_if_empty(42)
                ->to_vector();
            prevent_optimize(result.size());
        }
    }
    
    // none
    {
        Timer timer("Original: none");
        for (int i = 0; i < iterations; i++) {
            auto result = orig::from(data)
                ->none([](int n) { return n > 20000; });
            prevent_optimize(static_cast<int>(result));
        }
    }
    
    {
        Timer timer("Self-reference: none");
        for (int i = 0; i < iterations; i++) {
            auto result = self::from(data)
                ->none([](int n) { return n > 20000; });
            prevent_optimize(static_cast<int>(result));
        }
    }
}

int main() {
    std::cout << "Comparing relinx vs relinx_self performance benchmark\n";
    std::cout << "====================================================\n";

    // Generate test data
    size_t data_size = 10000;
    std::cout << "Generating test data (size: " << data_size << ")...\n";
    auto data = generate_data(data_size);
    
    // Run benchmarks
    int iterations = 10;
    benchmark_simple_operations(data, iterations);
    benchmark_complex_operations(data, iterations);
    benchmark_lazy_evaluation(data, iterations);
    benchmark_aggregate_operations(data, iterations);
    benchmark_memory_performance(10);  // Fewer iterations for memory benchmark
    benchmark_first_last_element_operations(data, iterations);
    benchmark_skip_operations(data, iterations);
    benchmark_single_operations(data, iterations);
    benchmark_min_max_operations(data, iterations);
    benchmark_group_by_operations(data, iterations);
    benchmark_reverse_operations(data, iterations);
    benchmark_orderby_operations(data, iterations);
    benchmark_to_map_operations(data, iterations);
    benchmark_select_operations(data, iterations);    // New benchmark for select operations
    benchmark_additional_methods(data, iterations);
    
    std::cout << "\nBenchmark complete. Lower numbers are better.\n";

    return 0;
}