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
#include <string>
#include "planar_movements_recognizer.hpp"

int main()
{
    using namespace std::string_literals;
    using namespace nstd::pmr;

    enum commands : uint8_t
    {
        open_file = 100, close_file,
        go_back, go_forward,
        reload
    };

    std::map<uint8_t, std::string> command_map
    {
        {0, "Unknown"s},
        {commands::open_file, "Open file"s},
        {commands::close_file, "Close file"s},
        {commands::go_back, "Go back"s},
        {commands::go_forward, "Go forward"s},
        {commands::reload, "Reload"s}
    };

    planar_movements_event_provider pmep;
    command_recognizer<planar_movements_event_provider::event, commands> cr;
    remove_noise_filter rnf;

    cr. add_command(commands::open_file, { planar_movements_event_provider::up }).
        add_command(commands::close_file, { planar_movements_event_provider::down }).
        add_command(commands::go_back, { planar_movements_event_provider::left }).
        add_command(commands::go_forward, { planar_movements_event_provider::right }).
        add_command(commands::reload, { planar_movements_event_provider::down, planar_movements_event_provider::up })
    ;

    {
        std::vector<planar_movements_event_provider::event> coords { pmep(100., 100.), pmep(150., 105.), pmep(200., 103.), pmep(250., 102.), pmep(300., 95.) }; // moving right

        std::cout << command_map[cr(rnf(std::move(coords)))] << std::endl;
    }

    {
        event_filter<planar_movements_event_provider::event> ef(true);
        ef.set(planar_movements_event_provider::event::right, planar_movements_event_provider::event::left);

         // moving right, but mapping the right event to the left one using event_filter
        std::vector<planar_movements_event_provider::event> coords { ef(pmep(100., 100.)), ef(pmep(150., 105.)), ef(pmep(200., 103.)), ef(pmep(250., 102.)), ef(pmep(300., 95.)) }; // moving right

        std::cout << command_map[cr(rnf(std::move(coords)))] << std::endl;
    }

    {
        std::vector<planar_movements_event_provider::event> coords { pmep(295., 239.), pmep(310., 202.), pmep(300., 150.), pmep(300., 120.), pmep(300., 95.) }; // moving up

        std::cout << command_map[cr(rnf(std::move(coords)))] << std::endl;
    }

    {
        std::vector<planar_movements_event_provider::event> coords { pmep(300., 95.), pmep(300., 120.), pmep(300., 150.), pmep(310., 202.), pmep(295., 239.) }; // moving down

        std::cout << command_map[cr(rnf(std::move(coords)))] << std::endl;
    }

    {
        std::vector<planar_movements_event_provider::event> coords { pmep(300., 95.), pmep(300., 120.), pmep(300., 150.), pmep(310., 202.), pmep(295., 239.),
                                                                     pmep(295., 239.), pmep(310., 202.), pmep(300., 150.), pmep(300., 120.), pmep(300., 95.) }; // moving down & up

        std::cout << command_map[cr(rnf(std::move(coords)))] << std::endl;
    }

    std::cout << "exiting..." << std::endl;

    return 0;
}
