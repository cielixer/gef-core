cmake_minimum_required(VERSION 3.28)

project(gef VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

add_compile_options(
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wextra>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_library(gef SHARED)
add_library(gef::gef ALIAS gef)

set_target_properties(gef PROPERTIES
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

include(cmake/GefModule.cmake)

target_include_directories(gef 
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    src
)

add_subdirectory(src)
add_subdirectory(modules)

enable_testing()
add_subdirectory(tests)

# ---------------------------------------------------------------------------
# Installation
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install the shared library
install(TARGETS gef
    EXPORT gef-targets
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install public headers
install(DIRECTORY include/gef
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install CMake helper for downstream module authors
install(FILES cmake/GefModule.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gef
)

# Generate and install the targets file
install(EXPORT gef-targets
    FILE gef-targets.cmake
    NAMESPACE gef::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gef
)

# Generate version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/gef-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Generate config file from template
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gef-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/gef-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gef
)

# Install config and version files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/gef-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/gef-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gef
)
