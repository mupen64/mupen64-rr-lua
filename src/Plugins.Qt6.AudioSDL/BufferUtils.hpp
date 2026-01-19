#ifndef AUDIOSDL_BUFFER_UTILS_HPP_INCLUDED
#define AUDIOSDL_BUFFER_UTILS_HPP_INCLUDED

#include <memory>
#include <vector>
namespace AudioSDL
{
namespace util
{

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