/*
MIT License
Copyright (c) 2019 Arlen Keshabyan (arlen.albert@gmail.com)
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
#include "blend2d.hpp"

int main()
{
    auto width { 2560 }, height { 1440 };

    BLImage img { width, height, BL_FORMAT_PRGB32 };
    BLContext ctx { img };

    BLGradient linear { BLLinearGradientValues { 0, 0, 0, double(width) } };

    linear.addStop(0.0, BLRgba32(0xFFFFFFFF));
    linear.addStop(0.5, BLRgba32(0xFF5FAFDF));
    linear.addStop(1.0, BLRgba32(0xFF2F5FDF));

    ctx.setFillStyle(linear);
    ctx.setCompOp(BL_COMP_OP_SRC_OVER);
    ctx.fillRect(0.0, 0.0, float(width), float(height));

    BLFontFace face;

    if (auto err = face.createFromFile("NotoSans-Regular.ttf"); err)
    {
        std::cout << "Failed to load a font-face (err=" << err << ")" << std::endl;

        return 1;
    }

    BLFont font;

    font.createFromFace(face, 72.0f);

    BLFontMetrics fm { font.metrics() };
    BLTextMetrics tm;
    BLGlyphBuffer gb;

    BLPoint p { 20, 390 + fm.ascent };
    const char* text = "Hello Blend2D!\n"
                       "I'm a simple multiline text example\n"
                       "that uses BLGlyphBuffer and fillGlyphRun!\n\n"
                       "Sanskrit: ﻿काचं शक्नोम्यत्तुम् । नोपहिनस्ति माम् ॥\n"
                       "Russian: Я могу есть стекло, оно мне не вредит.\n"
                       "Polska / Polish: Mogę jeść szkło i mi nie szkodzi."
                       ;
    while(true)
    {
        const char* end = strchr(text, '\n');

        gb.setUtf8Text(text, end ? (size_t)(end - text) : SIZE_MAX);
        font.shape(gb);
        font.getTextMetrics(gb, tm);

        p.x = (float(width) - (tm.boundingBox.x1 - tm.boundingBox.x0)) / 2.0;

        BLPoint sp { p };
        sp.x += 1;
        sp.y += 1;

        ctx.setFillStyle(BLRgba32(0xFF333333));
        ctx.fillGlyphRun(sp, font, gb.glyphRun());

        ctx.setFillStyle(BLRgba32(0xFFFFFFFF));
        ctx.fillGlyphRun(p, font, gb.glyphRun());

        p.y += fm.ascent + fm.descent + fm.lineGap;

        if (!end) break;

        text = end + 1;
    }

    ctx.end();

    BLImageCodec codec;

    codec.findByName("BMP");
    img.writeToFile("blend2d_example_output.bmp", codec);

    return 0;
}
