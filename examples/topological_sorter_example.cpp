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

#include "topological_sorter.hpp"
#include <iostream>
#include <memory>

int main()
{
    using namespace std::string_literals;

    struct task
    {
        std::string message;

        task(const std::string &v) : message { v } {}
        ~task() { std::cout << message[0] << " - destroyed" << std::endl; }
    };

    using task_ptr = std::shared_ptr<task>;

    std::vector<task_ptr> tasks
    {
        // defining simple tasks
        std::make_shared<task>("A - depends on B and C"s),    //0
        std::make_shared<task>("B - depends on none"s),       //1
        std::make_shared<task>("C - depends on D and E"s),    //2
        std::make_shared<task>("D - depends on none"s),       //3
        std::make_shared<task>("E - depends on F, G and H"s), //4
        std::make_shared<task>("F - depends on I"s),          //5
        std::make_shared<task>("G - depends on none"s),       //6
        std::make_shared<task>("H - depends on none"s),       //7
        std::make_shared<task>("I - depends on none"s),       //8
    };

    nstd::topological_sorter<task_ptr> resolver;

    // now setting relations between them as described above
    resolver.add(tasks[0], { tasks[1], tasks[2] });
    //resolver.add(tasks[1]); // no need for this since the task was already mentioned as a dependency
    resolver.add(tasks[2], { tasks[3], tasks[4] });
    //resolver.add(tasks[3]); // no need for this since the task was already mentioned as a dependency
    resolver.add(tasks[4], tasks[5], tasks[6], tasks[7]); // using templated add with fold expression
    resolver.add(tasks[5], tasks[8]);
    //resolver.add(tasks[6]); // no need for this since the task was already mentioned as a dependency
    //resolver.add(tasks[7]); // no need for this since the task was already mentioned as a dependency

    //resolver.add(tasks[3], tasks[0]); // uncomment this line to test cycled dependency

    const auto &[sorted, cycled] = resolver.sort();

    if (std::empty(cycled))
    {
        for (auto const& d: sorted)
            std::cout << d->message << std::endl;
    }
    else
    {
        std::cout << "Cycled dependencies detected: ";

        for (auto const& d: cycled)
            std::cout << d->message[0] << " ";

        std::cout << std::endl;
    }

    //tasks.clear(); // uncomment this line to distroy all tasks in sorted order.

    std::cout << "exiting..." << std::endl;

    return 0;
}
