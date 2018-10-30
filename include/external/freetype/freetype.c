/***************************************************************************/
/*                                                                         */
/*  FreeTypeAmalgam.c                                                      */
/*                                                                         */
/*  Copyright 2003-2007, 2011 by                                           */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/

#define FT2_BUILD_LIBRARY

#if defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif

#ifdef _MSC_VER
#pragma push_macro("_CRT_SECURE_NO_WARNINGS")
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

/*#define FT_CONFIG_OPTION_PIC*/

/*
 * These files appear inside conditional compilation directives
 *
 */

#include "freetype2/src/autofit/aftypes.h"
#include "freetype2/src/autofit/afdummy.h"
#include "freetype2/src/autofit/aflatin.h"
#include "freetype2/src/autofit/afcjk.h"
#include "freetype2/src/autofit/afindic.h"
#include "freetype2/include/freetype/internal/cfftypes.h"
#include "freetype2/src/cff/cffparse.h"
#include "freetype2/src/sfnt/ttcmap.h"
#include "freetype2/src/sfnt/ttbdf.h"

#include FT_INTERNAL_RFORK_H

#include FT_TRUETYPE_TABLES_H

#include FT_SERVICE_GLYPH_DICT_H
#include FT_SERVICE_POSTSCRIPT_INFO_H
#include FT_SERVICE_POSTSCRIPT_NAME_H
#include FT_SERVICE_TT_CMAP_H
#include FT_SERVICE_CID_H
#include FT_SERVICE_TRUETYPE_GLYF_H
#include FT_SERVICE_PROPERTIES_H

/*
 * Sources
 *
 */

#include "freetype2/src/autofit/autofit.c"
#include "freetype2/src/bdf/bdf.c"

#include "freetype2/src/cff/cff.c"

#include "freetype2/src/base/ftbase.c"
#undef FT_COMPONENT
#include "freetype2/src/base/ftbitmap.c"
#undef FT_COMPONENT
#include "freetype2/src/cache/ftcache.c"
#undef FT_COMPONENT
#include "freetype2/src/base/ftdebug.c"
#include "freetype2/src/base/ftfstype.c"
#include "freetype2/src/base/ftgasp.c"
#include "freetype2/src/base/ftglyph.c"
#include "freetype2/src/base/ftinit.c"
#include "freetype2/src/base/ftstroke.c"
#include "freetype2/src/base/ftsystem.c"
#include "freetype2/src/smooth/smooth.c"
#include "freetype2/src/base/ftlcdfil.c"
#include "freetype2/src/base/ftfntfmt.c"
/*
 * Modules
 *
 */

#include "freetype2/src/base/ftbbox.c"
#include "freetype2/src/base/ftmm.c"
#include "freetype2/src/base/ftpfr.c"
#include "freetype2/src/base/ftsynth.c"
#include "freetype2/src/base/fttype1.c"
#include "freetype2/src/base/ftwinfnt.c"
#include "freetype2/src/pcf/pcf.c"
#include "freetype2/src/pfr/pfr.c"
#include "freetype2/src/psaux/psaux.c"
#include "freetype2/src/pshinter/pshinter.c"
#include "freetype2/src/psnames/psmodule.c"
#include "freetype2/src/raster/raster.c"
#include "freetype2/src/sfnt/sfnt.c"
#include "freetype2/src/truetype/truetype.c"
#include "freetype2/src/type1/type1.c"
#include "freetype2/src/cid/type1cid.c"
#include "freetype2/src/type42/type42.c"
#include "freetype2/src/winfonts/winfnt.c"

/*
 * GZip
 *
 */

#ifdef FT_CONFIG_OPTION_SYSTEM_ZLIB
#undef FT_CONFIG_OPTION_SYSTEM_ZLIB
#endif
#undef NO_DUMMY_DECL
#define NO_DUMMY_DECL
#undef USE_ZLIB_ZCALLOC
#undef MY_ZCALLOC
#define MY_ZCALLOC /* prevent all zcalloc() & zfree() in zutils.c */
#include "freetype2/src/gzip/zlib.h"
#undef NO_DUMMY_DECL
#undef MY_ZCALLOC

#include "freetype2/src/gzip/ftgzip.c"

/*
 * LZW
 *
 */

#include "freetype2/src/lzw/ftlzw.c"

#ifdef _MSC_VER
#pragma pop_macro("_CRT_SECURE_NO_WARNINGS")
#endif
