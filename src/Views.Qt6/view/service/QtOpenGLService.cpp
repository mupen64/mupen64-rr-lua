#include "QtOpenGLService.hpp"
#include "core_types.h"
#include "mupapi.h"

QtOpenGLService::QtOpenGLService(MainWindow *main_window) : m_main_window(main_window)
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
void QtOpenGLService::populate_funcs(void* p_funcs) {
    auto funcs = (mupv_wm_gl_funcs*) p_funcs;
    *funcs = {
        .get_proc_address = [](void* p_self, const char* sym) {
            auto self = (QtOpenGLService*) (Mupen::IWindowService*) p_self;
            return self->get_proc_address(sym);
        },
        .request_attrs = [](void* p_self, const mupv_gl_buffer_attr* attrs, const int32_t* vals, size_t len) {
            auto self = (QtOpenGLService*) (Mupen::IWindowService*) p_self;
            return self->request_attrs(attrs, vals, len);
        },
        .request_version = [](void* p_self, mupv_gl_profile profile, uint32_t major, uint32_t minor) {
            auto self = (QtOpenGLService*) (Mupen::IWindowService*) p_self;
            return self->request_version(profile, major, minor);
        },
        .query_attrs = [](void* p_self, const mupv_gl_buffer_attr* attrs, int32_t* vals, size_t len) {
            auto self = (QtOpenGLService*) (Mupen::IWindowService*) p_self;
            return self->query_attrs(attrs, vals, len);
        },
        .query_version = [](void* p_self, mupv_gl_profile* profile, uint32_t* major, uint32_t* minor) {
            auto self = (QtOpenGLService*) (Mupen::IWindowService*) p_self;
            return self->query_version(profile, major, minor);
        },
        .query_default_fbo = [](void* p_self, uint32_t* fbo) {
            auto self = (QtOpenGLService*) (Mupen::IWindowService*) p_self;
            return self->query_default_fbo(fbo);
        },
        .swap_buffers = [](void* p_self) {
            auto self = (QtOpenGLService*) (Mupen::IWindowService*) p_self;
            return self->swap_buffers();
        },
    };
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
    if (fbo)
        *fbo = 0;
    return Res_Ok;
}

core_result QtOpenGLService::swap_buffers()
{
    return Res_Ok;
}