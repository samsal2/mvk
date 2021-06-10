#include "utility/trace.hpp"

#include <iostream>

namespace mvk::utility::detail
{

[[nodiscard]] std::string
trace_message(std::string const & message, size_t const line, std::string const & file)
{
        return "MVK_TRACE " + file + "(" + std::to_string(line) + ") " + message + "\n";
}

void
trace_impl(std::string_view message, size_t const line, std::string_view file)
{
        std::cout << "MVK_TRACE " << file << '(' << line << "): " << message << '\n';
}

} // namespace mvk::utility::detail
