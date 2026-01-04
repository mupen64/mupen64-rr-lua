#ifndef VIEW_SERVICE_QTOPENGLSERVICE_HPP_INCLUDED
#define VIEW_SERVICE_QTOPENGLSERVICE_HPP_INCLUDED

#include "../view/MainWindow.hpp"
#include "../model/Core.hpp"
#include <variant>

namespace impl {
  struct OpenGLRequestState {

  };
  struct OpenGLActiveState {

  };
}

class QtOpenGLService final : public Mupen::IOpenGLService {
  public:
    QtOpenGLService(MainWindow *main_window);

    virtual core_result request_attrs(const mupv_gl_buffer_attr *attrs, const int32_t *vals, size_t len) override;
    virtual core_result request_version(mupv_gl_profile profile, uint32_t major, uint32_t minor) override;

    virtual mupv_fptr get_proc_address(const char* symbol) override;

    virtual core_result open_window(uint32_t width, uint32_t height) override;
    virtual core_result close_window() override;
    virtual void populate_funcs(void* p_funcs) override;

    virtual core_result query_attrs(const mupv_gl_buffer_attr *attrs, int32_t *vals, size_t len) override;
    virtual core_result query_version(mupv_gl_profile *profile, uint32_t *major, uint32_t *minor) override;
    virtual core_result query_default_fbo(uint32_t *fbo) override;

    virtual core_result swap_buffers() override;
  private:
    MainWindow* m_main_window;
    std::variant<std::monostate, impl::OpenGLRequestState, impl::OpenGLActiveState> m_state;
    
};

#endif