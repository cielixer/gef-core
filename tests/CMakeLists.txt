find_package(Catch2 REQUIRED)

add_executable(test_plugin_system test_plugin_system.cpp)

target_include_directories(test_plugin_system
    PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(test_plugin_system
    PRIVATE
        gef
        Catch2::Catch2WithMain
)

add_test(NAME plugin_system_tests COMMAND test_plugin_system)
