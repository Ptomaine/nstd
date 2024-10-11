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
#include <chrono>
#include <exception>
#include <thread>
#include <tuple>
#include <vector>

#define into_var(varName) [&varName](decltype(varName) data) { varName = data; }

namespace sqlite
{

struct scoped_transaction
{
    scoped_transaction(database &db, bool autocommit = false) : _db(db), _rollback(!autocommit) { _db << _begin_cmd; };
    ~scoped_transaction()
    {
        if (std::uncaught_exceptions() && !_rollback) _rollback = true;

        _db << (_rollback ? _rollback_cmd : _commit_cmd);
    };

    void rollback() { _rollback = true; }
    void commit() { _rollback = false; }

private:
    inline constexpr static const char* const _begin_cmd { "begin" };
    inline constexpr static const char* const _rollback_cmd { "rollback" };
    inline constexpr static const char* const _commit_cmd { "commit" };
    database &_db;
    bool _rollback { true };
};

using namespace std::chrono_literals;

template <int page_size = -1, uint32_t sleep_time_ms = 0>
inline auto backup_database(const database &from, const database &to, const std::string_view from_db_name = "main", const std::string_view to_db_name = "main")
{
    using namespace std::literals;

    auto from_con { from.connection() };
    auto state { std::unique_ptr<::sqlite3_backup, decltype(&::sqlite3_backup_finish)>(::sqlite3_backup_init(to.connection().get(), std::data(to_db_name), from_con.get(), std::data(from_db_name)), ::sqlite3_backup_finish) };
    int rc { SQLITE_DONE };

    if (state)
    {
        do
        {
            rc = ::sqlite3_backup_step(state.get(), page_size);

            if constexpr (sleep_time_ms) std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_ms));
        }
        while(rc == SQLITE_OK || rc == SQLITE_BUSY || rc == SQLITE_LOCKED);
    }

    return rc;
}

}

namespace nstd::db
{

namespace sqlite = sqlite;

template<typename TargetObjectType, typename... ConstructorArgs>
struct object_records
{
    using data_container_type = std::vector<TargetObjectType>;
    data_container_type records;

    object_records() = default;
    object_records(std::size_t preallocate) { records.reserve(preallocate); }
    ~object_records() = default;

    void operator()(ConstructorArgs&&... args)
    {
        records.emplace_back(std::forward<ConstructorArgs>(args)...);
    };

    auto begin()
    {
        return std::begin(records);
    }

    auto end()
    {
        return std::end(records);
    }

    auto begin() const
    {
        return std::cbegin(records);
    }

    auto end() const
    {
        return std::cend(records);
    }

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

    void operator()(ColTypes&&... args)
    {
        records.emplace_back(std::forward_as_tuple(std::move(args)...));
    };

    auto begin()
    {
        return std::begin(records);
    }

    auto end()
    {
        return std::end(records);
    }

    auto begin() const
    {
        return std::cbegin(records);
    }

    auto end() const
    {
        return std::cend(records);
    }

    data_container_type &data()
    {
        return records;
    }
};

template<typename ColType>
struct column_records
{
    using data_container_type = std::vector<ColType>;
    data_container_type records;

    column_records() = default;
    column_records(std::size_t preallocate) { records.reserve(preallocate); }
    ~column_records() = default;

    void operator()(ColType arg)
    {
        records.push_back(std::forward<ColType>(arg));
    };

    auto begin()
    {
        return std::begin(records);
    }

    auto end()
    {
        return std::end(records);
    }

    auto begin() const
    {
        return std::cbegin(records);
    }

    auto end() const
    {
        return std::cend(records);
    }

    data_container_type& data()
    {
        return records;
    }
};

}
