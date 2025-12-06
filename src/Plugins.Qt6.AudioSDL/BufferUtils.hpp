#ifndef AUDIOSDL_BUFFER_UTILS_HPP_INCLUDED
#define AUDIOSDL_BUFFER_UTILS_HPP_INCLUDED

#include <memory>
#include <vector>
namespace AudioSDL
{
namespace util
{
// Source - https://stackoverflow.com/a
// Posted by Casey, modified by community. See post 'Timeline' for change history
// Retrieved 2025-12-06, License - CC BY-SA 3.0

// Allocator adaptor that interposes construct() calls to
// convert value initialization into default initialization.
template <typename T, typename A = std::allocator<T>> class default_init_allocator : public A
{
    typedef std::allocator_traits<A> a_t;

  public:
    template <typename U> struct rebind
    {
        using other = default_init_allocator<U, typename a_t::template rebind_alloc<U>>;
    };

    using A::A;

    template <typename U> void construct(U *ptr) noexcept(std::is_nothrow_default_constructible<U>::value)
    {
        ::new (static_cast<void *>(ptr)) U;
    }
    template <typename U, typename... Args> void construct(U *ptr, Args &&...args)
    {
        a_t::construct(static_cast<A &>(*this), ptr, std::forward<Args>(args)...);
    }
};

// std::vector that replaces the value-init allocator with this default-init allocator.
template <class T> using buffer = std::vector<T, default_init_allocator<T>>;

template <class T, auto F>
    requires(requires(T *ptr) { F(ptr); })
struct func_delete
{
    void operator()(T* ptr) {
        F(ptr);
    }
};

template <class T, auto F>
using c_unique_ptr = std::unique_ptr<T, func_delete<T, F>>;

} // namespace util

} // namespace AudioSDL

#endif