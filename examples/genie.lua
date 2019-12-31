solution "nstd_examples"
    configurations { "Debug", "Release" }
    userincludedirs { "../include" }
    flags { "StaticRuntime" }
    configuration "vs*"
        buildoptions { "/std:c++latest", "/EHsc", "/Wall" }

    configuration "not vs*"
        buildoptions_cpp { "-std=c++2a", "-fexceptions" }
        buildoptions "-Wall"

    language "C++"
    configuration { "windows", "not vs*" }
        linkoptions { "-static" }

    configuration { "Debug" }
        kind "ConsoleApp"
        flags { "Symbols" }

    configuration { "Release" }
        kind "ConsoleApp"
        defines { "NDEBUG" }
        flags { "OptimizeSpeed" }

    --[[project "asio_example"
        files { "asio_example.cpp" }
        userincludedirs { "../include/external/asio/asio/include" }
        includedirs { "../include/external/json/include" }
        configuration { "Debug" }
            objdir "obj/asio_example/Debug"
            targetdir "bin/asio_example/Debug"

        configuration { "Release" }
            objdir "obj/asio_example/Release"
            targetdir "bin/asio_example/Release"

        configuration "not vs*"
            buildoptions_cpp { "-fpermissive" }

        configuration { "windows" }
            links { "ws2_32" }

        configuration "not vs*"
            buildoptions_cpp { "-fpermissive" }

        configuration "linux"
            links { "pthread" }--]]

    project "base64_example"
        files { "base64_example.cpp" }
        configuration { "Debug" }
            objdir "obj/base64_example/Debug"
            targetdir "bin/base64_example/Debug"

        configuration { "Release" }
            objdir "obj/base64_example/Release"
            targetdir "bin/base64_example/Release"

    project "chaiscript_example"
        files { "chaiscript_example.cpp" }
        configuration { "Debug" }
            objdir "obj/chaiscript_example/Debug"
            targetdir "bin/chaiscript_example/Debug"
            flags { "Optimize" }

        configuration { "Release" }
            objdir "obj/chaiscript_example/Release"
            targetdir "bin/chaiscript_example/Release"

        configuration "linux"
            links { "pthread", "dl" }

    project "date_example"
        files { "date_example.cpp" }
        configuration { "Debug" }
            objdir "obj/date_example/Debug"
            targetdir "bin/date_example/Debug"

        configuration { "Release" }
            objdir "obj/date_example/Release"
            targetdir "bin/date_example/Release"

    project "blend2d_example"
        files { "blend2d_example.cpp", "../include/blend2d.cpp" }
        includedirs { "../include/external/asmjit/src" }

        postbuildcmd = "copy ../resources/example_resources/blend2d_example/fonts/*.ttf ./bin/blend2d_example/"

        configuration { "Debug" }
            objdir "obj/blend2d_example/Debug"
            targetdir "bin/blend2d_example/Debug"
	    cmd = postbuildcmd .. "Debug"
   	    postbuildcommands { iif(os.is("windows"), string.gsub(cmd, "/", "\\"), string.gsub(cmd, "copy ", "cp "))  }

        configuration { "Release" }
            objdir "obj/blend2d_example/Release"
            targetdir "bin/blend2d_example/Release"
	    cmd = postbuildcmd .. "Release"
   	    postbuildcommands { iif(os.is("windows"), string.gsub(cmd, "/", "\\"), string.gsub(cmd, "copy ", "cp "))  }

        configuration "not vs*"
            buildoptions { "-Wno-all" }

        configuration "linux"
            links { "pthread", "rt" }

    project "avir_example"
        files { "avir_example.cpp", "../include/blend2d.cpp" }
        includedirs { "../include/external/asmjit/src" }

        postbuildcmd = "copy ../resources/example_resources/avir_example/images/* ./bin/avir_example/"

        configuration { "Debug" }
            objdir "obj/avir_example/Debug"
            targetdir "bin/avir_example/Debug"
	    cmd = postbuildcmd .. "Debug"
   	    postbuildcommands { iif(os.is("windows"), string.gsub(cmd, "/", "\\"), string.gsub(cmd, "copy ", "cp "))  }

        configuration { "Release" }
            objdir "obj/avir_example/Release"
            targetdir "bin/avir_example/Release"
	    cmd = postbuildcmd .. "Release"
   	    postbuildcommands { iif(os.is("windows"), string.gsub(cmd, "/", "\\"), string.gsub(cmd, "copy ", "cp "))  }

        configuration "not vs*"
            buildoptions { "-Wno-all" }

        configuration "linux"
            links { "pthread", "rt" }

    project "giant_example"
        files { "giant_example.cpp" }
        configuration "not vs*"
            buildoptions { "-Wno-unused-variable" }

        configuration { "Debug" }
            objdir "obj/giant_example/Debug"
            targetdir "bin/giant_example/Debug"

        configuration { "Release" }
            objdir "obj/giant_example/Release"
            targetdir "bin/giant_example/Release"

    project "expiry_cache_example"
        files { "expiry_cache_example.cpp" }
        configuration { "Debug" }
            flags { "Symbols" }
            objdir "obj/expiry_cache_example/Debug"
            targetdir "bin/expiry_cache_example/Debug"

        configuration { "Release" }
            objdir "obj/expiry_cache_example/Release"
            targetdir "bin/expiry_cache_example/Release"

        configuration "linux"
            links { "pthread" }

    project "live_property_example"
        files { "live_property_example.cpp" }
        includedirs { "../include/external/json/include" }
        configuration { "Debug" }
            objdir "obj/live_property_example/Debug"
            targetdir "bin/live_property_example/Debug"

        configuration { "Release" }
            objdir "obj/live_property_example/Release"
            targetdir "bin/live_property_example/Release"

        configuration "not vs*"
            buildoptions_cpp { "-fpermissive" }

        configuration "linux"
            links { "pthread" }

    project "relinx_example"
        files { "relinx_example.cpp" }
        configuration { "Debug" }
            objdir "obj/relinx_example/Debug"
            targetdir "bin/relinx_example/Debug"

        configuration { "Release" }
            objdir "obj/relinx_example/Release"
            targetdir "bin/relinx_example/Release"

            configuration "not vs*"
                buildoptions { "-Wno-unused-variable", "-Wno-unused-but-set-variable" }

    --[[project "relinx_generator_example"
        files { "relinx_generator_example.cpp" }
        userincludedirs { "../include/external/asio/asio/include" }
        includedirs { "../include/external/json/include" }
        configuration { "Debug" }
            objdir "obj/relinx_generator_example/Debug"
            targetdir "bin/relinx_generator_example/Debug"

        configuration { "Release" }
            objdir "obj/relinx_generator_example/Release"
            targetdir "bin/relinx_generator_example/Release"

        configuration "not vs*"
            buildoptions_cpp { "-fpermissive" }

        configuration "windows"
            links { "ws2_32" }

        configuration "linux"
            links { "pthread" }--]]

    project "pmr_example"
        files { "pmr_example.cpp", "../include/process.cpp" }
        configuration { "Debug" }
            objdir "obj/pmr_example/Debug"
            targetdir "bin/pmr_example/Debug"

        configuration { "Release" }
            objdir "obj/pmr_example/Release"
            targetdir "bin/pmr_example/Release"

        configuration "linux"
            links { "pthread" }

    project "strings_example"
        files { "strings_example.cpp", "../include/external/string_id/database.cpp", "../include/external/string_id/error.cpp", "../include/external/string_id/generator.cpp", "../include/external/string_id/string_id.cpp" }
        configuration { "Debug" }
            objdir "obj/strings_example/Debug"
            targetdir "bin/strings_example/Debug"

        configuration { "Release" }
            objdir "obj/strings_example/Release"
            targetdir "bin/strings_example/Release"

    project "topological_sorter_example"
        files { "topological_sorter_example.cpp" }
        configuration { "Debug" }
            objdir "obj/topological_sorter_example/Debug"
            targetdir "bin/topological_sorter_example/Debug"

        configuration { "Release" }
            objdir "obj/topological_sorter_example/Release"
            targetdir "bin/topological_sorter_example/Release"

    project "sqlite_example"
        files { "sqlite_example.cpp", "../include/external/sqlite/sqlite3.c" }
        includedirs { "../include/external/sqlite", "../include/external/json/include" }
        configuration "not vs*"
            buildoptions { "-Wno-unused-but-set-variable", "-DSQLITE_DQS=0" }

        configuration { "Debug" }
            objdir "obj/sqlite_example/Debug"
            targetdir "bin/sqlite_example/Debug"

        configuration { "Release" }
            objdir "obj/sqlite_example/Release"
            targetdir "bin/sqlite_example/Release"

        configuration "not vs*"
            buildoptions_cpp { "-fpermissive" }

        configuration "linux"
            links { "pthread", "dl" }

    project "units_example"
        files { "units_example.cpp" }
        configuration { "Debug" }
            objdir "obj/units_example/Debug"
            targetdir "bin/units_example/Debug"

        configuration { "Release" }
            objdir "obj/units_example/Release"
            targetdir "bin/units_example/Release"

    --[[project "urdl_example"
        files { "urdl_example.cpp" }
        userincludedirs { "../include/external/asio/asio/include" }
        includedirs { "../include/external/json/include" }
        configuration { "Debug" }
            objdir "obj/urdl_example/Debug"
            targetdir "bin/urdl_example/Debug"

        configuration { "Release" }
            objdir "obj/urdl_example/Release"
            targetdir "bin/urdl_example/Release"

        configuration "windows"
            links { "ws2_32" }

        configuration "not vs*"
            buildoptions_cpp { "-fpermissive" }

        configuration "linux"
            links { "pthread" }--]]

    project "uuid_example"
        files { "uuid_example.cpp" }
        userincludedirs { "../include/external/asio/asio/include" }
        includedirs { "../include/external/json/include" }
        configuration { "Debug" }
            objdir "obj/uuid_example/Debug"
            targetdir "bin/uuid_example/Debug"

        configuration { "Release" }
            objdir "obj/uuid_example/Release"
            targetdir "bin/uuid_example/Release"

        configuration "windows"
            links { "ws2_32" }

        configuration "not vs*"
            buildoptions_cpp { "-fpermissive" }

        configuration "linux"
            links { "pthread" }

    project "freetype_example"
        files { "freetype_example.cpp", "../include/external/freetype/freetype.c" }
        includedirs { "../include/external/freetype/freetype2/include", "../include/external/freetype/brotli/c/include" }
        configuration "not vs*"
            buildoptions { "-Wno-maybe-uninitialized", "-Wno-multichar" }

        configuration { "Debug" }
            objdir "obj/freetype_example/Debug"
            targetdir "bin/freetype_example/Debug"

        configuration { "Release" }
            objdir "obj/freetype_example/Release"
            targetdir "bin/freetype_example/Release"

    project "agg_example"
        files { "agg_example.cpp", "../include/external/freetype/freetype.c",
		"../include/external/agg/agg/src/platform/agg_platform_support.cpp",
		"../include/external/agg/agg/font_freetype/agg_font_freetype1.cpp",
		"../include/external/agg/agg/src/ctrl/agg_cbox_ctrl.cpp",
		"../include/external/agg/agg/src/ctrl/agg_rbox_ctrl.cpp",
		"../include/external/agg/agg/src/ctrl/agg_slider_ctrl.cpp",
		"../include/external/agg/agg/src/agg_bezier_arc.cpp",
		"../include/external/agg/agg/src/agg_curves.cpp",
		"../include/external/agg/agg/src/agg_gsv_text.cpp",
		"../include/external/agg/agg/src/agg_trans_affine.cpp",
		"../include/external/agg/agg/src/agg_vcgen_contour.cpp",
		"../include/external/agg/agg/src/agg_vcgen_stroke.cpp" }
        includedirs { "../include/external/freetype/freetype2/include", "../include/external/freetype/brotli/c/include" }
        userincludedirs { "../include/external/agg/agg/include", "../include/external/agg/agg/font_freetype" }
        configuration "not vs*"
            buildoptions { "-Wno-maybe-uninitialized", "-Wno-unused-result" }

        postbuildcmd = "copy ../resources/example_resources/agg_example/fonts/*.ttf ./bin/agg_example/"

        configuration { "Debug" }
            objdir "obj/agg_example/Debug"
            targetdir "bin/agg_example/Debug"
	    cmd = postbuildcmd .. "Debug"
   	    postbuildcommands { iif(os.is("windows"), string.gsub(cmd, "/", "\\"), string.gsub(cmd, "copy ", "cp "))  }

        configuration { "Release" }
            objdir "obj/agg_example/Release"
            targetdir "bin/agg_example/Release"
	    cmd = postbuildcmd .. "Release"
   	    postbuildcommands { iif(os.is("windows"), string.gsub(cmd, "/", "\\"), string.gsub(cmd, "copy ", "cp "))  }

        configuration "windows"
            links { "gdi32" }

        configuration "linux"
            links { "X11" }

    project "sharp_tcp_server_example"
        files { "sharp_tcp_server_example.cpp" }
        configuration { "Debug" }
            objdir "obj/sharp_tcp_server_example/Debug"
            targetdir "bin/sharp_tcp_server_example/Debug"

        configuration { "Release" }
            objdir "obj/sharp_tcp_server_example/Release"
            targetdir "bin/sharp_tcp_server_example/Release"

        configuration { "windows" }
            links { "ws2_32" }

        configuration "linux"
            links { "pthread" }

    project "sharp_tcp_client_example"
        files { "sharp_tcp_client_example.cpp" }
        configuration { "Debug" }
            objdir "obj/sharp_tcp_client_example/Debug"
            targetdir "bin/sharp_tcp_client_example/Debug"

        configuration { "Release" }
            objdir "obj/sharp_tcp_client_example/Release"
            targetdir "bin/sharp_tcp_client_example/Release"

        configuration { "windows" }
            links { "ws2_32" }

        configuration "linux"
            links { "pthread" }

    project "remote_signal_slot_example"
        files { "remote_signal_slot_example.cpp" }
        includedirs { "../include/external/json/include" }
        configuration { "Debug" }
            objdir "obj/remote_signal_slot_example/Debug"
            targetdir "bin/remote_signal_slot_example/Debug"

        configuration { "Release" }
            objdir "obj/remote_signal_slot_example/Release"
            targetdir "bin/remote_signal_slot_example/Release"

        configuration { "windows" }
            links { "ws2_32" }

        configuration "linux"
            links { "pthread" }

    project "http_request_parser_example"
        files { "http_request_parser_example.cpp" }
        configuration { "Debug" }
            objdir "obj/http_request_parser_example/Debug"
            targetdir "bin/http_request_parser_example/Debug"

        configuration { "Release" }
            objdir "obj/http_request_parser_example/Release"
            targetdir "bin/http_request_parser_example/Release"

    project "http_resource_manager_example"
        files { "http_resource_manager_example.cpp" }
        configuration { "Debug" }
            objdir "obj/http_resource_manager_example/Debug"
            targetdir "bin/http_resource_manager_example/Debug"

        configuration { "Release" }
            objdir "obj/http_resource_manager_example/Release"
            targetdir "bin/http_resource_manager_example/Release"

        configuration { "windows" }
            links { "ws2_32", "stdc++fs" }

        configuration "linux"
            links { "pthread", "stdc++fs" }
