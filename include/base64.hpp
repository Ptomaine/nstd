#pragma once

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

#include <iterator>
#include <string>
#include <vector>

namespace nstd::base64
{

std::string base64_encode(const void *data, size_t length)
{
    static constexpr const uint8_t basis_64[]
    {
        'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
        '0','1','2','3','4','5','6','7','8','9','+','/'
    };
    auto bytes_to_encode { reinterpret_cast<const uint8_t*>(data) };
    std::string container((length + 2) / 3 * 4, '=');
    auto result { std::data(container) };
    uint64_t idx { 0 }, i { 0 };

	for (; i < length - 2; i += 3)
    {
		result[idx++] = basis_64[ (bytes_to_encode[i] >> 2) & 0x3f];
		result[idx++] = basis_64[((bytes_to_encode[i] & 0x3) << 4) | (int(bytes_to_encode[i + 1] & 0xf0) >> 4)];
		result[idx++] = basis_64[((bytes_to_encode[i + 1] & 0xf) << 2) | (int(bytes_to_encode[i + 2] & 0xc0) >> 6)];
		result[idx++] = basis_64[  bytes_to_encode[i + 2] & 0x3f];
	}

	if (i < length)
    {
		result[idx++] = basis_64[(bytes_to_encode[i] >> 2) & 0x3f];

		if (i == (length - 1))
        {
			result[idx++] = basis_64[((bytes_to_encode[i] & 0x3) << 4)];
		}
		else
        {
			result[idx++] = basis_64[((bytes_to_encode[i] & 0x3) << 4) | ((int)(bytes_to_encode[i + 1] & 0xf0) >> 4)];
			result[idx++] = basis_64[((bytes_to_encode[i + 1] & 0xf) << 2)];
		}
	}

    return container;
}

template<typename Container>
std::string base64_encode(const Container &container)
{
    return base64_encode(std::data(container), std::size(container));
}

template<typename ResultContainerType = std::vector<uint8_t>>
ResultContainerType base64_decode(const std::string &encoded_string)
{
    static constexpr const uint8_t base64_dtable[256]
    {
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 62  , 0x80, 62  , 0x80, 63  ,
        52  , 53  , 54  , 55  , 56  , 57  , 58  , 59  , 60  , 61  , 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0   , 1   , 2   , 3   , 4   , 5   , 6   , 7   , 8   , 9   , 10  , 11  , 12  , 13  , 14  ,
        15  , 16  , 17  , 18  , 19  , 20  , 21  , 22  , 23  , 24  , 25  , 0x80, 0x80, 0x80, 0x80, 63  ,
        0x80, 26  , 27  , 28  , 29  , 30  , 31  , 32  , 33  , 34  , 35  , 36  , 37  , 38  , 39  , 40  ,
        41  , 42  , 43  , 44  , 45  , 46  , 47  , 48  , 49  , 50  , 51  , 0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
        0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
    };
	auto idx { 0u };
    auto encoded_bytes { reinterpret_cast<const uint8_t*>(std::data(encoded_string)) };
    auto encoded_length { std::size(encoded_string) };
	auto pad { encoded_length > 0 && (encoded_length % 4 || encoded_bytes[encoded_length - 1] == '=') };
	const auto unpadded_lenth { ((encoded_length + 3) / 4 - pad) * 4 };
	ResultContainerType container(3 * ((encoded_length + 3) / 4), typename ResultContainerType::value_type());
	auto result { std::data(container) };

	for (size_t i { 0 }; i < unpadded_lenth; i += 4)
	{
		int n { base64_dtable[encoded_bytes[i]] << 18 | base64_dtable[encoded_bytes[i + 1]] << 12 | base64_dtable[encoded_bytes[i + 2]] << 6 | base64_dtable[encoded_bytes[i + 3]] };

		result[idx++] = n >> 16;
		result[idx++] = n >> 8 & 0xff;
		result[idx++] = n & 0xff;
	}

	if (pad)
	{
		int n { base64_dtable[encoded_bytes[unpadded_lenth]] << 18 | base64_dtable[encoded_bytes[unpadded_lenth + 1]] << 12 };

		result[idx++] = n >> 16;

		if (encoded_length > unpadded_lenth + 2 && encoded_bytes[unpadded_lenth + 2] != '=')
		{
			n |= base64_dtable[encoded_bytes[unpadded_lenth + 2]] << 6;
			result[idx++] = n >> 8 & 0xff;
		}
	}

	if (idx < std::size(container)) container.resize(idx);

	return container;
}

}

