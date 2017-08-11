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
#include "giant.hpp"

int main()
{
    uint16_t u16 = 0x1234;
    uint32_t u32 = 0x12345678;
    uint64_t u64 = 0x123456789ABCDEF0;

    uint16_t u16sw = nstd::giant::swap( u16 );
    uint32_t u32sw = nstd::giant::swap( u32 );
    uint64_t u64sw = nstd::giant::swap( u64 );

    std::cout << std::hex;
    std::cout << "Target is " << ( nstd::giant::is_little ? "little endian" : "big endian") << std::endl;

    // checks
    assert( nstd::giant::swap(u16sw) == u16 );
    assert( nstd::giant::swap(u32sw) == u32 );
    assert( nstd::giant::swap(u64sw) == u64 );

    if( nstd::giant::is_little )
        assert( nstd::giant::c_htole('unix') == 'unix' && nstd::giant::c_htobe('unix') == 'xinu' );
    else
        assert( nstd::giant::c_htobe('unix') == 'unix' && nstd::giant::c_htole('unix') == 'xinu' );

    // show conversions
    std::cout << "big to host:    " << nstd::giant::betoh(u64)    << std::endl;
    std::cout << "little to host: " << nstd::giant::letoh(u64) << std::endl;
    std::cout << "host to big:    " << nstd::giant::htobe(u64)    << std::endl;
    std::cout << "host to little: " << nstd::giant::htole(u64) << std::endl;

    // non-pod types wont get swapped neither converted
    assert( nstd::giant::swap( std::string("hello world") ) == "hello world" );
    assert( nstd::giant::letobe( std::string("hello world") ) == "hello world" );

    uint64_t t { 0 };

    if constexpr (nstd::giant::is_little)
    {
        t = nstd::giant::c_swap(0x0123456789012345); //compile time swap
    }
    else
    {
        t = nstd::giant::c_swap(0x4523018967452301); //compile time swap
    }

    std::cout << t << std::endl;

    return 0;
}
