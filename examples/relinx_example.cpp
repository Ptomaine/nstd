/*
MIT License

Copyright (c) 2017 Arlen Keshabyan (arlen.albert@gmail.com)

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

#include "relinx.hpp"

#include <cassert>
#include <chrono>
#include <deque>
#include <forward_list>
#include <iostream>
#include <deque>
#include <map>
#include <set>

using namespace nstd::relinx;
using hr_clock = std::chrono::high_resolution_clock;
using namespace std::literals;

struct Customer
{
    uint32_t Id {};
    std::string FirstName {};
    std::string LastName {};
    uint32_t Age {};

    bool operator== (const Customer &other) const
    {
        return Id == other.Id && FirstName == other.FirstName && LastName == other.LastName && Age == other.Age;
    }
};

struct Pet
{
    uint32_t OwnerId {};
    std::string NickName {};

    bool operator== (const Pet &other) const
    {
        return OwnerId == other.OwnerId && NickName == other.NickName;
    }
};

namespace std
{
    template<>
    struct hash<Customer>
    {
        using argument_type = Customer;
        using result_type = std::size_t;

        result_type operator()(argument_type const &s) const
        {
            result_type const h1 (std::hash<std::string>()(s.FirstName));
            result_type const h2 (std::hash<std::string>()(s.LastName));
            result_type const h3 (std::hash<uint32_t>()(s.Age));

            return h1 ^ (((h2 << 1) ^ h3) >> 1);
        }
    };
}

int main()
{
    auto &&print_duration = [](auto &&msg, auto &&start) { auto t = std::chrono::duration_cast<std::chrono::milliseconds>(hr_clock::now() - start).count() / 1000.; std::cout << msg << " " << t << " sec." << std::endl; };

    auto total_start = hr_clock::now();

    { //auto to_vector() const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        std::list<long> t1_data = {1, 2, 3};

        auto t1_res = from(t1_data)->to_vector();

        assert(typeid(t1_res) == typeid(std::vector<long>));
        assert(t1_res.size() == 3);
        assert(t1_res[0] == 1 && t1_res[1] == 2 && t1_res[2] == 3);

        print_duration("to_vector(f):", start);
    }

    { //auto where(FilterFunctor &&filterFunctor) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        auto t1_res = from({0, 0, 8, 0, 8, 8, 0, 0, 0, 0, 8, 0, 8, 0, 0, 8, 0})->where([](auto &&v) { return v > 0; })->to_vector();

        assert(t1_res.size() == 6);
        assert(t1_res[0] == 8 && t1_res[1] == 8 && t1_res[2] == 8 && t1_res[3] == 8 && t1_res[4] == 8 && t1_res[5] == 8);

        print_duration("where(f):", start);
    }

    { //auto aggregate(AggregateFunction &&aggregateFunction) const -> decltype(auto)
        std::deque<std::string> t2_data = { "1"s, " 2"s, " 3 "s, "4"s, " 5"s };
        auto t4_data = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

        auto start = hr_clock::now();

        auto t1_res = from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->aggregate([](auto &&a, auto &&b) { return a + b; });
        assert(t1_res == 55);

        auto t2_res = from(t2_data)->aggregate([](auto &&a, auto &&b) { return a + b; });
        assert(t2_res == "1 2 3 4 5"s);

        auto t3_res = from(t2_data)->where([](auto &&i){ return i.length() == 2; })->aggregate([](auto &&a, auto &&b) { return a + b; });
        assert(t3_res == " 2 5"s);

        auto t4_res = from(t4_data)->aggregate([](auto &&a, auto &&b) { return a + b; });
        assert(t4_res == 55);

        print_duration("aggregate(f):", start);
    }

    { //auto aggregate(SeedType &&seed, AggregateFunction &&aggregateFunction) const -> decltype(auto)
        std::deque<std::string> t2_data = { " 2"s, " 3 "s, "4"s, " 5"s };
        auto t4_data = {9, 8, 7, 6, 5, 4, 3, 2, 1};

        auto start = hr_clock::now();

        auto t1_res = from({2, 3, 4, 5, 6, 7, 8, 9, 10})->aggregate(1, [](auto &&a, auto &&b) { return a + b; });
        assert(t1_res == 55);

        auto t2_res = from(t2_data)->aggregate("1"s, [](auto &&a, auto &&b) { return a + b; });
        assert(t2_res == "1 2 3 4 5"s);

        auto t3_res = from(t2_data)->where([](auto &&i){ return i.length() == 2; })->aggregate("1"s, [](auto &&a, auto &&b) { return a + b; });
        assert(t3_res == "1 2 5"s);

        auto t4_res = from(t4_data)->aggregate(10, [](auto &&a, auto &&b) { return a + b; });
        assert(t4_res == 55);

        print_duration("aggregate(seed, f):", start);
    }

    { //auto aggregate(SeedType &&seed, AggregateFunction &&aggregateFunction, ResultSelector &&resultSelector) const -> decltype(auto)
        std::deque<std::string> t2_data = { " 2"s, " 3 "s, "4"s, " 5"s };
        auto t4_data = {9, 8, 7, 6, 5, 4, 3, 2, 1};

        auto start = hr_clock::now();

        auto t1_res = from({2, 3, 4, 5, 6, 7, 8, 9, 10})->aggregate(1, [](auto &&a, auto &&b) { return a + b; }, [](auto &&r) { return r * 2; });
        assert(t1_res == 110);

        auto t2_res = from(t2_data)->aggregate("1"s, [](auto &&a, auto &&b) { return a + b; }, [](auto &&r) { return "[" + r + "]"; });
        assert(t2_res == "[1 2 3 4 5]"s);

        auto t3_res = from(t2_data)->where([](auto &&i){ return i.length() == 2; })->aggregate("1"s, [](auto &&a, auto &&b) { return a + b; }, [](auto &&r) { return "[" + r + "]"; });
        assert(t3_res == "[1 2 5]"s);

        auto t4_res = from(t4_data)->aggregate(10, [](auto &&a, auto &&b) { return a + b; }, [](auto &&r) { return "("s + std::to_string(r * 2.5 + .5) + ")"; });
        assert(t4_res == "(138.000000)"s);

        print_duration("aggregate(seed, f1, f2):", start);
    }

    { //auto all(ConditionFunction &&conditionFunction) const -> bool
        auto start = hr_clock::now();

        assert(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->all([](auto &&r) { return r > 0; }) == true);
        assert(from(std::vector<int>())->all([](auto &&) { return false; }) == true);

        auto t2_res = from({"1"s, "2"s, "3"s, "4"s, "5"s, "6"s, "7"s, "8"s, "9"s, "10"s})->all([](auto &&r) { return r.length() > 1; });
        assert(t2_res == false);

        print_duration("all(f):", start);
    }

    { //auto any(ConditionFunction &&conditionFunction) const -> bool
        auto start = hr_clock::now();

        auto t1_res = from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->any([](auto &&r) { return r > 5; });
        assert(t1_res == true);

        auto t2_res = from({"1"s, "2"s, "3"s, "4"s, "5"s, "6"s, "7"s, "8"s, "9"s, "10"s})->any([](auto &&r) { return r.length() == 3; });
        assert(t2_res == false);
        assert(from(std::vector<int>())->any([](auto &&) { return true; }) == false);

        print_duration("any(f):", start);
    }

    { //auto any() const -> bool
        auto start = hr_clock::now();

        assert(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->any() == true);
        assert(from(std::vector<std::string>())->any() == false);

        print_duration("any():", start);
    }

    { //auto avarage(AvgFunction &&avgFunction) const -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->avarage([](auto &&r){ return r * 1.5; }) == 8.25);

        print_duration("avarage(f):", start);
    }

    { //auto avarage() const -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->avarage() == 5);

        print_duration("avarage():", start);
    }

    { //auto cast() const -> decltype(auto)
        struct base { virtual ~base(){} };
        struct derived : public base { virtual ~derived(){} };

        auto t1_data = {new derived(), new derived(), new derived()};

        auto start = hr_clock::now();

        auto t1_res = from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->cast<float>();
        auto t2_res = from({derived(), derived(), derived()})->cast<base>();
        auto t3_res = from(t1_data)->cast<base*>();

        assert(t1_res->all([](auto &&i){ return typeid(i) == typeid(float); }));
        assert(t2_res->all([](auto &&i){ return typeid(i) == typeid(base); }));
        assert(t3_res->all([](auto &&i){ return typeid(i) == typeid(base*); }));

        print_duration("cast():", start);

        for(auto &&i : t1_data){ delete i; };
    }

    { //auto concat(const ConcatIterator &begin, const ConcatIterator &end) noexcept -> decltype(auto)
        auto start = hr_clock::now();

        auto t3_data = {6, 7, 8};
        auto t3_res = from({1, 2, 3, 4, 5})->concat(std::rbegin(t3_data), std::rend(t3_data))->to_string();
        assert(t3_res.size() == 8);
        assert(t3_res == "12345876"s);

        print_duration("concat(b, e):", start);
    }

    { //auto concat(Container &&container) -> decltype(auto)
        auto start = hr_clock::now();

        auto t1_res = from({1, 2, 3, 4, 5})->concat(std::list<int>{6, 7})->concat({8, 9})->concat(from({10}))->to_vector();
        assert(t1_res.size() == 10);
        assert(t1_res[1] == 2);
        assert(t1_res[5] == 6);
        assert(t1_res[7] == 8);
        assert(t1_res[9] == 10);

        auto t2_res = from({1, 2, 3, 4, 5})->concat(std::vector<int>())->to_vector();
        assert(t2_res.size() == 5);

        auto t3_data = {6, 7, 8};
        auto t3_res = from({1, 2, 3, 4, 5})->concat(std::rbegin(t3_data), std::rend(t3_data))->to_vector();
        assert(t3_res.size() == 8);
        assert(t3_res[5] == 8);

        std::list<int> t4_data { 1, 2, 3 };
        std::deque<int> t5_data { 4, 5, 6};
        auto t4_res = from(t4_data)->concat(t5_data);
        auto t5_res = t4_res->to_string();

        assert(t5_res == "123456"s);

        print_duration("concat(c):", start);
    }

    { //auto contains(const value_type &value) const -> bool
        auto start = hr_clock::now();

        assert(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->contains(5));

        print_duration("contains(v):", start);
    }

    { //auto contains(CompareFunction &&compareFunction) const -> bool
        auto start = hr_clock::now();

        assert(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->contains([](auto &&i) { return i == 8; }));

        print_duration("contains(f):", start);
    }

    { //auto count() const -> std::size_t
        auto start = hr_clock::now();

        assert(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->count() == 10);

        print_duration("count():", start);
    }

    { //auto count(value_type &&value) const -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 5, 9, 8})->count(5) == 2);

        print_duration("count(v):", start);
    }

    { //auto count(FilterFunction &&filterFunction) const -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->count([](auto &&i) { return i % 2; }) == 5);

        print_duration("count(f):", start);
    }

    { //auto cycle(std::ptrdiff_t count = -1) -> decltype(auto)
        std::vector<int> t1_data = {1, 2, 3};

        auto start = hr_clock::now();

        auto t1_res = from(t1_data)->cycle(0)->to_vector();
        auto t2_res = from(t1_data)->cycle(1)->to_vector();
        auto t3_res = from(t1_data)->cycle(3)->to_vector();
        auto t4_res = from(t1_data)->cycle()->take(5)->to_vector();

        assert(t1_res.empty());
        assert(t2_res.size() == 1 * t1_data.size() && t2_res.back() == t1_data.back());
        assert(t3_res.size() == 3 * t1_data.size() && t3_res.back() == t1_data.back());
        assert(t4_res.size() == 5 && t4_res.back() != t1_data.back());
        assert(t4_res.back() == t1_data[t4_res.size() % (t1_data.size() + 1)]);

        print_duration("cycle(v):", start);
    }

    { //auto default_if_empty(value_type default_value = value_type()) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        auto t1_data = from({1, 2, 3})->default_if_empty()->to_vector();
        assert(t1_data.size() == 3);

        auto t2_data = from({1, 2, 3})->where([](auto &&v) { return v > 100;})->default_if_empty(111)->to_vector();
        assert(t2_data.size() == 1);
        assert(t2_data[0] == 111);

        auto t3_res = from(std::vector<std::string>())->default_if_empty()->to_vector();
        assert(t3_res.size() == 1);
        assert(t3_res[0] == std::string());

        print_duration("default_if_empty(v):", start);
    }

    { //auto distinct(KeyFunction &&keyFunction) const -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<Customer> t1_data =
        {
            Customer{0, "John"s, "Doe"s, 25},
            Customer{1, "Sam"s, "Doe"s, 35},
            Customer{2, "John"s, "Doe"s, 25},
            Customer{3, "Alex"s, "Poo"s, 23},
            Customer{4, "Sam"s, "Doe"s, 45},
            Customer{5, "Anna"s, "Poo"s, 23}
        };

        auto t1_res = from(t1_data)->distinct()->to_vector();
        auto t2_res = from(t1_data)->distinct([](auto &&i) { return i.LastName; })->to_vector(); //LastName as a key
        auto t3_res = from(t1_data)->distinct([](auto &&i) { return std::hash<std::string>()(i.LastName) ^ (std::hash<uint32_t>()(i.Age) << 1); })->to_vector(); //LastName and Age as a key

        assert(t1_res.size() == t1_data.size());
        assert(t1_res[3].FirstName == "Alex"s);
        assert(t2_res.size() == 2);
        assert(t2_res[1].LastName == "Poo"s);
        assert(t3_res.size() == 4);
        assert(t3_res[1].LastName == "Doe"s && t3_res[1].Age == 35);

        assert(from({1, 2, 3, 3, 2, 1})->distinct()->count() == 3);

        print_duration("distinct(f):", start);
    }

    { //auto element_at(std::size_t index) const -> value_type
        auto start = hr_clock::now();

        auto t1_data = {0, 1, 2, 3, 4, 5, 6, 7, 8};

        assert(from(t1_data)->element_at(0) == 0);
        assert(from(t1_data)->element_at(3) == 3);
        assert(from(t1_data)->element_at(8) == 8);

        print_duration("element_at(idx):", start);
    }

    { //auto element_at_or_default(std::size_t index) const -> value_type
        auto start = hr_clock::now();

        auto t1_data = {0, 1, 2, 3, 4, 5, 6, 7, 8};

        assert(from(t1_data)->element_at_or_default(0) == 0);
        assert(from(t1_data)->element_at_or_default(3) == 3);
        assert(from(t1_data)->element_at_or_default(8) == 8);
        assert(from(t1_data)->element_at_or_default(10) == 0);
        assert(from(t1_data)->element_at_or_default(100, -1) == -1);

        print_duration("element_at_or_default(idx, dv):", start);
    }

    { //auto except(Container &&container, std::function<bool(const value_type&, const value_type&)> &&compareFunction = [](auto &&a, auto &&b) { return a == b; }) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({0, 1, 2, 3, 4, 5, 6, 7, 8})->except({2, 3, 4, 5})->to_string() == "01678");

        print_duration("except:", start);
    }

    { //auto first(ConditionFunction &&conditionFunction) const -> value_type
        auto start = hr_clock::now();

        assert(from({0, 1, 2, 3, 4, 5, 6, 7, 8})->first([](auto &&v) { return v & 1; }) == 1);
        assert(from({5, 6, 7, 8})->first([](auto &&v) { return v == 7; }) == 7);

        try
        {
            from({0, 1, 2})->first([](auto &&v) { return v == 10;});

            assert(false);
        }
        catch(const not_found &ex)
        {
            if (std::string(ex.what()) == "first"s) assert(true); else assert(false);
        }

        print_duration("first(f):", start);
    }

    { //auto first() const -> value_type
        auto start = hr_clock::now();

        assert(from({5, 6, 7, 8})->first() == 5);

        try
        {
            from(std::vector<int>())->first();

            assert(false);
        }
        catch(const no_elements &ex)
        {
            if (std::string(ex.what()) == "first"s) assert(true); else assert(false);
        }

        print_duration("first():", start);
    }

    { //auto first_or_default(ConditionFunction &&conditionFunction, value_type default_value = value_type()) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({0, 1, 2, 3, 4, 5, 6, 7, 8})->first_or_default([](auto &&v) { return v == 100; }) == 0);
        assert(from({5, 6, 7, 8})->first_or_default([](auto &&v) { return v == 100; }, 100) == 100);

        print_duration("first_or_default(f):", start);
    }

    { //auto first_or_default(value_type default_value = value_type()) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({7, 8})->first_or_default() == 7);
        assert(from(std::vector<int>())->first_or_default() == 0);
        assert(from(std::vector<int>())->first_or_default(111) == 111);

        print_duration("first_or_default():", start);
    }

    { //auto range(T start, std::size_t count) -> decltype(auto)
        auto start = hr_clock::now();

        assert(range(0, 9)->to_string() == "012345678"s);

        print_duration("range(v, v):", start);
    }

    { //auto repeat(T e, std::size_t count) -> decltype(auto)
        auto start = hr_clock::now();

        assert(repeat(0, 9)->to_string() == "000000000"s);
        assert(repeat('0', 9)->to_string() == "000000000"s);
        assert(repeat("abc"s, 5)->to_string() == "abcabcabcabcabc"s);

        print_duration("repeat(v, v):", start);
    }

    { //auto for_each(ForeachFunction &&foreachFunction) const noexcept -> void
        auto start = hr_clock::now();

        std::ostringstream ostr;
        auto t1_data = range(0, 9);

        t1_data->for_each([&ostr](auto &&v) { ostr << v; });

        assert(ostr.str() == t1_data->to_string());

        print_duration("for_each(f):", start);
    }

    { //auto order_by(SelectFunction &&selectFunction = [](auto &&v) { return v; }) const -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<Customer> t1_data =
        {
            Customer{0, "John"s, "Doe"s, 25},
            Customer{1, "Sam"s, "Doe"s, 35},
            Customer{2, "John"s, "Doe"s, 25},
            Customer{3, "Alex"s, "Poo"s, 22},
            Customer{4, "Sam"s, "Doe"s, 45},
            Customer{5, "Anna"s, "Poo"s, 23}
        };

        auto t1_res = from(t1_data)->order_by([](auto &&v) { return v.Age; })->to_vector();

        assert(t1_res[0].FirstName == "Alex"s);
        assert(t1_res[5].FirstName == "Sam"s);
        assert(from({3, 2, 1})->order_by()->to_string() == "123"s);

        print_duration("order_by(f):", start);
    }

    { //auto order_by_descending(SelectFunction &&selectFunction = [](auto &&v) { return v; }) const -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<Customer> t1_data =
        {
            Customer{0, "John"s, "Doe"s, 25},
            Customer{1, "Sam"s, "Doe"s, 35},
            Customer{2, "John"s, "Doe"s, 25},
            Customer{3, "Alex"s, "Poo"s, 22},
            Customer{4, "Sam"s, "Doe"s, 45},
            Customer{5, "Anna"s, "Poo"s, 23}
        };

        auto t1_res = from(t1_data)->order_by_descending([](auto &&v) { return v.Age; })->to_vector();

        assert(t1_res[0].FirstName == "Sam"s);
        assert(t1_res[5].FirstName == "Alex"s);
        assert(from({1, 2, 3})->order_by_descending()->to_string() == "321"s);

        print_duration("order_by_descending(f):", start);
    }

    { //auto for_each_i(ForeachFunction &&foreachFunction) const noexcept -> void
        auto start = hr_clock::now();

        std::ostringstream ostr;
        auto t1_data = range(0, 9);

        t1_data->for_each_i([&ostr](auto &&v, auto &&idx) { ostr << idx << v; });

        assert(ostr.str() == t1_data->concat(range(0, 9))->order_by()->to_string());

        print_duration("for_each_i(f):", start);
    }

    { //auto single(ConditionFunctor &&conditionFunctor) const -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<int> t1_data = {1, 2, 3};
        std::vector<int> t2_data = {1, 2, 3, 4, 5, 6};
        std::vector<int> t3_data;

        assert(from(t1_data)->single([](auto &&v) { return !(v % 2); }));

        try
        {
            from(t2_data)->single([](auto &&v) { return !(v % 2); });

            assert(false);
        }
        catch(const invalid_operation &e)
        {
            if (e.what() == "single"s) assert(true);
        }

        try
        {
            from(t3_data)->single([](auto &&v) { return !(v % 2); });

            assert(false);
        }
        catch(const no_elements &e)
        {
            if (e.what() == "single"s) assert(true);
        }

        print_duration("single(f):", start);
    }

    { //auto group_by(KeyFunction &&keyFunction) const -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<Customer> t1_data =
        {
            Customer{0, "John"s, "Doe"s, 25},
            Customer{1, "Sam"s, "Doe"s, 35},
            Customer{2, "John"s, "Doe"s, 25},
            Customer{3, "Alex"s, "Poo"s, 23},
            Customer{4, "Sam"s, "Doe"s, 45},
            Customer{5, "Anna"s, "Poo"s, 23}
        };

        auto t1_res = from(t1_data)->group_by([](auto &&i) { return i.LastName; });
        auto t2_res = from(t1_data)->group_by([](auto &&i) { return std::hash<std::string>()(i.LastName) ^ (std::hash<std::string>()(i.FirstName) << 1); });

        assert(t1_res->count() == 2);
        assert(t1_res->first([](auto &&i){ return i.first == "Doe"s; }).second.size() == 4);
        assert(t1_res->first([](auto &&i){ return i.first == "Poo"s; }).second.size() == 2);
        assert(from(t1_res->first([](auto &&i){ return i.first == "Doe"s; }).second)->contains([](auto &&i) { return i.FirstName == "Sam"s; }));
        assert(from(t1_res->first([](auto &&i){ return i.first == "Poo"s; }).second)->contains([](auto &&i) { return i.FirstName == "Anna"s; }));
        assert(t2_res->single([](auto &&i){ return i.first == (std::hash<std::string>()("Doe"s) ^ (std::hash<std::string>()("John"s) << 1)); }).second.size() == 2);
        assert(t2_res->single([](auto &&i){ return i.first == (std::hash<std::string>()("Doe"s) ^ (std::hash<std::string>()("Sam"s) << 1)); }).second.size() == 2);

        print_duration("group_by(f):", start);
    }

    { //auto group_join(Container &&container, ThisKeyFunction &&thisKeyFunction, OtherKeyFunction &&otherKeyFunction, ResultFunction &&resultFunction, bool leftJoin = false) const -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<Customer> t1_data =
        {
            Customer{0, "John"s, "Doe"s, 25},
            Customer{1, "Sam"s, "Doe"s, 35},
            Customer{2, "John"s, "Doe"s, 25},
            Customer{3, "Alex"s, "Poo"s, 23},
            Customer{4, "Sam"s, "Doe"s, 45},
            Customer{5, "Anna"s, "Poo"s, 23}
        };

        std::vector<Pet> t2_data =
        {
            Pet{0, "Spotty"s},
            Pet{3, "Bubble"s},
            Pet{0, "Kitty"s},
            Pet{3, "Bob"s},
            Pet{1, "Sparky"s},
            Pet{3, "Fluffy"s}
        };

        auto t1_res = from(t1_data)->group_join(t2_data,
                                               [](auto &&i) { return i.Id; },
                                               [](auto &&i) { return i.OwnerId; },
                                               [](auto &&key, auto &&values)
                                               {
                                                   return std::make_pair(key.FirstName + " "s + key.LastName,
                                                                         from(values)->
                                                                         select([](auto &&i){ return i.NickName; })->
                                                                         order_by()->
                                                                         to_string(","s));
                                               }
                                               )->order_by([](auto &&p) { return p.first; })->to_vector();

        assert(t1_res.size() == 3);
        assert(t1_res[0].first == "Alex Poo"s && t1_res[0].second == "Bob,Bubble,Fluffy"s);
        assert(t1_res[1].first == "John Doe"s && t1_res[1].second == "Kitty,Spotty"s);
        assert(t1_res[2].first == "Sam Doe"s  && t1_res[2].second == "Sparky"s);

        auto t2_res = from(t1_data)->group_join(t2_data,
                                               [](auto &&i) { return i.Id; },
                                               [](auto &&i) { return i.OwnerId; },
                                               [](auto &&key, auto &&values)
                                               {
                                                   return std::make_pair(key.FirstName + " "s + key.LastName,
                                                                         from(values)->
                                                                         select([](auto &&i){ return i.NickName; })->
                                                                         order_by()->
                                                                         to_string(","s));
                                               }
                                               , true)->order_by([](auto &&p) { return p.first; })->to_vector();

        assert(t2_res.size() == 6);
        assert(t2_res[1].second == std::string() && t2_res[3].second == std::string() && t2_res[5].second == std::string());

        print_duration("group_join(f):", start);
    }

    { //auto intersect_with(Container &&container, std::function<bool(const value_type&, const value_type&)> &&compareFunction = [](auto &&a, auto &&b) { return a == b; }) const -> decltype(auto)
        auto start = hr_clock::now();

        auto t1_res = from({1, 2, 3, 7, 8})->intersect_with({2, 3, 5, 6, 7})->to_string();

        assert(t1_res == "237"s);

        print_duration("intersect_with(d):", start);
    }

    { //auto then_by(SelectFunctor &&selectFunctor = [](auto &&v) { return v; }) -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<Customer> t1_data =
        {
            Customer{0, "John"s, "Doe"s, 25},
            Customer{1, "Sam"s, "Doe"s, 35},
            Customer{2, "John"s, "Doe"s, 25},
            Customer{3, "Alex"s, "Poo"s, 25},
            Customer{4, "Sam"s, "Doe"s, 45},
            Customer{5, "Anna"s, "Poo"s, 23}
        };

        auto t1_res = from(t1_data)->order_by([](auto &&v) { return v.Age; })->then_by([](auto &&v) { return v.FirstName; })->to_vector();

        assert(t1_res[0].FirstName == "Anna"s);
        assert(t1_res[1].FirstName == "Alex"s);
        assert(t1_res[2].FirstName == "John"s);
        assert(t1_res[3].FirstName == "John"s);
        assert(t1_res[4].FirstName == "Sam"s);
        assert(t1_res[5].FirstName == "Sam"s);

        print_duration("then_by(f):", start);
    }

    { //auto then_by_descending(SelectFunctor &&selectFunctor = [](auto &&v) { return v; }) -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<Customer> t1_data =
        {
            Customer{0, "John"s, "Doe"s, 25},
            Customer{1, "Sam"s, "Doe"s, 35},
            Customer{2, "John"s, "Doe"s, 25},
            Customer{3, "Alex"s, "Poo"s, 25},
            Customer{4, "Sam"s, "Doe"s, 45},
            Customer{5, "Anna"s, "Poo"s, 23}
        };

        auto t1_res = from(t1_data)->order_by_descending([](auto &&v) { return v.Age; })->then_by_descending([](auto &&v) { return v.FirstName; })->to_vector();

        assert(t1_res[0].FirstName == "Sam"s);
        assert(t1_res[1].FirstName == "Sam"s);
        assert(t1_res[2].FirstName == "John"s);
        assert(t1_res[3].FirstName == "John"s);
        assert(t1_res[4].FirstName == "Alex"s);
        assert(t1_res[5].FirstName == "Anna"s);

        print_duration("then_by_descending(f):", start);
    }

    { //auto group_join(Container &&container, ThisKeyFunction &&thisKeyFunction, OtherKeyFunction &&otherKeyFunction, ResultFunction &&resultFunction, bool leftJoin = false) const -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<Customer> t1_data =
        {
            Customer{0, "John"s, "Doe"s, 25},
            Customer{1, "Sam"s, "Doe"s, 35},
            Customer{2, "John"s, "Doe"s, 25},
            Customer{3, "Alex"s, "Poo"s, 23},
            Customer{4, "Sam"s, "Doe"s, 45},
            Customer{5, "Anna"s, "Poo"s, 23}
        };

        std::vector<Pet> t2_data =
        {
            Pet{0, "Spotty"s},
            Pet{3, "Bubble"s},
            Pet{0, "Kitty"s},
            Pet{3, "Bob"s},
            Pet{1, "Sparky"s},
            Pet{3, "Fluffy"s}
        };

        auto t1_res = from(t1_data)->join(t2_data,
                                       [](auto &&i) { return i.Id; },
                                       [](auto &&i) { return i.OwnerId; },
                                       [](auto &&left, auto &&right)
                                       {
                                           return std::make_pair(left.FirstName + " "s + left.LastName, right.NickName);
                                       }
                                       )->order_by([](auto &&p) { return p.first; })->
                                            then_by([](auto &&p) { return p.second; })->to_vector();

        assert(t1_res.size() == 6);
        assert(t1_res[0].first == "Alex Poo"s && t1_res[0].second == "Bob"s);
        assert(t1_res[1].first == "Alex Poo"s && t1_res[1].second == "Bubble"s);
        assert(t1_res[2].first == "Alex Poo"s && t1_res[2].second == "Fluffy"s);
        assert(t1_res[3].first == "John Doe"s && t1_res[3].second == "Kitty"s);
        assert(t1_res[4].first == "John Doe"s && t1_res[4].second == "Spotty"s);
        assert(t1_res[5].first == "Sam Doe"s  && t1_res[5].second == "Sparky"s);

        auto t2_res = from(t1_data)->join(t2_data,
                                       [](auto &&i) { return i.Id; },
                                       [](auto &&i) { return i.OwnerId; },
                                       [](auto &&left, auto &&right)
                                       {
                                           return std::make_pair(left.FirstName + " "s + left.LastName, right.NickName);
                                       }
                                       , true)->order_by([](auto &&p) { return p.first; })->
                                            then_by([](auto &&p) { return p.second; })->to_vector();

        assert(t2_res.size() == 9);
        assert(t2_res[3].second == std::string() && t2_res[4].second == std::string() && t2_res[7].second == std::string());

        print_duration("join(f):", start);
    }

    { //auto last(ConditionFunction &&conditionFunction) const -> value_type
        auto start = hr_clock::now();

        assert(from({0, 1, 2, 3, 4, 5, 6, 7, 8})->last([](auto &&v) { return v & 1; }) == 7);
        assert(from({5, 6, 7, 8, 5, 6, 8})->last([](auto &&v) { return v < 8; }) == 6);

        try
        {
            from({0, 1, 2})->last([](auto &&v) { return v == 10;});

            assert(false);
        }
        catch(const not_found &ex)
        {
            if (std::string(ex.what()) == "last") assert(true); else assert(false);
        }

        print_duration("last(f):", start);
    }

    { //auto last() const -> value_type
        auto start = hr_clock::now();

        assert(from({5, 6, 7, 8})->last() == 8);

        try
        {
            from(std::vector<int>())->last();

            assert(false);
        }
        catch(const no_elements &ex)
        {
            if (std::string(ex.what()) == "last") assert(true); else assert(false);
        }

        print_duration("last():", start);
    }

    { //auto last_or_default(ConditionFunction &&conditionFunction, value_type default_value = value_type()) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({0, 1, 2, 3, 4, 5, 6, 7, 8})->last_or_default([](auto &&v) { return v == 100; }) == 0);
        assert(from({5, 6, 7, 8})->last_or_default([](auto &&v) { return v == 100; }, 100) == 100);

        print_duration("last_or_default(f):", start);
    }

    { //auto last_or_default(value_type default_value = value_type()) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({7, 8})->last_or_default() == 8);
        assert(from(std::vector<int>())->last_or_default() == 0);
        assert(from(std::vector<int>())->last_or_default(111) == 111);

        print_duration("last_or_default():", start);
    }

    { //auto max(TransformFunction &&transformFunction) const -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<Customer> t1_data =
        {
            Customer{0, "John"s, "Doe"s, 25},
            Customer{1, "Sam"s, "Doe"s, 35},
            Customer{2, "John"s, "Doe"s, 25},
            Customer{3, "Alex"s, "Poo"s, 23},
            Customer{4, "Sam"s, "Doe"s, 45},
            Customer{5, "Anna"s, "Poo"s, 23}
        };

        assert(from(t1_data)->max([](auto &&v) { return v.Age; }) == 45);

        try
        {
            from(std::vector<Customer>())->max([](auto &&v) { return v.Age; });

            assert(false);
        }
        catch(const no_elements &ex)
        {
            if (std::string(ex.what()) == "max"s) assert(true); else assert(false);
        }

        print_duration("max(f):", start);
    }

    { //auto max() const -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({1, 6, 2, 3, 8, 7, 6, 9, 2, 3, 8})->max() == 9);

        try
        {
            from(std::vector<int>())->max();

            assert(false);
        }
        catch(const no_elements &ex)
        {
            if (std::string(ex.what()) == "max"s) assert(true); else assert(false);
        }

        print_duration("max():", start);
    }

    { //auto min(TransformFunction &&transformFunction) const -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<Customer> t1_data =
        {
            Customer{0, "John"s, "Doe"s, 25},
            Customer{1, "Sam"s, "Doe"s, 35},
            Customer{2, "John"s, "Doe"s, 25},
            Customer{3, "Alex"s, "Poo"s, 23},
            Customer{4, "Sam"s, "Doe"s, 45},
            Customer{5, "Anna"s, "Poo"s, 23}
        };

        assert(from(t1_data)->min([](auto &&v) { return v.Age; }) == 23);

        try
        {
            from(std::vector<Customer>())->min([](auto &&v) { return v.Age; });

            assert(false);
        }
        catch(const no_elements &ex)
        {
            if (std::string(ex.what()) == "min"s) assert(true); else assert(false);
        }

        print_duration("min(f):", start);
    }

    { //auto min() const -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({1, 6, 2, 3, 8, 7, 6, 9, 0, 2, 3, 8})->min() == 0);

        try
        {
            from(std::vector<int>())->min();

            assert(false);
        }
        catch(const no_elements &ex)
        {
            if (std::string(ex.what()) == "min"s) assert(true); else assert(false);
        }

        print_duration("min():", start);
    }

    { //auto none(ConditionFunction &&conditionFunction) const -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({0, 1, 2, 3, 4, 5, 6, 7, 8})->none([](auto &&v) { return v == 100; }) == true);
        assert(from({0, 1, 2, 3, 4, 5, 6, 7, 8})->none([](auto &&v) { return v == 5; }) == false);
        assert(from(std::vector<int>())->none([](auto &&v) { return v == 5; }) == true);

        print_duration("none(f):", start);
    }

    { //auto of_type() const noexcept -> decltype(auto)
        struct base { virtual ~base(){} };
        struct derived : public base { virtual ~derived(){} };
        struct derived2 : public base { virtual ~derived2(){} };

        std::list<base*> t1_data = {new derived(), new derived2(), new derived(), new derived(), new derived2()};

        auto start = hr_clock::now();

        auto t1_res = from(t1_data)->of_type<derived2*>();

        assert(t1_res->all([](auto &&i){ return typeid(i) == typeid(derived2*); }));
        assert(t1_res->count() == 2);

        print_duration("of_type():", start);

        for(auto &&i : t1_data){ delete i; };
    }

    { //auto reverse() const -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({1, 2, 3})->reverse()->to_string() == "321"s);
        assert(from({"c"s, "b"s, "a"s})->reverse()->to_string() == "abc"s);

        auto t1_data = { 3, 2, 1 };

        assert(from(std::rbegin(t1_data), std::rend(t1_data))->to_string() == "123"s);

        print_duration("reverse():", start);
    }

    { //auto select(TransformFunctor &&transformFunctor) const -> decltype(auto)
        auto start = hr_clock::now();

        auto t0_data = {1, 2, 3};
        auto t1_data = from(t0_data)->select([](auto &&v) { return std::to_string(v); })->to_vector();
        auto t2_data = from(t0_data)->select([](auto &&v) { return float(v); })->to_vector();

        assert(t1_data.size() == 3);
        assert(typeid(decltype(t1_data)()) == typeid(std::vector<std::string>()));
        assert(t1_data[0] == "1"s);
        assert(t1_data[1] == "2"s);
        assert(t1_data[2] == "3"s);

        assert(t2_data.size() == 3);
        assert(typeid(decltype(t2_data)()) == typeid(std::vector<float>()));
        assert(t2_data[0] == float(1));
        assert(t2_data[1] == float(2));
        assert(t2_data[2] == float(3));

        print_duration("select(f):", start);
    }

    { //auto select_i(TransformFunctor &&transformFunctor) const -> decltype(auto)
        auto start = hr_clock::now();

        auto t0_data = {1, 2, 3};
        auto t1_data = from(t0_data)->select_i([](auto &&v, auto &&idx) { return std::to_string(idx) + std::to_string(v); })->to_vector();
        auto t2_data = from(t0_data)->select_i([](auto &&v, auto &&idx) { return std::make_pair(idx, v); })->to_vector();

        assert(t1_data.size() == 3);
        assert(typeid(decltype(t1_data)()) == typeid(std::vector<std::string>()));
        assert(t1_data[0] == "01"s);
        assert(t1_data[1] == "12"s);
        assert(t1_data[2] == "23"s);

        assert(t2_data.size() == 3);
        assert(typeid(decltype(t2_data)()) == typeid(std::vector<std::pair<std::size_t, int>>()));
        assert(t2_data[0].first == 0 && t2_data[0].second == 1);
        assert(t2_data[1].first == 1 && t2_data[1].second == 2);
        assert(t2_data[2].first == 2 && t2_data[2].second == 3);

        print_duration("select_i(f):", start);
    }

    { //auto select_many(ContainerSelectorFunctor &&containerSelectorFunctor) const -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<std::vector<int>> t1_data =
        {
            {1, 2, 3},
            {4, 5},
            {6},
            {7, 8},
            {9, 10}
        };

        auto t1_res = from(t1_data)->select_many([](auto &&v) { return v; })->to_string();

        assert(t1_res == "12345678910"s);

        print_duration("select_many(f):", start);
    }

    { //auto select_many_i(ContainerSelectorFunctor &&containerSelectorFunctor) const -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<std::vector<int>> t1_data =
        {
            {1, 2, 3},
            {4, 5},
            {6},
            {7, 8},
            {9, 10}
        };

        auto t1_res = from(t1_data)->select_many_i([](auto &&v, auto &&idx) { std::ostringstream oss; oss << from(v)->to_string() << idx; return std::vector<std::string>({oss.str()}); })->to_string();

        assert(t1_res == "1230451627839104"s);

        print_duration("select_many_i(f):", start);
    }

    { //auto sequence_equal(Container &&container) const -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<int> t1_data = {1, 2, 3};
        std::vector<int> t2_data = {1, 2, 3};
        std::vector<int> t3_data = {3, 2, 1};

        assert(from({1, 2, 3})->sequence_equal({1, 2, 3}));
        assert(from(t1_data)->sequence_equal(t2_data));
        assert(from(t2_data)->sequence_equal(t3_data) == false);

        print_duration("sequence_equal(c):", start);
    }

    { //auto sequence_equal(Container &&container, CompareFunctor &&compareFunctor) const -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<Customer> t1_data =
        {
            Customer{0, "John"s, "Doe"s, 25},
            Customer{1, "Sam"s, "Doe"s, 35},
            Customer{2, "John"s, "Doe"s, 25},
            Customer{3, "Alex"s, "Poo"s, 23},
            Customer{4, "Sam"s, "Doe"s, 45},
            Customer{5, "Anna"s, "Poo"s, 23}
        };

        std::vector<Customer> t2_data =
        {
            Customer{0, "John"s, "Doe"s, 25},
            Customer{1, "Sam"s, "Doe"s, 35},
            Customer{2, "John"s, "Doe"s, 25},
            Customer{3, "Alex"s, "Poo"s, 23},
            Customer{4, "Sam"s, "Doe"s, 45},
            Customer{5, "Anna"s, "Poo"s, 23}
        };

        std::vector<Customer> t3_data =
        {
            Customer{0, "John"s, "Doe"s, 25},
            Customer{5, "Anna"s, "Poo"s, 23},
            Customer{3, "Alex"s, "Poo"s, 23},
            Customer{1, "Sam"s, "Doe"s, 35},
            Customer{4, "Sam"s, "Doe"s, 45},
            Customer{2, "John"s, "Doe"s, 25}
        };

        assert(from(t1_data)->sequence_equal(t2_data, [](auto &&t1, auto &&t2){ return t1.Id == t2.Id && t1.FirstName == t2.FirstName; }));
        assert(from(t2_data)->sequence_equal(t3_data, [](auto &&t1, auto &&t2){ return t1.Id == t2.Id && t1.FirstName == t2.FirstName; }) == false);

        print_duration("sequence_equal(c, f):", start);
    }

    { //auto single() const -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<int> t1_data = {1};
        std::vector<int> t2_data = {1, 2};
        std::vector<int> t3_data;

        assert(from(t1_data)->single());

        try
        {
            from(t2_data)->single();

            assert(false);
        }
        catch(const invalid_operation &e)
        {
            if (e.what() == "single"s) assert(true);
        }

        try
        {
            from(t3_data)->single();

            assert(false);
        }
        catch(const no_elements &e)
        {
            if (e.what() == "single"s) assert(true);
        }

        print_duration("single():", start);
    }

    { //auto single_or_default(ConditionFunctor &&conditionFunctor, const value_type &defaultValue = value_type()) const -> value_type
        auto start = hr_clock::now();

        std::vector<int> t1_data = {1, 2, 3};
        std::vector<int> t2_data = {1, 3, 5};
        std::vector<int> t3_data;
        std::vector<int> t4_data = {2, 4, 6};

        assert(from(t1_data)->single_or_default([](auto &&v) { return !(v % 2); }) == 2);
        assert(from(t2_data)->single_or_default([](auto &&v) { return !(v % 2); }) == 0);
        assert(from(t2_data)->single_or_default([](auto &&v) { return !(v % 2); }, -1) == -1);
        assert(from(t3_data)->single_or_default([](auto &&v) { return !(v % 2); }) == 0);
        assert(from(t3_data)->single_or_default([](auto &&v) { return !(v % 2); }, 888) == 888);

        try
        {
            from(t4_data)->single_or_default([](auto &&v) { return !(v % 2); });

            assert(false);
        }
        catch(const invalid_operation &e)
        {
            if (e.what() == "single_or_default"s) assert(true);
        }

        print_duration("single_or_default(f):", start);
    }

    { //auto single_or_default(const value_type &defaultValue = value_type()) const -> value_type
        auto start = hr_clock::now();

        std::vector<int> t1_data = {1};
        std::vector<int> t2_data = {1, 2};
        std::vector<int> t3_data;

        assert(from(t1_data)->single_or_default() == 1);
        assert(from(t1_data)->single_or_default(111) == 1);

        try
        {
            from(t2_data)->single_or_default();

            assert(false);
        }
        catch(const invalid_operation &e)
        {
            if (e.what() == "single_or_default"s) assert(true);
        }

        assert(from(t3_data)->single_or_default() == 0);
        assert(from(t3_data)->single_or_default(555) == 555);

        print_duration("single_or_default():", start);
    }

    { //auto skip(std::size_t skip) -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({1, 2, 3, 4, 5, 6, 7, 8})->skip(5)->to_string() == "678"s);
        assert(from({1, 2, 3, 4, 5, 6, 7, 8})->skip(100)->any() == false);

        print_duration("skip(v):", start);
    }

    { //auto skip_while(SkipFunctor skipFunctor) -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({1, 2, 3, 4, 5, 6, 7, 8})->skip_while([](auto &&v){ return v < 6; })->to_string() == "678"s);
        assert(from({1, 2, 3, 4, 5, 6, 7, 8})->skip_while([](auto &&v){ return v < 100; })->any() == false);

        print_duration("skip_while(f):", start);
    }

    { //auto skip_while_i(SkipFunctor skipFunctor) -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({1, 2, 3, 4, 5, 6, 7, 8})->skip_while_i([](auto &&v, auto &&i){ return v + i < 6; })->to_string() == "45678"s);
        assert(from({1, 2, 3, 4, 5, 6, 7, 8})->skip_while_i([](auto &&v, auto &&i){ return v * i < 15; })->to_string() == "5678"s);

        print_duration("skip_while_i(f):", start);
    }

    { //auto sum(SelectFunctor &&selectFunctor) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<Customer> t1_data =
        {
            Customer{0, "John"s, "Doe"s, 25},
            Customer{1, "Sam"s, "Doe"s, 35},
            Customer{2, "John"s, "Doe"s, 25},
            Customer{3, "Alex"s, "Poo"s, 23},
            Customer{4, "Sam"s, "Doe"s, 45},
            Customer{5, "Anna"s, "Poo"s, 23}
        };

        std::vector<Customer> t2_data;

        assert(from(t1_data)->sum([](auto &&i) { return i.Age; }) == 176);
        assert(from(t2_data)->sum([](auto &&i) { return i.Age; }) == 0);

        print_duration("sum(f):", start);
    }

    { //auto sum() const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<int> t1_data;
        std::vector<std::string> t2_data = { "1.5"s, "3.2"s, "15.38"s, "2.79"s, "7.1"s, "5.55"s };

        assert(from({1, 2, 3, 4, 5, 6, 7, 8})->sum() == 36);
        assert(from({10, 10, 10, 10, 10, 10, 10, 10, 10, 5, 5})->sum() == 100);
        assert(from(t1_data)->sum() == 0);
        assert(from(t2_data)->sum() == from(t2_data)->to_string());

        print_duration("sum():", start);
    }

    { //auto take(std::ptrdiff_t limit) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<int> t1_data;

        assert(from({1, 2, 3, 4, 5, 6, 7, 8})->take(5)->to_string() == "12345"s);
        assert(from({1, 2, 3, 4, 5, 6, 7, 8})->take(0)->any() == false);
        assert(from(t1_data)->take(10)->any() == false);
        assert(from({1, 2, 3, 4, 5, 6, 7, 8})->take(-1)->to_string() == "12345678"s);
        assert(from({1, 2, 3, 4, 5, 6, 7, 8})->take(10)->to_string() == "12345678"s);

        print_duration("take(v):", start);
    }

    { //auto take_while(LimitFunctor &&limitFunctor) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<int> t1_data;

        assert(from({1, 2, 3, 4, 5, 6, 7, 8})->take_while([](auto &&v) { return v < 6; })->to_string() == "12345"s);
        assert(from({1, 2, 3, 4, 5, 6, 7, 8})->take_while([](auto &&v) { return v > 100; })->any() == false);
        assert(from(t1_data)->take_while([](auto &&) { return true; })->any() == false);
        assert(from({1, 2, 3, 4, 5, 6, 7, 8})->take_while([](auto &&v) { return v < 100; })->to_string() == "12345678"s);

        print_duration("take_while(f):", start);
    }

    { //auto take_while_i(LimitFunctor &&limitFunctor) noexcept
        auto start = hr_clock::now();

        assert(from({"1"s, "2"s, "3"s, "4"s, "5"s, "6"s, "7"s, "8"s})->take_while_i([](auto &&, auto &&idx) { return idx < 6; })->to_string() == "123456"s);

        print_duration("take_while_i(f):", start);
    }

    { //auto tee(TeeFunctor &&teeFunctor) noexcept
        auto start = hr_clock::now();

        int cnt { 0 };
        std::vector<int> t1_data { 1, 2, 3, 4, 5, 6, 7, 8 };

        auto s { from(t1_data)->tee([&cnt](auto &&v) { ++cnt; })->where([](auto &&v) { return v <= 3; })->count() };

        assert(cnt == std::size(t1_data));
        assert(s == 3);

        print_duration("tee(f):", start);
    }

    { //auto to_container() const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        std::initializer_list<int> t1_data = {1, 2, 3};
        auto t1_res = from(t1_data)->to_container<std::set<int>>();
        auto t2_res = from(t1_data)->to_container<std::forward_list<int>>();

        assert(typeid(t1_res) == typeid(std::set<int>));
        assert(t1_res.size() == t1_data.size());
        assert(typeid(t2_res) == typeid(std::forward_list<int>));
        assert(std::distance(std::begin(t2_res), std::end(t2_res)) == std::ptrdiff_t(t1_data.size()));

        print_duration("to_container():", start);
    }

    { //auto to_list() const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<long> t1_data = {1, 1, 1};

        auto t1_res = from(t1_data)->to_list();

        assert(typeid(t1_res) == typeid(std::list<long>));
        assert(from(t1_res)->count() == 3);
        assert(from(t1_res)->all([](auto &&v) { return v == 1; }));

        print_duration("to_list():", start);
    }

    { //auto to_map(KeySelectorFunctor &&keySelectorFunctor, ValueSelectorFunctor &&valueSelectorFunctor = [](const value_type &v) { return v; }) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<long> t1_data = {1, 2, 3};

        auto t1_res = from(t1_data)->to_map([](auto &&v) {  return v; }, [](auto &&v) { return double(v * 2); });

        assert(typeid(t1_res) == typeid(default_map<long, double>));
        assert(t1_res.size() == 3);
        assert(t1_res[1] == 2);
        assert(t1_res[2] == 4);
        assert(t1_res[3] == 6);

        print_duration("to_map(f, f):", start);
    }

    { //auto to_multimap(KeySelectorFunctor &&keySelectorFunctor, ValueSelectorFunctor &&valueSelectorFunctor = [](const value_type &v) { return v; }) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        std::vector<long> t1_data = {1, 2, 3, 1, 2, 3, 3};

        auto t1_res = from(t1_data)->to_multimap([](auto &&v) {  return v; }, [](auto &&v) { return double(v * 2); });

        assert(typeid(t1_res) == typeid(default_multimap<long, double>));
        assert(t1_res.size() == 7);

        auto r1_res = t1_res.equal_range(1);
        auto r2_res = t1_res.equal_range(2);
        auto r3_res = t1_res.equal_range(3);

        assert(from(r1_res.first, r1_res.second)->select([](auto &&v) { return v.second; })->to_string() == "22"s);
        assert(from(r2_res.first, r2_res.second)->select([](auto &&v) { return v.second; })->to_string() == "44"s);
        assert(from(r3_res.first, r3_res.second)->select([](auto &&v) { return v.second; })->to_string() == "666"s);

        print_duration("to_multimap(f, f):", start);
    }

    { //auto union_with(Container &&container, KeyFunctor &&keyFunctor = [](auto &&v) { return v; }) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        assert(from({1, 2, 3, 1, 1, 2, 3, 3, 3, 2, 2, 1})->union_with({1, 2, 3, 5, 7, 8, 9, 0})->order_by()->to_string() == "01235789"s);

        print_duration("union_with(c, f):", start);
    }

    { //auto where_i(FilterFunctor &&filterFunctor) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        auto t1_res = from({0, 0, 8, 0, 8, 8, 0, 0, 0, 0, 8, 0, 8, 0, 0, 8, 0})->where_i([](auto &&v, auto &&idx) { return v > 0 && idx < 8; })->to_vector();

        assert(t1_res.size() == 3);
        assert(t1_res[0] == 8 && t1_res[1] == 8 && t1_res[2] == 8);

        print_duration("where_i(f):", start);
    }

    { //auto zip(Container &&container, ResultFunctor &&resultFunctor) const noexcept -> decltype(auto)
        auto start = hr_clock::now();

        auto t1_res = from({1, 2, 3, 4, 5})->zip({"one"s, "two"s, "three"s}, [](auto &&a, auto &&b) { return std::to_string(a) + " " + b; })->to_vector();

        assert(t1_res.size() == 3);
        assert(t1_res[0] == "1 one"s);
        assert(t1_res[1] == "2 two"s);
        assert(t1_res[2] == "3 three"s);

        print_duration("zip(c, f):", start);
    }

    std::cout << std::endl;

    print_duration("All tests passed in:", total_start);

    return 0;
}
