#include "utility/verify.hpp"

#include <string>

namespace mvk::utility
{

verify_error::verify_error(std::string_view file, size_t const line) : file_(std::begin(file), std::end(file)), line_(std::to_string(line))
{
    // TODO(samuel): temporary solution
    message_ = detail::trace_message("MVK_VERIFY error", line, std::string(std::begin(file), std::end(file)));
}

[[nodiscard]] char const *
verify_error::what() const noexcept
{
    return message_.c_str();
}

} // namespace mvk::utility
