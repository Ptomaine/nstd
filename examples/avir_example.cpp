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

#include <chrono>
#include <iostream>
#include <memory>

#include "blend2d.hpp"
#include "avir.hpp"
#include "cmdline_options.hpp"

int main(int argc, char** argv)
{
    nstd::po::parser p;
    auto help_callback { [&p]{ std::cout << p << std::endl; } };

    auto &sc = p["scale"].abbreviation('s').type(nstd::po::f64).description("Scale factor. 0.5 means twice smaller.\n2.5 means 2 and a half times bigger.\n(Default is 0.5)").fallback(0.5);
    auto &help = p["help"].abbreviation('?').description("Prints the help screen");
    auto &files = p[""];

    if (argc == 1 || !p(argc, argv) || std::size(files) == 0) { help_callback(); return -1; }

    for (const auto& filename : files.to_vector<nstd::po::string>())
    {
        std::cout << "Working on: " << filename << std::endl;

        BLImage texture;

        if (BLResult err = texture.readFromFile(filename.c_str()); err)
        {
            std::cout << "Cannot open image file: " << err << std::endl;
            continue;
        }

        BLImageData image_data;

        texture.getData(&image_data);

        using fpclass_dith = nstd::avir::fpclass_def<float, float, nstd::avir::CImageResizerDithererErrdINL<float>>;

        uint8_t *image { reinterpret_cast<uint8_t*>(image_data.pixelData) };
        const int &width { image_data.size.w }, &height { image_data.size.h }, stride { static_cast<int>(image_data.stride) };
        const double scale { sc.get().f64 };
        const int channels { static_cast<int>(blFormatInfo[image_data.format].depth / 8) };
        const int new_width { static_cast<int>(width * scale + .5) }, new_height { static_cast<int>(height * scale + .5) }, new_stride { new_width * channels };
        auto new_image = std::make_unique<uint8_t[]>(new_height * new_stride);

        std::cout << "Scaling: " << scale << std::endl;
        std::cout << "w: " << width << "; h: " << height << "; s: " << stride << std::endl << "w: " << new_width << "; h: " << new_height << "; s: " << new_stride << std::endl;

        nstd::avir_scale_thread_pool scaling_pool;
        nstd::avir::CImageResizerVars vars; vars.ThreadPool = &scaling_pool;
        nstd::avir::CImageResizerParamsUltra roptions;
        nstd::avir::CImageResizer<fpclass_dith> image_resizer { 8, 0, roptions};
        std::cout << "the size of pool: " << scaling_pool.size() << std::endl;
        std::chrono::high_resolution_clock::time_point start_time { std::chrono::high_resolution_clock::now() };

        image_resizer.resizeImage(image, width, height, 0, new_image.get(), new_width, new_height, channels, 0, &vars);

        std::chrono::high_resolution_clock::time_point finish_time { std::chrono::high_resolution_clock::now() };
        std::chrono::milliseconds run_time { std::chrono::duration_cast<std::chrono::milliseconds>(finish_time - start_time) };

        std::cout << "AVIR resizing time: " << (static_cast<double>(run_time.count()) / 1000) << " seconds" << std::endl;

        BLImage new_img;

        if (BLResult err = new_img.createFromData(new_width, new_height, image_data.format, new_image.get(), new_stride); err)
        {
            std::cout << "Cannot store image data..." << err << std::endl;
            continue;
        }

        BLImageCodec codec;

        codec.findByName("BMP");

        if (BLResult err = new_img.writeToFile((filename + "_avir.bmp").c_str(), codec); err)
        {
            continue;
        }

        // Blend2D resize with Blackman
        BLImage blackman;
        BLSizeI size { new_width, new_height };
        BLImageScaleOptions options { .radius = (scale < 1. ? 2. : (scale <= 16. ? scale : 16.)) };

        start_time = std::chrono::high_resolution_clock::now();

        if (BLResult err = BLImage::scale(blackman, texture, size, BL_IMAGE_SCALE_FILTER_BLACKMAN, &options); err)
        {
            continue;
        }

        finish_time = std::chrono::high_resolution_clock::now();
        run_time = std::chrono::duration_cast<std::chrono::milliseconds>(finish_time - start_time);

        std::cout << "B2D (Blackman) resizing time: " << (static_cast<double>(run_time.count()) / 1000) << " seconds" << std::endl;

        if (BLResult err = blackman.writeToFile((filename + "_blackman.bmp").c_str(), codec); err)
        {
            continue;
        }

        std::cout << "-----------------------------------------------" << std::endl;
    }

    return 0;
}
