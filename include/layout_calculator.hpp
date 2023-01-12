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

        std::vector<std::vector<rect>> calculate_layout(rect parent_rect)
        {
            std::vector<std::vector<rect>> child_rects;
            int fixed_pixel_rows_size = 0;
            int y = parent_rect.y;
            float row_height_proportions_sum = 0.f;

            for (auto& row : _rows)
            {
                if (row.second < 0.f)
                {
                    fixed_pixel_rows_size += static_cast<int>(std::abs(row.second));
                }
                else
                {
                    row_height_proportions_sum += row.second;
                }
            }

            int parent_height = fixed_pixel_rows_size < parent_rect.height ? parent_rect.height - fixed_pixel_rows_size : 0;

            if (row_height_proportions_sum != 1.0)
            {
                for (auto& row : _rows)
                {
                    if (row.second >= 0.f)
                    {
                        row.second /= row_height_proportions_sum;
                    }
                }
            }

            child_rects.reserve(std::size(_rows));

            for (auto& row : _rows)
            {
                int fixed_pixel_columns_size = 0;
                float column_width_proportions_sum = 0.f;
                for (auto column_width : row.first)
                {
                    if (column_width < 0.f)
                    {
                        fixed_pixel_columns_size += static_cast<int>(std::abs(column_width));
                    }
                    else
                    {
                        column_width_proportions_sum += column_width;
                    }
                }

                int parent_width = (fixed_pixel_columns_size < parent_rect.width) ? parent_rect.width - fixed_pixel_columns_size : 0;

                if (column_width_proportions_sum != 1.f)
                {
                    for (auto& col : row.first)
                    {
                        if (col >= 0.f)
                        {
                            col /= column_width_proportions_sum;
                        }
                    }
                }

                int row_height = static_cast<int>(row.second < 0.f ? std::abs(row.second) : row.second * parent_height);
                int x = parent_rect.x;
                auto& child_row{ child_rects.emplace_back() };
                child_row.reserve(std::size(row.first));

                for (const auto col : row.first)
                {
                    int column_width = static_cast<int>(col < 0.f ? std::abs(col) : col * parent_width);

                    child_row.emplace_back(x, y, column_width, row_height);
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
