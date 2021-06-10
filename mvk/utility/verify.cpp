#include "utility/verify.hpp"

#include <string>

namespace mvk::utility
{

verify_error::verify_error(std::string_view file, size_t const line) : file_(std::begin(file), std::end(file)), line_(std::to_string(line))
{
        message_ = "\nat " + file_ + " in line " + line_;
}

[[nodiscard]] char const *
verify_error::what() const noexcept
{
        return message_.c_str();
}

} // namespace mvk::utility
