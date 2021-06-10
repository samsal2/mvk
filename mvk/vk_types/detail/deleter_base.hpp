#ifndef MVK_VK_TYPES_DELETER_BASE
#define MVK_VK_TYPES_DELETER_BASE

#include <type_traits>

namespace mvk::vk_types::detail
{

template <typename... Ts>
class deleter_base;

template <typename Instance>
class deleter_base<Instance>
{
public:
        using value_type = Instance;

        constexpr deleter_base() noexcept = default;

        using is_allocation = std::false_type;
};

template <typename Instance, typename Parent>
class deleter_base<Instance, Parent>
{
public:
        using parent_type = Parent;
        using value_type  = Instance;

        struct is_allocation : public std::false_type
        {
        };

        constexpr deleter_base() noexcept = default;

        explicit deleter_base(parent_type parent) noexcept : parent_(parent)
        {
        }

        [[nodiscard]] constexpr parent_type
        parent() const noexcept
        {
                return parent_;
        }

private:
        parent_type parent_ = nullptr;
};

template <typename Instance, typename Parent, typename Pool>
class deleter_base<Instance, Parent, Pool>
{
public:
        using parent_type = Parent;
        using pool_type   = Pool;
        using value_type  = Instance;

        struct is_allocation : public std::true_type
        {
        };

        constexpr deleter_base() noexcept = default;

        constexpr deleter_base(parent_type parent, pool_type pool) noexcept : parent_(parent), pool_(pool)
        {
        }

        [[nodiscard]] constexpr parent_type
        parent() const noexcept
        {
                return parent_;
        }

        [[nodiscard]] constexpr pool_type
        pool() const noexcept
        {
                return pool_;
        }

private:
        parent_type parent_ = nullptr;
        pool_type   pool_   = nullptr;
};

} // namespace mvk::vk_types::detail

#endif
