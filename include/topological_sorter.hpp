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

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace nstd
{

template <typename ValueType>
class topological_sorter
{
public:
    using value_type = ValueType;

protected:
    struct relations
    {
        std::size_t dependencies { 0 };
        std::unordered_set<value_type> dependents {};
    };

    std::unordered_map<value_type, relations> _map {};

public:
    void add(const value_type &object)
    {
        _map.try_emplace(object, relations {});
    }

    void add(const value_type &object, const value_type &dependency)
    {
        if (dependency == object) return;

        auto &dependents = _map[dependency].dependents;

        if (dependents.find(object) == dependents.end())
        {
            dependents.insert(object);

            ++_map[object].dependencies;
        }
    }

    template <typename Container>
    void add(const value_type &object, const Container &dependencies)
    {
        for (auto const &dependency : dependencies) add(object, dependency);
    }

    void add(const value_type &object, const std::initializer_list<value_type> &dependencies)
    {
        add<std::initializer_list<value_type>>(object, dependencies);
    }

    template<typename... Args>
    void add(const value_type &object, Args... args)
    {
        (add(object, args), ...);
    }

    auto sort()
    {
        std::vector<value_type> sorted, cycled;
        auto map { _map };

        for(const auto &[object, relations] : map) if (!relations.dependencies) sorted.push_back(object);

        for(decltype(std::size(sorted)) idx = 0; idx < std::size(sorted); ++idx)
            for(auto const& object : map[sorted[idx]].dependents)
                if (!--map[object].dependencies) sorted.push_back(object);

        for(const auto &[object, relations] : map) if(relations.dependencies) cycled.push_back(std::move(object));

        return std::pair(std::move(sorted), std::move(cycled));
    }

    void clear()
    {
        _map.clear();
    }
};

}
