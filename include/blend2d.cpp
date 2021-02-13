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

#define BL_STATIC

#ifndef _WIN32_WINNT
         #define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif

#include "external/asmjit/src/asmjit/core/archtraits.cpp"
#include "external/asmjit/src/asmjit/core/assembler.cpp"
#include "external/asmjit/src/asmjit/core/builder.cpp"
#include "external/asmjit/src/asmjit/core/codeholder.cpp"
#include "external/asmjit/src/asmjit/core/codewriter.cpp"
#include "external/asmjit/src/asmjit/core/compiler.cpp"
#include "external/asmjit/src/asmjit/core/constpool.cpp"
#include "external/asmjit/src/asmjit/core/cpuinfo.cpp"
#include "external/asmjit/src/asmjit/core/emithelper.cpp"
#include "external/asmjit/src/asmjit/core/emitter.cpp"
#include "external/asmjit/src/asmjit/core/emitterutils.cpp"
#include "external/asmjit/src/asmjit/core/environment.cpp"
#include "external/asmjit/src/asmjit/core/errorhandler.cpp"
#include "external/asmjit/src/asmjit/core/formatter.cpp"
#include "external/asmjit/src/asmjit/core/func.cpp"
#include "external/asmjit/src/asmjit/core/funcargscontext.cpp"
#include "external/asmjit/src/asmjit/core/globals.cpp"
#include "external/asmjit/src/asmjit/core/inst.cpp"
#include "external/asmjit/src/asmjit/core/jitallocator.cpp"
#include "external/asmjit/src/asmjit/core/jitruntime.cpp"
#include "external/asmjit/src/asmjit/core/logger.cpp"
#include "external/asmjit/src/asmjit/core/operand.cpp"
#include "external/asmjit/src/asmjit/core/osutils.cpp"
#include "external/asmjit/src/asmjit/core/ralocal.cpp"
#include "external/asmjit/src/asmjit/core/rapass.cpp"
#include "external/asmjit/src/asmjit/core/rastack.cpp"
#include "external/asmjit/src/asmjit/core/string.cpp"
#include "external/asmjit/src/asmjit/core/support.cpp"
#include "external/asmjit/src/asmjit/core/target.cpp"
#include "external/asmjit/src/asmjit/core/type.cpp"
#include "external/asmjit/src/asmjit/core/virtmem.cpp"
#include "external/asmjit/src/asmjit/core/zone.cpp"
#include "external/asmjit/src/asmjit/core/zonehash.cpp"
#include "external/asmjit/src/asmjit/core/zonelist.cpp"
#include "external/asmjit/src/asmjit/core/zonestack.cpp"
#include "external/asmjit/src/asmjit/core/zonetree.cpp"
#include "external/asmjit/src/asmjit/core/zonevector.cpp"

#include "external/asmjit/src/asmjit/x86/x86assembler.cpp"
#include "external/asmjit/src/asmjit/x86/x86builder.cpp"
#include "external/asmjit/src/asmjit/x86/x86compiler.cpp"
#include "external/asmjit/src/asmjit/x86/x86emithelper.cpp"
#include "external/asmjit/src/asmjit/x86/x86features.cpp"
#include "external/asmjit/src/asmjit/x86/x86formatter.cpp"
#include "external/asmjit/src/asmjit/x86/x86func.cpp"
#include "external/asmjit/src/asmjit/x86/x86instapi.cpp"
#include "external/asmjit/src/asmjit/x86/x86instdb.cpp"
#include "external/asmjit/src/asmjit/x86/x86operand.cpp"
#include "external/asmjit/src/asmjit/x86/x86rapass.cpp"

#include "external/blend2d/src/blend2d/api-nocxx.cpp"
#include "external/blend2d/src/blend2d/array.cpp"
#include "external/blend2d/src/blend2d/arrayops.cpp"
#include "external/blend2d/src/blend2d/bitarray.cpp"
#include "external/blend2d/src/blend2d/compop.cpp"
#include "external/blend2d/src/blend2d/context.cpp"
#include "external/blend2d/src/blend2d/filesystem.cpp"
#include "external/blend2d/src/blend2d/font.cpp"
#include "external/blend2d/src/blend2d/fontmanager.cpp"
#include "external/blend2d/src/blend2d/format.cpp"
#include "external/blend2d/src/blend2d/geometry.cpp"
#include "external/blend2d/src/blend2d/glyphbuffer.cpp"
#include "external/blend2d/src/blend2d/gradient.cpp"
#include "external/blend2d/src/blend2d/gradient_avx2.cpp"
#include "external/blend2d/src/blend2d/gradient_sse2.cpp"
#include "external/blend2d/src/blend2d/image.cpp"
#include "external/blend2d/src/blend2d/imagecodec.cpp"
#include "external/blend2d/src/blend2d/imagescale.cpp"
#include "external/blend2d/src/blend2d/math.cpp"
#include "external/blend2d/src/blend2d/matrix.cpp"
#include "external/blend2d/src/blend2d/matrix_avx.cpp"
#include "external/blend2d/src/blend2d/matrix_sse2.cpp"
#include "external/blend2d/src/blend2d/path.cpp"
#include "external/blend2d/src/blend2d/pathstroke.cpp"
#include "external/blend2d/src/blend2d/pattern.cpp"
#include "external/blend2d/src/blend2d/pipedefs.cpp"
#include "external/blend2d/src/blend2d/piperuntime.cpp"
#include "external/blend2d/src/blend2d/pixelconverter.cpp"
#include "external/blend2d/src/blend2d/pixelconverter_avx2.cpp"
#include "external/blend2d/src/blend2d/pixelconverter_sse2.cpp"
#include "external/blend2d/src/blend2d/pixelconverter_ssse3.cpp"
#include "external/blend2d/src/blend2d/pixelops.cpp"
#include "external/blend2d/src/blend2d/random.cpp"
#include "external/blend2d/src/blend2d/region.cpp"
#include "external/blend2d/src/blend2d/rgba.cpp"
#include "external/blend2d/src/blend2d/runtime.cpp"
#include "external/blend2d/src/blend2d/scopedallocator.cpp"
#include "external/blend2d/src/blend2d/string.cpp"
#include "external/blend2d/src/blend2d/style.cpp"
#include "external/blend2d/src/blend2d/support.cpp"
#include "external/blend2d/src/blend2d/tables.cpp"
#include "external/blend2d/src/blend2d/trace.cpp"
#include "external/blend2d/src/blend2d/unicode.cpp"
#include "external/blend2d/src/blend2d/variant.cpp"
#include "external/blend2d/src/blend2d/zeroallocator.cpp"
#include "external/blend2d/src/blend2d/zoneallocator.cpp"
#include "external/blend2d/src/blend2d/zonehash.cpp"
#include "external/blend2d/src/blend2d/zonelist.cpp"
#include "external/blend2d/src/blend2d/zonetree.cpp"

#include "external/blend2d/src/blend2d/codec/bmpcodec.cpp"
#include "external/blend2d/src/blend2d/codec/deflate.cpp"
#include "external/blend2d/src/blend2d/codec/jpegcodec.cpp"
#include "external/blend2d/src/blend2d/codec/jpeghuffman.cpp"
#include "external/blend2d/src/blend2d/codec/jpegops.cpp"
#include "external/blend2d/src/blend2d/codec/jpegops_sse2.cpp"
#include "external/blend2d/src/blend2d/codec/pngcodec.cpp"
#include "external/blend2d/src/blend2d/codec/pngops.cpp"
#include "external/blend2d/src/blend2d/codec/pngops_sse2.cpp"

#include "external/blend2d/src/blend2d/fixedpipe/fixedpiperuntime.cpp"

#include "external/blend2d/src/blend2d/opentype/otcff.cpp"
#include "external/blend2d/src/blend2d/opentype/otcmap.cpp"
#include "external/blend2d/src/blend2d/opentype/otcore.cpp"
#include "external/blend2d/src/blend2d/opentype/otface.cpp"
#include "external/blend2d/src/blend2d/opentype/otglyf.cpp"
#include "external/blend2d/src/blend2d/opentype/otkern.cpp"
#include "external/blend2d/src/blend2d/opentype/otlayout.cpp"
#include "external/blend2d/src/blend2d/opentype/otmetrics.cpp"
#include "external/blend2d/src/blend2d/opentype/otname.cpp"

#include "external/blend2d/src/blend2d/pipegen/compoppart.cpp"
#include "external/blend2d/src/blend2d/pipegen/fetchgradientpart.cpp"
#include "external/blend2d/src/blend2d/pipegen/fetchpart.cpp"
#include "external/blend2d/src/blend2d/pipegen/fetchpatternpart.cpp"
#include "external/blend2d/src/blend2d/pipegen/fetchpixelptrpart.cpp"
#include "external/blend2d/src/blend2d/pipegen/fetchsolidpart.cpp"
#include "external/blend2d/src/blend2d/pipegen/fetchutils.cpp"
#include "external/blend2d/src/blend2d/pipegen/fillpart.cpp"
#include "external/blend2d/src/blend2d/pipegen/pipecompiler.cpp"
#include "external/blend2d/src/blend2d/pipegen/pipegencore.cpp"
#include "external/blend2d/src/blend2d/pipegen/pipegenruntime.cpp"
#include "external/blend2d/src/blend2d/pipegen/pipepart.cpp"

#include "external/blend2d/src/blend2d/raster/analyticrasterizer.cpp"
#include "external/blend2d/src/blend2d/raster/rastercontext.cpp"
#include "external/blend2d/src/blend2d/raster/rastercontextops.cpp"
#include "external/blend2d/src/blend2d/raster/rasterfetchdata.cpp"
#include "external/blend2d/src/blend2d/raster/rasterworkdata.cpp"
#include "external/blend2d/src/blend2d/raster/rasterworkermanager.cpp"
#include "external/blend2d/src/blend2d/raster/rasterworkproc.cpp"
#include "external/blend2d/src/blend2d/raster/rasterworksynchronization.cpp"

#include "external/blend2d/src/blend2d/threading/thread.cpp"
#include "external/blend2d/src/blend2d/threading/threadpool.cpp"
#include "external/blend2d/src/blend2d/threading/uniqueidgenerator.cpp"
