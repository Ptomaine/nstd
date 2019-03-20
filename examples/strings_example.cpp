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

#include <iostream>
#include "strings.hpp"
#include "string_id.hpp"
#include "platform_utilities.hpp"

int main()
{
    nstd::platform::utilities::scoped_console_utf8 set_console_utf8;

    using namespace std::string_literals;
    using namespace nstd::str;
    using namespace nstd::str::sid::literals;

    std::u16string u16str(trim(u"    Hello World!      "));

    std::cout << u8"Всем привет :)" << std::endl;
    std::cout << std::string(trim("    Hello World!      ")) << std::endl;
    std::wcout << trim(L"    Hello World!      ") << std::endl;
    std::cout << from_utf16_to_utf8(u16str) << std::endl;
    std::cout << from_utf32_to_utf8(std::u32string(trim(U"    Hello World!      "))) << std::endl;

    std::cout << from_utf32_to_utf8(replace_all(U"good strings (UTF-32)"s, U"o"s, U"OO"s)) << std::endl;
    std::cout << replace_regex("good strings (string)"s, "o"s, "[$&]"s) << std::endl;
    std::wcout << replace_regex(L"good strings (wstring)"s, L"o"s, L"[$&]"s) << std::endl;

    auto str_container = { 1, 2, 3 };
    std::wcout << L"{" << join(str_container, L"}{"s) << L"}" << std::endl;

    auto str_to_split { L"let's  split       every  word   "s };

    std::wcout << "to split: '" << str_to_split << "'" << std::endl;

    auto splitted { split_regex(str_to_split, LR"(\s+)"s) };

    for (auto &&str : splitted) std::wcout << str << std::endl;

    std::wcout << "joined: '" << replace_all(join(splitted, L" "s), L"split"s, L"join"s) << "'" << std::endl;

    std::cout << "Is empty or white space: " << boolalpha[is_empty_or_ws(""s)] << std::endl;
    std::cout << "Is empty or white space: " << boolalpha[is_empty_or_ws(std::wstring{})] << std::endl;
    std::cout << "Is empty or white space: " << boolalpha[is_empty_or_ws("   \n\t      "s)] << std::endl;
    std::cout << "Is empty or white space: " << boolalpha[is_empty_or_ws(L"   \n\t      "s)] << std::endl;
    std::cout << "Is empty or white space: " << boolalpha[is_empty_or_ws("   \n\t  ABC  "s)] << std::endl;
    std::cout << "Is empty or white space: " << boolalpha[is_empty_or_ws(L"   \n\t ABC  "s)] << std::endl;

    auto nr { 3 };
    auto s { "'a const char array'" };
    auto true_val { true };
    std::cout << compose_string("\n", "The value is ", nr, " and another value is ", 8, "; string value is:\t", s, ";\nmax long double precision: ", std::numeric_limits<long double>::max_digits10,
                                            "; double: ", numeric_to_string(123.56789012L, std::numeric_limits<long double>::max_digits10),
                                            "\ninfinity: ", numeric_to_string(-std::numeric_limits<double>::infinity()), "\n", "\n", boolalpha[true_val], ", ",
                                            boolalpha[false]) << std::endl;

    std::cout << std::setprecision(std::numeric_limits<long double>::max_digits10) << wstring_to_numeric<long double>(L"123.56789012") << std::endl;

    // create database to store the strings in
    // it must stay valid as long as each string_id using it
    sid::default_database database;

    //=== string_id usage ===//
    // create an id
    sid::string_id sid("Test0815", database);
    std::cout << "Hash code " << sid.hash_code() << " belongs to string \"" << sid.string() << "\"\n";
    // Output (Database supports retrieving): Hash code 16741300784925887095 belongs to string "Test0815"
    // Output (Database doesn't): Hash code 16741300784925887095 belongs to string "string_id database disabled"

    sid::string_id a("Hello", database), b("World", database);

    // compare two ids
    std::cout << std::boolalpha << (a == b) << '\n';
    // Output: false

    // compare id with constant
    std::cout << (a == "Hello"_id) << '\n';
    // Output: true

    // literal is compile-time
    switch (b.hash_code())
    {
    case "Hello"_id:
        std::cout << "Hello\n";
        break;
    case "world"_id: // case-sensitive
        std::cout << "world\n";
        break;
    case "World"_id:
        std::cout << "World\n";
        break;
    }

    //=== generation ===//
    // the prefix for all generated ids
    sid::string_id prefix("entity-", database);
    try
    {
        // a generator type appending 8 random characters to the prefix
        // it uses the std::mt19937 generator for the actual generation
        typedef sid::random_generator<std::mt19937, 8> generator_t;
        // create a generator, seed the random number generator with the current time
        generator_t generator(prefix, std::mt19937(std::int_fast32_t(std::time(nullptr))));

        std::vector<sid::string_id> ids;
        for (auto i = 0; i != 10; ++i)
            // generate new identifier
            // it is guaranteed unique and will be stored in the database of the prefix
            ids.push_back(generator());

        // print the name of all the ids
        for (auto &id : ids)
            std::cout << id.string() << '\n';
        // possible generated name: entity-jXRnZAVG
    }
    catch (sid::generation_error &ex)
    {
        // the generator was unable to generate a new unique identifier (very unlikely)
        std::cerr << "[ERROR] " << ex.what() << '\n';
    }

    try
    {
        // a generator appending an increasing number to the prefix
        typedef sid::counter_generator generator_t;

        // create a generator starting with 0, each number will be 4 digits long
        generator_t generator(prefix, 0, 4);

        std::vector<sid::string_id> ids;
        for (auto i = 0; i != 10; ++i)
            // generate new identifier
            // it is guaranteed unique and will be stored in the database of the prefix
            ids.push_back(generator());

        // print the name of all the ids
        for (auto &id : ids)
            std::cout << id.string() << '\n';
        // possible generated name: entity-0006
    }
    catch (sid::generation_error &ex)
    {
        // the generator was unable to generate a new unique identifier (very unlikely)
        std::cerr << "[ERROR] " << ex.what() << '\n';
    }

    std::cout << "exitting..." << std::endl;

    return 0;
}
