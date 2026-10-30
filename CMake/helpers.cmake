include(FetchContent)
include(CMakePrintHelpers)

function(quieter_check_symbol symbol header result_var)
    set(CMAKE_REQUIRED_QUIET TRUE)  # Suppress all check output
    check_symbol_exists(${symbol} "${header}" ${result_var})
    if(${result_var})
        message(STATUS "Symbol '${symbol}' found")
    else()
        message(STATUS "Symbol '${symbol}' not found")
    endif()
endfunction()

function(check_type type result_var)
    set(CMAKE_REQUIRED_QUIET TRUE)
    check_type_size(${type} ${result_var})
    if(${result_var})
        message(STATUS "Type '${type}' found")
    else()
        message(STATUS "Type '${type}' not found")
    endif()
endfunction()

function(check_include header result_var)
    set(CMAKE_REQUIRED_QUIET TRUE)
    check_include_file(${header} ${result_var})
    if(${result_var})
        message(STATUS "Header '${header}' found")
    else()
        message(STATUS "Header '${header}' not found")
    endif()
endfunction()

function(set_if condition var value)
    if(${condition})
        set(${var} "${value}" PARENT_SCOPE)
    endif()
endfunction()

function(set_if_else condition var var_1 var_2)
    if(${condition})
        set(${var} "${var_1}" PARENT_SCOPE)
    else()
        set(${var} "${var_2}" PARENT_SCOPE)
    endif()
endfunction()

function(set_makeinfo_env env_var cmake_var)
    if(${cmake_var})
        set(ENV{${env_var}} "1")
    else()
        set(ENV{${env_var}} "0")
    endif()
endfunction()

function(set_env_if env_var value condition)
    if(${condition})
        set(ENV{${env_var}} "${value}")
    endif()
endfunction()

function(set_config_define var)
    if(${var})
        set(${var} "#define ${var}" PARENT_SCOPE)
    else()
        set(${var} "#undef ${var}" PARENT_SCOPE)
    endif()
endfunction()

macro(append_list list libraries condition)
    if(${condition})
        list(APPEND ${list} ${libraries})
    endif()
endmacro()

function(copy_dlls target)
    if(WIN32)
        add_custom_command(
            TARGET "${target}"
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                "$<TARGET_RUNTIME_DLLS:${target}>"
                "$<TARGET_FILE_DIR:${target}>"
            USES_TERMINAL
            COMMAND_EXPAND_LISTS
        )
    endif()
endfunction()

function (FetchPNG)
	message (STATUS "Fetching libPNG...")

	set (PNG_SHARED OFF CACHE BOOL "Build libpng as a shared library" FORCE)
	set (PNG_STATIC ON  CACHE BOOL "Build libpng as a static library" FORCE)

	set (CMAKE_MESSAGE_LOG_LEVEL "WARNING") # Disable status messages

	FetchContent_Declare (
			png
			URL https://github.com/pnggroup/libpng/archive/refs/tags/v1.6.54.tar.gz
			URL_HASH SHA256=ba7efce137409079989df4667706c339bebfbb10e9f413474718012a13c8cd4c
			SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libpng
			DOWNLOAD_NO_PROGRESS TRUE
			DOWNLOAD_EXTRACT_TIMESTAMP TRUE
			
	)
	FetchContent_MakeAvailable (png)

	set (CMAKE_MESSAGE_LOG_LEVEL "STATUS") # Re-enable status messages

	# rename pnglibconf.h.prebuilt to pnglibconf.h as per instructions in png.h
	if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libpng/pnglibconf.h")
		message(WARNING "pnglibconf.h not found. Copying scripts/pnglibconf.h.prebuilt as suggested in pnglib.h")
		file(COPY_FILE
		    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libpng/scripts/pnglibconf.h.prebuilt"
			"${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libpng/pnglibconf.h"
			)
	endif()

	# Set variables
	set (PNG_FOUND TRUE PARENT_SCOPE)

	add_library(PNG::PNG ALIAS png_static)
	set (PNG_CFLAGS_OTHER ""       PARENT_SCOPE)
	set (PNG_VERSION      "1.6.54" PARENT_SCOPE)

	add_custom_command(TARGET ${PROJECT_NAME}  PRE_LINK
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE:PNG::PNG>
		$<TARGET_FILE_DIR:${PROJECT_NAME}>
	)
endfunction ()

function (FetchOgg)
	message (STATUS "Fetching libogg...")

	set (CMAKE_MESSAGE_LOG_LEVEL "WARNING") # Disable status messages

	FetchContent_Declare (
			libogg
			URL https://github.com/xiph/ogg/releases/download/v1.3.6/libogg-1.3.6.tar.gz
			URL_HASH SHA256=83e6704730683d004d20e21b8f7f55dcb3383cdf84c0daedf30bde175f774638
			SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libogg
			DOWNLOAD_NO_PROGRESS TRUE
			DOWNLOAD_EXTRACT_TIMESTAMP TRUE
			
	)
	FetchContent_MakeAvailable (libogg)

	set (CMAKE_MESSAGE_LOG_LEVEL "STATUS") # Re-enable status messages

	set (OGG_VERSION "1.3.6" PARENT_SCOPE)
endfunction ()

function (FetchVorbis)
	message (STATUS "Fetching libvorbis...")

	set (CMAKE_MESSAGE_LOG_LEVEL "WARNING") # Disable status messages

	FetchContent_Declare (
			libvorbis
			URL https://github.com/JHGuitarFreak/vorbis/archive/refs/tags/uqm-megamod.tar.gz
			URL_HASH SHA256=1f51792ad82269317866c1f347ca4aa35bfbaade9ddab80dd3db9fd3d8fe094c
			SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libvorbis
			DOWNLOAD_NO_PROGRESS TRUE
			DOWNLOAD_EXTRACT_TIMESTAMP TRUE
			CMAKE_ARGS
			-DBUILD_TESTING=OFF
			-DBUILD_FRAMEWORK=OFF
			
	)
	FetchContent_MakeAvailable (libvorbis)

	set (CMAKE_MESSAGE_LOG_LEVEL "STATUS") # Re-enable status messages

	set (VORBIS_VERSION "1.3.7" PARENT_SCOPE)
	set (UQM_OGG_CODEC "standard"   PARENT_SCOPE)

	add_custom_command(TARGET ${PROJECT_NAME}  PRE_LINK
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:Vorbis::vorbis>
			$<TARGET_FILE_DIR:${PROJECT_NAME}>
	)

	add_custom_command(TARGET ${PROJECT_NAME}  PRE_LINK
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:Vorbis::vorbisfile>
			$<TARGET_FILE_DIR:${PROJECT_NAME}>
	)
	
endfunction ()

function (FetchZLIB)
	message (STATUS "Fetching ZLIB...")

	set (CMAKE_MESSAGE_LOG_LEVEL "WARNING") # Disable status messages

	FetchContent_Declare (
			zlib
			URL https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.gz
			URL_HASH SHA256=9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23
			SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/zlib
			DOWNLOAD_NO_PROGRESS TRUE
			DOWNLOAD_EXTRACT_TIMESTAMP TRUE
			CMAKE_ARGS
			-DBUILD_SHARED_LIBS=ON
			-DZLIB_BUILD_EXAMPLES=OFF
			)
	FetchContent_MakeAvailable (zlib)

	set (CMAKE_MESSAGE_LOG_LEVEL "STATUS") # Re-enable status messages

	# Pick the actual zlib target
	if (TARGET zlibstatic)
		set(_zlib_target zlibstatic)
		set(_zlib_libname zlibstatic)
	elseif (TARGET zlib)
		set(_zlib_target zlib)
		set(_zlib_libname zlib)
	else()
		message(FATAL_ERROR "No zlib target found")
	endif()

	# ---- Legacy variables for FindZLIB (libpng) ----

	# Include dir (configure-time safe)
	set(ZLIB_INCLUDE_DIR
		"${zlib_SOURCE_DIR};${zlib_BINARY_DIR}"
		CACHE PATH "ZLIB include directory"
		FORCE
	)

	# Library path (configure-time safe, multi-config safe)
	set(ZLIB_LIBRARY
		"${zlib_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${_zlib_libname}${CMAKE_STATIC_LIBRARY_SUFFIX}"
		CACHE FILEPATH "ZLIB library"
		FORCE
	)

	message(STATUS "zlib lib=${ZLIB_LIBRARY}")
	
	set(ZLIB_FOUND TRUE CACHE BOOL "ZLIB found" FORCE)

	# ---- Modern target for your code ----
	add_library(ZLIB::ZLIB ALIAS ${_zlib_target})

	set (ZLIB_VERSION "1.3.1" PARENT_SCOPE)

	add_custom_command(TARGET ${PROJECT_NAME}  PRE_LINK
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			$<TARGET_FILE:ZLIB::ZLIB>
			$<TARGET_FILE_DIR:${PROJECT_NAME}>
	)
endfunction ()

function (FetchOpenAL)
	message (STATUS "Fetching OpenAL...")

	FetchContent_Declare (
		OpenAL-Lib
		URL https://github.com/kcat/openal-soft/releases/download/1.25.1/openal-soft-1.25.1-bin.zip
		URL_HASH SHA256=0915af01928b17e46f7c59ffc44a7413486bbef8424a3465109f489a80cb68a2
		SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/OpenAL-Soft
		DOWNLOAD_NO_PROGRESS TRUE
		DOWNLOAD_EXTRACT_TIMESTAMP TRUE
	)
	FetchContent_MakeAvailable (OpenAL-Lib)

	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_openal_arch Win64)
	elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(_openal_arch Win32)
	else()
		message(FATAL_ERROR "Unknown pointer size")
	endif()

	add_Library(OpenAL::OpenAL SHARED IMPORTED)
	set(_openal_root "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/OpenAL-Soft/")

	# set header location
	set_target_properties(OpenAL::OpenAL PROPERTIES
    	INTERFACE_INCLUDE_DIRECTORIES
        "${_openal_root}/include/AL"
	)

	# set import lib and dll locations.
	set_target_properties(OpenAL::OpenAL PROPERTIES
		IMPORTED_IMPLIB
			"${_openal_root}/libs/${_openal_arch}/OpenAL32.lib"
		IMPORTED_LOCATION
			"${_openal_root}/bin/${_openal_arch}/soft_oal.dll"
	)

	set(_openal_target_dll_file "${CMAKE_CURRENT_BINARY_DIR}/OpenAL32.dll")

	add_custom_command(TARGET ${PROJECT_NAME}  
		PRE_LINK
    	COMMAND ${CMAKE_COMMAND} -E copy_if_different
        	$<TARGET_FILE:OpenAL::OpenAL>
        	${_openal_target_dll_file}
	)
endfunction()

function(copy_runtime_dll target dll_target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${dll_target}>
            $<TARGET_FILE_DIR:${target}>
    )
endfunction()

function(explicitly_link_clang_asan_on_windows target)
	execute_process(
        COMMAND "${CMAKE_CXX_COMPILER}" -print-resource-dir
        OUTPUT_VARIABLE _clang_resource_dir
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Normalize path (VERY important on Windows)
    file(REAL_PATH "${_clang_resource_dir}" _clang_resource_dir)

    set(_asan_lib_dir "${_clang_resource_dir}/lib/windows")
	message (STATUS "asan lb dir=${_asan_lib_dir}")

    if(NOT EXISTS "${_asan_lib_dir}")
        message(FATAL_ERROR "ASan runtime not found at: ${_asan_lib_dir}")
    endif()

 	set(_asan_lib "${_asan_lib_dir}/clang_rt.asan_dynamic-x86_64.lib")
	set(_asan_thunk "${_asan_lib_dir}/clang_rt.asan_static_runtime_thunk-x86_64.lib")
    set(_asan_dll "${_asan_lib_dir}/clang_rt.asan_dynamic-x86_64.dll")

    #target_link_directories(${target} PRIVATE "${_asan_lib_dir}")

	if(NOT EXISTS "${_asan_lib}") 
        message(FATAL_ERROR "ASan import library not found: ${_asan_lib}")
    endif()
	if(NOT EXISTS "${_asan_thunk}") 
        message(FATAL_ERROR "ASan thunk import library not found: ${_asan_thunk}")
    endif()
	if(NOT EXISTS "${_asan_dll}")
        message(FATAL_ERROR "ASan runtime dll not found at: ${_asan_dll}")
    endif()

    target_link_libraries(${target} 
		PRIVATE
			${_asan_lib}
			${_asan_thunk}
    )


    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_asan_dll}"
            "$<TARGET_FILE_DIR:${target}>"
    )
endfunction()

function(print_library_properties lib)
	cmake_print_properties(TARGETS ${lib} PROPERTIES LOCATION INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES BINARY_DIR LOCATION LINK_LIBRARIES)
# 	cmake_print_properties(TARGETS OpenAL::OpenAL PROPERTIES
#         ALIASED_TARGET
#         ARCHIVE_OUTPUT_DIRECTORY
#         ARCHIVE_OUTPUT_DIRECTORY_DEBUG
#         ARCHIVE_OUTPUT_DIRECTORY_RELEASE
#         ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO
#         ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL
#         AUTOMOC
#         AUTOMOC_EXECUTABLE
#         AUTORCC
#         AUTOUIC
#         AUTOUIC_EXECUTABLE
#         BINARY_DIR
#         BUILD_RPATH
#         BUILD_RPATH_USE_ORIGIN
#         BUILD_WITH_INSTALL_RPATH
#         COMPILE_DEFINITIONS
#         COMPILE_DEFINITIONS_DEBUG
#         COMPILE_DEFINITIONS_RELEASE
#         COMPILE_DEFINITIONS_RELWITHDEBINFO
#         COMPILE_DEFINITIONS_MINSIZEREL
#         COMPILE_FEATURES
#         COMPILE_OPTIONS
#         DEBUG_POSTFIX
#         ENABLE_EXPORTS
#         EXCLUDE_FROM_ALL
#         EXPORT_NAME
#         FOLDER
#         FRAMEWORK
#         IMPORTED
#         IMPORTED_CONFIGURATIONS
#         IMPORTED_IMPLIB
#         IMPORTED_IMPLIB_DEBUG
#         IMPORTED_IMPLIB_RELEASE
#         IMPORTED_LOCATION
#         IMPORTED_LOCATION_DEBUG
#         IMPORTED_LOCATION_RELEASE
#         INCLUDE_DIRECTORIES
#         INSTALL_NAME_DIR
#         INTERFACE_COMPILE_DEFINITIONS
#         INTERFACE_COMPILE_FEATURES
#         INTERFACE_COMPILE_OPTIONS
#         INTERFACE_INCLUDE_DIRECTORIES
#         INTERFACE_LINK_DIRECTORIES
#         INTERFACE_LINK_LIBRARIES
#         INTERFACE_POSITION_INDEPENDENT_CODE
#         INTERFACE_SOURCES
#         INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
#         LINK_DEPENDS
#         LINK_DIRECTORIES
#         LINK_INTERFACE_LIBRARIES
#         LINK_OPTIONS
#         LINK_SEARCH_END_STATIC
#         LINK_SEARCH_START_STATIC
#         LINK_WHAT_YOU_USE
#         LOCATION
#         MACHO_COMPATIBILITY_VERSION
#         MACHO_CURRENT_VERSION
#         MACOSX_BUNDLE
#         MACOSX_BUNDLE_INFO_PLIST
#         MAP_IMPORTED_CONFIG_DEBUG
#         MAP_IMPORTED_CONFIG_RELEASE
#         NAME
#         NO_SONAME
#         OUTPUT_NAME
#         POSITION_INDEPENDENT_CODE
#         PREFIX
#         PUBLIC_HEADER
#         RUNTIME_OUTPUT_DIRECTORY
#         RUNTIME_OUTPUT_DIRECTORY_DEBUG
#         RUNTIME_OUTPUT_DIRECTORY_RELEASE
#         RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO
#         RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL
#         SKIP_BUILD_RPATH
#         SOURCES
#         SOVERSION
#         STATIC_LIBRARY_OPTIONS
#         SUFFIX
#         TYPE
#         VERSION
#         VS_DEBUGGER_ENVIRONMENT
#         VS_DEBUGGER_WORKING_DIRECTORY
#         WINDOWS_EXPORT_ALL_SYMBOLS
# )

endfunction()