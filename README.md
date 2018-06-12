# nstd

This is a C++17 utility library. nstd stands for non-standard.
The library currently contains the following features:
   * signal_slot    - a signal/slot implementation that is thread safe and supports auto-disconnection from slots.
   * live_property - a templated wrapper class that emits signals when a value is changing/changed.
   * expiry_cache  - a container where a data can expire.
   * json - Niels Lohmann's json library [https://github.com/nlohmann/json]
   * chaiscript - ChaiScript is an easy to use embedded scripting language for C++ [https://github.com/ChaiScript/ChaiScript]
   * asio - boost's networking library, but no dependancy on boost [http://think-async.com/]
   * urdl - the library to download web content (depends on asio) [https://github.com/chriskohlhoff/urdl]
   * date - the date and time manipulations library created by Howard Hinnant [https://github.com/HowardHinnant/date]
   * units - r-lyeh's library to provide numerical quantities with units [https://github.com/r-lyeh/units]
   * base64, crc32 - base64 encoding/decoding, CRC32/constexpr CRC32.
   * string_id - A small C++ library to handle hashed strings serving as identifiers [https://github.com/foonathan/string_id]
   * uuid - unique id generator
   * uri - class for URI manipulations.
   * http_request_parser - a simple class to parse incoming http requests.
   * relinx - LINQ-like data transformations
   * planar_movements_recognizer - a set of classes to track and recognize planar movements and to execute corresponding commands
   * topological_sorter - the class to sort objects with dependencies (dependency solver)
   * sqlite3/sqlite c++ wrapper - sql database support [https://github.com/aminroosta/sqlite_modern_cpp]
   * quantum random number provider (using QRNG internet service: http://qrng.anu.edu.au)
   * strings - a library to do string manipulations like: trim, unicode convertions, replace, join, split, composing (aka format) etc.
   * cmdline_options - a commandline parser [https://github.com/Fytch/ProgramOptions.hxx]
   * giant - r-lyeh's tiny library to handle little/big endianness [https://github.com/r-lyeh/giant]
   * ordered_map, ordered_set - Tessil's C++ hash map/set which preserves the order of insertion [https://github.com/Tessil/ordered-map]
   * Google fonts were added [https://github.com/google/fonts]

Many of these libraries are sub-modules. So, don't forget to update sub-modules after cloning this repository.

For Windows users, you can download MinGW from the following sites:

https://gcc-mcf.lhmouse.com/ (It doesn't contain mingw32-make utility. Download it separately)
https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/

GENie can be downloaded from https://github.com/bkaradzic/GENie and genie executable should be placed to the example folder or be in path.

**A simple usage on Windows using GENie and MinGW:**

Go to the example folder and type:
```
> genie gmake
> mingw32-make
```
On Linux do the same but use 'make' instead of 'mingw32-make'
