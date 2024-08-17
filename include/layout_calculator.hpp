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

#include <vector>
#include <utility>
#include <cmath>

namespace nstd
{
    struct layout_calculator
    {
        using row_height_type = float;
        using column_width_type = float;
        using row_type = std::pair<std::vector<column_width_type>, row_height_type>;

        struct rect
        {
            int x, y, width, height;
        };

        explicit layout_calculator(std::vector<row_type> rows)
            : _rows(std::move(rows))
        {
        }

        std::vector<std::vector<rect>> calculate_layout(const rect& parent_rect)
        {
            std::vector<std::vector<rect>> child_rects;
            child_rects.reserve(_rows.size());

            int fixed_pixel_rows_height = 0;
            int remaining_height = parent_rect.height;
            int y = parent_rect.y;
            row_height_type row_height_proportions_sum = 0.f;

            for (const auto& row : _rows)
            {
                if (row.second < 0.f)
                    fixed_pixel_rows_height += static_cast<int>(std::abs(row.second));
                else
                    row_height_proportions_sum += row.second;
            }

            remaining_height -= fixed_pixel_rows_height;

            if (row_height_proportions_sum != 1.0f && row_height_proportions_sum > 0.f)
            {
                for (auto& row : _rows)
                {
                    if (row.second >= 0.f)
                        row.second /= row_height_proportions_sum;
                }
            }

            for (auto& row : _rows)
            {
                int fixed_pixel_columns_width = 0;
                int remaining_width = parent_rect.width;
                column_width_type column_width_proportions_sum = 0.f;

                for (const auto& column_width : row.first)
                {
                    if (column_width < 0.f)
                        fixed_pixel_columns_width += static_cast<int>(std::abs(column_width));
                    else
                        column_width_proportions_sum += column_width;
                }

                remaining_width -= fixed_pixel_columns_width;

                if (column_width_proportions_sum != 1.0f && column_width_proportions_sum > 0.f)
                {
                    for (auto& col : row.first)
                    {
                        if (col >= 0.f)
                            col /= column_width_proportions_sum;
                    }
                }

                int row_height = static_cast<int>((row.second < 0.f) ? std::abs(row.second) : row.second * remaining_height);
                int x = parent_rect.x;
                auto& child_row = child_rects.emplace_back();
                child_row.reserve(row.first.size());

                for (const auto& col : row.first)
                {
                    int column_width = static_cast<int>((col < 0.f) ? std::abs(col) : col * remaining_width);
                    child_row.emplace_back(rect{ x, y, column_width, row_height });
                    x += column_width;
                }

                y += row_height;
            }

            return child_rects;
        }

        void clear()
        {
            _rows.clear();
        }

        row_type& get_row(size_t row_index)
        {
            return _rows.at(row_index);
        }

        const std::vector<row_type>& get_rows() const
        {
            return _rows;
        }

        size_t get_row_count() const
        {
            return _rows.size();
        }

        void add_row(row_type row)
        {
            _rows.push_back(std::move(row));
        }

        row_type replace_row(size_t row_index, row_type row)
        {
            row_type result = std::move(_rows.at(row_index));
            _rows.at(row_index) = std::move(row);
            return result;
        }

        row_type remove_row(size_t row_index)
        {
            row_type result = std::move(_rows.at(row_index));
            _rows.erase(_rows.begin() + row_index);
            return result;
        }

        row_type pop_back()
        {
            row_type result = std::move(_rows.back());
            _rows.pop_back();
            return result;
        }

        row_type pop_front()
        {
            row_type result = std::move(_rows.front());
            _rows.erase(_rows.begin());
            return result;
        }

    private:
        std::vector<row_type> _rows;
    };
}
