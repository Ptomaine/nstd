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

int main()
{
    using namespace std::string_literals;
    using namespace nstd::str;

	std::cout << std::string(trim("    Hello World!      ")) << std::endl;
	std::wcout << trim(L"    Hello World!      ") << std::endl;
	std::wcout << from_utf16_to_wchar(std::u16string(trim(u"    Hello World!      "))) << std::endl;
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
	std::cout << "exitting..." << std::endl;

    return 0;
}
