#include <iostream>
#include <vector>
#include <string>
#include "../include/relinx_self.hpp"

using namespace nstd::relinx_self;

// A simple Person struct to demonstrate operations on custom types
struct Person {
    std::string name;
    int age;
    
    Person(std::string n, int a) : name(std::move(n)), age(a) {}
    
    // Default constructor needed for mutable value_type defaults
    Person() : name(""), age(0) {}
    
    // For displaying in cout
    friend std::ostream& operator<<(std::ostream& os, const Person& p) {
        os << p.name << " (" << p.age << ")";
        return os;
    }
};

int main() {
    // Example 1: Basic operations with numbers
    std::cout << "Example 1: Basic operations with numbers\n";
    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // Using where and select
    auto result1 = from(numbers)
        ->where([](int n) { return n % 2 == 0; })     // Filter even numbers
        ->select([](int n) { return n * n; })         // Square them
        ->to_vector();                                // Materialize as vector
    
    std::cout << "Even numbers squared (first 3): ";
    for (auto num : result1) {
        std::cout << num << " ";
    }
    std::cout << "\n\n";
    
    // Example 2: Working with custom types
    std::cout << "Example 2: Working with custom types\n";
    std::vector<Person> people = {
        {"Alice", 25},
        {"Bob", 30},
        {"Charlie", 22},
        {"David", 35},
        {"Eve", 28}
    };
    
    // Filter by age and project to name
    auto result2 = from(people)
        ->where([](const Person& p) { return p.age > 25; })
        ->select([](const Person& p) { return p.name; })
        ->to_vector();
    
    std::cout << "Names of people older than 25: ";
    for (const auto& name : result2) {
        std::cout << name << " ";
    }
    std::cout << "\n\n";
    
    // Example 3: Chaining multiple operations
    std::cout << "Example 3: Chaining multiple operations\n";
    
    // Generate range, filter, transform and count
    auto count = range(1, 100)  // Numbers 1-100
        ->where([](int n) { return n % 3 == 0 || n % 5 == 0; })  // Divisible by 3 or 5
        ->select([](int n) { return n * 2; })  // Double each value
        ->where([](int n) { return n < 100; }) // Less than 100
        ->count();
    
    std::cout << "Count of numbers (1-100) divisible by 3 or 5, doubled, and less than 100: " 
              << count << "\n\n";
    
    // Example 4: Using tee for debugging and side effects
    std::cout << "Example 4: Using tee for debugging\n";
    
    auto result4 = from(numbers)
        ->where([](int n) { return n > 5; })
        ->tee([](int n) { std::cout << "After where: " << n << "\n"; })
        ->select([](int n) { return n * 10; })
        ->tee([](int n) { std::cout << "After select: " << n << "\n"; })
        ->to_vector();
    
    std::cout << "\nFinal result: ";
    for (auto num : result4) {
        std::cout << num << " ";
    }
    std::cout << "\n\n";
    
    // Example 5: Using distinct 
    std::cout << "Example 5: Distinct values\n";
    
    std::vector<int> duplicates = {1, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 5};
    
    auto result5 = from(duplicates)
        ->distinct()
        ->to_vector();
    
    std::cout << "Distinct numbers: ";
    for (auto num : result5) {
        std::cout << num << " ";
    }
    std::cout << "\n\n";

    // Example 6: Using concat
    std::cout << "Example 6: Concatenating sequences\n";
    
    std::vector<int> first = {1, 2, 3};
    std::vector<int> second = {4, 5, 6};
    
    auto result6 = from(first)
        ->concat(second)
        ->to_vector();
    
    std::cout << "Concatenated vectors: ";
    for (auto num : result6) {
        std::cout << num << " ";
    }
    std::cout << "\n\n";
    
    // Example 7: Using aggregate
    std::cout << "Example 7: Aggregate operations\n";
    
    auto sum = from(numbers)->aggregate([](int acc, int val) { 
        return acc + val; 
    });
    
    std::cout << "Sum of all numbers: " << sum << "\n";
    
    auto commaSeparated = from(numbers)
        ->aggregate(std::string(), [](const std::string& acc, int val) {
            return acc.empty() ? std::to_string(val) : acc + ", " + std::to_string(val);
        });
    
    std::cout << "Comma-separated numbers: " << commaSeparated << "\n\n";
    
    // Example 8: Combining multiple operations with People
    std::cout << "Example 8: Complex operations with custom types\n";
    
    // Add more people to the collection
    people.push_back({"Frank", 42});
    people.push_back({"Grace", 25});
    people.push_back({"Helen", 30});
    
    // Find names of people over 25, take only first 3, make uppercase
    auto result8 = from(people)
        ->where([](const Person& p) { return p.age > 25; })
        ->take(3)
        ->select([](const Person& p) { 
            std::string name = p.name;
            std::transform(name.begin(), name.end(), name.begin(), 
                          [](unsigned char c) { return std::toupper(c); });
            return name;
        })
        ->to_vector();
    
    std::cout << "First 3 names of people over 25 (uppercase): ";
    for (const auto& name : result8) {
        std::cout << name << " ";
    }
    std::cout << "\n";
    
    return 0;
}