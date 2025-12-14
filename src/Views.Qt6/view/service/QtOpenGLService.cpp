#include "QtOpenGLService.hpp"

QtOpenGLService::QtOpenGLService(MainWindow *main_window)
{
}

core_result QtOpenGLService::request_attrs(const mupv_gl_buffer_attr *attrs, const int32_t *vals, size_t len)
{
}
core_result QtOpenGLService::request_version(mupv_gl_profile profile, uint32_t major, uint32_t minor)
{
}

core_result QtOpenGLService::open_window(uint32_t width, uint32_t height)
{
}
core_result QtOpenGLService::close_window()
{
}

core_result QtOpenGLService::query_attrs(const mupv_gl_buffer_attr *attrs, int32_t *vals, size_t len)
{
}
core_result QtOpenGLService::query_version(mupv_gl_profile *profile, uint32_t *major, uint32_t *minor)
{
}
core_result QtOpenGLService::query_default_fbo(uint32_t *fbo)
{
}

core_result QtOpenGLService::swap_buffers()
{
}