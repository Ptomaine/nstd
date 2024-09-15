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
#include <stdexcept>
#include <algorithm>

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
        // Constructors
        Column(double size, SizeType sizeType)
            : size_(size), sizeType_(sizeType) {}

        // Getters
        double getSize() const { return size_; }
        SizeType getSizeType() const { return sizeType_; }

        // Setters
        void setSize(double size) { size_ = size; }
        void setSizeType(SizeType sizeType) { sizeType_ = sizeType; }

    private:
        double size_;
        SizeType sizeType_;
    };

    class Row
    {
    public:
        // Constructors
        Row(double size, SizeType sizeType)
            : size_(size), sizeType_(sizeType) {}

        Row(double size, SizeType sizeType, std::initializer_list<Column> cols)
            : size_(size), sizeType_(sizeType), columns_(cols) {}

        // Getters
        double getSize() const { return size_; }
        SizeType getSizeType() const { return sizeType_; }
        const std::vector<Column>& getColumns() const { return columns_; }
        std::vector<Column>& getColumns() { return columns_; }

        // Setters
        void setSize(double size) { size_ = size; }
        void setSizeType(SizeType sizeType) { sizeType_ = sizeType; }

        // Column Manipulation Methods
        void addColumn(const Column& column)
        {
            columns_.push_back(column);
        }

        void insertColumn(size_t index, const Column& column)
        {
            if (index <= columns_.size())
            {
                columns_.insert(columns_.begin() + index, column);
            }
        }

        void removeColumn(size_t index)
        {
            if (index < columns_.size())
            {
                columns_.erase(columns_.begin() + index);
            }
        }

        void swapColumns(size_t index1, size_t index2)
        {
            if (index1 < columns_.size() && index2 < columns_.size())
            {
                std::swap(columns_[index1], columns_[index2]);
            }
        }

        void clearColumns()
        {
            columns_.clear();
        }

        Column& getColumn(size_t index)
        {
            if (index >= columns_.size())
            {
                throw std::out_of_range("Column index out of range");
            }

            return columns_[index];
        }

    private:
        double size_;
        SizeType sizeType_;
        std::vector<Column> columns_;
    };

    class Grid
    {
    public:
        // Constructors
        Grid() = default;

        Grid(std::initializer_list<Row> rws)
            : rows_(rws) {}

        // Getters
        const std::vector<Row>& getRows() const { return rows_; }
        std::vector<Row>& getRows() { return rows_; }

        // Row Manipulation Methods
        void addRow(const Row& row)
        {
            rows_.push_back(row);
        }

        void insertRow(size_t index, const Row& row)
        {
            if (index <= rows_.size())
            {
                rows_.insert(rows_.begin() + index, row);
            }
        }

        void removeRow(size_t index)
        {
            if (index < rows_.size())
            {
                rows_.erase(rows_.begin() + index);
            }
        }

        void swapRows(size_t index1, size_t index2)
        {
            if (index1 < rows_.size() && index2 < rows_.size())
            {
                std::swap(rows_[index1], rows_[index2]);
            }
        }

        void clearRows()
        {
            rows_.clear();
        }

        Row& getRow(size_t index)
        {
            if (index >= rows_.size())
            {
                throw std::out_of_range("Row index out of range");
            }
            return rows_[index];
        }

        // Clear the entire grid
        void clear()
        {
            rows_.clear();
        }

        // Layout Calculation
        std::vector<std::vector<Cell>> calculateLayout(int x, int y, int width, int height) const
        {
            std::vector<std::vector<Cell>> layout;
            layout.reserve(rows_.size());

            // Calculate total fixed and proportional sizes for rows
            double totalFixedHeight = 0.0;
            double totalProportionalHeight = 0.0;

            for (const auto& row : rows_)
            {
                if (row.getSizeType() == SizeType::Fixed)
                {
                    totalFixedHeight += row.getSize();
                }
                else
                {
                    totalProportionalHeight += row.getSize();
                }
            }

            double remainingHeight = height - totalFixedHeight;
            if (remainingHeight < 0.0) remainingHeight = 0.0;

            int currentY = y;

            // Calculate heights for each row
            for (const auto& row : rows_)
            {
                double rowHeight = 0.0;
                if (row.getSizeType() == SizeType::Fixed)
                {
                    rowHeight = row.getSize();
                }
                else if (totalProportionalHeight > 0.0)
                {
                    rowHeight = (row.getSize() / totalProportionalHeight) * remainingHeight;
                }

                // Calculate total fixed and proportional sizes for columns
                const auto& columns = row.getColumns();
                double totalFixedWidth = 0.0;
                double totalProportionalWidth = 0.0;

                for (const auto& column : columns)
                {
                    if (column.getSizeType() == SizeType::Fixed)
                    {
                        totalFixedWidth += column.getSize();
                    }
                    else
                    {
                        totalProportionalWidth += column.getSize();
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
                    if (column.getSizeType() == SizeType::Fixed)
                    {
                        columnWidth = column.getSize();
                    }
                    else if (totalProportionalWidth > 0.0)
                    {
                        columnWidth = (column.getSize() / totalProportionalWidth) * remainingWidth;
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

    private:
        std::vector<Row> rows_;
    };
}
