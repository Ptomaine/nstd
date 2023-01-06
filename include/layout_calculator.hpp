#pragma once

/*
MIT License
Copyright (c) 2022 Arlen Keshabyan (arlen.albert@gmail.com)
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
#include <cmath>
#include <vector>

namespace nstd
{
    struct layout_calculator
    {
        struct rect
        {
            int x, y, width, height;
        };

        layout_calculator(const std::vector<std::pair<std::vector<float>, float>>& rows) : _rows(rows) {}

        std::vector<std::vector<rect>> calculate_layout(rect parent_rect, bool fit_width_into_parent_rectangle = false, bool fit_height_into_parent_rectangle = false)
        {
            std::vector<std::vector<rect>> child_rects;
            child_rects.reserve(_rows.size());

            int y = parent_rect.y;
            const int scaling_factor = 10000;
            for (size_t i = 0; i < _rows.size(); ++i)
            {
                const int num_columns = _rows[i].first.size();
                int row_height = std::round(_rows[i].second * parent_rect.height * scaling_factor);
                row_height = row_height / scaling_factor;
                int x = parent_rect.x;
                std::vector<rect> row;
                row.reserve(num_columns);

                if (fit_width_into_parent_rectangle)
                {
                    float column_width_proportions_sum = std::accumulate(_rows[i].first.begin(), _rows[i].first.end(), 0.0f);

                    if (column_width_proportions_sum != 1.0)
                    {
                        std::transform(_rows[i].first.begin(), _rows[i].first.end(), _rows[i].first.begin(), [column_width_proportions_sum](float proportion) { return proportion / column_width_proportions_sum; });
                    }
                }

                if (fit_height_into_parent_rectangle)
                {
                    float row_height_proportions_sum = std::accumulate(_rows.begin(), _rows.end(), 0.0f, [](float sum, const auto& row) { return sum + row.second; });

                    if (row_height_proportions_sum != 1.0)
                    {
                        std::transform(_rows.begin(), _rows.end(), _rows.begin(), [row_height_proportions_sum](auto& row) { row.second /= row_height_proportions_sum; return row; });
                    }
                }

                for (size_t j = 0; j < num_columns; ++j)
                {
                    int column_width = std::round(_rows[i].first[j] * parent_rect.width * scaling_factor);
                    column_width = column_width / scaling_factor;
                    row.emplace_back(rect{ x, y, column_width, row_height });
                    x += column_width;
                }

                child_rects.emplace_back(std::move(row));
                y += row_height;
            }

            return child_rects;
        }

        void clear()
        {
            _rows.clear();
        }

        auto& get_row(size_t row_index)
        {
            return _rows[row_index];
        }

        auto& get_rows()
        {
            return _rows;
        }

        auto get_row_count() const
        {
            return std::size(_rows);
        }

        void add_row(std::pair<std::vector<float>, float> row)
        {
            _rows.push_back(std::move(row));
        }

        auto replace_row(size_t row_index, std::pair<std::vector<float>, float> row)
        {
            std::pair<std::vector<float>, float> result{ std::move(_rows[row_index]) };

            _rows[row_index] = std::move(row);

            return result;
        }

        auto remove_row(size_t row_index)
        {
            std::pair<std::vector<float>, float> result{ std::move(_rows[row_index]) };

            _rows.erase(std::begin(_rows) + row_index);

            return result;
        }

        auto pop_back()
        {
            std::pair<std::vector<float>, float> result{ std::move(_rows.back()) };

            _rows.pop_back();

            return result;
        }

        auto pop_front()
        {
            std::pair<std::vector<float>, float> result{ std::move(_rows.front()) };

            _rows.erase(std::begin(_rows));

            return result;
        }

    private:
        std::vector<std::pair<std::vector<float>, float>> _rows;
    };
}
