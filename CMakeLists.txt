# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.10)

# project name, version, and languages
project(pdf2img VERSION 1.0 LANGUAGES C)

# project version macro
add_definitions(-DPDF2IMG_VERSION="${pdf2img_VERSION}")

# Don't use DPI?
set(GDIPM_NO_DPI ON)

# "Don't make test program?" Option
option(PDF2IMG_NO_TESTS "Don't create test programs" ON)
if(PDF2IMG_NO_TESTS)
    add_definitions(-DPDF2IMG_NO_TESTS)
endif()

# libgdipm.a
add_library(libgdipm STATIC gdipm.c)
set_target_properties(libgdipm PROPERTIES PREFIX "")
target_include_directories(libgdipm INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(libgdipm gdi32 shlwapi)

# libpdf_load.a
add_library(pdf_load STATIC pdf_load.c)
target_link_libraries(pdf_load gdi32 shlwapi)

# libpdf2bitmap.a
add_library(pdf2bitmap STATIC pdf2bitmap.c)
target_include_directories(pdf2bitmap INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(pdf2bitmap gdi32 pdf_load libgdipm)

if(NOT PDF2IMG_NO_TESTS)
    # pdf2img.exe
    add_executable(pdf2img main.c)
    target_link_libraries(pdf2img libgdipm pdf2bitmap)
endif()

# display status
message(STATUS "PDF2IMG_NO_TESTS = ${PDF2IMG_NO_TESTS}")
