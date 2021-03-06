# nstd

This is a C++20 utility library. nstd stands for non-standard.
The library currently contains the following features:
   * signal_slot    - a signal/slot implementation that is thread safe and supports auto-disconnection from slots.
   * relinx - LINQ-like data transformations
   * live_property - a templated wrapper class that emits signals when a value is changing/changed.
   * expiry_cache  - a container where a data can expire.
   * json - Niels Lohmann's json library [https://github.com/nlohmann/json]
   * chaiscript - ChaiScript is an easy to use embedded scripting language for C++ [https://github.com/ChaiScript/ChaiScript]
   * asio - boost's networking library, but no dependancy on boost [http://think-async.com/]
   * urdl - the library to download web content (depends on asio) [https://github.com/chriskohlhoff/urdl]
   * date - the date and time manipulations library created by Howard Hinnant [https://github.com/HowardHinnant/date]
   * units - a compile-time, header-only, dimensional analysis and unit conversion library built on c++14 with no dependencies. [https://github.com/nholthaus/units]
   * base64, crc32 - base64 encoding/decoding, CRC32/constexpr CRC32.
   * string_id - A small C++ library to handle hashed strings serving as identifiers (see notes at the end of this page) [https://github.com/foonathan/string_id]
   * uuid - unique id generator
   * uri - class for URI manipulations.
   * http_request_parser - a simple class to parse incoming http requests.
   * http_resource_manager - header-only HTTP server/router.
   * sharp_tcp - header-only TCP server/client classes.
   * planar_movements_recognizer - a set of classes to track and recognize planar movements and to execute corresponding commands
   * topological_sorter - the class to sort objects with dependencies (dependency solver)
   * sqlite3/sqlite c++ wrapper - sql database support [https://github.com/aminroosta/sqlite_modern_cpp]
   * quantum random number provider (using QRNG internet service: http://qrng.anu.edu.au)
   * strings - a library to do string manipulations like: trim, unicode convertions, replace, join, split, composing (aka format) etc.
   * cmdline_options - a commandline parser [https://github.com/Fytch/ProgramOptions.hxx]
   * giant - r-lyeh's tiny library to handle little/big endianness [https://github.com/r-lyeh/giant]
   * fifo_map - a FIFO-ordered associative container for C++
   * ...and many more

Many of these libraries are sub-modules. So, don't forget to initialize and update sub-modules after cloning this repository. Also, make sure all submodules are switched to their master branch.

For Windows users, you can download MinGW from the following sites:

https://gcc-mcf.lhmouse.com/ (recommended! After extraction from the archive file, to install it, just put its /bin folder into the PATH environment variable)
or
https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/

MinGW/GCC >= 9.1 is now the reqired compiler that supports some features of C++20.
Latest MSVC 2019 and Clang were tested successfully.

GENie can be downloaded from https://github.com/bkaradzic/GENie and genie executable should be placed to the example folder or be in path.

**A simple usage on Windows using GENie and MinGW:**

Go to the example folder and type:
```
> genie gmake
> mingw32-make config=release
```
On Linux do the same but use 'make' instead of 'mingw32-make'


:small_orange_diamond: ***NOTE**: To compile the '**string_id**' source code, you need to install '**cmake**' first and then run it using '**string_id**' folder as a source.
On Ubuntu Linux it might look like this:*
```
> sudo apt-get install cmake
> cd include/external/string_id
> cmake . .
```
