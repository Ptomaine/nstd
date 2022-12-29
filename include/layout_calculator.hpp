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
#include <tuple>
#include <vector>

struct layout_calculator
{
    struct rect
    {
        int x, y, width, height;
    };

    layout_calculator(const std::vector<std::vector<float>>& row_column_width_proportions, const std::vector<float>& row_height_proportions)
        : _row_column_width_proportions(row_column_width_proportions), _row_height_proportions(row_height_proportions) {}

    std::vector<std::vector<rect>> calculate_layout(const rect& parent_rect) const
    {
        std::vector<std::vector<rect>> child_rects;
        child_rects.reserve(_row_column_width_proportions.size());

        int y = parent_rect.y;
        const int scaling_factor = 10000;
        
        for (size_t i = 0; i < _row_column_width_proportions.size(); ++i)
        {
            const size_t num_columns = _row_column_width_proportions[i].size();
            const int row_height = static_cast<int>(std::round(_row_height_proportions[i] * parent_rect.height * scaling_factor));
            const int row_height_scaled = row_height / scaling_factor;
            int x = parent_rect.x;
            std::vector<rect> row;
            
            row.reserve(num_columns);
            
            for (size_t j = 0; j < num_columns; ++j)
            {
                const float column_width_proportion = _row_column_width_proportions[i][j];
                const int column_width = static_cast<int>(std::round(column_width_proportion * parent_rect.width * scaling_factor));
                const int column_width_scaled = column_width / scaling_factor;
                
                row.emplace_back(rect{ x, y, column_width_scaled, row_height_scaled });
                
                x += column_width_scaled;
            }

            child_rects.emplace_back(std::move(row));
            y += row_height_scaled;
        }

        return child_rects;
    }

private:
    std::vector<std::vector<float>> _row_column_width_proportions;
    std::vector<float> _row_height_proportions;
};
