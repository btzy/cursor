cmake_minimum_required (VERSION 3.10)
project (libcursor)

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_LIST_DIR}/cmake)

add_library(libcursor
    src/cursor.cpp
)

target_include_directories(libcursor
    PUBLIC 
        ${CMAKE_CURRENT_LIST_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
)

set_property(TARGET libcursor PROPERTY CXX_STANDARD 17)

if(MSVC)
    # Force MSVC standards conformance mode to detect non-portable code
    add_compile_options(/permissive-)
    add_definitions(/D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_WARNINGS)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT AZURE_CI)
      message("Enable LTCG optimization.")
      add_compile_options(/GL)
      set(linker_flags
        CMAKE_EXE_LINKER_FLAGS
        CMAKE_SHARED_LINKER_FLAGS
        CMAKE_MODULE_LINKER_FLAGS
      )
      foreach (flag ${linker_flags})
        set("${flag}" "${${flag}} /LTCG")
      endforeach()
    endif()
    
    if (AZURE_CI)
      # Run tests with assertions in Azure CI.
      add_compile_options(/UNDEBUG)
    endif()
endif()

#add_subdirectory(src/cli)
