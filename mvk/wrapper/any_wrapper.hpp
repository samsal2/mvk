#ifndef MVK_WRAPPER_ANY_WRAPPER_HPP_INCLUDED
#define MVK_WRAPPER_ANY_WRAPPER_HPP_INCLUDED

#include "wrapper/handle_get.hpp"
#include "wrapper/handle_only.hpp"
#include "wrapper/object_allocate.hpp"
#include "wrapper/object_create.hpp"
#include "wrapper/object_destroy.hpp"
#include "wrapper/object_free.hpp"
#include "wrapper/owner_create.hpp"
#include "wrapper/owner_destroy.hpp"
#include "wrapper/unique.hpp"

namespace mvk::wrapper
{
template <typename... Args>
using any_wrapper_base =
    detail::selected_t<decltype(storage_selector<Args...>(
        select<options::storage>(Args{}...)))>;

template <typename... Args>
using any_wrapper_creator =
    detail::selected_t<decltype(creator_selector<Args...>(
        select<options::creator>(Args{}...)))>;

template <typename... Args>
class any_wrapper : public any_wrapper_base<Args...>,
                    public any_wrapper_creator<Args...>
{
public:
  using any_wrapper_base<Args...>::any_wrapper_base;
};

} // namespace mvk::wrapper

#endif
