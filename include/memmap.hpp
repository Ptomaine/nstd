#pragma once

/*
MIT License

Copyright (c) 2021 Arlen Keshabyan (arlen.albert@gmail.com)

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

#include "platform.hpp"

#ifdef OS_FAMILY_UNIX

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using mmap_size_t = off_t;

#elif defined(OS_FAMILY_WINDOWS)

#include <windows.h>

using mmap_size_t = DWORD;

#else
#error OS not supported
#endif

#include <exception>
#include <filesystem>

namespace nstd
{

template <typename T>
concept non_pointer_type = !std::is_pointer_v<T>;

template <non_pointer_type T>
class memmap
{
    using data_ptr_type = std::add_pointer_t<T>;

    data_ptr_type _data;
    mmap_size_t _size;

#ifdef OS_FAMILY_UNIX
    int _filedesc;
#elif defined(OS_FAMILY_WINDOWS)
    HANDLE _file;
    HANDLE _mapping;
#endif

    static inline constexpr const bool is_const_t { std::is_const_v<T> };
    static inline constexpr const bool is_non_const_t { !is_const_t };

public:

    memmap(const std::filesystem::path& p) : _data(nullptr),
#ifdef OS_FAMILY_UNIX
                               _filedesc(-1)
#elif defined(OS_FAMILY_WINDOWS)
                               _file(INVALID_HANDLE_VALUE), _mapping(NULL)
#endif
    {
        if (!std::filesystem::is_regular_file(p) || !std::filesystem::exists(p)) throw std::invalid_argument("File doesn't exist");

        auto path { p.c_str() };

#ifdef OS_FAMILY_UNIX

        if constexpr (is_const_t) _filedesc = ::open(path, O_RDONLY); else _filedesc = open(path, O_RDWR);

        if (_filedesc == -1) throw std::invalid_argument("Failed to open the requested file");
        
        _size = ::lseek(_filedesc, 0, SEEK_END);
        
        if (_size == (static_cast<mmap_size_t>(-1))) throw std::invalid_argument("Size could not be determined");
        
        if constexpr (is_const_t)
            _data = reinterpret_cast<data_ptr_type>(::mmap(0, _size, PROT_READ, MAP_SHARED, _filedesc, 0));
        else
            _data = reinterpret_cast<data_ptr_type>(::mmap(0, _size, PROT_READ | PROT_WRITE, MAP_SHARED, _filedesc, 0));
        
        if ((void*)_data == MAP_FAILED)
        {
            ::close(_filedesc);
            throw std::invalid_argument("File could not be mapped");
        }

#elif defined(OS_FAMILY_WINDOWS)
        
        if constexpr (is_const_t)
            _file = ::CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        else
            _file = ::CreateFileW(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (_file == INVALID_HANDLE_VALUE) throw std::runtime_error("Failed to open the requested file");

        _size = ::GetFileSize(_file, NULL);
        
        if constexpr (is_const_t)
            _mapping = ::CreateFileMapping(_file, NULL, PAGE_READONLY, 0, 0, NULL);
        else
            _mapping = ::CreateFileMapping(_file, NULL, PAGE_READWRITE, 0, 0, NULL);

        if (_mapping == NULL)
        {
            ::CloseHandle(_file);
            throw std::invalid_argument("Mapping failed");
        }

        if constexpr (is_const_t)
            _data = reinterpret_cast<data_ptr_type>(::MapViewOfFile(_mapping, FILE_MAP_READ, 0, 0, 0));
        else
            _data = reinterpret_cast<data_ptr_type>(::MapViewOfFile(_mapping, FILE_MAP_WRITE, 0, 0, 0));

#endif
    }

    auto begin() requires (is_non_const_t)
    {
        return _data;
    }

    auto end() requires (is_non_const_t)
    {
        return _data + _size;
    }

    auto begin() const
    {
        return _data;
    }

    auto end() const
    {
        return _data + _size;
    }

    auto data() requires (is_non_const_t)
    {
        return _data;
    }
    
    auto data() const
    {
        return _data;
    }

    auto& front() requires (is_non_const_t)
    {
        return *_data;
    }
    
    auto& front() const
    {
        return *_data;
    }

    auto& back() requires (is_non_const_t)
    {
        return *(_data + _size - 1);
    }
    
    auto& back() const
    {
        return *(_data + _size - 1);
    }

    auto& operator[](size_t i) requires (is_non_const_t)
    {
        return _data[i];
    }
    
    auto& operator[](size_t i) const
    {
        return _data[i];
    }
    
    
    mmap_size_t size() const
    {
        return _size;
    }

    void resize(mmap_size_t size) requires (is_non_const_t)
    {
#ifdef OS_FAMILY_UNIX

        if (::ftruncate(_filedesc, size) == -1) throw std::invalid_argument("ftruncate failed");

        if (::munmap((void*)_data, _size)) throw std::invalid_argument("munmap failed");

        _size = size;
        _data = reinterpret_cast<data_ptr_type>(::mmap(0, _size, PROT_READ | PROT_WRITE, MAP_SHARED, _filedesc, 0));
        
        if ((void*)_data == MAP_FAILED) throw std::invalid_argument("File could not be mapped");

#elif defined(OS_FAMILY_WINDOWS)

        ::UnmapViewOfFile(_data);

        _data = nullptr;

        ::CloseHandle(_mapping);

        _mapping = NULL;

        ::SetFilePointer(_file, size, 0, FILE_BEGIN);
        ::SetEndOfFile(_file);

        _size = size;
        _mapping = ::CreateFileMapping(_file, NULL, PAGE_READWRITE, 0, 0, NULL);

        if (_mapping == NULL) throw std::invalid_argument("Mapping failed");

        _data = reinterpret_cast<data_ptr_type>(::MapViewOfFile(_mapping, FILE_MAP_WRITE, 0, 0, 0));

#endif
    }

    void resize_relational(mmap_size_t amount) requires (is_non_const_t)
    {
        resize(_size + amount);
    }

    ~memmap()
    {
#ifdef OS_FAMILY_UNIX

        if (_data) ::munmap((void*)_data, _size);
        if (_filedesc != -1) ::close(_filedesc);

#elif defined(OS_FAMILY_WINDOWS)
        
        if (_data) ::UnmapViewOfFile(_data);
        if (_mapping) ::CloseHandle(_mapping);
        if (_file != INVALID_HANDLE_VALUE) ::CloseHandle(_file);

#endif
    }
};

}
