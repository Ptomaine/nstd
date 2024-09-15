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
        using row_height_type = double;
        using column_width_type = double;
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
            const size_t num_rows = _rows.size();
            child_rects.reserve(num_rows);

            int fixed_pixel_rows_height = 0;
            double row_height_proportions_sum = 0.0;

            // Preallocate vectors for fixed and proportional rows
            std::vector<int> fixed_row_heights;
            std::vector<double> proportional_row_heights;
            fixed_row_heights.reserve(num_rows);
            proportional_row_heights.reserve(num_rows);

            // First pass: calculate fixed row heights and sum proportions
            for (const auto& row : _rows)
            {
                if (row.second < 0.0)
                {
                    int height = static_cast<int>(-row.second + 0.5); // Round to nearest integer
                    fixed_row_heights.push_back(height);
                    fixed_pixel_rows_height += height;
                }
                else
                {
                    proportional_row_heights.push_back(row.second);
                    row_height_proportions_sum += row.second;
                }
            }

            int remaining_height = parent_rect.height - fixed_pixel_rows_height;
            if (remaining_height < 0)
                remaining_height = 0;

            // Precompute reciprocal of row proportions sum
            double row_proportions_reciprocal = (row_height_proportions_sum > 0.0) ? (1.0 / row_height_proportions_sum) : 0.0;

            // Normalize proportional row heights
            for (double& proportion : proportional_row_heights)
            {
                proportion *= row_proportions_reciprocal;
            }

            size_t row_index = 0;
            size_t proportional_row_index = 0;
            int y = parent_rect.y;

            for (const auto& row : _rows)
            {
                // Determine row height
                int row_height = 0;
                bool is_fixed_row = (row.second < 0.0);
                if (is_fixed_row)
                {
                    row_height = fixed_row_heights[row_index++];
                }
                else
                {
                    double proportion = proportional_row_heights[proportional_row_index++];
                    row_height = static_cast<int>(proportion * remaining_height + 0.5); // Round to nearest integer
                }

                // Column calculations
                const size_t num_columns = row.first.size();
                std::vector<rect> child_row;
                child_row.reserve(num_columns);

                int fixed_pixel_columns_width = 0;
                double column_width_proportions_sum = 0.0;

                std::vector<int> fixed_column_widths;
                std::vector<double> proportional_column_widths;
                fixed_column_widths.reserve(num_columns);
                proportional_column_widths.reserve(num_columns);

                // First pass: calculate fixed column widths and sum proportions
                for (const auto& col_width : row.first)
                {
                    if (col_width < 0.0)
                    {
                        int width = static_cast<int>(-col_width + 0.5); // Round to nearest integer
                        fixed_column_widths.push_back(width);
                        fixed_pixel_columns_width += width;
                    }
                    else
                    {
                        proportional_column_widths.push_back(col_width);
                        column_width_proportions_sum += col_width;
                    }
                }

                int remaining_width = parent_rect.width - fixed_pixel_columns_width;
                if (remaining_width < 0)
                    remaining_width = 0;

                // Precompute reciprocal of column proportions sum
                double col_proportions_reciprocal = (column_width_proportions_sum > 0.0) ? (1.0 / column_width_proportions_sum) : 0.0;

                // Normalize proportional column widths
                for (double& proportion : proportional_column_widths)
                {
                    proportion *= col_proportions_reciprocal;
                }

                size_t col_index = 0;
                size_t proportional_col_index = 0;
                int x = parent_rect.x;

                for (const auto& col_width : row.first)
                {
                    // Determine column width
                    int column_width = 0;
                    bool is_fixed_column = (col_width < 0.0);
                    if (is_fixed_column)
                    {
                        column_width = fixed_column_widths[col_index++];
                    }
                    else
                    {
                        double proportion = proportional_column_widths[proportional_col_index++];
                        column_width = static_cast<int>(proportion * remaining_width + 0.5); // Round to nearest integer
                    }

                    child_row.emplace_back(rect{ x, y, column_width, row_height });
                    x += column_width;
                }

                child_rects.emplace_back(std::move(child_row));
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
