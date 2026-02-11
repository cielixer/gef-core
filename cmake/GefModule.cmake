# GEF Module CMake Helper
# Provides add_gef_module() function for building GEF modules

function(add_gef_module NAME SOURCE)
    # Create MODULE library (shared library for dlopen)
    add_library(${NAME} MODULE ${SOURCE})
    
    # Link against core GEF library
    target_link_libraries(${NAME} PRIVATE gef)
    
    # Include GEF headers
    target_include_directories(${NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    
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
