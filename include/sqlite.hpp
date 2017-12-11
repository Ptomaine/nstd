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

extern "C"
{
#include "external/sqlite/sqlite3.h"
}

#define MODERN_SQLITE_STD_OPTIONAL_SUPPORT
#include "external/sqlite_modern_cpp/hdr/sqlite_modern_cpp.h"
#include <tuple>
#include <vector>

namespace nstd::db
{

namespace sqlite = sqlite;

struct scoped_transaction
{
    scoped_transaction(nstd::db::sqlite::database &db, bool autocommit = false) : _db(db), _rollback(!autocommit) { _db << "begin"; };
    ~scoped_transaction() { _db << (_rollback ? "rollback" : "commit"); };

    void rollback() { _rollback = true; }
    void commit() { _rollback = false; }

private:
    nstd::db::sqlite::database &_db;
    bool _rollback { true };
};

template<typename Target, typename... ColTypes>
struct records
{
    using data_container_type = std::vector<Target>;
    data_container_type records;

    records() = default;
    records(std::size_t preallocate) { records.reserve(preallocate); }
    ~records() = default;

    void operator()(ColTypes... args)
    {
        records.emplace_back(std::forward_as_tuple(std::move(args)...));
    };

    data_container_type &data()
    {
        return records;
    }
};

template<typename... ColTypes>
struct tuple_records
{
    using data_container_type = std::vector<std::tuple<ColTypes...>>;
    data_container_type records;

    tuple_records() = default;
    tuple_records(std::size_t preallocate) { records.reserve(preallocate); }
    ~tuple_records() = default;

    void operator()(ColTypes... args)
    {
        records.emplace_back(std::forward_as_tuple(std::move(args)...));
    };

    data_container_type &data()
    {
        return records;
    }
};

}

