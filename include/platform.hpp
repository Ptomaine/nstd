#pragma once

/*
MIT License
Copyright (c) 2017 Arlen Keshabyan (arlen.albert@gmail.com)
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

namespace nstd::platform
{

inline constexpr const char *const platform =
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
	#define OS_FAMILY_UNIX
	#define OS_FAMILY_BSD
	#define OS_FREE_BSD
	"FreeBSD"
#elif defined(_AIX) || defined(__TOS_AIX__)
	#define OS_FAMILY_UNIX
	#define OS_AIX
	"AIX"
#elif defined(hpux) || defined(_hpux) || defined(__hpux)
	#define OS_FAMILY_UNIX
	#define OS_HPUX
	"HPUX"
#elif defined(__digital__) || defined(__osf__)
	#define OS_FAMILY_UNIX
	#define OS_TRU64
	"Tru64"
#elif defined(__NACL__)
	#define OS_FAMILY_UNIX
	#define OS_NACL
	"NaCl"
#elif defined(__EMSCRIPTEN__) || defined(EMSCRIPTEN)
	#define OS_FAMILY_UNIX
	#define OS_EMSCRIPTEN
	"Emscripten"
#elif defined(linux) || defined(__linux) || defined(__linux__) || defined(__TOS_LINUX__)
	#define OS_FAMILY_UNIX
	#define OS_LINUX
	"Linux"
#elif defined(__APPLE__) || defined(__TOS_MACOS__)
	#define OS_FAMILY_UNIX
	#define OS_FAMILY_BSD
	#define OS_MAC_OS_X
	"macOS X"
#elif defined(__NetBSD__)
	#define OS_FAMILY_UNIX
	#define OS_FAMILY_BSD
	#define OS_NET_BSD
	"NetBSD"
#elif defined(__OpenBSD__)
	#define OS_FAMILY_UNIX
	#define OS_FAMILY_BSD
	#define OS_OPEN_BSD
	"OpenBSD"
#elif defined(sgi) || defined(__sgi)
	#define OS_FAMILY_UNIX
	#define OS_IRIX
	"IRIX"
#elif defined(sun) || defined(__sun)
	#define OS_FAMILY_UNIX
	#define OS_SOLARIS
	"Solaris"
#elif defined(__QNX__)
	#define OS_FAMILY_UNIX
	#define OS_QNX
	"QNX"
#elif defined(__CYGWIN__)
	#define OS_FAMILY_UNIX
	#define OS_CYGWIN
	"Cygwin"
#elif defined(VXWORKS)
	#define OS_FAMILY_UNIX
	#define OS_VXWORKS
	"VxWorks"
#elif defined(unix) || defined(__unix) || defined(__unix__)
	#define OS_FAMILY_UNIX
	#define OS_UNKNOWN_UNIX
	"Unix"
#elif defined(_WIN32_WCE)
	#define OS_FAMILY_WINDOWS
	#define OS_WINDOWS_CE
	"Windows CE"
#elif defined(_WIN32) || defined(_WIN64)
	#define OS_FAMILY_WINDOWS
	#define OS_WINDOWS_NT
	"Windows NT"
#elif defined(__VMS)
	#define OS_FAMILY_VMS
	#define OS_VMS
	"VMS"
#else
	#error "Unknown Platform!"
#endif
;

#if defined(__ALPHA) || defined(__alpha) || defined(__alpha__) || defined(_M_ALPHA)
	#define ARCH_ALPHA
	#define ARCH_LITTLE_ENDIAN
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86) || defined(EMSCRIPTEN) || defined(__EMSCRIPTEN__)
	#define ARCH_IA32
	#define ARCH_LITTLE_ENDIAN
#elif defined(_IA64) || defined(__IA64__) || defined(__ia64__) || defined(__ia64) || defined(_M_IA64)
	#define ARCH_IA64
	#if defined(hpux) || defined(_hpux)
		#define ARCH_BIG_ENDIAN
	#else
		#define ARCH_LITTLE_ENDIAN
	#endif
#elif defined(__x86_64__) || defined(_M_X64)
	#define ARCH_AMD64
	#define ARCH_LITTLE_ENDIAN
#elif defined(__mips__) || defined(__mips) || defined(__MIPS__) || defined(_M_MRX000)
	#define ARCH_MIPS
	#if defined(OS_FAMILY_WINDOWS)
		#define ARCH_LITTLE_ENDIAN
	#elif defined(__MIPSEB__) || defined(_MIPSEB) || defined(__MIPSEB)
		#define ARCH_BIG_ENDIAN
	#elif defined(__MIPSEL__) || defined(_MIPSEL) || defined(__MIPSEL)
		#define ARCH_LITTLE_ENDIAN
	#else
		#error "Unknown MIPS!"
	#endif
#elif defined(__hppa) || defined(__hppa__)
	#define ARCH_HPPA
	#define ARCH_BIG_ENDIAN
#elif defined(__PPC) || defined(__POWERPC__) || defined(__powerpc) || defined(__PPC__) || \
      defined(__powerpc__) || defined(__ppc__) || defined(__ppc) || defined(_ARCH_PPC) || defined(_M_PPC)
	#define ARCH_PPC
	#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
		#define ARCH_LITTLE_ENDIAN
	#else
		#define ARCH_BIG_ENDIAN
	#endif
#elif defined(_POWER) || defined(_ARCH_PWR) || defined(_ARCH_PWR2) || defined(_ARCH_PWR3) || \
      defined(_ARCH_PWR4) || defined(__THW_RS6000)
	#define ARCH_POWER
	#define ARCH_BIG_ENDIAN
#elif defined(__sparc__) || defined(__sparc) || defined(sparc)
	#define ARCH_SPARC
	#define ARCH_BIG_ENDIAN
#elif defined(__arm__) || defined(__arm) || defined(ARM) || defined(_ARM_) || defined(__ARM__) || defined(_M_ARM)
	#define ARCH_ARM
	#if defined(__ARMEB__)
		#define ARCH_BIG_ENDIAN
	#else
		#define ARCH_LITTLE_ENDIAN
	#endif
#elif defined(__arm64__) || defined(__arm64)
	#define ARCH_ARM64
	#if defined(__ARMEB__)
		#define ARCH_BIG_ENDIAN
	#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		#define ARCH_BIG_ENDIAN
	#else
		#define ARCH_LITTLE_ENDIAN
	#endif
#elif defined(__m68k__)
	#define ARCH_M68K
	#define ARCH_BIG_ENDIAN
#elif defined(__s390__)
	#define ARCH_S390
	#define ARCH_BIG_ENDIAN
#elif defined(__sh__) || defined(__sh) || defined(SHx) || defined(_SHX_)
	#define ARCH_SH
	#if defined(__LITTLE_ENDIAN__) || (OS == OS_WINDOWS_CE)
		#define ARCH_LITTLE_ENDIAN
	#else
		#define ARCH_BIG_ENDIAN
	#endif
#elif defined (nios2) || defined(__nios2) || defined(__nios2__)
	#define ARCH_NIOS2
	#if defined(__nios2_little_endian) || defined(nios2_little_endian) || defined(__nios2_little_endian__)
		#define ARCH_LITTLE_ENDIAN
	#else
		#define ARCH_BIG_ENDIAN
	#endif
#elif defined(__AARCH64EL__)
	#define ARCH_AARCH64
	#define ARCH_LITTLE_ENDIAN
#elif defined(__AARCH64EB__)
	#define ARCH_AARCH64
	#define ARCH_BIG_ENDIAN
#endif

inline constexpr const char *const compiler =
#if defined(_MSC_VER)
	#define COMPILER_MSVC
	"MSVC"
#elif defined(__clang__)
	#define COMPILER_CLANG
	"Clang"
#elif defined (__MINGW32__) || defined (__MINGW64__)
	#define COMPILER_MINGW
	"MinGW"
#elif defined (__GNUC__)
	#define COMPILER_GCC
	"GCC"
#elif defined (__INTEL_COMPILER) || defined(__ICC) || defined(__ECC) || defined(__ICL)
	#define COMPILER_INTEL
	"Intel"
#elif defined (__SUNPRO_CC)
	#define COMPILER_SUN
	"Sun"
#elif defined (__MWERKS__) || defined(__CWCC__)
	#define COMPILER_CODEWARRIOR
	"CodeWarrior"
#elif defined (__sgi) || defined(sgi)
	#define COMPILER_SGI
	"SGI"
#elif defined (__HP_aCC)
	#define COMPILER_HP_ACC
	"HP aCC"
#elif defined (__BORLANDC__) || defined(__CODEGEARC__)
	#define COMPILER_CBUILDER
	"Borland"
#elif defined (__DMC__)
	#define COMPILER_DMARS
	"Digital Mars"
#elif defined (__DECCXX)
	#define COMPILER_COMPAC
	"Compac"
#elif (defined (__xlc__) || defined (__xlC__)) && defined(__IBMCPP__)
	#define COMPILER_IBM_XLC
	"IBM XL"
#elif defined (__IBMCPP__) && defined(__COMPILER_VER__)
	#define COMPILER_IBM_XLC_ZOS
	"IBM z/OS"
#else
	#error "Unknown Hardware Architecture!"
#endif
;

inline constexpr bool is_64bit = sizeof(void*) == sizeof(long long);

inline constexpr bool is_little_endian =
#if defined(ARCH_LITTLE_ENDIAN)
true
#else
false
#endif
;

inline uint32_t change_endianness(uint32_t value)
{
    uint32_t result = 0;
    result |= (value & 0x000000FF) << 24;
    result |= (value & 0x0000FF00) << 8;
    result |= (value & 0x00FF0000) >> 8;
    result |= (value & 0xFF000000) >> 24;
    return result;
}

}
