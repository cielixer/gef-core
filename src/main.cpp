// main.cpp - Main entry point
#include <iostream>
#include <print>
#include "core/config.h"

// Import modules
import gef.math;
import gef.utils;

int main() {
    // Print project info
    std::println("=== {} v{} ===", 
                 gef::config::get_project_name(),
                 gef::config::get_version());
    std::println("");
    
    // Test utils module
    std::println("Utils Module Demo:");
    std::println("  {}", gef::utils::format_greeting("C++ Modules"));
    std::println("  Current time: {}", gef::utils::get_timestamp());
    std::println("  Uppercase: {}", gef::utils::to_upper("modern c++"));
    std::println("  Lowercase: {}", gef::utils::to_lower("MODULES ARE COOL"));
    std::println("");
    
    // Test math module
    std::println("Math Module Demo:");
    std::println("  5 + 3 = {}", gef::math::add(5, 3));
    std::println("  4 * 7 = {}", gef::math::multiply(4, 7));
    std::println("  2^8 = {}", gef::math::power(2.0, 8.0));
    std::println("  sqrt(16) = {}", gef::math::sqrt(16.0));
    std::println("");
    
    std::println("All modules working correctly!");
    
    return 0;
}
