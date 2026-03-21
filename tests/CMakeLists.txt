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
