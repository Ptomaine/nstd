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

enum class os_type : uint8_t
{
    Unknown = 0,
    FreeBSD,
    AIX,
    HPUX,
    Tru64,
    NaCl,
    Emscripten,
    Linux,
    macOS,
    NetBSD,
    OpenBSD,
    IRIX,
    Solaris,
    QNX,
    Cygwin,
    VxWorks,
    Unix,
    WindowsCE,
    WindowsNT,
    VMS
};

enum class os_family : uint8_t
{
    Unknown = 0,
    Unix,
    UnixBSD,
    Windows,
    VMS
};

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
	#define OS_FAMILY_UNIX
	#define OS_FAMILY_BSD
	#define OS_FREE_BSD
	inline constexpr const os_type current_os_type = os_type::FreeBSD;
	inline constexpr const os_family current_os_family = os_family::UnixBSD;
#elif defined(_AIX) || defined(__TOS_AIX__)
	#define OS_FAMILY_UNIX
	#define OS_AIX
	inline constexpr const os_type current_os_type = os_type::AIX;
	inline constexpr const os_family current_os_family = os_family::Unix;
#elif defined(hpux) || defined(_hpux) || defined(__hpux)
	#define OS_FAMILY_UNIX
	#define OS_HPUX
	inline constexpr const os_type current_os_type = os_type::HPUX;
	inline constexpr const os_family current_os_family = os_family::Unix;
#elif defined(__digital__) || defined(__osf__)
	#define OS_FAMILY_UNIX
	#define OS_TRU64
	inline constexpr const os_type current_os_type = os_type::Tru64;
	inline constexpr const os_family current_os_family = os_family::Unix;
#elif defined(__NACL__)
	#define OS_FAMILY_UNIX
	#define OS_NACL
	inline constexpr const os_type current_os_type = os_type::NaCl;
	inline constexpr const os_family current_os_family = os_family::Unix;
#elif defined(__EMSCRIPTEN__) || defined(EMSCRIPTEN)
	#define OS_FAMILY_UNIX
	#define OS_EMSCRIPTEN
	inline constexpr const os_type current_os_type = os_type::Emscripten;
	inline constexpr const os_family current_os_family = os_family::Unix;
#elif defined(linux) || defined(__linux) || defined(__linux__) || defined(__TOS_LINUX__)
	#define OS_FAMILY_UNIX
	#define OS_LINUX
	inline constexpr const os_type current_os_type = os_type::Linux;
	inline constexpr const os_family current_os_family = os_family::Unix;
#elif defined(__APPLE__) || defined(__TOS_MACOS__)
	#define OS_FAMILY_UNIX
	#define OS_FAMILY_BSD
	#define OS_MAC_OS_X
	inline constexpr const os_type current_os_type = os_type::macOS;
	inline constexpr const os_family current_os_family = os_family::UnixBSD;
#elif defined(__NetBSD__)
	#define OS_FAMILY_UNIX
	#define OS_FAMILY_BSD
	#define OS_NET_BSD
	inline constexpr const os_type current_os_type = os_type::NetBSD;
	inline constexpr const os_family current_os_family = os_family::UnixBSD;
#elif defined(__OpenBSD__)
	#define OS_FAMILY_UNIX
	#define OS_FAMILY_BSD
	#define OS_OPEN_BSD
	inline constexpr const os_type current_os_type = os_type::OpenBSD;
	inline constexpr const os_family current_os_family = os_family::UnixBSD;
#elif defined(sgi) || defined(__sgi)
	#define OS_FAMILY_UNIX
	#define OS_IRIX
	inline constexpr const os_type current_os_type = os_type::IRIX;
	inline constexpr const os_family current_os_family = os_family::Unix;
#elif defined(sun) || defined(__sun)
	#define OS_FAMILY_UNIX
	#define OS_SOLARIS
	inline constexpr const os_type current_os_type = os_type::Solaris;
	inline constexpr const os_family current_os_family = os_family::Unix;
#elif defined(__QNX__)
	#define OS_FAMILY_UNIX
	#define OS_QNX
	inline constexpr const os_type current_os_type = os_type::QNX;
	inline constexpr const os_family current_os_family = os_family::Unix;
#elif defined(__CYGWIN__)
	#define OS_FAMILY_UNIX
	#define OS_CYGWIN
	inline constexpr const os_type current_os_type = os_type::Cygwin;
	inline constexpr const os_family current_os_family = os_family::Unix;
#elif defined(VXWORKS)
	#define OS_FAMILY_UNIX
	#define OS_VXWORKS
	inline constexpr const os_type current_os_type = os_type::VxWorks;
	inline constexpr const os_family current_os_family = os_family::Unix;
#elif defined(unix) || defined(__unix) || defined(__unix__)
	#define OS_FAMILY_UNIX
	#define OS_UNKNOWN_UNIX
	inline constexpr const os_type current_os_type = os_type::Unix;
	inline constexpr const os_family current_os_family = os_family::Unix;
#elif defined(_WIN32_WCE)
	#define OS_FAMILY_WINDOWS
	#define OS_WINDOWS_CE
	inline constexpr const os_type current_os_type = os_type::WindowsCE
	inline constexpr const os_family current_os_family = os_family::Windows;
#elif defined(_WIN32) || defined(_WIN64)
	#define OS_FAMILY_WINDOWS
	#define OS_WINDOWS_NT
	inline constexpr const os_type current_os_type = os_type::WindowsNT;
	inline constexpr const os_family current_os_family = os_family::Windows;
#elif defined(__VMS)
	#define OS_FAMILY_VMS
	#define OS_VMS
	inline constexpr const os_type current_os_type = os_type::VMS;
	inline constexpr const os_family current_os_family = os_family::VMS;
#else
	#error "Unknown Platform!"
#endif

#if defined(__ALPHA) || defined(__alpha) || defined(__alpha__) || defined(_M_ALPHA)
	#define ARCH_ALPHA
    #if __cplusplus <= 201703L
        #define ARCH_LITTLE_ENDIAN
    #endif
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86) || defined(EMSCRIPTEN) || defined(__EMSCRIPTEN__)
	#define ARCH_IA32
	#if __cplusplus <= 201703L
        #define ARCH_LITTLE_ENDIAN
    #endif
#elif defined(_IA64) || defined(__IA64__) || defined(__ia64__) || defined(__ia64) || defined(_M_IA64)
	#define ARCH_IA64
	#if __cplusplus <= 201703L
        #if defined(hpux) || defined(_hpux)
            #define ARCH_BIG_ENDIAN
        #else
            #define ARCH_LITTLE_ENDIAN
        #endif
    #endif
#elif defined(__x86_64__) || defined(_M_X64)
	#define ARCH_AMD64
	#if __cplusplus <= 201703L
        #define ARCH_LITTLE_ENDIAN
    #endif
#elif defined(__mips__) || defined(__mips) || defined(__MIPS__) || defined(_M_MRX000)
	#define ARCH_MIPS
	#if __cplusplus <= 201703L
        #if defined(OS_FAMILY_WINDOWS)
            #define ARCH_LITTLE_ENDIAN
        #elif defined(__MIPSEB__) || defined(_MIPSEB) || defined(__MIPSEB)
            #define ARCH_BIG_ENDIAN
        #elif defined(__MIPSEL__) || defined(_MIPSEL) || defined(__MIPSEL)
            #define ARCH_LITTLE_ENDIAN
        #else
            #error "Unknown MIPS!"
        #endif
    #endif
#elif defined(__hppa) || defined(__hppa__)
	#define ARCH_HPPA
	#if __cplusplus <= 201703L
        #define ARCH_BIG_ENDIAN
    #endif
#elif defined(__PPC) || defined(__POWERPC__) || defined(__powerpc) || defined(__PPC__) || \
      defined(__powerpc__) || defined(__ppc__) || defined(__ppc) || defined(_ARCH_PPC) || defined(_M_PPC)
	#define ARCH_PPC
	#if __cplusplus <= 201703L
        #if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
            #define ARCH_LITTLE_ENDIAN
        #else
            #define ARCH_BIG_ENDIAN
        #endif
    #endif
#elif defined(_POWER) || defined(_ARCH_PWR) || defined(_ARCH_PWR2) || defined(_ARCH_PWR3) || \
      defined(_ARCH_PWR4) || defined(__THW_RS6000)
	#define ARCH_POWER
	#if __cplusplus <= 201703L
        #define ARCH_BIG_ENDIAN
    #endif
#elif defined(__sparc__) || defined(__sparc) || defined(sparc)
	#define ARCH_SPARC
	#if __cplusplus <= 201703L
        #define ARCH_BIG_ENDIAN
    #endif
#elif defined(__arm__) || defined(__arm) || defined(ARM) || defined(_ARM_) || defined(__ARM__) || defined(_M_ARM)
	#define ARCH_ARM
	#if __cplusplus <= 201703L
        #if defined(__ARMEB__)
            #define ARCH_BIG_ENDIAN
        #else
            #define ARCH_LITTLE_ENDIAN
        #endif
    #endif
#elif defined(__arm64__) || defined(__arm64)
	#define ARCH_ARM64
	#if __cplusplus <= 201703L
        #if defined(__ARMEB__)
            #define ARCH_BIG_ENDIAN
        #elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            #define ARCH_BIG_ENDIAN
        #else
            #define ARCH_LITTLE_ENDIAN
        #endif
    #endif
#elif defined(__m68k__)
	#define ARCH_M68K
	#if __cplusplus <= 201703L
        #define ARCH_BIG_ENDIAN
    #endif
#elif defined(__s390__)
	#define ARCH_S390
	#if __cplusplus <= 201703L
        #define ARCH_BIG_ENDIAN
    #endif
#elif defined(__sh__) || defined(__sh) || defined(SHx) || defined(_SHX_)
	#define ARCH_SH
	#if __cplusplus <= 201703L
        #if defined(__LITTLE_ENDIAN__) || (OS == OS_WINDOWS_CE)
            #define ARCH_LITTLE_ENDIAN
        #else
            #define ARCH_BIG_ENDIAN
        #endif
    #endif
#elif defined (nios2) || defined(__nios2) || defined(__nios2__)
	#define ARCH_NIOS2
	#if __cplusplus <= 201703L
        #if defined(__nios2_little_endian) || defined(nios2_little_endian) || defined(__nios2_little_endian__)
            #define ARCH_LITTLE_ENDIAN
        #else
            #define ARCH_BIG_ENDIAN
        #endif
    #endif
#elif defined(__AARCH64EL__)
	#define ARCH_AARCH64
	#if __cplusplus <= 201703L
        #define ARCH_LITTLE_ENDIAN
    #endif
#elif defined(__AARCH64EB__)
	#define ARCH_AARCH64
	#if __cplusplus <= 201703L
        #define ARCH_BIG_ENDIAN
    #endif
#endif

//Fallback
#if !defined(ARCH_LITTLE_ENDIAN) && !defined(ARCH_BIG_ENDIAN)
    #ifdef __GNUC__
        #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            #define ARCH_LITTLE_ENDIAN
        #else
            #define ARCH_BIG_ENDIAN
        #endif
    #else
        #define ARCH_LITTLE_ENDIAN
    #endif
#endif

enum class compiler : uint8_t
{
    Unknown = 0,
    MSVC,
    Clang,
    MinGW,
    GCC,
    Intel,
    Sun,
    CodeWarrior,
    SGI,
    HPaCC,
    Borland,
    DigitalMars,
    Compac,
    IBMXL,
    IBMzOS
};

inline constexpr const compiler current_compiler =
#if defined(_MSC_VER)
	#define COMPILER_MSVC
	compiler::MSVC
#elif defined (__MINGW32__) || defined (__MINGW64__)
	#define COMPILER_MINGW
	compiler::MinGW
#elif defined(__clang__)
	#define COMPILER_CLANG
	compiler::Clang
#elif defined (__GNUC__)
	#define COMPILER_GCC
	compiler::GCC
#elif defined (__INTEL_COMPILER) || defined(__ICC) || defined(__ECC) || defined(__ICL)
	#define COMPILER_INTEL
	compiler::Intel
#elif defined (__SUNPRO_CC)
	#define COMPILER_SUN
	compiler::Sun
#elif defined (__MWERKS__) || defined(__CWCC__)
	#define COMPILER_CODEWARRIOR
	compiler::CodeWarrior
#elif defined (__sgi) || defined(sgi)
	#define COMPILER_SGI
	compiler::SGI
#elif defined (__HP_aCC)
	#define COMPILER_HP_ACC
	compiler::HPaCC
#elif defined (__BORLANDC__) || defined(__CODEGEARC__)
	#define COMPILER_CBUILDER
	compiler::Borland
#elif defined (__DMC__)
	#define COMPILER_DMARS
	compiler::DigitalMars
#elif defined (__DECCXX)
	#define COMPILER_COMPAC
	compiler::Compac
#elif (defined (__xlc__) || defined (__xlC__)) && defined(__IBMCPP__)
	#define COMPILER_IBM_XLC
	compiler::IBMXL
#elif defined (__IBMCPP__) && defined(__COMPILER_VER__)
	#define COMPILER_IBM_XLC_ZOS
	compiler::IBMzOS
#else
	#error "Unknown Hardware Architecture!"
#endif
;

inline constexpr bool is_64bit = sizeof(void*) == sizeof(long long);

#if __cplusplus > 201703L
    #include <type_traits>
    inline constexpr bool is_little_endian = (std::endian::native == std::endian::little);
#else
    inline constexpr bool is_little_endian =
    #if defined(ARCH_LITTLE_ENDIAN)
    true
    #else
    false
    #endif
    ;
#endif

inline constexpr const char *get_current_os_type_name()
{
    if constexpr (current_os_type == os_type::FreeBSD)          return "FreeBSD";
    else if constexpr (current_os_type == os_type::AIX)         return "AIX";
    else if constexpr (current_os_type == os_type::HPUX)        return "HPUX";
    else if constexpr (current_os_type == os_type::Tru64)       return "Tru64";
    else if constexpr (current_os_type == os_type::NaCl)        return "NaCl";
    else if constexpr (current_os_type == os_type::Emscripten)  return "Emscripten";
    else if constexpr (current_os_type == os_type::Linux)       return "Linux";
    else if constexpr (current_os_type == os_type::macOS)       return "macOS";
    else if constexpr (current_os_type == os_type::NetBSD)      return "NetBSD";
    else if constexpr (current_os_type == os_type::OpenBSD)     return "OpenBSD";
    else if constexpr (current_os_type == os_type::IRIX)        return "IRIX";
    else if constexpr (current_os_type == os_type::Solaris)     return "Solaris";
    else if constexpr (current_os_type == os_type::QNX)         return "QNX";
    else if constexpr (current_os_type == os_type::Cygwin)      return "Cygwin";
    else if constexpr (current_os_type == os_type::VxWorks)     return "VxWorks";
    else if constexpr (current_os_type == os_type::Unix)        return "Unix";
    else if constexpr (current_os_type == os_type::WindowsCE)   return "Windows CE";
    else if constexpr (current_os_type == os_type::WindowsNT)   return "Windows NT";
    else if constexpr (current_os_type == os_type::VMS)         return "VMS";
    else                                                        return "Unknown";
}

inline constexpr const char *get_current_os_family_name()
{
    if constexpr (current_os_family == os_family::Unix)         return "Unix";
    else if constexpr (current_os_family == os_family::UnixBSD) return "Unix,BSD";
    else if constexpr (current_os_family == os_family::Windows) return "Windows";
    else if constexpr (current_os_family == os_family::VMS)     return "VMS";
    else                                                        return "Unknown";
}

inline constexpr const char *get_current_compiler_name()
{
    if constexpr (current_compiler == compiler::MSVC)               return "MSVC";
    else if constexpr (current_compiler == compiler::Clang)         return "Clang";
    else if constexpr (current_compiler == compiler::MinGW)         return "MinGW";
    else if constexpr (current_compiler == compiler::GCC)           return "GCC";
    else if constexpr (current_compiler == compiler::Intel)         return "Intel";
    else if constexpr (current_compiler == compiler::Sun)           return "Sun";
    else if constexpr (current_compiler == compiler::CodeWarrior)   return "Code Warrior";
    else if constexpr (current_compiler == compiler::SGI)           return "SGI";
    else if constexpr (current_compiler == compiler::HPaCC)         return "HP aCC";
    else if constexpr (current_compiler == compiler::Borland)       return "Borland";
    else if constexpr (current_compiler == compiler::DigitalMars)   return "Digital Mars";
    else if constexpr (current_compiler == compiler::Compac)        return "Compac";
    else if constexpr (current_compiler == compiler::IBMXL)         return "IBM XL";
    else if constexpr (current_compiler == compiler::IBMzOS)        return "IBM z/OS";
    else                                                            return "Unknown";
}

inline uint32_t change_endianness(uint32_t value)
{
    uint32_t result = 0;
    result |= (value & 0x000000FF) << 24;
    result |= (value & 0x0000FF00) << 8;
    result |= (value & 0x00FF0000) >> 8;
    result |= (value & 0xFF000000) >> 24;
    return result;
}

enum endianness_type : uint8_t
{
    ENDIAN_UNKNOWN,
    ENDIAN_BIG,
    ENDIAN_LITTLE,
    ENDIAN_BIG_WORD,   /* Middle-endian, Honeywell 316 style */
    ENDIAN_LITTLE_WORD /* Middle-endian, PDP-11 style */
};

inline endianness_type endianness()
{
    union
    {
        uint32_t value;
        uint8_t data[sizeof(uint32_t)];
    } number;

    number.data[0] = 0x00;
    number.data[1] = 0x01;
    number.data[2] = 0x02;
    number.data[3] = 0x03;

    switch (number.value)
    {
    case UINT32_C(0x00010203): return ENDIAN_BIG;
    case UINT32_C(0x03020100): return ENDIAN_LITTLE;
    case UINT32_C(0x02030001): return ENDIAN_BIG_WORD;
    case UINT32_C(0x01000302): return ENDIAN_LITTLE_WORD;
    default:                   return ENDIAN_UNKNOWN;
    }
}

}
