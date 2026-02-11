// math.cppm - Math module
module;

#include <cmath>
#include <concepts>

export module gef.math;

export namespace gef::math {
    
    // Concept for numeric types
    template<typename T>
    concept Numeric = std::integral<T> || std::floating_point<T>;
    
    // Add two numbers
    template<Numeric T>
    constexpr T add(T a, T b) noexcept {
        return a + b;
    }
    
    // Multiply two numbers
    template<Numeric T>
    constexpr T multiply(T a, T b) noexcept {
        return a * b;
    }
    
    // Power function
    template<std::floating_point T>
    T power(T base, T exponent) noexcept {
        return std::pow(base, exponent);
    }
    
    // Square root
    template<std::floating_point T>
    T sqrt(T value) noexcept {
        return std::sqrt(value);
    }
    
} // namespace gef::math
