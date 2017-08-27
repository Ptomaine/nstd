#if defined (AMIGA) || defined (__amigaos__)
	#include "AmigaOS/agg_platform_support.cpp"
#elif defined (__BEOS__)
	#include "BeOS/agg_platform_support.cpp"
#elif defined (__APPLE__) && defined (__MACH__)
	#include "mac/agg_platform_support.cpp"
	#include "mac/agg_mac_pmap.cpp"
#elif defined (SDL_VERSION)
	#include "sdl/agg_platform_support.cpp"
#elif defined (_WIN32)
	#include "win32/agg_platform_support.cpp"
	#include "win32/agg_win32_bmp.cpp"
#elif defined (__linux__)
	#include "X11/agg_platform_support.cpp"
#endif

