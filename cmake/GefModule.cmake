# GEF Module CMake Helper
# Provides add_gef_module() function for building GEF modules
#
# Works in both build tree (include() from root CMakeLists.txt)
# and install tree (find_package(gef) from downstream projects).

function(add_gef_module NAME SOURCE)
    # Create MODULE library (shared library for dlopen)
    add_library(${NAME} MODULE ${SOURCE})

    # Link against gef::gef — works in both build tree (ALIAS) and install tree (exported)
    target_link_libraries(${NAME} PRIVATE gef::gef)

    # Inject module name as compile definition so GEF_MODULE macro can use it
    target_compile_definitions(${NAME} PRIVATE GEF_MODULE_NAME="${NAME}")

    # Set output directory for modules
    set_target_properties(${NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/modules)

    # Enable Position Independent Code for all platforms
    set_target_properties(${NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    # Platform-specific settings
    if(APPLE)
        # macOS: use .so extension for consistency with dlopen across platforms
        set_target_properties(${NAME} PROPERTIES SUFFIX ".so")
    elseif(WIN32)
        # Windows: .dll is default for MODULE, no additional settings needed
    else()
        # Linux/Unix: .so is default for MODULE, no additional settings needed
    endif()
endfunction()
