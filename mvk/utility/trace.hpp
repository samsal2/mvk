#ifndef MVK_UTILITY_TRACE_HPP_INCLUDED
#define MVK_UTILITY_TRACE_HPP_INCLUDED

#include <string>
#include <string_view>

namespace mvk::utility
{
namespace detail
{

[[nodiscard]] std::string
trace_message(std::string const & message, size_t line,
              std::string const & file);

void
trace_impl(std::string_view message, size_t line, std::string_view file);

} // namespace detail

#define MVK_TRACE(message)                                                   \
  ::mvk::utility::detail::trace_impl(message, __LINE__, __FILE__)

} // namespace mvk::utility

#endif
