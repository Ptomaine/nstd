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

#include <chrono>
#include <deque>
#include <forward_list>
#include <iostream>
#include <deque>
#include <map>
#include <set>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

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


TEST_CASE( "to_vector", "[relinx]" )
{
    std::list<long> t1_data = {1, 2, 3};

    auto t1_res = from(t1_data)->to_vector();

    CHECK(typeid(t1_res) == typeid(std::vector<long>));
    CHECK(t1_res.size() == 3);
    CHECK(t1_res[0] == 1); CHECK(t1_res[1] == 2); CHECK(t1_res[2] == 3);
}

TEST_CASE( "where(f)", "[relinx]" )
{
    auto t1_res = from({0, 0, 8, 0, 8, 8, 0, 0, 0, 0, 8, 0, 8, 0, 0, 8, 0})->where([](auto &&v) { return v > 0; })->to_vector();

    CHECK(t1_res.size() == 6);
    CHECK(t1_res[0] == 8); CHECK(t1_res[1] == 8); CHECK(t1_res[2] == 8); CHECK(t1_res[3] == 8); CHECK(t1_res[4] == 8); CHECK(t1_res[5] == 8);
}

TEST_CASE( "aggregate(f)", "[relinx]" )
{
    std::deque<std::string> t2_data = { "1"s, " 2"s, " 3 "s, "4"s, " 5"s };
    auto t4_data = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

    auto t1_res = from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->aggregate([](auto &&a, auto &&b) { return a + b; });
    CHECK(t1_res == 55);

    auto t2_res = from(t2_data)->aggregate([](auto &&a, auto &&b) { return a + b; });
    CHECK(t2_res == "1 2 3 4 5"s);

    auto t3_res = from(t2_data)->where([](auto &&i){ return i.length() == 2; })->aggregate([](auto &&a, auto &&b) { return a + b; });
    CHECK(t3_res == " 2 5"s);

    auto t4_res = from(t4_data)->aggregate([](auto &&a, auto &&b) { return a + b; });
    CHECK(t4_res == 55);
}

TEST_CASE( "aggregate(seed, f)", "[relinx]" )
{
    std::deque<std::string> t2_data = { " 2"s, " 3 "s, "4"s, " 5"s };
    auto t4_data = {9, 8, 7, 6, 5, 4, 3, 2, 1};

    auto t1_res = from({2, 3, 4, 5, 6, 7, 8, 9, 10})->aggregate(1, [](auto &&a, auto &&b) { return a + b; });
    CHECK(t1_res == 55);

    auto t2_res = from(t2_data)->aggregate("1"s, [](auto &&a, auto &&b) { return a + b; });
    CHECK(t2_res == "1 2 3 4 5"s);

    auto t3_res = from(t2_data)->where([](auto &&i){ return i.length() == 2; })->aggregate("1"s, [](auto &&a, auto &&b) { return a + b; });
    CHECK(t3_res == "1 2 5"s);

    auto t4_res = from(t4_data)->aggregate(10, [](auto &&a, auto &&b) { return a + b; });
    CHECK(t4_res == 55);
}

TEST_CASE( "aggregate(seed, f1, f2)", "[relinx]" )
{
    std::deque<std::string> t2_data = { " 2"s, " 3 "s, "4"s, " 5"s };
    auto t4_data = {9, 8, 7, 6, 5, 4, 3, 2, 1};

    auto t1_res = from({2, 3, 4, 5, 6, 7, 8, 9, 10})->aggregate(1, [](auto &&a, auto &&b) { return a + b; }, [](auto &&r) { return r * 2; });
    CHECK(t1_res == 110);

    auto t2_res = from(t2_data)->aggregate("1"s, [](auto &&a, auto &&b) { return a + b; }, [](auto &&r) { return "[" + r + "]"; });
    CHECK(t2_res == "[1 2 3 4 5]"s);

    auto t3_res = from(t2_data)->where([](auto &&i){ return i.length() == 2; })->aggregate("1"s, [](auto &&a, auto &&b) { return a + b; }, [](auto &&r) { return "[" + r + "]"; });
    CHECK(t3_res == "[1 2 5]"s);

    auto t4_res = from(t4_data)->aggregate(10, [](auto &&a, auto &&b) { return a + b; }, [](auto &&r) { return "("s + std::to_string(r * 2.5 + .5) + ")"; });
    CHECK(t4_res == "(138.000000)"s);
}

TEST_CASE( "all(f)", "[relinx]" )
{
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->all([](auto &&r) { return r > 0; }) == true);
    CHECK(from(std::vector<int>())->all([](auto &&) { return false; }) == true);

    auto t2_res = from({"1"s, "2"s, "3"s, "4"s, "5"s, "6"s, "7"s, "8"s, "9"s, "10"s})->all([](auto &&r) { return r.length() > 1; });
    CHECK_FALSE(t2_res);
}

TEST_CASE( "any(f)", "[relinx]" )
{
    auto t1_res = from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->any([](auto &&r) { return r > 5; });
    CHECK(t1_res == true);

    auto t2_res = from({"1"s, "2"s, "3"s, "4"s, "5"s, "6"s, "7"s, "8"s, "9"s, "10"s})->any([](auto &&r) { return r.length() == 3; });
    CHECK_FALSE(t2_res);
    CHECK_FALSE(from(std::vector<int>())->any([](auto &&) { return true; }));
}

TEST_CASE( "any()", "[relinx]" )
{
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->any());
    CHECK_FALSE(from(std::vector<std::string>())->any());
}

TEST_CASE( "avarage(f)", "[relinx]" )
{
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->avarage([](auto &&r){ return r * 1.5; }) == 8.25);
}

TEST_CASE( "avarage()", "[relinx]" )
{
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->avarage() == 5);
}

TEST_CASE( "cast()", "[relinx]" )
{
    struct base { virtual ~base(){} };
    struct derived : public base { virtual ~derived(){} };

    auto t1_data = {new derived(), new derived(), new derived()};

    auto t1_res = from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->cast<float>();
    auto t2_res = from({derived(), derived(), derived()})->cast<base>();
    auto t3_res = from(t1_data)->cast<base*>();

    CHECK(t1_res->all([](auto &&i){ return typeid(i) == typeid(float); }));
    CHECK(t2_res->all([](auto &&i){ return typeid(i) == typeid(base); }));
    CHECK(t3_res->all([](auto &&i){ return typeid(i) == typeid(base*); }));

    for(auto &&i : t1_data){ delete i; };
}

TEST_CASE( "concat(begin, end)", "[relinx]" )
{
    auto t3_data = {6, 7, 8};
    auto t3_res = from({1, 2, 3, 4, 5})->concat(std::rbegin(t3_data), std::rend(t3_data))->to_string();
    CHECK(t3_res.size() == 8);
    CHECK(t3_res == "12345876"s);
}

TEST_CASE( "concat(c)", "[relinx]" )
{
    auto t1_res = from({1, 2, 3, 4, 5})->concat(std::list<int>{6, 7})->concat({8, 9})->concat(from({10}))->to_vector();
    CHECK(t1_res.size() == 10);
    CHECK(t1_res[1] == 2);
    CHECK(t1_res[5] == 6);
    CHECK(t1_res[7] == 8);
    CHECK(t1_res[9] == 10);

    auto t2_res = from({1, 2, 3, 4, 5})->concat(std::vector<int>())->to_vector();
    CHECK(t2_res.size() == 5);

    auto t3_data = {6, 7, 8};
    auto t3_res = from({1, 2, 3, 4, 5})->concat(std::rbegin(t3_data), std::rend(t3_data))->to_vector();
    CHECK(t3_res.size() == 8);
    CHECK(t3_res[5] == 8);

    std::list<int> t4_data { 1, 2, 3 };
    std::deque<int> t5_data { 4, 5, 6};
    auto t4_res = from(t4_data)->concat(t5_data);
    auto t5_res = t4_res->to_string();

    CHECK(t5_res == "123456"s);
}

TEST_CASE( "contains(v)", "[relinx]" )
{
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->contains(5));
}

TEST_CASE( "contains(f)", "[relinx]" )
{
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->contains([](auto &&i) { return i == 8; }));
}

TEST_CASE( "count()", "[relinx]" )
{
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->count() == 10);
}

TEST_CASE( "count(v)", "[relinx]" )
{
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 5, 9, 8})->count(5) == 2);
}

TEST_CASE( "count(f)", "[relinx]" )
{
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8, 9, 10})->count([](auto &&i) { return i % 2; }) == 5);
}

TEST_CASE( "cycle(v)", "[relinx]" )
{
    std::vector<int> t1_data = {1, 2, 3};

    auto t1_res = from(t1_data)->cycle(0)->to_vector();
    auto t2_res = from(t1_data)->cycle(1)->to_vector();
    auto t3_res = from(t1_data)->cycle(3)->to_vector();
    auto t4_res = from(t1_data)->cycle()->take(5)->to_vector();

    CHECK(t1_res.empty());
    CHECK(t2_res.size() == 1 * t1_data.size()); CHECK(t2_res.back() == t1_data.back());
    CHECK(t3_res.size() == 3 * t1_data.size()); CHECK(t3_res.back() == t1_data.back());
    CHECK(t4_res.size() == 5); CHECK(t4_res.back() != t1_data.back());
    CHECK(t4_res.back() == t1_data[t4_res.size() % (t1_data.size() + 1)]);
}

TEST_CASE( "default_if_empty(v)", "[relinx]" )
{
    auto t1_data = from({1, 2, 3})->default_if_empty()->to_vector();
    CHECK(t1_data.size() == 3);

    auto t2_data = from({1, 2, 3})->where([](auto &&v) { return v > 100;})->default_if_empty(111)->to_vector();
    CHECK(t2_data.size() == 1);
    CHECK(t2_data[0] == 111);

    auto t3_res = from(std::vector<std::string>())->default_if_empty()->to_vector();
    CHECK(t3_res.size() == 1);
    CHECK(t3_res[0] == std::string());
}

TEST_CASE( "distinct(f)", "[relinx]" )
{
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

    CHECK(t1_res.size() == t1_data.size());
    CHECK(t1_res[3].FirstName == "Alex"s);
    CHECK(t2_res.size() == 2);
    CHECK(t2_res[1].LastName == "Poo"s);
    CHECK(t3_res.size() == 4);
    CHECK(t3_res[1].LastName == "Doe"s); CHECK(t3_res[1].Age == 35);

    CHECK(from({1, 2, 3, 3, 2, 1})->distinct()->count() == 3);
}

TEST_CASE( "element_at(idx)", "[relinx]" )
{
    auto t1_data = {0, 1, 2, 3, 4, 5, 6, 7, 8};

    CHECK(from(t1_data)->element_at(0) == 0);
    CHECK(from(t1_data)->element_at(3) == 3);
    CHECK(from(t1_data)->element_at(8) == 8);
}

TEST_CASE( "element_at_or_default(idx, dv)", "[relinx]" )
{
    auto t1_data = {0, 1, 2, 3, 4, 5, 6, 7, 8};

    CHECK(from(t1_data)->element_at_or_default(0) == 0);
    CHECK(from(t1_data)->element_at_or_default(3) == 3);
    CHECK(from(t1_data)->element_at_or_default(8) == 8);
    CHECK(from(t1_data)->element_at_or_default(10) == 0);
    CHECK(from(t1_data)->element_at_or_default(100, -1) == -1);
}

TEST_CASE( "except(c)", "[relinx]" )
{
    CHECK(from({0, 1, 2, 3, 4, 5, 6, 7, 8})->except({2, 3, 4, 5})->to_string() == "01678");
}

TEST_CASE( "first(f)", "[relinx]" )
{
    CHECK(from({0, 1, 2, 3, 4, 5, 6, 7, 8})->first([](auto &&v) { return v & 1; }) == 1);
    CHECK(from({5, 6, 7, 8})->first([](auto &&v) { return v == 7; }) == 7);

    try
    {
        from({0, 1, 2})->first([](auto &&v) { return v == 10;});

        CHECK(false);
    }
    catch(const not_found &ex)
    {
        if (std::string(ex.what()) == "first"s) CHECK(true); else CHECK(false);
    }
}

TEST_CASE( "first()", "[relinx]" )
{
    CHECK(from({5, 6, 7, 8})->first() == 5);

    try
    {
        from(std::vector<int>())->first();

        CHECK(false);
    }
    catch(const no_elements &ex)
    {
        if (std::string(ex.what()) == "first"s) CHECK(true); else CHECK(false);
    }
}

TEST_CASE( "first_or_default(f)", "[relinx]" )
{
    CHECK(from({0, 1, 2, 3, 4, 5, 6, 7, 8})->first_or_default([](auto &&v) { return v == 100; }) == 0);
    CHECK(from({5, 6, 7, 8})->first_or_default([](auto &&v) { return v == 100; }, 100) == 100);
}

TEST_CASE( "first_or_default()", "[relinx]" )
{
    CHECK(from({7, 8})->first_or_default() == 7);
    CHECK(from(std::vector<int>())->first_or_default() == 0);
    CHECK(from(std::vector<int>())->first_or_default(111) == 111);
}

TEST_CASE( "range(v, v)", "[relinx]" )
{
    CHECK(range(0, 9)->to_string() == "012345678"s);
}

TEST_CASE( "repeat(v, v)", "[relinx]" )
{
    CHECK(repeat(0, 9)->to_string() == "000000000"s);
    CHECK(repeat('0', 9)->to_string() == "000000000"s);
    CHECK(repeat("abc"s, 5)->to_string() == "abcabcabcabcabc"s);
}

TEST_CASE( "for_each(f)", "[relinx]" )
{
    std::ostringstream ostr;
    auto t1_data = range(0, 9);

    t1_data->for_each([&ostr](auto &&v) { ostr << v; });

    CHECK(ostr.str() == t1_data->to_string());
}

TEST_CASE( "order_by(f)", "[relinx]" )
{
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

    CHECK(t1_res[0].FirstName == "Alex"s);
    CHECK(t1_res[5].FirstName == "Sam"s);
    CHECK(t1_res[5].Age == 45);
    CHECK(from({3, 2, 1})->order_by()->to_string() == "123"s);
}

TEST_CASE( "order_by_descending(f)", "[relinx]" )
{
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

    CHECK(t1_res[0].FirstName == "Sam"s);
    CHECK(t1_res[0].Age == 45);
    CHECK(t1_res[5].FirstName == "Alex"s);
    CHECK(from({1, 2, 3})->order_by_descending()->to_string() == "321"s);
}

TEST_CASE( "for_each_i(f)", "[relinx]" )
{
    std::ostringstream ostr;
    auto t1_data = range(0, 9);

    t1_data->for_each_i([&ostr](auto &&v, auto &&idx) { ostr << idx << v; });

    CHECK(ostr.str() == t1_data->concat(range(0, 9))->order_by()->to_string());
}

TEST_CASE( "single(f)", "[relinx]" )
{
    std::vector<int> t1_data = {1, 2, 3};
    std::vector<int> t2_data = {1, 2, 3, 4, 5, 6};
    std::vector<int> t3_data;

    CHECK(from(t1_data)->single([](auto &&v) { return !(v % 2); }));

    try
    {
        from(t2_data)->single([](auto &&v) { return !(v % 2); });

        CHECK(false);
    }
    catch(const invalid_operation &e)
    {
        if (e.what() == "single"s) CHECK(true); else CHECK(false);
    }

    try
    {
        from(t3_data)->single([](auto &&v) { return !(v % 2); });

        CHECK(false);
    }
    catch(const no_elements &e)
    {
        if (e.what() == "single"s) CHECK(true); else CHECK(false);
    }
}

TEST_CASE( "group_by(f)", "[relinx]" )
{
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

    CHECK(t1_res->count() == 2);
    CHECK(t1_res->first([](auto &&i){ return i.first == "Doe"s; }).second.size() == 4);
    CHECK(t1_res->first([](auto &&i){ return i.first == "Poo"s; }).second.size() == 2);
    CHECK(from(t1_res->first([](auto &&i){ return i.first == "Doe"s; }).second)->contains([](auto &&i) { return (*i).FirstName == "Sam"s; }));
    CHECK(from(t1_res->first([](auto &&i){ return i.first == "Poo"s; }).second)->contains([](auto &&i) { return (*i).FirstName == "Anna"s; }));
    CHECK(t2_res->single([](auto &&i){ return i.first == (std::hash<std::string>()("Doe"s) ^ (std::hash<std::string>()("John"s) << 1)); }).second.size() == 2);
    CHECK(t2_res->single([](auto &&i){ return i.first == (std::hash<std::string>()("Doe"s) ^ (std::hash<std::string>()("Sam"s) << 1)); }).second.size() == 2);
}

TEST_CASE( "group_join(f)", "[relinx]" )
{
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

    CHECK(t1_res.size() == 3);
    CHECK(t1_res[0].first == "Alex Poo"s); CHECK(t1_res[0].second == "Bob,Bubble,Fluffy"s);
    CHECK(t1_res[1].first == "John Doe"s); CHECK(t1_res[1].second == "Kitty,Spotty"s);
    CHECK(t1_res[2].first == "Sam Doe"s); CHECK(t1_res[2].second == "Sparky"s);

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

    CHECK(t2_res.size() == 6);
    CHECK(t2_res[1].second == std::string()); CHECK(t2_res[3].second == std::string()); CHECK(t2_res[5].second == std::string());
}

TEST_CASE( "intersect_with(v)", "[relinx]" )
{
    auto t1_res = from({1, 2, 3, 7, 8})->intersect_with({2, 3, 5, 6, 7})->to_string();

    CHECK(t1_res == "237"s);
}

TEST_CASE( "then_by(f)", "[relinx]" )
{
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

    CHECK(t1_res[0].FirstName == "Anna"s);
    CHECK(t1_res[1].FirstName == "Alex"s);
    CHECK(t1_res[2].FirstName == "John"s);
    CHECK(t1_res[3].FirstName == "John"s);
    CHECK(t1_res[4].FirstName == "Sam"s);
    CHECK(t1_res[5].FirstName == "Sam"s);
}

TEST_CASE( "then_by_descending(f)", "[relinx]" )
{
    std::vector<Customer> t1_data =
    {
        Customer{0, "John"s, "Doe"s, 25},
        Customer{1, "Sam"s, "Doe"s, 35},
        Customer{2, "John"s, "Doe"s, 25},
        Customer{3, "Alex"s, "Poo"s, 25},
        Customer{4, "Sam"s, "Doe"s, 45},
        Customer{5, "Anna"s, "Poo"s, 23}
    };

    auto t1_res = from(t1_data)->order_by_descending([](auto &&v) { return v.FirstName; })->then_by_descending([](auto &&v) { return v.Age; })->to_vector();

    CHECK(t1_res[0].FirstName == "Sam"s);
    CHECK(t1_res[0].Age == 45);
    CHECK(t1_res[1].FirstName == "Sam"s);
    CHECK(t1_res[1].Age == 35);
    CHECK(t1_res[2].FirstName == "John"s);
    CHECK(t1_res[3].FirstName == "John"s);
    CHECK(t1_res[4].FirstName == "Anna"s);
    CHECK(t1_res[5].FirstName == "Alex"s);
}

TEST_CASE( "join(f)", "[relinx]" )
{
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

    CHECK(t1_res.size() == 6);
    CHECK(t1_res[0].first == "Alex Poo"s); CHECK(t1_res[0].second == "Bob"s);
    CHECK(t1_res[1].first == "Alex Poo"s); CHECK(t1_res[1].second == "Bubble"s);
    CHECK(t1_res[2].first == "Alex Poo"s); CHECK(t1_res[2].second == "Fluffy"s);
    CHECK(t1_res[3].first == "John Doe"s); CHECK(t1_res[3].second == "Kitty"s);
    CHECK(t1_res[4].first == "John Doe"s); CHECK(t1_res[4].second == "Spotty"s);
    CHECK(t1_res[5].first == "Sam Doe"s);  CHECK(t1_res[5].second == "Sparky"s);

    auto t2_res = from(t1_data)->join(t2_data,
                                   [](auto &&i) { return i.Id; },
                                   [](auto &&i) { return i.OwnerId; },
                                   [](auto &&left, auto &&right)
                                   {
                                       return std::make_pair(left.FirstName + " "s + left.LastName, right.NickName);
                                   }
                                   , true)->order_by([](auto &&p) { return p.first; })->
                                        then_by([](auto &&p) { return p.second; })->to_vector();

    CHECK(t2_res.size() == 9);
    CHECK(t2_res[3].second == std::string()); CHECK(t2_res[4].second == std::string()); CHECK(t2_res[7].second == std::string());
}

TEST_CASE( "last(f)", "[relinx]" )
{
    CHECK(from({0, 1, 2, 3, 4, 5, 6, 7, 8})->last([](auto &&v) { return v & 1; }) == 7);
    CHECK(from({5, 6, 7, 8, 5, 6, 8})->last([](auto &&v) { return v < 8; }) == 6);

    try
    {
        from({0, 1, 2})->last([](auto &&v) { return v == 10;});

        CHECK(false);
    }
    catch(const not_found &ex)
    {
        if (std::string(ex.what()) == "last") CHECK(true); else CHECK(false);
    }
}

TEST_CASE( "last()", "[relinx]" )
{
    CHECK(from({5, 6, 7, 8})->last() == 8);

    try
    {
        from(std::vector<int>())->last();

        CHECK(false);
    }
    catch(const no_elements &ex)
    {
        if (std::string(ex.what()) == "last") CHECK(true); else CHECK(false);
    }
}

TEST_CASE( "last_or_default(f)", "[relinx]" )
{
    CHECK(from({0, 1, 2, 3, 4, 5, 6, 7, 8})->last_or_default([](auto &&v) { return v == 100; }) == 0);
    CHECK(from({5, 6, 7, 8})->last_or_default([](auto &&v) { return v == 100; }, 100) == 100);
}

TEST_CASE( "last_or_default()", "[relinx]" )
{
    CHECK(from({7, 8})->last_or_default() == 8);
    CHECK(from(std::vector<int>())->last_or_default() == 0);
    CHECK(from(std::vector<int>())->last_or_default(111) == 111);
}

TEST_CASE( "max(f)", "[relinx]" )
{
    std::vector<Customer> t1_data =
    {
        Customer{0, "John"s, "Doe"s, 25},
        Customer{1, "Sam"s, "Doe"s, 35},
        Customer{2, "John"s, "Doe"s, 25},
        Customer{3, "Alex"s, "Poo"s, 23},
        Customer{4, "Sam"s, "Doe"s, 45},
        Customer{5, "Anna"s, "Poo"s, 23}
    };

    CHECK(from(t1_data)->max([](auto &&v) { return v.Age; }) == 45);

    try
    {
        from(std::vector<Customer>())->max([](auto &&v) { return v.Age; });

        CHECK(false);
    }
    catch(const no_elements &ex)
    {
        if (std::string(ex.what()) == "max"s) CHECK(true); else CHECK(false);
    }
}

TEST_CASE( "max()", "[relinx]" )
{
    CHECK(from({1, 6, 2, 3, 8, 7, 6, 9, 2, 3, 8})->max() == 9);

    try
    {
        from(std::vector<int>())->max();

        CHECK(false);
    }
    catch(const no_elements &ex)
    {
        if (std::string(ex.what()) == "max"s) CHECK(true); else CHECK(false);
    }
}

TEST_CASE( "min(f)", "[relinx]" )
{
    std::vector<Customer> t1_data =
    {
        Customer{0, "John"s, "Doe"s, 25},
        Customer{1, "Sam"s, "Doe"s, 35},
        Customer{2, "John"s, "Doe"s, 25},
        Customer{3, "Alex"s, "Poo"s, 23},
        Customer{4, "Sam"s, "Doe"s, 45},
        Customer{5, "Anna"s, "Poo"s, 23}
    };

    CHECK(from(t1_data)->min([](auto &&v) { return v.Age; }) == 23);

    try
    {
        from(std::vector<Customer>())->min([](auto &&v) { return v.Age; });

        CHECK(false);
    }
    catch(const no_elements &ex)
    {
        if (std::string(ex.what()) == "min"s) CHECK(true); else CHECK(false);
    }
}

TEST_CASE( "min()", "[relinx]" )
{
    CHECK(from({1, 6, 2, 3, 8, 7, 6, 9, 0, 2, 3, 8})->min() == 0);

    try
    {
        from(std::vector<int>())->min();

        CHECK(false);
    }
    catch(const no_elements &ex)
    {
        if (std::string(ex.what()) == "min"s) CHECK(true); else CHECK(false);
    }
}

TEST_CASE( "none(f)", "[relinx]" )
{
    CHECK(from({0, 1, 2, 3, 4, 5, 6, 7, 8})->none([](auto &&v) { return v == 100; }) == true);
    CHECK(from({0, 1, 2, 3, 4, 5, 6, 7, 8})->none([](auto &&v) { return v == 5; }) == false);
    CHECK(from(std::vector<int>())->none([](auto &&v) { return v == 5; }) == true);
}

TEST_CASE( "of_type()", "[relinx]" )
{
    struct base { virtual ~base(){} };
    struct derived : public base { virtual ~derived(){} };
    struct derived2 : public base { virtual ~derived2(){} };

    std::list<base*> t1_data = {new derived(), new derived2(), new derived(), new derived(), new derived2()};

    auto start = hr_clock::now();

    auto t1_res = from(t1_data)->of_type<derived2*>();

    CHECK(t1_res->all([](auto &&i){ return typeid(i) == typeid(derived2*); }));
    CHECK(t1_res->count() == 2);

    for(auto &&i : t1_data){ delete i; };
}

TEST_CASE( "of_type<any>()", "[relinx]" )
{
    std::list<std::any> t1_data = { 1, "string"s, 3, "string view"sv, 3.5f };

    auto t1_res = from(t1_data)->of_type<int>();

    CHECK(t1_res->count() == 2);
}

TEST_CASE( "of_type<variant>()", "[relinx]" )
{
    std::list<std::variant<std::string, std::string_view, int, float>> t1_data = { 1, "string"s, 3, "string view"sv, 3.5f };

    auto start = hr_clock::now();

    auto t1_res = from(t1_data)->of_type<int>();

    CHECK(t1_res->count() == 2);
}

TEST_CASE( "reverse()", "[relinx]" )
{
    CHECK(from({1, 2, 3})->reverse()->to_string() == "321"s);
    CHECK(from({"c"s, "b"s, "a"s})->reverse()->to_string() == "abc"s);

    auto t1_data = { 3, 2, 1 };

    CHECK(from(std::rbegin(t1_data), std::rend(t1_data))->to_string() == "123"s);

    std::forward_list<int> lst {1, 2, 3};

    CHECK(from(lst)->reverse()->to_string() == "321"s);
}

TEST_CASE( "select(f)", "[relinx]" )
{
    auto t0_data = {1, 2, 3};
    auto t1_data = from(t0_data)->select([](auto &&v) { return std::to_string(v); })->to_vector();
    auto t2_data = from(t0_data)->select([](auto &&v) { return float(v); })->to_vector();

    CHECK(t1_data.size() == 3);
    CHECK(typeid(decltype(t1_data)()) == typeid(std::vector<std::string>()));
    CHECK(t1_data[0] == "1"s);
    CHECK(t1_data[1] == "2"s);
    CHECK(t1_data[2] == "3"s);

    CHECK(t2_data.size() == 3);
    CHECK(typeid(decltype(t2_data)()) == typeid(std::vector<float>()));
    CHECK(t2_data[0] == float(1));
    CHECK(t2_data[1] == float(2));
    CHECK(t2_data[2] == float(3));
}

TEST_CASE( "select_i(f)", "[relinx]" )
{
    auto t0_data = {1, 2, 3};
    auto t1_data = from(t0_data)->select_i([](auto &&v, auto &&idx) { return std::to_string(idx) + std::to_string(v); })->to_vector();
    auto t2_data = from(t0_data)->select_i([](auto &&v, auto &&idx) { return std::make_pair(idx, v); })->to_vector();

    CHECK(t1_data.size() == 3);
    CHECK(typeid(decltype(t1_data)()) == typeid(std::vector<std::string>()));
    CHECK(t1_data[0] == "01"s);
    CHECK(t1_data[1] == "12"s);
    CHECK(t1_data[2] == "23"s);

    CHECK(t2_data.size() == 3);
    CHECK(typeid(decltype(t2_data)()) == typeid(std::vector<std::pair<int, int>>()));
    CHECK(t2_data[0].first == 0); CHECK(t2_data[0].second == 1);
    CHECK(t2_data[1].first == 1); CHECK(t2_data[1].second == 2);
    CHECK(t2_data[2].first == 2); CHECK(t2_data[2].second == 3);
}

TEST_CASE( "select_many(f)", "[relinx]" )
{
    std::vector<std::vector<int>> t1_data =
    {
        {1, 2, 3},
        {4, 5},
        {6},
        {7, 8},
        {9, 10}
    };

    auto t1_res = from(t1_data)->select_many([](auto &&v) { return std::move(v); })->to_string();

    CHECK(t1_res == "12345678910"s);
}

TEST_CASE( "select_many_i(f)", "[relinx]" )
{
    std::vector<std::vector<int>> t1_data =
    {
        {1, 2, 3},
        {4, 5},
        {6},
        {7, 8},
        {9, 10}
    };

    auto t1_res = from(t1_data)->select_many_i([](auto &&v, auto &&idx) { std::ostringstream oss; oss << from(v)->to_string() << idx; return std::vector<std::string>({oss.str()}); })->to_string();

    CHECK(t1_res == "1230451627839104"s);
}

TEST_CASE( "sequence_equal(c)", "[relinx]" )
{
    std::vector<int> t1_data = {1, 2, 3};
    std::vector<int> t2_data = {1, 2, 3};
    std::vector<int> t3_data = {3, 2, 1};

    CHECK(from({1, 2, 3})->sequence_equal({1, 2, 3}));
    CHECK(from(t1_data)->sequence_equal(t2_data));
    CHECK_FALSE(from(t2_data)->sequence_equal(t3_data));
}

TEST_CASE( "sequence_equal(c, f)", "[relinx]" )
{
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

    CHECK(from(t1_data)->sequence_equal(t2_data, [](auto &&t1, auto &&t2){ return t1.Id == t2.Id && t1.FirstName == t2.FirstName; }));
    CHECK_FALSE(from(t2_data)->sequence_equal(t3_data, [](auto &&t1, auto &&t2){ return t1.Id == t2.Id && t1.FirstName == t2.FirstName; }));
}

TEST_CASE( "single()", "[relinx]" )
{
    std::vector<int> t1_data = {1};
    std::vector<int> t2_data = {1, 2};
    std::vector<int> t3_data;

    CHECK(from(t1_data)->single());

    try
    {
        from(t2_data)->single();

        CHECK(false);
    }
    catch(const invalid_operation &e)
    {
        if (e.what() == "single"s) CHECK(true); else CHECK(false);
    }

    try
    {
        from(t3_data)->single();

        CHECK(false);
    }
    catch(const no_elements &e)
    {
        if (e.what() == "single"s) CHECK(true); else CHECK(false);
    }
}

TEST_CASE( "single_or_default(f)", "[relinx]" )
{
    std::vector<int> t1_data = {1, 2, 3};
    std::vector<int> t2_data = {1, 3, 5};
    std::vector<int> t3_data;
    std::vector<int> t4_data = {2, 4, 6};

    CHECK(from(t1_data)->single_or_default([](auto &&v) { return !(v % 2); }) == 2);
    CHECK(from(t2_data)->single_or_default([](auto &&v) { return !(v % 2); }) == 0);
    CHECK(from(t2_data)->single_or_default([](auto &&v) { return !(v % 2); }, -1) == -1);
    CHECK(from(t3_data)->single_or_default([](auto &&v) { return !(v % 2); }) == 0);
    CHECK(from(t3_data)->single_or_default([](auto &&v) { return !(v % 2); }, 888) == 888);

    try
    {
        from(t4_data)->single_or_default([](auto &&v) { return !(v % 2); });

        CHECK(false);
    }
    catch(const invalid_operation &e)
    {
        if (e.what() == "single_or_default"s) CHECK(true); else CHECK(false);
    }
}

TEST_CASE( "single_or_default()", "[relinx]" )
{
    std::vector<int> t1_data = {1};
    std::vector<int> t2_data = {1, 2};
    std::vector<int> t3_data;

    CHECK(from(t1_data)->single_or_default() == 1);
    CHECK(from(t1_data)->single_or_default(111) == 1);

    try
    {
        from(t2_data)->single_or_default();

        CHECK(false);
    }
    catch(const invalid_operation &e)
    {
        if (e.what() == "single_or_default"s) CHECK(true); else CHECK(false);
    }

    CHECK(from(t3_data)->single_or_default() == 0);
    CHECK(from(t3_data)->single_or_default(555) == 555);
}

TEST_CASE( "skip(v)", "[relinx]" )
{
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8})->skip(5)->to_string() == "678"s);
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8})->skip(100)->any() == false);
}

TEST_CASE( "skip_while(f)", "[relinx]" )
{
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8})->skip_while([](auto &&v){ return v < 6; })->to_string() == "678"s);
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8})->skip_while([](auto &&v){ return v < 100; })->any() == false);
}

TEST_CASE( "skip_while_i(f)", "[relinx]" )
{
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8})->skip_while_i([](auto &&v, auto &&i){ return v + i < 6; })->to_string() == "45678"s);
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8})->skip_while_i([](auto &&v, auto &&i){ return v * i < 15; })->to_string() == "5678"s);
}

TEST_CASE( "sum(f)", "[relinx]" )
{
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

    CHECK(from(t1_data)->sum([](auto &&i) { return i.Age; }) == 176);
    CHECK(from(t2_data)->sum([](auto &&i) { return i.Age; }) == 0);
}

TEST_CASE( "sum()", "[relinx]" )
{
    std::vector<int> t1_data;
    std::vector<std::string> t2_data = { "1.5"s, "3.2"s, "15.38"s, "2.79"s, "7.1"s, "5.55"s };

    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8})->sum() == 36);
    CHECK(from({10, 10, 10, 10, 10, 10, 10, 10, 10, 5, 5})->sum() == 100);
    CHECK(from(t1_data)->sum() == 0);
    CHECK(from(t2_data)->sum() == from(t2_data)->to_string());
}

TEST_CASE( "take(v)", "[relinx]" )
{
    std::vector<int> t1_data;

    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8})->take(5)->to_string() == "12345"s);
    CHECK_FALSE(from({1, 2, 3, 4, 5, 6, 7, 8})->take(0)->any());
    CHECK_FALSE(from(t1_data)->take(10)->any());
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8})->take(-1)->to_string() == "12345678"s);
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8})->take(10)->to_string() == "12345678"s);
}

TEST_CASE( "take_while(f)", "[relinx]" )
{
    std::vector<int> t1_data;

    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8})->take_while([](auto &&v) { return v < 6; })->to_string() == "12345"s);
    CHECK_FALSE(from({1, 2, 3, 4, 5, 6, 7, 8})->take_while([](auto &&v) { return v > 100; })->any());
    CHECK_FALSE(from(t1_data)->take_while([](auto &&) { return true; })->any());
    CHECK(from({1, 2, 3, 4, 5, 6, 7, 8})->take_while([](auto &&v) { return v < 100; })->to_string() == "12345678"s);
}

TEST_CASE( "take_while_i(f)", "[relinx]" )
{
    CHECK(from({"1"s, "2"s, "3"s, "4"s, "5"s, "6"s, "7"s, "8"s})->take_while_i([](auto &&, auto &&idx) { return idx < 6; })->to_string() == "123456"s);
}

TEST_CASE( "tee(f)", "[relinx]" )
{
    int cnt { 0 };
    std::vector<int> t1_data { 1, 2, 3, 4, 5, 6, 7, 8 };

    auto s { from(t1_data)->tee([&cnt](auto &&) { ++cnt; })->where([](auto &&v) { return v <= 3; })->count() };

    CHECK(cnt == static_cast<int>(std::size(t1_data)));
    CHECK(s == 3);
}

TEST_CASE( "to_container()", "[relinx]" )
{
    std::initializer_list<int> t1_data = {1, 2, 3};
    auto t1_res = from(t1_data)->to_container<std::set<int>>();
    auto t2_res = from(t1_data)->to_container<std::forward_list<int>>();

    CHECK(typeid(t1_res) == typeid(std::set<int>));
    CHECK(t1_res.size() == t1_data.size());
    CHECK(typeid(t2_res) == typeid(std::forward_list<int>));
    CHECK(std::distance(std::begin(t2_res), std::end(t2_res)) == std::ptrdiff_t(t1_data.size()));
}

TEST_CASE( "to_list()", "[relinx]" )
{
    std::vector<long> t1_data = {1, 1, 1};

    auto t1_res = from(t1_data)->to_list();

    CHECK(typeid(t1_res) == typeid(std::list<long>));
    CHECK(from(t1_res)->count() == 3);
    CHECK(from(t1_res)->all([](auto &&v) { return v == 1; }));
}

TEST_CASE( "to_map(f, f)", "[relinx]" )
{
    std::vector<long> t1_data = {1, 2, 3};

    auto t1_res = from(t1_data)->to_map([](auto &&v) {  return v; }, [](auto &&v) { return double(v * 2); });

    CHECK(typeid(t1_res) == typeid(default_map<long, double>));
    CHECK(t1_res.size() == 3);
    CHECK(t1_res[1] == 2);
    CHECK(t1_res[2] == 4);
    CHECK(t1_res[3] == 6);
}

TEST_CASE( "to_multimap(f, f)", "[relinx]" )
{
    std::vector<long> t1_data = {1, 2, 3, 1, 2, 3, 3};

    auto t1_res = from(t1_data)->to_multimap([](auto &&v) {  return v; }, [](auto &&v) { return double(v * 2); });

    CHECK(typeid(t1_res) == typeid(default_multimap<long, double>));
    CHECK(t1_res.size() == 7);

    auto r1_res = t1_res.equal_range(1);
    auto r2_res = t1_res.equal_range(2);
    auto r3_res = t1_res.equal_range(3);

    CHECK(from(r1_res.first, r1_res.second)->select([](auto &&v) { return v.second; })->to_string() == "22"s);
    CHECK(from(r2_res.first, r2_res.second)->select([](auto &&v) { return v.second; })->to_string() == "44"s);
    CHECK(from(r3_res.first, r3_res.second)->select([](auto &&v) { return v.second; })->to_string() == "666"s);
}

TEST_CASE( "union_with(c, f)", "[relinx]" )
{
    CHECK(from({1, 2, 3, 1, 1, 2, 3, 3, 3, 2, 2, 1})->union_with({1, 2, 3, 5, 7, 8, 9, 0})->order_by()->to_string() == "01235789"s);
}

TEST_CASE( "where_i(f)", "[relinx]" )
{
    auto t1_res = from({0, 0, 8, 0, 8, 8, 0, 0, 0, 0, 8, 0, 8, 0, 0, 8, 0})->where_i([](auto &&v, auto &&idx) { return idx < 3 && v > 0; })->to_vector();

    CHECK(t1_res.size() == 3);
    CHECK(t1_res[0] == 8); CHECK(t1_res[1] == 8); CHECK(t1_res[2] == 8);
}

TEST_CASE( "zip(c, f)", "[relinx]" )
{
    auto t1_res = from({1, 2, 3, 4, 5})->zip({"one"s, "two"s, "three"s}, [](auto &&a, auto &&b) { return std::to_string(a) + " " + b; })->to_vector();

    CHECK(t1_res.size() == 3);
    CHECK(t1_res[0] == "1 one"s);
    CHECK(t1_res[1] == "2 two"s);
    CHECK(t1_res[2] == "3 three"s);
}
