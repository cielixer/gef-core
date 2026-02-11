// utils.cppm - Utility module
module;

#include <string>
#include <string_view>
#include <format>
#include <chrono>

export module gef.utils;

export namespace gef::utils {
    
    // Format a greeting message
    std::string format_greeting(std::string_view name) {
        return std::format("Hello, {}!", name);
    }
    
    // Get current timestamp as string
    std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        return std::format("{:%Y-%m-%d %H:%M:%S}", 
                          std::chrono::system_clock::from_time_t(time));
    }
    
    // String utility - to uppercase
    std::string to_upper(std::string_view str) {
        std::string result;
        result.reserve(str.size());
        for (char c : str) {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return result;
    }
    
    // String utility - to lowercase
    std::string to_lower(std::string_view str) {
        std::string result;
        result.reserve(str.size());
        for (char c : str) {
            result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return result;
    }
    
} // namespace gef::utils
