#ifndef MVK_UTILITY_COMMON_HPP
#define MVK_UTILITY_COMMON_HPP

#include "utility/trace.hpp"

#include <string>

namespace mvk::utility
{

class verify_error : public std::exception
{
public:
        verify_error(std::string_view file, size_t line);

        [[nodiscard]] char const *
        what() const noexcept override;

private:
        std::string message_;
        std::string file_;
        std::string line_;
};

#ifndef NDEBUG
#        define MVK_VERIFY_NOT_REACHED() throw ::mvk::utility::verify_error(__FILE__, __LINE__)
#else
#        define MVK_VERIFY_NOT_REACHED()
#endif

namespace detail
{

static constexpr void
verify_impl(bool const statement, std::string_view const file, size_t const line)
{
        if (!statement) [[unlikely]]
        {
                throw verify_error(file, line);
        }
}

} // namespace detail

#ifndef NDEBUG
#        define MVK_VERIFY(expression) ::mvk::utility::detail::verify_impl(expression, __FILE__, __LINE__)
#else
#        define MVK_VERIFY(expression)
#endif

} // namespace mvk::utility

#endif
