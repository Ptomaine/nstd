/*
MIT License

Copyright (c) 2025 Arlen Keshabyan (arlen.albert@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include "../include/relinx_self.hpp"

using namespace nstd::relinx_self;
using namespace std::literals;

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

    bool operator==(const Person& other) const {
        return name == other.name && age == other.age;
    }
};

// Structure that simulates the one causing the error
struct StaticBlockProperties {
    void updateUI() {
        std::vector<int> data = {1, 2, 3, 4, 5};
        // This is the kind of operation that causes the error in VS2022
        auto result = from(data)->select([](int val) { return val * 2; })->to_vector();
        
        std::cout << "StaticBlockProperties::updateUI test passed!" << std::endl;
    }
};

void test_transform_iterator_adapter() {
    std::cout << "Testing transform_iterator_adapter..." << std::endl;
    
    // Test with vector<int> and lambda that transforms to int
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto result1 = from(data)->select([](int val) { return val * 2; })->to_vector();
    assert(result1.size() == 5);
    assert(result1[0] == 2);
    assert(result1[4] == 10);
    
    // Test with vector<string> and lambda that transforms to string
    std::vector<std::string> strings = {"a", "b", "c"};
    auto result2 = from(strings)->select([](const std::string& s) { return s + s; })->to_vector();
    assert(result2.size() == 3);
    assert(result2[0] == "aa");
    assert(result2[2] == "cc");
    
    // Test with vector<int> and lambda that transforms to string
    auto result3 = from(data)->select([](int val) { return std::to_string(val); })->to_vector();
    assert(result3.size() == 5);
    assert(result3[0] == "1");
    assert(result3[4] == "5");
    
    // Test with vector<Person> and lambda that transforms to string
    std::vector<Person> people = {
        {"Alice", 25},
        {"Bob", 30},
        {"Charlie", 22}
    };
    auto result4 = from(people)->select([](const Person& p) { return p.name; })->to_vector();
    assert(result4.size() == 3);
    assert(result4[0] == "Alice");
    assert(result4[2] == "Charlie");
    
    // Test with vector<Person> and lambda that transforms to int
    auto result5 = from(people)->select([](const Person& p) { return p.age; })->to_vector();
    assert(result5.size() == 3);
    assert(result5[0] == 25);
    assert(result5[2] == 22);
    
    // Test with vector<Person> and lambda that transforms to another Person
    auto result6 = from(people)->select([](const Person& p) { 
        return Person(p.name + " Jr.", p.age - 20); 
    })->to_vector();
    assert(result6.size() == 3);
    assert(result6[0].name == "Alice Jr.");
    assert(result6[2].age == 2);
    
    // Test chaining multiple select operations
    auto result7 = from(data)
        ->select([](int val) { return val * 2; })
        ->select([](int val) { return std::to_string(val) + "!"; })
        ->to_vector();
    assert(result7.size() == 5);
    assert(result7[0] == "2!");
    assert(result7[4] == "10!");
    
    std::cout << "All transform_iterator_adapter tests passed!" << std::endl;
}

void test_transform_iterator_adapter_constructor() {
    std::cout << "Testing transform_iterator_adapter constructor specifically..." << std::endl;
    
    std::vector<int> data = {1, 2, 3, 4, 5};
    
    // This is what causes problems in Visual Studio 2022
    auto doubler = [](const int& val) { return val * 2; };
    
    // Create a basic transform operation
    auto results = from(data)
        ->select(doubler)
        ->to_vector();
    
    // Verify the results
    assert(results.size() == 5);
    assert(results[0] == 2);
    assert(results[4] == 10);
    
    std::cout << "All transform_iterator_adapter constructor tests passed!" << std::endl;
}

// Test the basic container operations
void test_basic_container_ops() {
    std::cout << "Testing basic container operations..." << std::endl;
    
    // Test to_vector
    auto vec = from({1, 2, 3})->to_vector();
    assert(vec.size() == 3);
    assert(vec[0] == 1);
    
    // Test to_list
    auto list = from({1, 2, 3})->to_list();
    assert(list.size() == 3);
    assert(*list.begin() == 1);
    
    // Test to_string
    auto str = from({1, 2, 3})->to_string();
    assert(str == "123");
    
    auto str_with_delim = from({1, 2, 3})->to_string(",");
    assert(str_with_delim == "1,2,3");
    
    std::cout << "All basic container operations tests passed!" << std::endl;
}

// Test sequence operations
void test_sequence_ops() {
    std::cout << "Testing sequence operations..." << std::endl;
    
    // Test where
    auto filtered = from({1, 2, 3, 4, 5, 6})->where([](int i) { return i % 2 == 0; })->to_vector();
    assert(filtered.size() == 3);
    assert(filtered[0] == 2);
    assert(filtered[1] == 4);
    assert(filtered[2] == 6);
    
    // Test take
    auto taken = from({1, 2, 3, 4, 5, 6})->take(3)->to_vector();
    assert(taken.size() == 3);
    assert(taken[0] == 1);
    assert(taken[2] == 3);
    
    // Test skip
    auto skipped = from({1, 2, 3, 4, 5, 6})->skip(3)->to_vector();
    assert(skipped.size() == 3);
    assert(skipped[0] == 4);
    assert(skipped[2] == 6);
    
    // Test distinct
    auto distinct = from({1, 2, 2, 3, 3, 3, 4})->distinct()->to_vector();
    assert(distinct.size() == 4);
    
    std::cout << "All sequence operations tests passed!" << std::endl;
}

int main() {
    std::cout << "Running tests for relinx_self.hpp..." << std::endl;
    
    // Test the transform_iterator_adapter
    test_transform_iterator_adapter();
    
    // Test transform_iterator_adapter constructor specifically
    test_transform_iterator_adapter_constructor();
    
    // Test basic container operations
    test_basic_container_ops();
    
    // Test sequence operations
    test_sequence_ops();
    
    // Simulate the issue scenario
    StaticBlockProperties props;
    props.updateUI();
    
    std::cout << "All tests completed successfully!" << std::endl;
    
    return 0;
}