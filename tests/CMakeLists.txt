find_package(Catch2 REQUIRED)

add_executable(test_module_system test_module_system.cpp)

target_include_directories(test_module_system
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_definitions(test_module_system
    PRIVATE
        GEF_MODULE_DIR="${CMAKE_BINARY_DIR}/modules"
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
)

target_link_libraries(test_flow
    PRIVATE
        gef
        Catch2::Catch2WithMain
)

add_test(NAME flow_tests COMMAND test_flow)

add_executable(test_composite_module test_composite_module.cpp)

target_include_directories(test_composite_module
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_composite_module
    PRIVATE
        gef
        Catch2::Catch2WithMain
)

add_test(NAME composite_module_tests COMMAND test_composite_module)

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
