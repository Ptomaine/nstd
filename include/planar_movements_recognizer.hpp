#pragma once

/*
MIT License
Copyright (c) 2008, 2017 Arlen Keshabyan (arlen.albert@gmail.com) and Vlad Marahduhdah
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

#include <algorithm>
#include <complex>
#include <map>
#include <vector>

namespace nstd::pmr
{

class planar_movements_event_provider
{
public:
    using point_type = std::complex<double>;

    planar_movements_event_provider(const planar_movements_event_provider&) = default;
    planar_movements_event_provider &operator = (const planar_movements_event_provider&) = default;
    planar_movements_event_provider(planar_movements_event_provider&&) = default;
    planar_movements_event_provider &operator = (planar_movements_event_provider&&) = default;

private:
    using precision_type = std::pair<int, double>;

    bool _is_empty { true };
    bool _invert_x { false };
    bool _invert_y { false };
    point_type _current_point {0, 0};
    precision_type _precision {};
    static constexpr const double _m_pi_4 { .785398163397448309616 };

public:
    enum event : uint8_t
    {
        unknown = 0,
        right = 1,
        down_right = 2,
        down = 3,
        down_left = 4,
        left = 5,
        up_left = 6,
        up = 7,
        up_right = 8
    };

    planar_movements_event_provider(int min_step = 20, double section_delta = 1.) :
        _precision(precision_type((min_step < 0) ? 0 : min_step, (section_delta < 0 || section_delta > 1) ? .5 : section_delta / 2)) {}

    void set_invertion(bool invert_x, bool invert_y)
    {
        _invert_x = invert_x;
        _invert_y = invert_y;
    }

    auto input(point_type point)
    {
        if(_invert_y || _invert_x)
            point = point_type((_invert_x) ? -point.real() : point.real(), (_invert_y) ? -point.imag() : point.imag());

        if(_is_empty)
        {
            _current_point = point;
            _is_empty = false;

            return event::unknown;
        }

        point_type result { point - _current_point };

        if(std::abs(result) < _precision.first) return event::unknown;

        _current_point = point;

        double section { std::arg(result) / _m_pi_4 };

        if(section < 0) section += 8.;

        int section_index { static_cast<int>(section + .5) };
        double difference { 0 };

        if(section_index == 8)
        {
            section_index = 0;
            difference = 8 - section;
        }
        else
        {
            difference = section - static_cast<double>(section_index);

            if(difference < 0) difference = -difference;
        }

        if(difference > 1) difference -= static_cast<int>(difference);

        if(difference <= _precision.second) return static_cast<event>(section_index + 1);

        return event::unknown;
    }

    template<class T>
    auto input(T x, T y)
    {
        return input(point_type { x, y });
    }

    auto operator()(point_type point)
    {
        return input(point);
    }

    template<class T>
    auto operator()(T x, T y)
    {
        return input(x, y);
    }

    planar_movements_event_provider& clear()
    {
        _is_empty = true;

        return *this;
    }

    planar_movements_event_provider& operator ~()
    {
        return clear();
    }
};

template<typename event_type, typename command_type, typename event_sequence_type = std::vector<event_type>, template<typename, typename> typename map_type = std::map>
class command_recognizer
{
private:
    map_type<event_sequence_type, command_type> _map_to_command {};
    mutable bool _remove_repetitions { true };

    bool discover(event_sequence_type full_sequence, event_sequence_type current_event_sequence, bool strict, bool from_end) const
    {
        if (from_end)
        {
            reverse(current_event_sequence.begin(), current_event_sequence.end());
            reverse(full_sequence.begin(), full_sequence.end());
        }

        auto it = std::search(full_sequence.begin(), full_sequence.end(), current_event_sequence.begin(), current_event_sequence.end());

        if (it == full_sequence.end()) return false;
        else if (strict && it != full_sequence.begin()) return false;

        return true;
    }

public:
    command_recognizer(bool remove_repetitions = true) {}

    command_recognizer& set_remove_repetitions(bool remove_repetitions = true) const
    {
        _remove_repetitions = remove_repetitions;

        return *this;
    }

    command_recognizer& add_command(command_type command, event_sequence_type sequence)
    {
        if(!std::empty(sequence))
        {
            sequence.erase(std::remove_if(std::begin(sequence), std::end(sequence), std::logical_not<event_type>()), std::end(sequence));

            if(_remove_repetitions) sequence.erase(std::unique(std::begin(sequence), std::end(sequence)), std::end(sequence));

            if(!std::empty(sequence)) _map_to_command[sequence] = command;
        }

        return *this;
    }

    command_recognizer& remove_sequence(event_sequence_type sequence)
    {
        if(!std::empty(sequence))
        {
            sequence.erase(std::remove_if(std::begin(sequence), std::end(sequence), std::logical_not<event_type>()), std::end(sequence));

            if(_remove_repetitions) sequence.erase(std::unique(std::begin(sequence), std::end(sequence)), std::end(sequence));

            _map_to_command.remove(sequence);
        }

        return *this;
    }

    command_recognizer& remove_command(command_type command)
    {
        for(auto it { std::begin(_map_to_command) }, end { std::end(_map_to_command) }; it != end; ++it)
            if(it->second == command) _map_to_command.remove(it->first);

        return *this;
    }

    command_type get_command(event_sequence_type current_event_sequence, bool recover = false, bool is_strict = true, bool from_end = false) const
    {
        if(std::empty(current_event_sequence)) return { static_cast<command_type>(0) };

        if(_remove_repetitions)
            current_event_sequence.erase(std::unique(std::begin(current_event_sequence), std::end(current_event_sequence)), std::end(current_event_sequence));

        if(recover)
        {
            current_event_sequence.erase(std::remove_if(std::begin(current_event_sequence), std::end(current_event_sequence), std::logical_not<event_type>()), std::end(current_event_sequence));

            for(auto it = _map_to_command.begin(), end = _map_to_command.end(); it != end; ++it)
                if (discover(it->first, current_event_sequence, is_strict, from_end)) return it->second;
        }
        else
        {
            auto it = _map_to_command.find(current_event_sequence);

            if (it != std::end(_map_to_command)) return it->second;
        }

        return { static_cast<command_type>(0) };
    }

    auto operator()(event_sequence_type current_event_sequence, bool recover = false, bool is_strict = true, bool from_end = false) const
    {
        return get_command(current_event_sequence, recover, is_strict, from_end);
    }

    bool is_command_here(command_type command) const
    {
        for(auto it { std::begin(_map_to_command) }, end(std::end(_map_to_command)); it != end; ++it)
            if(it->second == command) return true;

        return false;
    }

    bool is_sequence_here(event_sequence_type sequence) const
    {
        if(!sequence.empty())
        {
            sequence.erase(std::remove_if(std::begin(sequence), std::end(sequence), std::logical_not<event_type>()), std::end(sequence));

            if(_remove_repetitions)
                sequence.erase(std::unique(std::begin(sequence), std::end(sequence)), std::end(sequence));
        }

        return (_map_to_command.find(sequence) != _map_to_command.end());
    }

    auto &get_data()
    {
        return _map_to_command;
    }
};

class remove_noise_filter
{
public:
    template<typename sequence_type>
    sequence_type filter(sequence_type &&sequence, int min_length_limit = 2) const
    {
        if(sequence.empty()) return {};

        sequence_type result_sequence;
        auto current_value { sequence.front() };
        auto begin { sequence.begin() }, current { begin }, end { sequence.end() };
        int current_sequence_length { 1 };

        for(++current; current != end; ++current)
        {
            if(*current != current_value)
            {
                if(current_sequence_length >= min_length_limit)
                    std::move(begin, current, std::back_inserter(result_sequence));

                begin = current;
                current_value = *current;
                current_sequence_length = 0;
            }

            ++current_sequence_length;
        }

        if(current_sequence_length >= min_length_limit)
            std::move(begin, current, std::back_inserter(result_sequence));

        return result_sequence;
    }

    template<typename event_sequence_type>
    event_sequence_type operator() (event_sequence_type &&sequence, int min_length_limit = 2) const
    {
        return filter(std::forward<event_sequence_type>(sequence), min_length_limit);
    }
};

template<typename event_type, template<typename, typename> typename map_type = std::map>
class event_filter
{
private:
    map_type<event_type, event_type> _event_map {};
    bool _transparent { false };
    event_type _default_event { event_type { 0 } };

public:
    event_filter() = default;
    event_filter(bool transparent, event_type default_event = event_type { 0 }) : _transparent { transparent }, _default_event { default_event } {}

    event_filter& set_transparent(bool transparent)
    {
        _transparent = transparent;

        return *this;
    }

    event_filter& set_default_event(event_type default_event)
    {
        _default_event = default_event;

        return *this;
    }

    void clear()
    {
        _event_map.clear();
    }

    event_filter& set(event_type source_event, event_type dest_event)
    {
        _event_map[source_event] = dest_event;

        return *this;
    }

    event_filter& set(event_type event)
    {
        return set(event, event);
    }

    event_type& operator[](event_type source_event)
    {
        return _event_map[source_event];
    }

    size_t size() const
    {
        return std::size(_event_map);
    }

    event_type filter(event_type source_event) const
    {
        if(std::empty(_event_map)) return source_event;

        auto it { _event_map.find(source_event) };

        if (it != std::end(_event_map)) return it->second;
        else if (_transparent) return source_event;

        return _default_event;
    }

    event_type operator()(event_type source_event) const
    {
        return filter(source_event);
    }

    void remove(event_type source_event)
    {
        _event_map.remove(source_event);
    }
};

}
