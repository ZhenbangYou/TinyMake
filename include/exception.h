#pragma once

#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @brief Convenient wrapper based on `std::runtime_error`.
 *
 * Example: throw RuntimeException({"This", "is", "an", "exception:",
 * std::to_string(100)}), the error message will be "This is an exception: 100".
 */
class RuntimeException : public std::runtime_error {
   public:
    /**
     * @brief Construct a new Runtime Exception object.
     *
     * @param whatArgs Strings in this vector will be concatenated in order with
     * a whitespace.
     */
    explicit RuntimeException(const std::vector<std::string>& whatArgs)
        : std::runtime_error(
              std::accumulate(whatArgs.cbegin(), whatArgs.cend(), std::string(),
                              [](const std::string& a, const std::string& b) {
                                  return a + " " + b;
                              })) {}
};