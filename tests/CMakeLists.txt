find_package(Catch2 REQUIRED)

add_executable(test_module_system test_module_system.cpp)

target_include_directories(test_module_system
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(test_module_system
    PRIVATE
        GEF_MODULE_DIR="${CMAKE_BINARY_DIR}/modules"
        GEF_MODULE_PREFIX="${CMAKE_SHARED_MODULE_PREFIX}"
        GEF_MODULE_SUFFIX="${CMAKE_SHARED_MODULE_SUFFIX}"
)

target_link_libraries(test_module_system
    PRIVATE
        gef
        Catch2::Catch2WithMain
)

add_test(NAME module_system_tests COMMAND test_module_system)

add_executable(test_flow test_flow.cpp)

target_include_directories(test_flow
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(test_flow
    PRIVATE
        GEF_MODULE_DIR="${CMAKE_BINARY_DIR}/modules"
        GEF_MODULE_PREFIX="${CMAKE_SHARED_MODULE_PREFIX}"
        GEF_MODULE_SUFFIX="${CMAKE_SHARED_MODULE_SUFFIX}"
)

target_link_libraries(test_flow
    PRIVATE
        gef
        Catch2::Catch2WithMain
)

add_test(NAME flow_tests COMMAND test_flow)

add_library(missing_metadata MODULE fixtures/modules/missing_metadata.cpp)
target_link_libraries(missing_metadata PRIVATE gef)
set_target_properties(missing_metadata PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    POSITION_INDEPENDENT_CODE ON
)

add_library(missing_execute MODULE fixtures/modules/missing_execute.cpp)
target_link_libraries(missing_execute PRIVATE gef)
set_target_properties(missing_execute PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    POSITION_INDEPENDENT_CODE ON
)

add_library(null_metadata MODULE fixtures/modules/null_metadata.cpp)
target_link_libraries(null_metadata PRIVATE gef)
set_target_properties(null_metadata PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/modules"
    POSITION_INDEPENDENT_CODE ON
)

add_library(test_facade_module OBJECT test_facade_module.cpp)

target_include_directories(test_facade_module
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
)

add_library(test_facade_app OBJECT test_facade_app.cpp)

target_include_directories(test_facade_app
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
)
