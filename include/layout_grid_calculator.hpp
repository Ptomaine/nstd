#pragma once

/*
MIT License
Copyright (c) 2024 Arlen Keshabyan (arlen.albert@gmail.com)
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

namespace nstd
{

    enum class SizeType
    {
        Fixed,
        Proportional
    };

    struct Cell
    {
        int x, y, width, height;
    };

    class Column
    {
    public:
        double size;
        SizeType sizeType;

        Column(double size, SizeType sizeType)
            : size(size), sizeType(sizeType) {}
    };

    class Row
    {
    public:
        double size;
        SizeType sizeType;
        std::vector<Column> columns;

        Row(double size, SizeType sizeType, std::initializer_list<Column> cols)
            : size(size), sizeType(sizeType), columns(cols) {}
    };

    class Grid
    {
    public:
        std::vector<Row> rows;

        Grid(std::initializer_list<Row> rws)
            : rows(rws) {}

        std::vector<std::vector<Cell>> calculateLayout(int x, int y, int width, int height) const
        {
            std::vector<std::vector<Cell>> layout;
            layout.reserve(rows.size());

            // Calculate total fixed and proportional sizes for rows
            double totalFixedHeight = 0.0;
            double totalProportionalHeight = 0.0;

            for (const auto& row : rows)
            {
                if (row.sizeType == SizeType::Fixed)
                {
                    totalFixedHeight += row.size;
                }
                else
                {
                    totalProportionalHeight += row.size;
                }
            }

            double remainingHeight = height - totalFixedHeight;
            if (remainingHeight < 0.0) remainingHeight = 0.0;

            int currentY = y;

            // Calculate heights for each row
            for (const auto& row : rows)
            {
                double rowHeight = 0.0;
                if (row.sizeType == SizeType::Fixed)
                {
                    rowHeight = row.size;
                }
                else if (totalProportionalHeight > 0.0)
                {
                    rowHeight = (row.size / totalProportionalHeight) * remainingHeight;
                }

                // Calculate total fixed and proportional sizes for columns
                const auto& columns = row.columns;
                double totalFixedWidth = 0.0;
                double totalProportionalWidth = 0.0;

                for (const auto& column : columns)
                {
                    if (column.sizeType == SizeType::Fixed)
                    {
                        totalFixedWidth += column.size;
                    }
                    else
                    {
                        totalProportionalWidth += column.size;
                    }
                }

                double remainingWidth = width - totalFixedWidth;
                if (remainingWidth < 0.0) remainingWidth = 0.0;

                int currentX = x;
                std::vector<Cell> rowCells;

                // Calculate widths for each column
                for (const auto& column : columns)
                {
                    double columnWidth = 0.0;
                    if (column.sizeType == SizeType::Fixed)
                    {
                        columnWidth = column.size;
                    }
                    else if (totalProportionalWidth > 0.0)
                    {
                        columnWidth = (column.size / totalProportionalWidth) * remainingWidth;
                    }

                    // Create the cell
                    Cell cell =
                    {
                        currentX,
                        currentY,
                        static_cast<int>(columnWidth + 0.5), // Rounding
                        static_cast<int>(rowHeight + 0.5)    // Rounding
                    };

                    rowCells.push_back(cell);
                    currentX += cell.width;
                }

                layout.push_back(std::move(rowCells));
                currentY += static_cast<int>(rowHeight + 0.5);
            }

            return layout;
        }
    };
}
