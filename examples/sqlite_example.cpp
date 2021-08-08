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
#include <optional>
#include <string>
#include <tuple>
#include "random_provider_default.hpp"
#include "relinx.hpp"
#include "relinx_generator_uuid.hpp"
#include "sqlite.hpp"
#include "json.hpp"

int main()
{
    nstd::db::sqlite::database db{ ":memory:" };

    db << "create table example(id int primary key, name text, password text)";

    {
        nstd::db::sqlite::scoped_transaction tr(db, true);

        nstd::relinx::range(1, 100)->for_each([&db](auto &&idx)
        {
            auto name { std::to_string(nstd::random_provider_default<>()()) };
            auto password { std::to_string(nstd::random_provider_default<>()()) };

            db << "insert into example(id, name, password) values (?, ?, ?)" << idx << name << password;
        });
    }

    using example_rows = nstd::db::tuple_records<int, std::string, std::string>;

    example_rows records { 50 };

    db << "select id, name, password from example where id between 30 and 80" >> records;

    nstd::relinx::from(std::data(records))->for_each([](auto &&row)
    {
        auto &[id, name, password] = row;

        std::cout << "id: " << id << ";\tname: " << name << ";\tpassword: " << password << std::endl;
    });

    db << "select name, password from example order by name desc limit 20" >> [](std::string name, std::string password)
    {
        std::cout << "name: " << name << ";\tpassword: " << password << std::endl;
    };

    {
        struct Rec
        {
            int id;
            std::string name;
            std::string pass;

            Rec(int _id, const std::string &_name, const std::string &_pass) : id {_id}, name {_name}, pass {_pass} {}
            Rec() = default;
        };
        nstd::db::object_records<Rec, int, std::string, std::string> orecs;
        db << "select id, name, password from example where id between 30 and 80 order by id desc" >> orecs;
        nstd::relinx::from(std::data(orecs))->for_each([] (auto &&obj)
        {
            std::cout << "id: " << obj.id << ";\tname: " << obj.name << ";\tpassword: " << obj.pass << std::endl;
        });
    }

    db << "drop table example";
    db << "create table example(id text primary key, name text, password text, json_data text)";

    {
        nstd::db::sqlite::scoped_transaction tr(db, true);

        nstd::from_uuid()->take(100)->for_each([&db](auto &&uuid)
        {
            auto uuid_str { uuid.to_string() };
            auto name { std::to_string(nstd::random_provider_default<>()()) };
            auto password { std::to_string(nstd::random_provider_default<>()()) };
            nstd::json::json json_data;
            std::optional<std::string> json_data_str;

            if (nstd::random_provider_default<>()() & 1)
            {
                json_data["id"] = uuid_str;
                json_data["name"] = name;
                json_data["password"] = password;

                json_data_str = json_data.dump();
            }

            db << "insert into example(id, name, password, json_data) values (?, ?, ?, ?)" << uuid_str << name << password << json_data_str;
        });
    }

    db << "select json_data from example order by name desc limit 20" >> [](std::optional<std::string> json_data)
    {
        std::cout << "json_data: " << json_data.value_or("null") << std::endl;
    };

    auto js_o { R"({"3":null, "1":"the middle one...", "2":null})"_json };

    std::cout << js_o << std::endl;

    auto iff { js_o.value("8", "***") };
    std::cout << iff << std::endl;

    nstd::db::sqlite::backup_database(db, nstd::db::sqlite::database { "sqlite_example_backup.db" }); //from memory to file

    nstd::db::sqlite::database mem_db { ":memory:" };
    nstd::db::sqlite::backup_database(nstd::db::sqlite::database { "sqlite_example_backup.db" }, mem_db); //from file to memory

    std::cout << "restored db..." << std::endl;
    mem_db << "select name, password from example order by name desc limit 5" >> [](std::string name, std::string password)
    {
        std::cout << "name: " << name << ";\tpassword: " << password << std::endl;
    };

    std::cout << "exiting..." << std::endl;

    return 0;
}
