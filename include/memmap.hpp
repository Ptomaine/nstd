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

#include <Windows.h>
using mmap_size_t = DWORD;

#else
#error OS not supported
#endif

#include <cassert>
#include <exception>
#include <filesystem>
#include <type_traits>

namespace nstd
{

template <typename T>
requires std::is_pointer_v<T>
struct memmap
{
private:
    static inline constexpr bool is_const_t { std::is_const_v<std::remove_pointer<T>> };
    T _data;

#ifdef OS_FAMILY_UNIX
    int filedesc;
#elif defined(OS_FAMILY_WINDOWS)
    HANDLE _file;
    HANDLE _mapping;
#endif
    mmap_size_t _size;

public:
    memmap(const std::filesystem::path& paf) : memmap(paf.c_str()) {}
    memmap(const std::string& path) : memmap(path.c_str()) {}
    memmap(const char* path) : _data(nullptr),
#ifdef OS_FAMILY_UNIX
                                filedesc(-1)
#elif defined(OS_FAMILY_WINDOWS)
                                _file(INVALID_HANDLE_VALUE), _mapping(NULL)
#endif
    {
#ifdef OS_FAMILY_UNIX

        if constexpr (is_const_t) filedesc = open(path, O_RDONLY); else filedesc = open(path, O_RDWR);

        if (filedesc == -1) throw std::invalid_argument("File could not be opened");
        
        _size = lseek(filedesc, 0, SEEK_END);
        
        if (_size == (static_cast<mmap_size_t>(-1))) throw std::invalid_argument("Size could not be determined");
        
        if constexpr (is_const_t)
            _data = (T*)mmap(0, _size, PROT_READ, MAP_SHARED, filedesc, 0);
        else
            _data = (T*)mmap(0, _size, PROT_READ | PROT_WRITE, MAP_SHARED, filedesc, 0);
        
        if ((void*)_data == MAP_FAILED)
        {
            ::close(filedesc);
            throw std::invalid_argument("File could not be mapped");
        }

#elif defined(OS_FAMILY_WINDOWS)
        
        
        if constexpr (is_const_t)
            _file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        else
            _file = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (_file == INVALID_HANDLE_VALUE) throw std::runtime_error("Opening file failed");

        _size = GetFileSize(_file, NULL);
        
        if constexpr (is_const_t)
            _mapping = CreateFileMapping(_file, NULL, PAGE_READONLY, 0, 0, NULL);
        else
            _mapping = CreateFileMapping(_file, NULL, PAGE_READWRITE, 0, 0, NULL);

        if (_mapping == NULL) throw std::invalid_argument("Mapping failed");

        if constexpr (is_const_t)
            _data = reinterpret_cast<T>(MapViewOfFile(_mapping, FILE_MAP_READ, 0, 0, 0));
        else
            _data = reinterpret_cast<T>(MapViewOfFile(_mapping, FILE_MAP_WRITE, 0, 0, 0));

#endif
    }

    auto begin() requires (!is_const_t)
    {
        return _data;
    }

    auto end() requires (!is_const_t)
    {
        return _data + size();
    }

    auto begin() const
    {
        return _data;
    }

    auto end() const
    {
        return _data + size();
    }

    auto data() requires (!is_const_t)
    {
        return _data;
    }
    
    auto data() const
    {
        return _data;
    }

    auto& operator[](size_t i) requires (!is_const_t)
    {
        assert(i < _size);

        return _data[i];
    }
    
    auto& operator[](size_t i) const
    {
        assert(i < _size);

        return _data[i];
    }
    
    void resize(mmap_size_t size)
    {
#ifdef OS_FAMILY_UNIX

        static_assert(!is_const_t);

        if (ftruncate(filedesc, size) == -1) throw std::invalid_argument("ftruncate failed");

        if (munmap((void*)_data, _size)) throw std::invalid_argument("munmap failed");

        _size = size;

        if constexpr (is_const_t)
            _data = reinterpret_cast<T>(mmap(0, _size, PROT_READ, MAP_SHARED, filedesc, 0));
        else
            _data = reinterpret_cast<T>(mmap(0, _size, PROT_READ | PROT_WRITE, MAP_SHARED, filedesc, 0));
        
        if ((void*)_data == MAP_FAILED) throw std::invalid_argument("File could not be mapped");

#elif defined(OS_FAMILY_WINDOWS)

        static_assert(!is_const_t);

        UnmapViewOfFile(_data);

        _data = nullptr;

        CloseHandle(_mapping);

        _mapping = NULL;

        SetFilePointer(_file, size, 0, FILE_BEGIN);
        SetEndOfFile(_file);

        _size = size;

        if constexpr (is_const_t)
            _mapping = CreateFileMapping(_file, NULL, PAGE_READONLY, 0, 0, NULL);
        else
            _mapping = CreateFileMapping(_file, NULL, PAGE_READWRITE, 0, 0, NULL);

        if (_mapping == NULL) throw std::invalid_argument("Mapping failed");

        
        if constexpr (is_const_t)
            _data = reinterpret_cast<T>(MapViewOfFile(_mapping, FILE_MAP_READ, 0, 0, 0));
        else
            _data = reinterpret_cast<T>(MapViewOfFile(_mapping, FILE_MAP_WRITE, 0, 0, 0));
#endif
    }

    void close()
    {
#ifdef OS_FAMILY_UNIX

        if (_data)
        {
            munmap((void*)_data, _size);

            _data = nullptr;
        }

        if (filedesc != -1)
        {
            ::close(filedesc);
        
            filedesc = -1;
        }

#elif defined(OS_FAMILY_WINDOWS)
        
        if (_data)
        {
            UnmapViewOfFile(_data);
        
            _data = nullptr;
        }
        
        if (_mapping)
        {
            CloseHandle(_mapping);
        
            _mapping = NULL;
        }
        
        if (_file != INVALID_HANDLE_VALUE)
        {
            CloseHandle(_file);
        
            _file = INVALID_HANDLE_VALUE;
        }
#endif
    }

    ~memmap()
    {
        close();
    }
    
    mmap_size_t size() const
    {
        return _size;
    }
};

}
