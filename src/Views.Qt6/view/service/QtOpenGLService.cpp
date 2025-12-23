#include "QtOpenGLService.hpp"
#include "core_types.h"
#include "mupapi.h"

QtOpenGLService::QtOpenGLService(MainWindow *main_window)
{
}

core_result QtOpenGLService::request_attrs(const mupv_gl_buffer_attr *attrs, const int32_t *vals, size_t len)
{
    return Res_Ok;
}
core_result QtOpenGLService::request_version(mupv_gl_profile profile, uint32_t major, uint32_t minor)
{
    return Res_Ok;
}

core_result QtOpenGLService::open_window(uint32_t width, uint32_t height)
{
    return Res_Ok;
}
core_result QtOpenGLService::close_window()
{
    return Res_Ok;
}

mupv_fptr QtOpenGLService::get_proc_address(const char *sym)
{
    return nullptr;
}

core_result QtOpenGLService::query_attrs(const mupv_gl_buffer_attr *attrs, int32_t *vals, size_t len)
{
    return Res_Ok;
}
core_result QtOpenGLService::query_version(mupv_gl_profile *profile, uint32_t *major, uint32_t *minor)
{
    return Res_Ok;
}
core_result QtOpenGLService::query_default_fbo(uint32_t *fbo)
{
    return Res_Ok;
}

core_result QtOpenGLService::swap_buffers()
{
    return Res_Ok;
}